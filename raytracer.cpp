#include "precomp.h"

void RayTracer::AddLamp()
{
	LightPos initLightPos;
	initLightPos.position = make_float2(0.0f, 0.0f);
	initLightPos.duration = 1;
	lightPositions.push_back(initLightPos);
}

void RayTracer::Init(Mesh* mesh)
{
	this->mesh = mesh;
	LoadRoute(defaultRouteFile);

	//if (lightPositions.empty()) { // If no route has previously been saved, initialise with single light
	//	LightPos initLightPos;
	//	initLightPos.position = make_float2(0.5f, 0.5f);
	//	initLightPos.duration = 1;
	//	lightPositions.push_back(initLightPos);
	//}

	//initLightPos.position = make_float3(-0.1f, 0.6f + floorOffset, -1.9f);
	//initLightPos.duration = 1;
	//lightPositions.push_back(initLightPos);

	// compile and load kernel "render" from file "kernels.cl"
	generateKernel = new Kernel("cl/generate.cl", "render");
	extendKernel = new Kernel("cl/extend.cl", "render");
	shadeDosageKernel = new Kernel("cl/shade.cl", "computeDosage");
	shadeColorKernel = new Kernel("cl/shade.cl", "dosageToColor");
	resetKernel = new Kernel("cl/reset.cl", "render");
	accumulateKernel = new Kernel("cl/accumulate.cl", "render");

	verticesBuffer = new Buffer(mesh->triangleCount * sizeof(Tri), Buffer::DEFAULT, mesh->triangles);
	verticesBuffer->CopyToDevice();
	//delete[] mesh->triangles;
	
	bvhNodesBuffer = new Buffer(mesh->bvh->nodesUsed * sizeof(BVHNode), Buffer::DEFAULT, mesh->bvh->bvhNode);
	triIdxBuffer = new Buffer(mesh->triangleCount * sizeof(uint), Buffer::DEFAULT, mesh->bvh->triIdx);
	bvhNodesBuffer->CopyToDevice();
	triIdxBuffer->CopyToDevice();

	photonMapBuffer = new Buffer(sizeof(double) * mesh->triangleCount, Buffer::DEFAULT);
	maxPhotonMapBuffer = new Buffer(sizeof(double) * mesh->triangleCount, Buffer::DEFAULT);
	tempPhotonMapBuffer = new Buffer(sizeof(int) * mesh->triangleCount, Buffer::DEFAULT);
	dosageBuffer = new Buffer(sizeof(float) * mesh->triangleCount, Buffer::DEFAULT, dosageMap);

	colorBuffer = new Buffer(mesh->dosageBufferID, Buffer::GLARRAY | Buffer::WRITEONLY);
	//generateKernel->SetArgument(0, rayBuffer);

	extendKernel->SetArgument(0, tempPhotonMapBuffer);
	extendKernel->SetArgument(1, verticesBuffer);
	//extendKernel->SetArgument(2, rayBuffer);
	extendKernel->SetArgument(3, bvhNodesBuffer);
	extendKernel->SetArgument(4, triIdxBuffer);
	extendKernel->SetArgument(5, mesh->triangleCount);

	shadeDosageKernel->SetArgument(1, dosageBuffer);
	shadeDosageKernel->SetArgument(2, verticesBuffer);

	shadeColorKernel->SetArgument(0, dosageBuffer);
	shadeColorKernel->SetArgument(1, colorBuffer);

	resetKernel->SetArgument(0, photonMapBuffer);
	resetKernel->SetArgument(1, maxPhotonMapBuffer);
	resetKernel->SetArgument(2, colorBuffer);

	accumulateKernel->SetArgument(0, photonMapBuffer);
	accumulateKernel->SetArgument(1, maxPhotonMapBuffer);
	accumulateKernel->SetArgument(2, tempPhotonMapBuffer);

	//timeStepKernel->SetArgument(0, photonMapBuffer);
	//size_t size = 0;
	//clGetDeviceInfo(extendKernel->GetDevice(), CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &size, 0);//CL_DEVICE_MAX_WORK_GROUP_SIZE
	//cout << " CL_DEVICE_MAX_WORK_GROUP_SIZE: " << size << endl;
}

void RayTracer::ComputeDosageMap(vector<LightPos> lightPositions, int photonCount)
{
	//Round down the photons per light to the nearest number divisble by 2, to prevent tanking the performance of the kernels
	int photonsPerLight = ((photonCount / lightPositions.size()) & ~1);
	for (int i = 0; i < lightPositions.size(); ++i)
	{
		ComputeDosageMap(lightPositions[i], photonsPerLight, mesh->triangleCount);
	}
}

void RayTracer::ComputeDosageMap(LightPos lightPos, int photonsPerLight, int triangleCount)
{
	cout << " computing " << endl;
	float3 lightposition = make_float3(lightPos.position.x, lightHeight, lightPos.position.y);
	generateKernel->SetArgument(1, lightposition);
	generateKernel->SetArgument(2, lightLength);
	cout << " computingquarter " << endl;
	generateKernel->Run(photonsPerLight);
	cout << " computinghalf " << endl;

	extendKernel->Run(photonsPerLight);

	accumulateKernel->SetArgument(3, lightPos.duration);
	accumulateKernel->Run(triangleCount);

	photonMapSize += photonsPerLight;
	cout << " computingend " << endl;
}

