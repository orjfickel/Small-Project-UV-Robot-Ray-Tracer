#include "precomp.h"
#include "myapp.h"
#include "glm/glm.hpp"
#include "glm/ext.hpp"
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>

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
Shader* shader3D;

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
	printf("hello world! \n");
	const tinygltf::Accessor& positionAccessor = model.accessors[primitive.attributes["POSITION"]];
	const tinygltf::Accessor& texcoordAccessor = model.accessors[primitive.attributes["TEXCOORD_0"]];
	const tinygltf::Accessor& indicesAccessor = model.accessors[primitive.indices];
	const tinygltf::BufferView& positionBufferView = model.bufferViews[positionAccessor.bufferView];
	const tinygltf::BufferView& texcoordBufferView = model.bufferViews[texcoordAccessor.bufferView];
	const tinygltf::BufferView& indicesBufferView = model.bufferViews[indicesAccessor.bufferView];
	const tinygltf::Buffer& positionBuffer = model.buffers[positionBufferView.buffer];
	const tinygltf::Buffer& texcoordBuffer = model.buffers[texcoordBufferView.buffer];
	const tinygltf::Buffer& indicesBuffer = model.buffers[indicesBufferView.buffer];
	const float* positions = reinterpret_cast<const float*>(&positionBuffer.data[positionBufferView.byteOffset + positionAccessor.byteOffset]);
	const float* texcoords = reinterpret_cast<const float*>(&texcoordBuffer.data[texcoordBufferView.byteOffset + texcoordAccessor.byteOffset]);
	rayTracer.triangles = reinterpret_cast<const unsigned short*>(&indicesBuffer.data[indicesBufferView.byteOffset + indicesAccessor.byteOffset]);

#if 1
	//rayTracer.triangles = indices; // Point triangles towards the indices
	// Convert the position array into float3*
	rayTracer.vertices = new float[positionAccessor.count * 5];
	for (size_t i = 0; i < positionAccessor.count; ++i) {
		rayTracer.vertices[i * 5 + 0] = positions[i * 3 + 0];
		rayTracer.vertices[i * 5 + 1] = positions[i * 3 + 1];
		rayTracer.vertices[i * 5 + 2] = positions[i * 3 + 2];
		rayTracer.vertices[i * 5 + 3] = texcoords[i * 2 + 0];
		rayTracer.vertices[i * 5 + 4] = texcoords[i * 2 + 1];

		//std::cout << "(" << positions[indices[i] * 3 + 0] << ", "// x
		//	<< positions[indices[i] * 3 + 1] << ", " // y
		//	<< positions[indices[i] * 3 + 2] << ")" // z
		//	<< "\n";

		//cout << rayTracer.triangles[i] << " ind" << endl;

		// Insert the vertices for the OpenGL rendering
		//vertices.insert(vertices.end(),
			//{ positions[i * 3 + 0], positions[i * 3 + 1], positions[i * 3 + 2], texcoords[i * 2 + 0], texcoords[i * 2 + 1] });
	}

	/*for (size_t i = 0; i < indicesAccessor.count; ++i) {
		vertices.insert(vertices.end(),
			{ positions[rayTracer.triangles[i] * 3 + 0], positions[rayTracer.triangles[i] * 3 + 1], positions[rayTracer.triangles[i] * 3 + 2], texcoords[rayTracer.triangles[i] * 2 + 0], texcoords[rayTracer.triangles[i] * 2 + 1]});
	}*/
	//MyApp::indices = indices;

	rayTracer.triangleCount = indicesAccessor.count;
	rayTracer.vertexCount = positionAccessor.count * 5;
	std::cout << rayTracer.triangleCount << endl;
	std::cout << rayTracer.vertexCount << endl;
#endif

	BindMesh();
}

