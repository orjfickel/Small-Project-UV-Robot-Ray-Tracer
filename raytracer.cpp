#include "precomp.h"

void RayTracer::AddLamp()
{
	LightPos initLightPos;
	initLightPos.position = make_float2(0.0f, 0.0f);
	initLightPos.duration = 1;
	lightPositions.push_back(initLightPos);
	UpdatePhotonsPerLight();
}

void RayTracer::Init(Mesh* mesh)
{
	this->mesh = mesh;
	LoadRoute(defaultRouteFile);

	generateKernel = new Kernel("cl/generate.cl", "render");
	extendKernel = new Kernel("cl/extend.cl", "render");
	shadeDosageKernel = new Kernel("cl/shade.cl", "computeDosage");
	shadeColorKernel = new Kernel("cl/shade.cl", "dosageToColor");
	resetKernel = new Kernel("cl/reset.cl", "render");
	accumulateKernel = new Kernel("cl/accumulate.cl", "render");

	verticesBuffer = new Buffer(mesh->triangleCount * sizeof(Tri), Buffer::DEFAULT, mesh->triangles);
	verticesBuffer->CopyToDevice();
	
	bvhNodesBuffer = new Buffer(mesh->bvh->nodesUsed * sizeof(BVHNode), Buffer::DEFAULT, mesh->bvh->bvhNode);
	triIdxBuffer = new Buffer(mesh->triangleCount * sizeof(uint), Buffer::DEFAULT, mesh->bvh->triIdx);
	bvhNodesBuffer->CopyToDevice();
	triIdxBuffer->CopyToDevice();

	photonMapBuffer = new Buffer(sizeof(double) * mesh->triangleCount, Buffer::DEFAULT);
	maxPhotonMapBuffer = new Buffer(sizeof(double) * mesh->triangleCount, Buffer::DEFAULT);
	tempPhotonMapBuffer = new Buffer(sizeof(int) * mesh->triangleCount, Buffer::DEFAULT);
	dosageBuffer = new Buffer(sizeof(float) * mesh->triangleCount, Buffer::DEFAULT, dosageMap);

	colorBuffer = new Buffer(mesh->dosageBufferID, Buffer::GLARRAY | Buffer::WRITEONLY);

	extendKernel->SetArgument(0, tempPhotonMapBuffer);
	extendKernel->SetArgument(1, verticesBuffer);
	extendKernel->SetArgument(3, bvhNodesBuffer);
	extendKernel->SetArgument(4, triIdxBuffer);
	extendKernel->SetArgument(5, mesh->triangleCount);

	shadeDosageKernel->SetArgument(1, dosageBuffer);
	shadeDosageKernel->SetArgument(2, verticesBuffer);

	shadeColorKernel->SetArgument(0, dosageBuffer);
	shadeColorKernel->SetArgument(1, colorBuffer);

	resetKernel->SetArgument(0, photonMapBuffer);
	resetKernel->SetArgument(1, maxPhotonMapBuffer);
	resetKernel->SetArgument(2, tempPhotonMapBuffer);
	resetKernel->SetArgument(3, colorBuffer);

	accumulateKernel->SetArgument(0, photonMapBuffer);
	accumulateKernel->SetArgument(1, maxPhotonMapBuffer);
	accumulateKernel->SetArgument(2, tempPhotonMapBuffer);
}

void RayTracer::UpdatePhotonsPerLight(){
	//Round down the photons per light to the nearest number divisible by 2, to prevent tanking the performance of the kernels
	photonsPerLight = (photonCount / lightPositions.size()) & ~1;
}

void RayTracer::ComputeDosageMap()
{
	for(LightPos& lightPosition : lightPositions)
	{
		ComputeSingleLightDosageMap(lightPosition, photonsPerLight, mesh->triangleCount);
	}
}

// photonsPerLight and triangleCount are used as parameters, since we want to use different values in CalibratePower()
void RayTracer::ComputeSingleLightDosageMap(LightPos lightPos, int photonsPerLight, int triangleCount)
{
	float3 lightposition = make_float3(lightPos.position.x, mesh->floorHeight + lightHeight, lightPos.position.y);
	generateKernel->SetArgument(1, lightposition);
	generateKernel->SetArgument(2, lightLength);
	generateKernel->Run(photonsPerLight);

	extendKernel->Run(photonsPerLight);

	accumulateKernel->SetArgument(3, lightPos.duration);
	accumulateKernel->Run(triangleCount);

	photonMapSize += photonsPerLight;
}

/**
 * \brief Convert the photon counts into dosage or irradiance, and then convert to colors for the heatmap
 */