void RayTracer::Shade()
{
	cout << " shading " << endl;
	if (viewMode == maxpower)
	{
		//Scale by 100 to convert from W/m^2 to microW/cm^2
		shadeDosageKernel->SetArgument(4, lightIntensity * 100);
		shadeDosageKernel->SetArgument(0, maxPhotonMapBuffer);
		// Only the number of photons per light of a single iteration
		shadeDosageKernel->SetArgument(3, (int)((photonCount / lightPositions.size()) & ~1));//TODO: move to compute only once to prevent bugs etc
		shadeColorKernel->SetArgument(2, minPower);
	}
	else
	{
		//Scale by 0.1 too convert from J/m^2 to mJ/cm^2
		shadeDosageKernel->SetArgument(4, lightIntensity * 0.1f);
		shadeDosageKernel->SetArgument(0, photonMapBuffer);
		// The number of photons per area should be divided by the number of photons per light,
		// as each photon carries a fraction of a single light's power
		shadeDosageKernel->SetArgument(3, photonMapSize / (int)lightPositions.size());
		shadeColorKernel->SetArgument(2, minDosage);
	}

	shadeDosageKernel->Run(mesh->triangleCount);
	shadeColorKernel->Run(colorBuffer, mesh->triangleCount);
}

void RayTracer::ResetDosageMap() {
	startedComputation = true;
	compTime = 0;
	timerClock.reset();
	SaveRoute(defaultRouteFile);
	progress = 0;
	reachedMaxPhotons = false;
	currIterations = 0;
	ClearBuffers(true);
}

void RayTracer::ClearBuffers(bool resetColor)
{
	photonMapSize = 0;
	delete rayBuffer;
	rayBuffer = new Buffer(32 * photonCount, Buffer::DEFAULT);
	generateKernel->SetArgument(0, rayBuffer);
	extendKernel->SetArgument(2, rayBuffer);

	resetKernel->SetArgument(3, resetColor);
	if (resetColor)
		resetKernel->Run(colorBuffer, mesh->triangleCount);
	else
		resetKernel->Run(mesh->triangleCount);
}

void RayTracer::CalibratePower()
{
	LightPos singleLightPos;
	singleLightPos.position = make_float2(0.0f, 0.0f);
	Tri* square = new Tri[2];
	square[0].vertex0 = make_float3_strict(singleLightPos.position.x + 0.1f, measureHeight + 0.1f, singleLightPos.position.y + measureDist);
	square[0].vertex1 = make_float3_strict(singleLightPos.position.x - 0.1f, measureHeight + 0.1f, singleLightPos.position.y + measureDist);
	square[0].vertex2 = make_float3_strict(singleLightPos.position.x + 0.1f, measureHeight - 0.1f, singleLightPos.position.y + measureDist);
	square[1].vertex0 = make_float3_strict(singleLightPos.position.x - 0.1f, measureHeight - 0.1f, singleLightPos.position.y + measureDist);
	square[1].vertex1 = make_float3_strict(singleLightPos.position.x - 0.1f, measureHeight + 0.1f, singleLightPos.position.y + measureDist);
	square[1].vertex2 = make_float3_strict(singleLightPos.position.x + 0.1f, measureHeight - 0.1f, singleLightPos.position.y + measureDist);
	delete verticesBuffer;
	verticesBuffer = new Buffer(2 * sizeof(Tri), Buffer::DEFAULT, square);
	verticesBuffer->CopyToDevice();
	extendKernel->SetArgument(1, verticesBuffer);
	shadeDosageKernel->SetArgument(2, verticesBuffer);

	ClearBuffers(false);

	BVHNode* temp = mesh->bvh->bvhNode;
	mesh->bvh->bvhNode = new BVHNode[2];
	bvhNodesBuffer->CopyToDevice();

	//later:
	mesh->bvh->bvhNode = temp;
	bvhNodesBuffer->CopyToDevice();


	triIdxBuffer = new Buffer(mesh->triangleCount * sizeof(uint), Buffer::DEFAULT, mesh->bvh->triIdx);
	triIdxBuffer->CopyToDevice();

	extendKernel->SetArgument(5, 2);

	for (int i = 0; i < maxIterations; ++i)
	{
		ComputeDosageMap(singleLightPos, photonCount, 2);
	}
	
	//Scale by 100 to convert from W/m^2 to microW/cm^2
	shadeDosageKernel->SetArgument(4, 1 * 100.0f); // Use 1 as power, so that dividing the measured irradiance by the resulting irradiance yields the calibrated power
	shadeDosageKernel->SetArgument(0, maxPhotonMapBuffer);
	// Only the number of photons per light of a single iteration
	shadeDosageKernel->SetArgument(3, photonCount);//TODO: move to compute only once to prevent bugs etc
	shadeDosageKernel->Run(2);

	dosageBuffer->CopyFromDevice();
	cout << " d1 " << dosageMap[0] << " d2 " << dosageMap[1] << endl;
	float avgPower = (dosageMap[0] + dosageMap[1]) / 2.0f;
	calibratedPower = measurePower / avgPower;
	//lightIntensity = calibratedPower;
	//TODO: do not use bvh for this! (or create simple one?)

	cout << " tr1 " << mesh->triangles[mesh->triangleCount-2].vertex0.x << " tr2 " << mesh->triangles[mesh->triangleCount-1].vertex0.x << endl;
	delete verticesBuffer;
	verticesBuffer = new Buffer(mesh->triangleCount * sizeof(Tri), Buffer::DEFAULT, mesh->triangles);
	verticesBuffer->CopyToDevice();//TODO: at least sometimes this causes the crash
	extendKernel->SetArgument(1, verticesBuffer);
	shadeDosageKernel->SetArgument(2, verticesBuffer);
	extendKernel->SetArgument(5, mesh->triangleCount);
	
	clFinish(Kernel::GetQueue());
	cout << " done "  << endl;
}


