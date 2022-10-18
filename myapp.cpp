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
	seed = time(0);

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

	LoadMesh();

	Kernel::InitCL();
	cout << "Initialised OpenCL " << endl;
	BindMesh();
	rayTracer.Init();
	rayTracer.ComputeDosageMap();
	//UpdateDosageMap();
}

void MyApp::LoadMesh()
{
	cout << "Loading mesh " << endl;
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
	const tinygltf::Accessor& indicesAccessor = model.accessors[primitive.indices];
	const tinygltf::BufferView& positionBufferView = model.bufferViews[positionAccessor.bufferView];
	const tinygltf::BufferView& indicesBufferView = model.bufferViews[indicesAccessor.bufferView];
	const tinygltf::Buffer& positionBuffer = model.buffers[positionBufferView.buffer];
	const tinygltf::Buffer& indicesBuffer = model.buffers[indicesBufferView.buffer];
	const float* positions = reinterpret_cast<const float*>(&positionBuffer.data[positionBufferView.byteOffset + positionAccessor.byteOffset]);
	const unsigned short* temps;
	const unsigned int* tempi;
	bool shortIndices = indicesAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
	if (shortIndices)
	{
		cout << "Loading unsigned short type indices " << endl;
		temps = reinterpret_cast<const unsigned short*>(&indicesBuffer.data[indicesBufferView.byteOffset + indicesAccessor.byteOffset]);
	} else if (indicesAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
		cout << "Loading unsigned int type indices " << endl;
		tempi = reinterpret_cast<const unsigned int*>(&indicesBuffer.data[indicesBufferView.byteOffset + indicesAccessor.byteOffset]);
	}

	rayTracer.vertices = new float[indicesAccessor.count * 3];
	for (size_t i = 0; i < indicesAccessor.count; ++i) {
		rayTracer.vertices[i * 3 + 0] = positions[(shortIndices ? temps[i] : tempi[i]) * 3 + 0];
		rayTracer.vertices[i * 3 + 1] = positions[(shortIndices ? temps[i] : tempi[i]) * 3 + 1];
		rayTracer.vertices[i * 3 + 2] = positions[(shortIndices ? temps[i] : tempi[i]) * 3 + 2];
	}

	rayTracer.vertexCount = indicesAccessor.count * 3;
}

/**
 * \brief Bind the mesh to the OpenGL context
 */
void MyApp::BindMesh()
{
	shader3D = new ShaderGL("shader3D.vert", "shader3D.frag", false);
	

	cout << "Binding the mesh " << endl;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &rayTracer.dosageBufferID);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, rayTracer.vertexCount * sizeof(float), rayTracer.vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, rayTracer.dosageBufferID);
	glBufferData(GL_ARRAY_BUFFER, rayTracer.vertexCount * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
	
	// vertex positions
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
	glBindBuffer(GL_ARRAY_BUFFER, rayTracer.dosageBufferID);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
  
	shader3D->Bind();
	camera.projection = glm::perspective(glm::radians(camera.FOV), (float)SCRWIDTH / (float)SCRHEIGHT, 0.1f, 100.0f);
	shader3D->SetInputMatrixGLM("projection", camera.projection);
	shader3D->Unbind();
}

void MyApp::UpdateDosageMap()
{//Not necessary anymore, if done in OpenCL?
#ifdef GPU_RAYTRACING
	

#else
	uint dosagePointCount = rayTracer.photonMapSize;
	bool recreateTexture = dosagePointCount > texSize;
	if (recreateTexture)
	{
		texHeight = ceil(dosagePointCount / 2048.0f); // Depends on the max photon count (max tex size is 2048). * 10f means max 2 mil photons
		texWidth = 2048;
		texSize = texWidth * texHeight;
	}
	cout << "count " << dosagePointCount << " texwidth " << texWidth << " texheight " << texHeight << " texSize " << texSize << endl;
	int maxSize = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
	if (texHeight > maxSize)
		cout << " TEXTURE TOO LARGE?" << endl;
	
	vector<float> imageData;
	for (int i = 0; i < texHeight; ++i)
	{
		for (int j = 0; j < texWidth; ++j)
		{
			const uint index = i * texWidth + j;
			float4 dosagePoint = index < dosagePointCount ? rayTracer.dosageMap[index] : make_float4(-1, -1, -1, -1);
			imageData.insert(imageData.end(), {
				dosagePoint.x, dosagePoint.y, dosagePoint.z, dosagePoint.w
				});
		}
	}
	glBindTexture(GL_TEXTURE_2D, rayTracer.dosageTexture);
	if (recreateTexture)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, texWidth, texHeight, 0,
			GL_RGBA, GL_FLOAT, &imageData.at(0));
	} else {
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texWidth, texHeight,
			GL_RGBA, GL_FLOAT, &imageData.at(0));
	}
	glBindTexture(GL_TEXTURE_2D, 0);
