#include "precomp.h"
#include "myapp.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.

#include <tiny_gltf.h>
#include <glm/gtx/string_cast.hpp>

TheApp* CreateApp() { return new MyApp(); }

// -----------------------------------------------------------
// Initialize the application
// -----------------------------------------------------------
void MyApp::Init(GLFWwindow* window, UserInterface* userInterface)
{
	seed = time(0);

	this->userInterface = userInterface;
	userInterface->Init(window, &rayTracer);
	Kernel::InitCL();
	cout << "Initialised OpenCL " << endl;

	LoadMesh();

	BindMesh();
	rayTracer.Init();
	//rayTracer.ResetDosageMap();
	//rayTracer.ComputeDosageMap();
	//UpdateDosageMap();
}

void MyApp::LoadMesh()
{
	cout << "Loading mesh " << endl;
	// Load the mesh (TODO: allow specifying model in gui and load after pushing button)
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

	rayTracer.triangles = new Tri[indicesAccessor.count / 3];
	rayTracer.vertices = new float[indicesAccessor.count * 3];
	uvcoords = new float[indicesAccessor.count * 2];
	for (size_t i = 0; i < indicesAccessor.count/3; ++i) {
		//cout << " vertexposz: " << positions[(shortIndices ? temps[i] : tempi[i]) * 3 + 2] << endl;
		Tri* triangle = &rayTracer.triangles[i];
		int v1ID = i * 3 + 0, v2ID = i * 3 + 1, v3ID = i * 3 + 2;
		triangle->vertex0 = make_float3(
			positions[(shortIndices ? temps[v1ID] : tempi[v1ID]) * 3 + 0],
			positions[(shortIndices ? temps[v1ID] : tempi[v1ID]) * 3 + 1],
			positions[(shortIndices ? temps[v1ID] : tempi[v1ID]) * 3 + 2]);
		triangle->vertex1 = make_float3(
			positions[(shortIndices ? temps[v2ID] : tempi[v2ID]) * 3 + 0],
			positions[(shortIndices ? temps[v2ID] : tempi[v2ID]) * 3 + 1],
			positions[(shortIndices ? temps[v2ID] : tempi[v2ID]) * 3 + 2]);
		triangle->vertex2 = make_float3(
			positions[(shortIndices ? temps[v3ID] : tempi[v3ID]) * 3 + 0],
			positions[(shortIndices ? temps[v3ID] : tempi[v3ID]) * 3 + 1],
			positions[(shortIndices ? temps[v3ID] : tempi[v3ID]) * 3 + 2]);
		rayTracer.vertices[i * 9 + 0] = triangle->vertex0.x;
		rayTracer.vertices[i * 9 + 1] = triangle->vertex0.y;
		rayTracer.vertices[i * 9 + 2] = triangle->vertex0.z;
		rayTracer.vertices[i * 9 + 3] = triangle->vertex1.x;
		rayTracer.vertices[i * 9 + 4] = triangle->vertex1.y;
		rayTracer.vertices[i * 9 + 5] = triangle->vertex1.z;
		rayTracer.vertices[i * 9 + 6] = triangle->vertex2.x;
		rayTracer.vertices[i * 9 + 7] = triangle->vertex2.y;
		rayTracer.vertices[i * 9 + 8] = triangle->vertex2.z;
		uvcoords[i * 6 + 0] = texcoords[(shortIndices ? temps[v1ID] : tempi[v1ID]) * 2 + 0];
		uvcoords[i * 6 + 1] = texcoords[(shortIndices ? temps[v1ID] : tempi[v1ID]) * 2 + 1];
		uvcoords[i * 6 + 2] = texcoords[(shortIndices ? temps[v2ID] : tempi[v2ID]) * 2 + 0];
		uvcoords[i * 6 + 3] = texcoords[(shortIndices ? temps[v2ID] : tempi[v2ID]) * 2 + 1];
		uvcoords[i * 6 + 4] = texcoords[(shortIndices ? temps[v3ID] : tempi[v3ID]) * 2 + 0];
		uvcoords[i * 6 + 5] = texcoords[(shortIndices ? temps[v3ID] : tempi[v3ID]) * 2 + 1];
	}

	rayTracer.vertexCount = indicesAccessor.count * 3;
	rayTracer.triangleCount = indicesAccessor.count / 3;
	rayTracer.verticesBuffer = new Buffer(rayTracer.triangleCount * 16, Buffer::DEFAULT, rayTracer.triangles);
	rayTracer.verticesBuffer->CopyToDevice();	
}

/**
 * \brief Bind the mesh to the OpenGL context
 */
