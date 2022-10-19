#include "precomp.h"

void RayTracer::Init()
{
	LightPos initLightPos;
	initLightPos.position = make_float3(0.4f, 0.6f + floorOffset, 0.5f);
	initLightPos.duration = 1;
	lightPositions.push_back(initLightPos);

	initLightPos.position = make_float3(-0.1f, 0.6f + floorOffset, -1.9f);
	initLightPos.duration = 1;
	lightPositions.push_back(initLightPos);

	// compile and load kernel "render" from file "kernels.cl"
	generateKernel = new Kernel("generate.cl", "render");
	extendKernel = new Kernel("extend.cl", "render");
	shadeKernel = new Kernel("shade.cl", "render");
	//TODO: new shade kernel for determining triangle color & updating opengl texture
	// create an OpenCL buffer over using bitmap.pixels
	photonMapBuffer = new Buffer(vertexCount / 9, Buffer::DEFAULT);//Texture not necessary as per triangle dosage can be done with OpenCL as well.
	rayBuffer = new Buffer(8*photonCount, Buffer::DEFAULT);//*8 because buffer is in uints
	//lightPosBuffer = new Buffer(4 * lightPositions.size(), Buffer::DEFAULT, lightPositions.data());

	//triangleBuffer = new Buffer(triangleCount, Buffer::DEFAULT, triangles);	
	verticesBuffer = new Buffer(vertexCount, Buffer::DEFAULT, vertices);

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
	shadeKernel->SetArgument(2, dosageBuffer);
	shadeKernel->SetArgument(3, verticesBuffer);

	verticesBuffer->CopyToDevice();
}

void RayTracer::ComputeDosageMap()
{
	cout << " beforecompute: " << timerClock.elapsed() * 1000.0f << endl;
	int photonsPerLight = photonCount / lightPositions.size();
	for (int i = 0; i < lightPositions.size(); ++i)
	{

		generateKernel->SetArgument(1, lightPositions[i].position);
		generateKernel->SetArgument(2, lightLength);
		generateKernel->SetArgument(3, (int)lightPositions.size());
		generateKernel->Run(photonsPerLight);
		clFinish(Kernel::GetQueue());
		cout << " generated: " <<  timerClock.elapsed() * 1000.0f << endl;
		
		extendKernel->SetArgument(1, photonMapSize);
		extendKernel->Run(photonsPerLight);
		clFinish(Kernel::GetQueue());
		cout << " extended: " <<  timerClock.elapsed() * 1000.0f << endl;

		//TODO: separate kernel to multiply photoncount per triangle by the timestep and light power?
		//TODO: why at the ~third to last iteraion do red outliers suddenly show up?

		photonMapSize += photonsPerLight;
	}
	// The number of photons per area should be divided by the number of photons per light,
	// as each photon carries a fraction of a single light's power
	shadeKernel->SetArgument(1, photonMapSize);
	shadeKernel->SetArgument(4, photonMapSize / (int)lightPositions.size());
	
	shadeKernel->Run(dosageBuffer, vertexCount / 9);
	clFinish(Kernel::GetQueue());
	//clFinish(Kernel::GetQueue());//todo
	cout << " photon count: " << photonMapSize << " delta time: " << timerClock.elapsed() * 1000.0f << endl;
	
}

void RayTracer::ResetDosageMap() {
	photonMapSize = 0;
	shadeKernel->SetArgument(1, 0);
	shadeKernel->SetArgument(4, 0);//TODO:bad idea

	shadeKernel->Run(dosageBuffer, vertexCount / 9);
}