#endif
	
}

// -----------------------------------------------------------
// Main application tick function - Executed once per frame
// -----------------------------------------------------------
void MyApp::Tick(float deltaTime)
{
	timer += deltaTime;

	// Update the camera
	camera.UpdateView(keyPresses, deltaTime);

	screen->Clear(0);
	for (int i = 0; i < rayTracer.lightPositions.size(); ++i)
	{
		float3 lightPos = rayTracer.lightPositions[i].position;
		glm::vec4 lightClipPosBottom = camera.projection * camera.view * glm::vec4(lightPos.x, lightPos.y, lightPos.z, 1);
		glm::vec4 lightClipPosTop = camera.projection * camera.view * glm::vec4(lightPos.x, lightPos.y + rayTracer.lightLength, lightPos.z, 1);
		glm::vec2 lightScreenPosBottom = ((glm::vec2(lightClipPosBottom.x, -lightClipPosBottom.y) / lightClipPosBottom.w + glm::vec2(1)) / 2.0f) * glm::vec2(SCRWIDTH, SCRHEIGHT);
		glm::vec2 lightScreenPosTop = ((glm::vec2(lightClipPosTop.x, -lightClipPosTop.y) / lightClipPosTop.w + glm::vec2(1)) / 2.0f) * glm::vec2(SCRWIDTH, SCRHEIGHT);
		//cout << "lightpos " << lightScreenPosBottom.x << " y " << lightScreenPosBottom.y << endl;
		
		screen->Line(lightScreenPosBottom.x, lightScreenPosBottom.y, lightScreenPosTop.x, lightScreenPosTop.y, MAXUINT, 3);
	}

	bool updatedMap = (timer > 0 && rayTracer.photonMapSize + rayTracer.photonCount <= rayTracer.maxPhotonCount);
	if (/*timerStart > 100 && */updatedMap) {
		timer = 0;
		rayTracer.ComputeDosageMap();
		UpdateDosageMap();
	}
	//if ((timerStart <= 100 || updatedMap || bufferSwapDraw || CameraKeyPressed())) {
	DrawMesh();
		//timerStart += deltaTime;
		//bufferSwapDraw = !bufferSwapDraw;
	//}

	DrawUI();	
}

void MyApp::DrawMesh()
{
	//glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	shader3D->Bind();

	shader3D->SetInputMatrixGLM("view", camera.view);
	glEnable(GL_CULL_FACE);
	glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLES, 0, rayTracer.vertexCount);
	glBindVertexArray(0);

	shader3D->Unbind();
}

void MyApp::DrawUI()
{
	//TODO: ensure it is drawn in front of the lights
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGui::Begin("Render statistics", 0);
	ImGui::SetWindowFontScale(1.5f);
	ImGui::Text("Triangle count: %u", rayTracer.vertexCount / 3);
	ImGui::End();

	ImGui::Begin("Light positions", 0);
	//ImGui::Text("Vertex count: %u", rayTracer.vertexCount);
	for (int i = 0; i < rayTracer.lightPositions.size(); ++i)
	{
		std::string str = "Position " + std::to_string(i);
		const char* chars = str.c_str();
		ImGui::InputFloat3(chars, rayTracer.lightPositions[i].position.cell);
			
	}
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

bool MyApp::CameraKeyPressed()
{
	return keyPresses.wPress || keyPresses.aPress || keyPresses.sPress || keyPresses.dPress || keyPresses.qPress
		|| keyPresses.ePress || keyPresses.upPress || keyPresses.leftPress || keyPresses.downPress || keyPresses.rightPress || keyPresses.leftClick;
}

void MyApp::Shutdown()
{
	camera.Save();
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &rayTracer.dosageBufferID);
	//glDeleteBuffers(1, &EBO);
	//nanogui::shutdown();
}
