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
	LightPos initLightPos;
	initLightPos.position = make_float2(0.5f, 0.5f);
	initLightPos.duration = 1;
	lightPositions.push_back(initLightPos);

	//initLightPos.position = make_float3(-0.1f, 0.6f + floorOffset, -1.9f);
	//initLightPos.duration = 1;
	//lightPositions.push_back(initLightPos);

	// compile and load kernel "render" from file "kernels.cl"
	generateKernel = new Kernel("generate.cl", "render");
	extendKernel = new Kernel("extend.cl", "render");
	shadeKernel = new Kernel("shade.cl", "render");
	resetKernel = new Kernel("reset.cl", "render");
	timeStepKernel = new Kernel("timestep.cl", "render");
	//TODO: new shade kernel for determining triangle color & updating opengl texture
	// create an OpenCL buffer over using bitmap.pixels
	photonMapBuffer = new Buffer(vertexCount / 9, Buffer::DEFAULT);//Texture not necessary as per triangle dosage can be done with OpenCL as well.
	rayBuffer = new Buffer(8*photonCount, Buffer::DEFAULT);//*8 because buffer is in uints
	//lightPosBuffer = new Buffer(4 * lightPositions.size(), Buffer::DEFAULT, lightPositions.data());

	//triangleBuffer = new Buffer(triangleCount, Buffer::DEFAULT, triangles);	

	//TODO: since one vertex belongs to multiple faces, we cannot give it a single color, as it would be wrongly interpolated in the fragment shader.
	//		Therefore a texture map should be used and sent to the fragment shader after all...
	// Or otherwise render opengl with fat triangle data after all, but use the compressed data for the OpenCL kernels?
	dosageBuffer = new Buffer(dosageBufferID, Buffer::GLARRAY | Buffer::WRITEONLY);

	generateKernel->SetArgument(0, rayBuffer);

	extendKernel->SetArgument(0, photonMapBuffer);
	extendKernel->SetArgument(2, rayBuffer);
	extendKernel->SetArgument(3, verticesBuffer);
	extendKernel->SetArgument(4, vertexCount);

	shadeKernel->SetArgument(0, photonMapBuffer);
	shadeKernel->SetArgument(1, dosageBuffer);
	shadeKernel->SetArgument(2, verticesBuffer);

	resetKernel->SetArgument(0, photonMapBuffer);
	resetKernel->SetArgument(1, dosageBuffer);

	timeStepKernel->SetArgument(0, photonMapBuffer);

}

void RayTracer::ComputeDosageMap()
{
	//cout << " beforecompute: " << timerClock.elapsed() * 1000.0f << endl;
	int photonsPerLight = photonCount / lightPositions.size();
	for (int i = 0; i < lightPositions.size(); ++i)
	{
		float3 lightposition = make_float3(lightPositions[i].position.x, lightHeight, lightPositions[i].position.y);
		generateKernel->SetArgument(1, lightposition);
		generateKernel->SetArgument(2, lightLength);
		generateKernel->SetArgument(3, (int)lightPositions.size());
		generateKernel->Run(photonsPerLight);
		//clFinish(Kernel::GetQueue());
		//cout << " generated: " <<  timerClock.elapsed() * 1000.0f << endl;
		
		extendKernel->SetArgument(1, photonMapSize);
		extendKernel->Run(photonsPerLight);

		timeStepKernel->SetArgument(1, lightPositions[i].duration);
		timeStepKernel->Run(vertexCount / 9);
		//clFinish(Kernel::GetQueue());
		//cout << " extended: " <<  timerClock.elapsed() * 1000.0f << endl;

		//TODO: separate kernel to multiply photoncount per triangle by the timestep and light power?
		//TODO: why at the ~third to last iteraion do red outliers suddenly show up?

		photonMapSize += photonsPerLight;
	}
	// The number of photons per area should be divided by the number of photons per light,
	// as each photon carries a fraction of a single light's power
	shadeKernel->SetArgument(3, photonMapSize / (int)lightPositions.size());
	shadeKernel->SetArgument(4, lightIntensity);
	shadeKernel->SetArgument(5, minDosage);
	
	shadeKernel->Run(dosageBuffer, vertexCount / 9);
	clFinish(Kernel::GetQueue());
	//clFinish(Kernel::GetQueue());//todo
	cout << " photon count: " << photonMapSize << " delta time: " << timerClock.elapsed() * 1000.0f << endl;
	
}

void RayTracer::ResetDosageMap() {
	photonMapSize = 0;

	resetKernel->Run(dosageBuffer, vertexCount / 9);
}

#include "tinyxml2.h"
using namespace tinyxml2;

void Camera::Save()
{
	XMLDocument doc;
	XMLNode* root = doc.NewElement("camera");
	doc.InsertFirstChild(root);
	XMLElement* viewElem = doc.NewElement("view");
	viewElem->SetAttribute("m00", view[0][0]);
	root->InsertEndChild(viewElem);
	((XMLElement*)root->InsertEndChild(doc.NewElement("FOV")))->SetText(FOV);
	doc.SaveFile(cameraFile);
}

void Camera::Load()
{
	XMLDocument doc;
	XMLError result = doc.LoadFile(cameraFile);
	if (result != XML_SUCCESS) return;
	XMLNode* root = doc.FirstChild();
	if (root == nullptr) return;
	XMLElement* docElem;
	if ((docElem = root->FirstChildElement("view"))) {
		docElem->QueryFloatAttribute("m00", &view[0][0]);
		docElem->QueryFloatAttribute("m01", &view[0][1]);
		docElem->QueryFloatAttribute("m02", &view[0][2]);
		docElem->QueryFloatAttribute("m03", &view[0][3]);
		docElem->QueryFloatAttribute("m10", &view[1][0]);
		docElem->QueryFloatAttribute("m11", &view[1][1]);
		docElem->QueryFloatAttribute("m12", &view[1][2]);
		docElem->QueryFloatAttribute("m13", &view[1][3]);
		docElem->QueryFloatAttribute("m20", &view[2][0]);
		docElem->QueryFloatAttribute("m21", &view[2][1]);
		docElem->QueryFloatAttribute("m22", &view[2][2]);
		docElem->QueryFloatAttribute("m23", &view[2][3]);
		docElem->QueryFloatAttribute("m30", &view[3][0]);
		docElem->QueryFloatAttribute("m31", &view[3][1]);
		docElem->QueryFloatAttribute("m32", &view[3][2]);
		docElem->QueryFloatAttribute("m33", &view[3][3]);
	}
	if ((docElem = root->FirstChildElement("FOV"))) docElem->QueryFloatText(&FOV);
}
