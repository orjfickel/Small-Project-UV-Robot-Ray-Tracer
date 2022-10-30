#include "precomp.h"

void RayTracer::AddLamp()
{
	LightPos initLightPos;
	initLightPos.position = make_float2(0.0f, 0.0f);
	initLightPos.duration = 1;
	lightPositions.push_back(initLightPos);
}

void RayTracer::Init()
{
	LoadRoute(defaultRouteFile);

	if (lightPositions.empty()) { // If no route has previously been saved, initialise with single light
		LightPos initLightPos;
		initLightPos.position = make_float2(0.5f, 0.5f);
		initLightPos.duration = 1;
		lightPositions.push_back(initLightPos);
	}

	//initLightPos.position = make_float3(-0.1f, 0.6f + floorOffset, -1.9f);
	//initLightPos.duration = 1;
	//lightPositions.push_back(initLightPos);

	// compile and load kernel "render" from file "kernels.cl"
	generateKernel = new Kernel("generate.cl", "render");
	extendKernel = new Kernel("extend.cl", "render");
	shadeKernel = new Kernel("shade.cl", "render");
	resetKernel = new Kernel("reset.cl", "render");
	accumulateKernel = new Kernel("accumulate.cl", "render");

	// create an OpenCL buffer over using bitmap.pixels
	photonMapBuffer = new Buffer(2 * triangleCount, Buffer::DEFAULT);//Texture not necessary as per triangle dosage can be done with OpenCL as well.
	maxPhotonMapBuffer = new Buffer(2 * triangleCount, Buffer::DEFAULT);
	tempPhotonMapBuffer = new Buffer(triangleCount, Buffer::DEFAULT);
	//rayBuffer = new Buffer(8*photonCount, Buffer::DEFAULT);//*8 because buffer is in uints
	//lightPosBuffer = new Buffer(4 * lightPositions.size(), Buffer::DEFAULT, lightPositions.data());

	//triangleBuffer = new Buffer(triangleCount, Buffer::DEFAULT, triangles);	

	//TODO: since one vertex belongs to multiple faces, we cannot give it a single color, as it would be wrongly interpolated in the fragment shader.
	//		Therefore a texture map should be used and sent to the fragment shader after all...
	// Or otherwise render opengl with fat triangle data after all, but use the compressed data for the OpenCL kernels?
	colorBuffer = new Buffer(dosageBufferID, Buffer::GLARRAY | Buffer::WRITEONLY);

	//generateKernel->SetArgument(0, rayBuffer);

	extendKernel->SetArgument(0, tempPhotonMapBuffer);
	//extendKernel->SetArgument(2, rayBuffer);
	extendKernel->SetArgument(3, verticesBuffer);
	extendKernel->SetArgument(4, triangleCount);

	shadeKernel->SetArgument(1, colorBuffer);
	shadeKernel->SetArgument(2, verticesBuffer);

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

void RayTracer::ComputeDosageMap()
{
	//Round down the photons per light to the nearest number divisble by 2, to prevent tanking the performance of the kernels
	int photonsPerLight = ((photonCount / lightPositions.size()) & ~1);
	for (int i = 0; i < lightPositions.size(); ++i)
	{
		float3 lightposition = make_float3(lightPositions[i].position.x, lightHeight, lightPositions[i].position.y);
		generateKernel->SetArgument(1, lightposition);
		generateKernel->SetArgument(2, lightLength);
		generateKernel->Run(photonsPerLight);
		//clFinish(Kernel::GetQueue());
		//cout << " generated: " <<  timerClock.elapsed() * 1000.0f << endl;

		extendKernel->SetArgument(1, photonMapSize);
		extendKernel->Run(photonsPerLight);

		// Every timestep gets accumulated into the same map, so we cannot just multiply the entire thing by timestep (could be possible by dividing by (total timestep - timestep so far))
		//timeStepKernel->SetArgument(1, lightPositions[i].duration);
		//timeStepKernel->Run(vertexCount / 9);
		//clFinish(Kernel::GetQueue());
		//cout << " extended: " <<  timerClock.elapsed() * 1000.0f << endl;

		accumulateKernel->SetArgument(3, lightPositions[i].duration);
		accumulateKernel->Run(triangleCount);

		//cout << " accumul: " << timerClock.elapsed() * 1000.0f << endl;
		//TODO: separate kernel to multiply photoncount per triangle by the timestep and light power?
		//TODO: why at the ~third to last iteraion do red outliers suddenly show up?

		photonMapSize += photonsPerLight;
	}
	Shade();
	//cout << " shed: " << timerClock.elapsed() * 1000.0f << endl;
	//clFinish(Kernel::GetQueue());//todo	
}

void RayTracer::Shade()
{
	shadeKernel->SetArgument(4, lightIntensity);
	if (viewMode == maxpower)
	{
		shadeKernel->SetArgument(0, maxPhotonMapBuffer);
		// Only the number of photons per light of a single iteration
		shadeKernel->SetArgument(3, (int)((photonCount / lightPositions.size()) & ~1));//TODO: move to compute only once to prevent bugs etc
		shadeKernel->SetArgument(5, minPower);
	}
	else
	{
		shadeKernel->SetArgument(0, photonMapBuffer);
		// The number of photons per area should be divided by the number of photons per light,
		// as each photon carries a fraction of a single light's power
		shadeKernel->SetArgument(3, photonMapSize / (int)lightPositions.size());
		shadeKernel->SetArgument(5, minDosage);
	}

	shadeKernel->Run(colorBuffer, triangleCount);
}

void RayTracer::ResetDosageMap() {
	startedComputation = true;
	compTime = 0;
	timerClock.reset();
	SaveRoute(defaultRouteFile);
	photonMapSize = 0;
	reachedMaxPhotons = false;
	currIterations = 0;
	delete rayBuffer;
	rayBuffer = new Buffer(8 * photonCount, Buffer::DEFAULT);
	generateKernel->SetArgument(0, rayBuffer);
	extendKernel->SetArgument(2, rayBuffer);

	resetKernel->Run(colorBuffer, triangleCount);
}

#include "tinyxml2.h"
using namespace tinyxml2;

void RayTracer::SaveRoute(char fileName[32])
{
	XMLDocument doc;
	XMLNode* root = doc.NewElement("route");
	doc.InsertFirstChild(root);
	((XMLElement*)root->InsertEndChild(doc.NewElement("lamp_sterkte")))->SetText(lightIntensity);
	((XMLElement*)root->InsertEndChild(doc.NewElement("minimale_dosering")))->SetText(minDosage);
	((XMLElement*)root->InsertEndChild(doc.NewElement("lamp_lengte")))->SetText(lightLength);
	((XMLElement*)root->InsertEndChild(doc.NewElement("lamp_height")))->SetText(lightHeight);
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
	char prefix[64] = "routes/";
	doc.SaveFile(strcat(strcat(prefix, fileName), ".xml"));
}

void RayTracer::LoadRoute(char fileName[32])
{
	char prefix[64] = "routes/";
	XMLDocument doc;
	XMLError result = doc.LoadFile(strcat(strcat(prefix, fileName), ".xml"));
	if (result != XML_SUCCESS) return;
	XMLNode* root = doc.FirstChild();
	if (root == nullptr) return;
	XMLElement* docElem;
	if ((docElem = root->FirstChildElement("lamp_sterkte"))) docElem->QueryFloatText(&lightIntensity);
	if ((docElem = root->FirstChildElement("minimale_dosering"))) docElem->QueryFloatText(&minDosage);
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