void RayTracer::Shade()
{
	shadeColorKernel->SetArgument(3, thresholdView);
	if (viewMode == maxpower)
	{
		shadeDosageKernel->SetArgument(0, maxPhotonMapBuffer); // photonMap
		// Only the number of photons per light of a single iteration
		shadeDosageKernel->SetArgument(3, photonsPerLight);
		// Scale by 100 to convert from W/m^2 to microW/cm^2
		shadeDosageKernel->SetArgument(4, lightIntensity * 100); // scaledPower

		shadeColorKernel->SetArgument(2, minPower);
	}
	else // viewMode is dosage
	{
		shadeDosageKernel->SetArgument(0, photonMapBuffer);
		// The number of photons per area should be divided by the number of photons per light,
		// as each photon carries a fraction of a single light's power
		shadeDosageKernel->SetArgument(3, photonMapSize / (int)lightPositions.size()); // photonsPerLight
		// Scale by 0.1 too convert from J/m^2 to mJ/cm^2
		shadeDosageKernel->SetArgument(4, lightIntensity * 0.1f); // scaledPower

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
	finishedComputation = false;
	currIterations = 0;
	ClearBuffers(true);
}

void RayTracer::ClearBuffers(bool resetColor)
{
	photonMapSize = 0;
	delete rayBuffer;// Photon count might be higher so just reallocate memory to be safe
	rayBuffer = new Buffer(32 * photonCount, Buffer::DEFAULT);
	generateKernel->SetArgument(0, rayBuffer);
	extendKernel->SetArgument(2, rayBuffer);

	resetKernel->SetArgument(4, resetColor);
	resetKernel->Run(colorBuffer, mesh->triangleCount);
}

/**
 * \brief Adjust the light intensity linearly based on how the measurement power compares to the ray traced power
 * \param measurePower 
 * \param measureHeight 
 * \param measureDist 
 */
void RayTracer::CalibratePower(float measurePower, float measureHeight, float measureDist)
{
	measureHeight += mesh->floorHeight;

	// Create a small square as the sample geometry
	LightPos singleLightPos;
	singleLightPos.position = make_float2(0.0f, 0.0f);
	Tri* square = new Tri[2];
	float triWidth = 0.1f;
	square[0].vertex0 = make_float3_strict(singleLightPos.position.x + triWidth, measureHeight + triWidth, singleLightPos.position.y + measureDist);
	square[0].vertex1 = make_float3_strict(singleLightPos.position.x - triWidth, measureHeight + triWidth, singleLightPos.position.y + measureDist);
	square[0].vertex2 = make_float3_strict(singleLightPos.position.x + triWidth, measureHeight - triWidth, singleLightPos.position.y + measureDist);
	square[1].vertex0 = make_float3_strict(singleLightPos.position.x - triWidth, measureHeight - triWidth, singleLightPos.position.y + measureDist);
	square[1].vertex1 = make_float3_strict(singleLightPos.position.x - triWidth, measureHeight + triWidth, singleLightPos.position.y + measureDist);
	square[1].vertex2 = make_float3_strict(singleLightPos.position.x + triWidth, measureHeight - triWidth, singleLightPos.position.y + measureDist);
	verticesBuffer->hostBuffer = (uint*)square;
	int tempVerticesSize = verticesBuffer->size;
	verticesBuffer->size = 2 * sizeof(Tri);
	verticesBuffer->CopyToDevice();

	ClearBuffers(false);
	
	BVHNode* hostNode = new BVHNode[1];
	hostNode[0].leftFirst = 0;
	hostNode[0].triCount = 2;
	bvhNodesBuffer->hostBuffer = (uint*)hostNode;
	int tempsize = bvhNodesBuffer->size;
	bvhNodesBuffer->size = 1 * sizeof(BVHNode);
	bvhNodesBuffer->CopyToDevice();
	
	uint* hostTriIdx = new uint[2];
	hostTriIdx[0] = 0;
	hostTriIdx[1] = 1;
	triIdxBuffer->hostBuffer = hostTriIdx;
	int tempTriIdxSize = triIdxBuffer->size;
	triIdxBuffer->size = 2 * sizeof(uint);
	triIdxBuffer->CopyToDevice();

	// Set the triangle count to 2
	extendKernel->SetArgument(5, 2);

	for (int i = 0; i < maxIterations; ++i)
	{
		ComputeSingleLightDosageMap(singleLightPos, photonCount, 2);
	}
	
	// Use 1 as power, so that dividing the measured irradiance by the resulting irradiance yields the calibrated power
	shadeDosageKernel->SetArgument(4, 1.0f); 
	shadeDosageKernel->SetArgument(0, maxPhotonMapBuffer);
	shadeDosageKernel->SetArgument(3, photonCount);
	shadeDosageKernel->Run(2);
	clFinish(Kernel::GetQueue());

	int tempDosageSize = dosageBuffer->size;
	dosageBuffer->size = 2 * sizeof(float);
	dosageBuffer->CopyFromDevice();
	dosageBuffer->size = tempDosageSize;
	float avgPower = (dosageMap[0] + dosageMap[1]) / 2.0f;
	calibratedPower = 0.01f * (measurePower / avgPower);
	lightIntensity = calibratedPower;
	
	extendKernel->SetArgument(5, mesh->triangleCount);

	verticesBuffer->hostBuffer = (uint*)mesh->triangles;
	verticesBuffer->size = tempVerticesSize;
	verticesBuffer->CopyToDevice();
	
	bvhNodesBuffer->hostBuffer = (uint*)mesh->bvh->bvhNode;
	bvhNodesBuffer->size = tempsize;
	bvhNodesBuffer->CopyToDevice();

	triIdxBuffer->hostBuffer = mesh->bvh->triIdx;
	triIdxBuffer->size = tempTriIdxSize;
	triIdxBuffer->CopyToDevice();
	
	cout << "Done calibrating "  << endl;
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
	UpdatePhotonsPerLight();
}
