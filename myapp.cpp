#include "precomp.h"
#include "myapp.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include "tiny_gltf.h"

TheApp* CreateApp() { return new MyApp(); }
tinygltf::Model model;
tinygltf::TinyGLTF loader;
string err;
string warn;
string modelFile = "assets/simple.glb";//headphones

// -----------------------------------------------------------
// Initialize the application
// -----------------------------------------------------------
void MyApp::Init()
{
	//bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, modelFile);
	bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, modelFile); // for binary glTF(.glb)

	if (!warn.empty()) {
		printf("Warn: %s\n", warn.c_str());
	}

	if (!err.empty()) {
		printf("Err: %s\n", err.c_str());
	}

	if (!ret) {
		printf("Failed to parse glTF\n");
		return;
	}
	tinygltf::Primitive& primitive = model.meshes[0].primitives[0];
	printf("hello world! %i \n");
	const tinygltf::Accessor& positionAccessor = model.accessors[primitive.attributes["POSITION"]];
	const tinygltf::Accessor& indicesAccessor = model.accessors[primitive.indices];
	const tinygltf::BufferView& positionBufferView = model.bufferViews[positionAccessor.bufferView];
	const tinygltf::BufferView& indicesBufferView = model.bufferViews[indicesAccessor.bufferView];
	const tinygltf::Buffer& positionBuffer = model.buffers[positionBufferView.buffer];
	const tinygltf::Buffer& indicesBuffer = model.buffers[indicesBufferView.buffer];
	const float* positions = reinterpret_cast<const float*>(&positionBuffer.data[positionBufferView.byteOffset + positionAccessor.byteOffset]);
	const unsigned short* indices = reinterpret_cast<const unsigned short*>(&indicesBuffer.data[indicesBufferView.byteOffset + indicesAccessor.byteOffset]);

	rayTracer.triangles = indices; // Point triangles towards the indices
	// Convert the position array into float3*
	rayTracer.vertices = new float3[positionAccessor.count];
	for (size_t i = 0; i < positionAccessor.count; ++i) {
		rayTracer.vertices[i] = make_float3(positions[i * 3 + 0], positions[i * 3 + 1], positions[i * 3 + 2]);
		//std::cout << "(" << positions[indices[i] * 3 + 0] << ", "// x
		//	<< positions[indices[i] * 3 + 1] << ", " // y
		//	<< positions[indices[i] * 3 + 2] << ")" // z
		//	<< "\n";
	}

	rayTracer.triangleCount = indicesAccessor.count;
	rayTracer.vertexCount = positionAccessor.count;
}

// -----------------------------------------------------------
// Main application tick function - Executed once per frame
// -----------------------------------------------------------
void MyApp::Tick( float deltaTime )
{
	printf("hello world!\n");
	// Update the camera
	camera.UpdateView();

	// NOTE: clear this function before actual use; code is only for 
	// demonstration purposes. See _ getting started.pdf for details.
	
	// clear the screen to black
	screen->Clear( 0 );
	int screenWidth = screen->width;
	int screenHeight = screen->height;

	float3 color = make_float3(0,0,0);
	Ray newray{};
	for (int u = 0; u < screenWidth; u++) {
		for (int v = 0; v < screenHeight; v++) {
			float closestdist = 0;//TODO: find closest triangle
			color = make_float3(0, 0, 0);
			float3 umult = (camera.p2 - camera.p1) / screenWidth;
			float3 vmult = (camera.p3 - camera.p1) / screenHeight;
			newray.origin = camera.p1 + u * umult + v * vmult;
			newray.dir = normalize(newray.origin - camera.position);
			for (int i = 0; i < rayTracer.triangleCount; i+=3)
			{
				float u, v;
				bool hit = TriangleIntersect(newray, rayTracer.vertices[rayTracer.triangles[i]], rayTracer.vertices[rayTracer.triangles[i+1]], rayTracer.vertices[rayTracer.triangles[i+2]], u, v);
				if (hit)
				{
					//printf("ray hit %f p1: %f  %f %f \n", newray.dist * newray.dist * 0.008F, camera.p1.x, camera.p1.y, camera.p1.z);
					color = make_float3(newray.dist * newray.dist * 0.006F, newray.dist * newray.dist * 0.006F, newray.dist * newray.dist * 0.006F);
					break;
				}
			}

			//color = raytracer.hostcolorBuffer[u + v * (screenWidth)];
			screen->Plot(u, v, ((int)(min(color.z, 1.0f) * 255.0f) << 16) +
				((int)(min(color.y, 1.0f) * 255.0f) << 8) + (int)(min(color.x, 1.0f) * 255.0f));
		}
	}

	//// plot some colors
	//for (int red = 0; red < 256; red++) for (int green = 0; green < 256; green++)
	//{
	//	int x = red, y = green;
	//	screen->Plot( x + 200, y + 100, (red << 16) + (green << 8) );
	//}
	// plot a white pixel in the bottom right corner
	//screen->Plot( SCRWIDTH - 2, SCRHEIGHT - 2, 0xffffff );

#if 0

	static Kernel* kernel = 0;			// statics should be members of MyApp of course.
	static Surface bitmap( 512, 512 );	// having them here allows us to disable the OpenCL
	static Buffer* clBuffer = 0;		// demonstration using a single #if 0.
	static int offset = 0;
	if (!kernel)
	{
		// prepare for OpenCL work
		Kernel::InitCL();		
		// compile and load kernel "render" from file "kernels.cl"
		kernel = new Kernel( "cl/kernels.cl", "render" );
		// create an OpenCL buffer over using bitmap.pixels
		clBuffer = new Buffer( 512 * 512, Buffer::DEFAULT, bitmap.pixels );
	}
	// pass arguments to the OpenCL kernel
	kernel->SetArgument( 0, clBuffer );
	kernel->SetArgument( 1, offset++ );
	// run the kernel; use 512 * 512 threads
	kernel->Run( 512 * 512 );
	// get the results back from GPU to CPU (and thus: into bitmap.pixels)
	clBuffer->CopyFromDevice();
	// show the result on screen
	bitmap.CopyTo( screen, 500, 200 );

#endif

}