#include "tinyxml2.h"
using namespace tinyxml2;

void RayTracer::SaveRoute(char fileName[32])
{
	XMLDocument doc;
	XMLNode* root = doc.NewElement("route");
	doc.InsertFirstChild(root);
	((XMLElement*)root->InsertEndChild(doc.NewElement("aantal_fotonen")))->SetText(photonCount);
	((XMLElement*)root->InsertEndChild(doc.NewElement("aantal_iteraties")))->SetText(maxIterations);
	((XMLElement*)root->InsertEndChild(doc.NewElement("lamp_sterkte")))->SetText(lightIntensity);
	((XMLElement*)root->InsertEndChild(doc.NewElement("minimale_dosis")))->SetText(minDosage);
	((XMLElement*)root->InsertEndChild(doc.NewElement("minimale_bestralingssterkte")))->SetText(minPower);
	((XMLElement*)root->InsertEndChild(doc.NewElement("lamp_lengte")))->SetText(lightLength);
	((XMLElement*)root->InsertEndChild(doc.NewElement("lamp_hoogte")))->SetText(lightHeight);
	XMLElement* viewElem = doc.NewElement("route");
	for (int i = 0; i < lightPositions.size(); i++)
	{
		XMLElement* lightPosElem = doc.NewElement(("lamp_positie_" + std::to_string(i)).c_str());
		LightPos lightPos = lightPositions[i];
		lightPosElem->SetAttribute("positie_x", lightPos.position.x);
		lightPosElem->SetAttribute("positie_y", lightPos.position.y);
		lightPosElem->SetAttribute("duration", lightPos.duration);

		viewElem->InsertEndChild(lightPosElem);
	}
	root->InsertEndChild(viewElem);
	char prefix[64] = "positions/";
	doc.SaveFile(strcat(strcat(prefix, fileName), ".xml"));
}

void RayTracer::LoadRoute(char fileName[32])
{
	char prefix[64] = "positions/";
	XMLDocument doc;
	XMLError result = doc.LoadFile(strcat(strcat(prefix, fileName), ".xml"));
	if (result != XML_SUCCESS) return;
	XMLNode* root = doc.FirstChild();
	if (root == nullptr) return;
	XMLElement* docElem;
	if ((docElem = root->FirstChildElement("aantal_fotonen"))) docElem->QueryIntText(&photonCount);
	if ((docElem = root->FirstChildElement("aantal_iteraties"))) docElem->QueryIntText(&maxIterations);
	if ((docElem = root->FirstChildElement("lamp_sterkte"))) docElem->QueryFloatText(&lightIntensity);
	if ((docElem = root->FirstChildElement("minimale_dosis"))) docElem->QueryFloatText(&minDosage);
	if ((docElem = root->FirstChildElement("minimale_bestralingssterkte"))) docElem->QueryFloatText(&minPower);
	if ((docElem = root->FirstChildElement("lamp_lengte"))) docElem->QueryFloatText(&lightLength);
	if ((docElem = root->FirstChildElement("lamp_hoogte"))) docElem->QueryFloatText(&lightHeight);

	//int routeSize;
	//if ((docElem = root->FirstChildElement("route_grootte"))) docElem->QueryIntText(&routeSize);

	if (docElem = root->FirstChildElement("route")) {
		bool foundPos = true;
		int i = 0;
		lightPositions.clear();
		while(foundPos)
		{
			XMLElement* lightPosElem;
			foundPos = (lightPosElem = docElem->FirstChildElement(("lamp_positie_" + std::to_string(i)).c_str()));
			if (foundPos) {
				LightPos lightPos;
				lightPosElem->QueryFloatAttribute("positie_x", &lightPos.position.x);
				lightPosElem->QueryFloatAttribute("positie_y", &lightPos.position.y);
				lightPosElem->QueryFloatAttribute("duration", &lightPos.duration);
				lightPositions.push_back(lightPos);
				i++;
			}
		}
	}
}
