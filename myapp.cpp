#include "precomp.h"
#include "myapp.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <tiny_gltf.h>
#include <glm/gtx/string_cast.hpp>

TheApp* CreateApp() { return new MyApp(); }

// -----------------------------------------------------------
// Initialize the application
// -----------------------------------------------------------
void MyApp::Init(GLFWwindow* window)
{
	// Initialise ImGui
	IMGUI_CHECKVERSION();
	if (!ImGui::CreateContext())
	{
		printf("ImGui::CreateContext failed.\n");
		exit(EXIT_FAILURE);
	}
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::StyleColorsDark(); // or ImGui::StyleColorsClassic();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 130");

	// Load the mesh (TODO: allow specifying model in gui and load after pushing button)
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	string err;
	string warn;
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
	if (indicesAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
	{
		unsigned int* temp;
		copy_n(&indicesBuffer.data[indicesBufferView.byteOffset + indicesAccessor.byteOffset], indicesAccessor.count, temp);
		rayTracer.triangles = temp;
		
		
	} else if (indicesAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
		rayTracer.triangles = reinterpret_cast<const unsigned int*>(&indicesBuffer.data[indicesBufferView.byteOffset + indicesAccessor.byteOffset]);
	}

	rayTracer.vertices = new float[positionAccessor.count * 5];
	for (size_t i = 0; i < positionAccessor.count; ++i) {
		rayTracer.vertices[i * 5 + 0] = positions[i * 3 + 0];
		rayTracer.vertices[i * 5 + 1] = positions[i * 3 + 1];
		rayTracer.vertices[i * 5 + 2] = positions[i * 3 + 2];
		rayTracer.vertices[i * 5 + 3] = texcoords[i * 2 + 0];
		rayTracer.vertices[i * 5 + 4] = texcoords[i * 2 + 1];
	}
	
	rayTracer.triangleCount = indicesAccessor.count;
	rayTracer.vertexCount = positionAccessor.count * 5;
	// Apparently number of triangles == 3 * number of vertices, so the vertex data must be fat even though you'd think having separate indices would allow preventing that...

	rayTracer.computeDosageMap();

	BindMesh();// TODO: This now also sets the texture, but this should be updated whenever the dosageMap is updated!
}

/**
 * \brief Bind the mesh to the OpenGL context
 */
void MyApp::BindMesh()
{
	shader3D = new ShaderGL("shader3D.vert", "shader3D.frag", false);
	glEnable(GL_CULL_FACE);

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, rayTracer.vertexCount * sizeof(float), rayTracer.vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, rayTracer.triangleCount * sizeof(unsigned int),
		rayTracer.triangles, GL_STATIC_DRAW);

	// vertex positions
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
	// vertex texture coords
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));

	int dosagePointCount = 3;

	glGenTextures(1, &texture);
	
	glBindTexture(GL_TEXTURE_2D, texture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);//TODO: ensure no mipmaps are used?
	
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
	//TODO: save the texture layout and checkers images & get rid of OpenCV
	//namedWindow("image", cv::WINDOW_AUTOSIZE);
	//cv::imshow("image", imageMatrix);
	//cv::waitKey();
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, texWidth, texHeight, 0,
		GL_RGBA, GL_FLOAT, &imageData.at(0));

	glBindTexture(GL_TEXTURE_2D, 0);
  
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
	glm::mat4 projection = glm::perspective(glm::radians(camera.FOV), (float)SCRWIDTH / (float)SCRHEIGHT, 0.1f, 100.0f);
	shader3D->SetInputMatrixGLM("projection", projection);
	shader3D->Unbind();
}