void MyApp::BindMesh()
{
	shader3D = new Shader("shader3D.vert", "shader3D.frag", false);

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, rayTracer.vertexCount * sizeof(float), rayTracer.vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, rayTracer.triangleCount * sizeof(unsigned short),
		rayTracer.triangles, GL_STATIC_DRAW);

	// vertex positions
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
	// vertex texture coords
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));

	int dosagePointCount = 3;
	if (model.textures.size() > 0) {
		tinygltf::Texture& tex = model.textures[model.materials[0].pbrMetallicRoughness.baseColorTexture.index];

		glGenTextures(1, &texture);

		tinygltf::Image& image = model.images[tex.source];

		glBindTexture(GL_TEXTURE_2D, texture);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		GLenum format = GL_RGBA;

		if (image.component == 1) {
			format = GL_RED;
		}
		else if (image.component == 2) {
			format = GL_RG;
		}
		else if (image.component == 3) {
			format = GL_RGB;
		}

		GLenum type = image.bits == 16 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_BYTE;

		cout << "format " << format << endl;
		cout << "type " << type << endl;
		//for (int i = 0; i < rayTracer.vertexCount; ++i)
		//{
		//	//if (rayTracer.vertices[i * 5 + 2] > 0)// If z > 0
		//		//continue;

		//	size_t v = rayTracer.vertices[i * 5 + 4] * vstride;
		//	size_t u = rayTracer.vertices[i * 5 + 3] * image.component;
		//	//TODO: how to convert textcoords into actual uv coords?
		//	cout << "uv " << u + v << " u " << rayTracer.vertices[i * 5 + 3] << " v " << rayTracer.vertices[i * 5 + 4] << endl;
		//	
		//	image.image[v + u + 0] = 255;
		//	image.image[v + u + 1] = 255;
		//	image.image[v + u + 2] = 255;
		//}

		//rayTracer.dosageMap.push_back(make_float4(0, 0, 0, 900));
		rayTracer.dosageMap.push_back(make_float4(0.8, 0, 0, 100));
		rayTracer.dosageMap.push_back(make_float4(-0.5f, 0.4, -0.5f, 400));
		rayTracer.dosageMap.push_back(make_float4(0, 0, 0.8, 1));

		int texWidth = ceil(dosagePointCount / 10.0f); // Depends on the max photon count (max tex size is 2048). * 10f means max 2 mil photons
		int texHeight = ceil(dosagePointCount / texWidth);

		size_t vstride = texWidth * 4;
		vector<float> imageData;
		for (int i = 0; i < texHeight; ++i)
		{
			for (int j = 0; j < texWidth; ++j)
			{
				size_t v = i * vstride;
				size_t u = j * 4;
				int index = j * texWidth + i;
				float4 dosagePoint = index < texWidth* texHeight ? rayTracer.dosageMap[index] : make_float4(-1, -1, -1, -1);
				imageData.insert(imageData.end(), { 
					dosagePoint.x, dosagePoint.y, dosagePoint.z, dosagePoint.w
				});
			}
		}
		//namedWindow("image", cv::WINDOW_AUTOSIZE);
		//cv::imshow("image", imageMatrix);
		//cv::waitKey();
		//cout << imageMatrix.size;

		//cv::Mat flat = imageMatrix.reshape(1, imageMatrix.total() * imageMatrix.channels());
		//vector<unsigned char> newImage = imageMatrix.isContinuous() ? flat : flat.clone();


		
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, texWidth, texHeight, 0,
			GL_RGBA, GL_FLOAT, &imageData.at(0));

		glBindTexture(GL_TEXTURE_2D, 0);
	}
  
#if 0
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	// set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// set texture filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	// load image, create texture and generate mipmaps
	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
	unsigned char* data = stbi_load("container.jpg", &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		//glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);
	glBindTexture(GL_TEXTURE_2D, 0);
	CheckGL();
#endif

	shader3D->Bind();
	shader3D->SetInt("pointCount", dosagePointCount);
	shader3D->SetInt("tex", 0);
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCRWIDTH / (float)SCRHEIGHT, 0.1f, 100.0f);
	shader3D->SetInputMatrixGLM("projection", projection);
	shader3D->Unbind();
}

void MyApp::DrawMesh()
{
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

	shader3D->Bind();

	//mat4 view = mat4::Identity(); // make sure to initialize matrix to identity matrix first
	//float radius = 10.0f;
	//float camX = static_cast<float>(sin(glfwGetTime()) * radius);
	//float camZ = static_cast<float>(cos(glfwGetTime()) * radius);
	//view = mat4::LookAt(float3(camX, 0.0f, camZ), float3(0.0f, 0.0f, 0.0f), float3(0.0f, 1.0f, 0.0f));

	glm::mat4 view = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
	view = glm::lookAt(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	view = translate(view, glm::vec3(camera.position.x, camera.position.y, camera.position.z));
	shader3D->SetInputMatrixGLM("view", view);

	// draw mesh
	glBindVertexArray(VAO);

	//mat4 model = mat4::Identity();
	//model = mat4::translate(model, cubePositions[i]);
	//float angle = 20.0f * i;
	//model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
	//glm::mat4 model = glm::mat4(1.0f);
	//model = glm::translate(model, {0,0,0});
	//model = glm::rotate(model, glm::radians(0.0f), glm::vec3(1.0f, 0.3f, 0.5f));
	//shader3D->SetInputMatrixGLM("model", model);
	//glDrawArrays(GL_TRIANGLES, 0, vertices.size()/5.0f);

	glDrawElements(GL_TRIANGLES, rayTracer.triangleCount, GL_UNSIGNED_SHORT, 0);
	glBindVertexArray(0);

	shader3D->Unbind();
}

// -----------------------------------------------------------
// Main application tick function - Executed once per frame
// -----------------------------------------------------------
void MyApp::Tick( float deltaTime )
{
	//printf("hello world!\n");
	// Update the camera
	//camera.UpdateView();

	DrawMesh();

	// NOTE: clear this function before actual use; code is only for 
	// demonstration purposes. See _ getting started.pdf for details.

#if 0
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
#endif

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

void MyApp::Shutdown()
{
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
}