void MyApp::BindMesh()
{
	shader3D = new ShaderGL("shader3D.vert", "shader3D.frag", false);
	rayTracer.simpleShader = new ShaderGL("simpleshader.vert", "simpleshader.frag", false);

	cout << "Binding the mesh " << endl;
	glGenVertexArrays(1, &VAO); 
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &UVBuffer);
	glGenBuffers(1, &rayTracer.dosageBufferID);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, rayTracer.vertexCount * sizeof(float), rayTracer.vertices, GL_STATIC_DRAW);
	delete[] rayTracer.vertices;// Free up memory on host
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	glBindBuffer(GL_ARRAY_BUFFER, UVBuffer);
	glBufferData(GL_ARRAY_BUFFER, (rayTracer.vertexCount * 2 / 3) * sizeof(float), uvcoords, GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	glBindBuffer(GL_ARRAY_BUFFER, rayTracer.dosageBufferID);
	glBufferData(GL_ARRAY_BUFFER, rayTracer.vertexCount * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
		
	if (model.textures.size() > 0) {
		tinygltf::Texture& tex = model.textures[model.materials[0].pbrMetallicRoughness.baseColorTexture.index];

		if (tex.source > -1) {

			glGenTextures(1, &textureBuffer);

			tinygltf::Image& image = model.images[tex.source];

			glBindTexture(GL_TEXTURE_2D, textureBuffer);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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
			else {
				// ???
			}

			GLenum type = GL_UNSIGNED_BYTE;
			if (image.bits == 8) {
				// ok
			}
			else if (image.bits == 16) {
				type = GL_UNSIGNED_SHORT;
			}
			else {
				// ???
			}

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0,
				format, type, &image.image.at(0));

			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}

	camera.projection = glm::perspective(glm::radians(camera.FOV), (float)SCRWIDTH / (float)SCRHEIGHT, 0.1f, 100.0f);

	shader3D->Bind();
	shader3D->SetInputMatrixGLM("projection", camera.projection);
	shader3D->Unbind();
	rayTracer.simpleShader->Bind();
	rayTracer.simpleShader->SetInt("tex", 0);
	rayTracer.simpleShader->SetInputMatrixGLM("projection", camera.projection);
	rayTracer.simpleShader->Unbind();
}

//void MyApp::UpdateDosageMap()
//{//Not necessary anymore, if done in OpenCL?
//#ifdef GPU_RAYTRACING
//	
//
//#else
//	uint dosagePointCount = rayTracer.photonMapSize;
//	bool recreateTexture = dosagePointCount > texSize;
//	if (recreateTexture)
//	{
//		texHeight = ceil(dosagePointCount / 2048.0f); // Depends on the max photon count (max tex size is 2048). * 10f means max 2 mil photons
//		texWidth = 2048;
//		texSize = texWidth * texHeight;
//	}
//	cout << "count " << dosagePointCount << " texwidth " << texWidth << " texheight " << texHeight << " texSize " << texSize << endl;
//	int maxSize = 0;
//	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
//	if (texHeight > maxSize)
//		cout << " TEXTURE TOO LARGE?" << endl;
//	
//	vector<float> imageData;
//	for (int i = 0; i < texHeight; ++i)
//	{
//		for (int j = 0; j < texWidth; ++j)
//		{
//			const uint index = i * texWidth + j;
//			float4 dosagePoint = index < dosagePointCount ? rayTracer.dosageMap[index] : make_float4(-1, -1, -1, -1);
//			imageData.insert(imageData.end(), {
//				dosagePoint.x, dosagePoint.y, dosagePoint.z, dosagePoint.w
//				});
//		}
//	}
//	glBindTexture(GL_TEXTURE_2D, rayTracer.dosageTexture);
//	if (recreateTexture)
//	{
//		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, texWidth, texHeight, 0,
//			GL_RGBA, GL_FLOAT, &imageData.at(0));
//	} else {
//		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texWidth, texHeight,
//			GL_RGBA, GL_FLOAT, &imageData.at(0));
//	}
//	glBindTexture(GL_TEXTURE_2D, 0);
//#endif
//	
//}

// -----------------------------------------------------------
// Main application tick function - Executed once per frame
// -----------------------------------------------------------
void MyApp::Tick(float deltaTime)
{
	//cout << " time since last frame: " << deltaTime << endl;
	rayTracer.timer += deltaTime;
	if (rayTracer.progressTextTimer > 0) rayTracer.progressTextTimer -= deltaTime;

	bool moveLightPos = userInterface->selectedLightPos > -1 && userInterface->selectedLightPos < rayTracer.lightPositions.size();
	if (moveLightPos)
	{
		userInterface->MoveLightPos(keyPresses, deltaTime, camera.view);
	}
	// Update the camera
	camera.UpdateView(keyPresses, deltaTime, !moveLightPos);

	screen->Clear(0);
	//TODO: move to separate function
	if (userInterface->showLights) {
		for (int i = 0; i < rayTracer.lightPositions.size(); ++i)
		{
			float3 lightPos = make_float3(rayTracer.lightPositions[i].position.x, rayTracer.lightHeight, rayTracer.lightPositions[i].position.y);
			glm::vec4 lightClipPosBottom = camera.projection * camera.view * glm::vec4(lightPos.x, lightPos.y, lightPos.z, 1);
			glm::vec4 lightClipPosTop = camera.projection * camera.view * glm::vec4(lightPos.x, lightPos.y + rayTracer.lightLength, lightPos.z, 1);
			glm::vec2 lightScreenPosBottom = ((glm::vec2(lightClipPosBottom.x, -lightClipPosBottom.y) / lightClipPosBottom.w + glm::vec2(1)) / 2.0f) * glm::vec2(SCRWIDTH, SCRHEIGHT);
			glm::vec2 lightScreenPosTop = ((glm::vec2(lightClipPosTop.x, -lightClipPosTop.y) / lightClipPosTop.w + glm::vec2(1)) / 2.0f) * glm::vec2(SCRWIDTH, SCRHEIGHT);
			//cout << "lightpos " << lightScreenPosBottom.x << " y " << lightScreenPosBottom.y << endl;

			screen->Line(lightScreenPosBottom.x, lightScreenPosBottom.y, lightScreenPosTop.x, lightScreenPosTop.y,
				userInterface->selectedLightPos == i ? 255 | 170 << 8 | 170 << 16 | 255 << 24
				: 255 | 255 << 8 | 255 << 16 | 255 << 24, 3);
		}
	}

	if (!rayTracer.reachedMaxPhotons) { // Only check if we reached the max photon count if we haven't already		
		rayTracer.reachedMaxPhotons = rayTracer.currIterations >= rayTracer.maxIterations;
		if (!rayTracer.reachedMaxPhotons) {
			rayTracer.ComputeDosageMap();
			if (rayTracer.viewMode == texture) rayTracer.viewMode = dosage;
			rayTracer.currIterations++;
			rayTracer.progress = 100.0f * (float)rayTracer.currIterations / (float)rayTracer.maxIterations;

			clFinish(Kernel::GetQueue());// Make sure heatmap computation is finished
			float time = rayTracer.timerClock.elapsed();
			rayTracer.compTime += time;
			cout << "Progress: " << rayTracer.progress << "% photon count: " << rayTracer.photonMapSize << " delta time: " << rayTracer.timerClock.elapsed() * 1000.0f << 
				" total time: " << rayTracer.compTime * 1000.0f << endl;
			rayTracer.timer = 0;
			rayTracer.timerClock.reset();
		} else // We have reached the max number of iterations
		{
			// Set the timer to display the "computation done" popup text
			rayTracer.progressTextTimer = 1000;
		}
	}

	//cout << " beforedraw: " << rayTracer.timerClock.elapsed() * 1000.0f << endl;
	//if ((timerStart <= 100 || updatedMap || bufferSwapDraw || CameraKeyPressed())) {
	DrawMesh();
		//timerStart += deltaTime;
		//bufferSwapDraw = !bufferSwapDraw;
	//}
	//cout << " afterdraw: " << rayTracer.timerClock.elapsed() * 1000.0f << endl;
}

void MyApp::DrawMesh()
{
	//cout << "drawmesh photon count: " << rayTracer.photonMapSize << " delta time: " << rayTracer.timerClock.elapsed() * 1000.0f << endl;

	//glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	ShaderGL* shader;
	if (rayTracer.viewMode != texture)
		shader = shader3D;
	else {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureBuffer);
		shader = rayTracer.simpleShader;
	}

	shader->Bind();

	shader->SetInputMatrixGLM("view", camera.view);
	glEnable(GL_CULL_FACE);
	glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLES, 0, rayTracer.vertexCount);
	glBindVertexArray(0);

	shader->Unbind();
	//cout << "drawmesh2 photon count: " << rayTracer.photonMapSize << " delta time: " << rayTracer.timerClock.elapsed() * 1000.0f << endl;
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
	rayTracer.SaveRoute(rayTracer.defaultRouteFile);
	camera.Save();
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &rayTracer.dosageBufferID);
	//glDeleteBuffers(1, &EBO);
	//nanogui::shutdown();
}