// -----------------------------------------------------------
// Main application tick function - Executed once per frame
// -----------------------------------------------------------
void MyApp::Tick(float deltaTime)
{
	screen->Clear(0);

	//printf("hello world!\n");
	// Update the camera
	camera.UpdateView(keyPresses, deltaTime);

	DrawMesh();
	DrawUI();


#if 0
	// clear the screen to black
	screen->Clear(0);
	int screenWidth = screen->width;
	int screenHeight = screen->height;

	float3 color = make_float3(0, 0, 0);
	Ray newray{};
	for (int u = 0; u < screenWidth; u++) {
		for (int v = 0; v < screenHeight; v++) {
			float closestdist = 0;//TODO: find closest triangle
			color = make_float3(0, 0, 0);
			float3 umult = (camera.p2 - camera.p1) / screenWidth;
			float3 vmult = (camera.p3 - camera.p1) / screenHeight;
			newray.origin = camera.p1 + u * umult + v * vmult;
			newray.dir = normalize(newray.origin - camera.position);
			for (int i = 0; i < rayTracer.triangleCount; i += 3)
			{
				float u, v;
				bool hit = TriangleIntersect(newray, rayTracer.vertices[rayTracer.triangles[i]], rayTracer.vertices[rayTracer.triangles[i + 1]], rayTracer.vertices[rayTracer.triangles[i + 2]], u, v);
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


#if 0

	static Kernel* kernel = 0;			// statics should be members of MyApp of course.
	static Surface bitmap(512, 512);	// having them here allows us to disable the OpenCL
	static Buffer* clBuffer = 0;		// demonstration using a single #if 0.
	static int offset = 0;
	if (!kernel)
	{
		// prepare for OpenCL work
		Kernel::InitCL();
		// compile and load kernel "render" from file "kernels.cl"
		kernel = new Kernel("cl/kernels.cl", "render");
		// create an OpenCL buffer over using bitmap.pixels
		clBuffer = new Buffer(512 * 512, Buffer::DEFAULT, bitmap.pixels);
	}
	// pass arguments to the OpenCL kernel
	kernel->SetArgument(0, clBuffer);
	kernel->SetArgument(1, offset++);
	// run the kernel; use 512 * 512 threads
	kernel->Run(512 * 512);
	// get the results back from GPU to CPU (and thus: into bitmap.pixels)
	clBuffer->CopyFromDevice();
	// show the result on screen
	bitmap.CopyTo(screen, 500, 200);

#endif

}

void MyApp::DrawMesh()
{
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

	shader3D->Bind();

	shader3D->SetInputMatrixGLM("view", camera.view);

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

	glDrawElements(GL_TRIANGLES, rayTracer.triangleCount, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	shader3D->Unbind();
}

void MyApp::DrawUI()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGui::Begin("Render statistics", 0);
	ImGui::SetWindowFontScale(1.5f);
	ImGui::Text("Triangle count: %u", rayTracer.triangleCount / 3);
	ImGui::Text("Vertex count: %u", rayTracer.vertexCount / 5);
	ImGui::InputFloat3("Light position", rayTracer.lightPos.cell);
	ImGui::End();
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	
	//rayTracer.lightPos = make_float3(lightPosInput[0], lightPosInput[1], lightPosInput[2]);
}

void MyApp::KeyDown(int key)
{
	//cout << " pressed " << key;
	switch (key)
	{
	case GLFW_KEY_W:
		keyPresses.wPress = true;
		break;
	case GLFW_KEY_A:
		keyPresses.aPress = true;
		break;
	case GLFW_KEY_S:
		keyPresses.sPress = true;
		break;
	case GLFW_KEY_D:
		keyPresses.dPress = true;
		break;
	case GLFW_KEY_Q:
		keyPresses.qPress = true;
		break;
	case GLFW_KEY_E:
		keyPresses.ePress = true;
		break;
	case GLFW_KEY_UP:
		keyPresses.upPress = true;
		break;
	case GLFW_KEY_LEFT:
		keyPresses.leftPress = true;
		break;
	case GLFW_KEY_DOWN:
		keyPresses.downPress = true;
		break;
	case GLFW_KEY_RIGHT:
		keyPresses.rightPress = true;
		break;
	case GLFW_KEY_LEFT_SHIFT:
		keyPresses.shiftPress = true;
		break;
	default:
		break;
	}
}
void MyApp::KeyUp(int key)
{
	switch (key)
	{
	case GLFW_KEY_W:
		keyPresses.wPress = false;
		break;
	case GLFW_KEY_A:
		keyPresses.aPress = false;
		break;
	case GLFW_KEY_S:
		keyPresses.sPress = false;
		break;
	case GLFW_KEY_D:
		keyPresses.dPress = false;
		break;
	case GLFW_KEY_Q:
		keyPresses.qPress = false;
		break;
	case GLFW_KEY_E:
		keyPresses.ePress = false;
		break;
	case GLFW_KEY_UP:
		keyPresses.upPress = false;
		break;
	case GLFW_KEY_LEFT:
		keyPresses.leftPress = false;
		break;
	case GLFW_KEY_DOWN:
		keyPresses.downPress = false;
		break;
	case GLFW_KEY_RIGHT:
		keyPresses.rightPress = false;
		break;
	case GLFW_KEY_LEFT_SHIFT:
		keyPresses.shiftPress = false;
		break;
	default:
		break;
	}
}

void MyApp::Shutdown()
{
	camera.Save();
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	//nanogui::shutdown();
}
