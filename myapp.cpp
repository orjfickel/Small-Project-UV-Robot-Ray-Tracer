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

	mesh.LoadMesh(modelFile);

	BindMesh();
	// Free up memory on host
	delete[] mesh.vertices;

	rayTracer.Init(&mesh);
	
	//rayTracer.ResetDosageMap();
	//rayTracer.ComputeDosageMap();
	//UpdateDosageMap();
}


/**
 * \brief Bind the mesh to the OpenGL context
 */
void MyApp::BindMesh()
{
	shader3D = new ShaderGL("shaders/shader3D.vert", "shaders/shader3D.frag", false);
	rayTracer.simpleShader = new ShaderGL("shaders/simpleshader.vert", "shaders/simpleshader.frag", false);

	cout << "Binding the mesh " << endl;
	glGenVertexArrays(1, &VAO); 
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &UVBuffer);
	glGenBuffers(1, &rayTracer.dosageBufferID);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, mesh.vertexCount * sizeof(float), mesh.vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	glBindBuffer(GL_ARRAY_BUFFER, UVBuffer);
	glBufferData(GL_ARRAY_BUFFER, (mesh.vertexCount * 2 / 3) * sizeof(float), mesh.uvcoords, GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	glBindBuffer(GL_ARRAY_BUFFER, rayTracer.dosageBufferID);
	glBufferData(GL_ARRAY_BUFFER, mesh.vertexCount * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
		
	if (mesh.model.textures.size() > 0) {
		tinygltf::Texture& tex = mesh.model.textures[mesh.model.materials[0].pbrMetallicRoughness.baseColorTexture.index];

		if (tex.source > -1) {

			glGenTextures(1, &textureBuffer);

			tinygltf::Image& image = mesh.model.images[tex.source];

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

void MyApp::Draw3DLine(glm::vec3 bottom, glm::vec3 top, uint color)
{
	glm::vec4 lightClipPosBottom = camera.projection * camera.view * glm::vec4(bottom, 1);
	glm::vec4 lightClipPosTop = camera.projection * camera.view * glm::vec4(top, 1);
	glm::vec2 lightScreenPosBottom = ((glm::vec2(lightClipPosBottom.x, -lightClipPosBottom.y) / lightClipPosBottom.w + glm::vec2(1)) / 2.0f) * glm::vec2(SCRWIDTH, SCRHEIGHT);
	glm::vec2 lightScreenPosTop = ((glm::vec2(lightClipPosTop.x, -lightClipPosTop.y) / lightClipPosTop.w + glm::vec2(1)) / 2.0f) * glm::vec2(SCRWIDTH, SCRHEIGHT);
	screen->Line(lightScreenPosBottom.x, lightScreenPosBottom.y, lightScreenPosTop.x, lightScreenPosTop.y,
		color, 3);
}
uint MyApp::greyscale_to_heatmap(float intensity) {
	float minDosageColor = 0.5f;
	float upperHalfColor = minDosageColor + (1.0 - minDosageColor) / 2;
	float lowerHalfColor = minDosageColor / 2.0f;
	if (intensity > minDosageColor) {
		if (intensity > upperHalfColor)
			return (255 | ((int)((1.0f - intensity) / (1.0f - upperHalfColor)) << 8) | 0 << 16 | 255 << 24);
		else
			return (int)((intensity - minDosageColor) / (upperHalfColor - minDosageColor)) | 255 << 8 | 0 << 16 | 255 << 24;
	}
	else {
		if (intensity > lowerHalfColor)
			return (0 | 255 << 8 | (int)((minDosageColor - intensity) / (minDosageColor - lowerHalfColor)) << 16 | 255 << 24);
		else
			return (0 | ((int)((intensity) / (lowerHalfColor)) << 8) | 255 << 16 | 255 << 24);
	}
}
void MyApp::drawBVH()
{
	static bool firsttime = true;
	BVHNode* node = &mesh.bvh->bvhNode[0], * stack[64], * newstack[64];
	int stackPtr = 0, newStackPtr = 0;
	stack[stackPtr] = node;
	int levels = 8;
	for (int i = 0; i < levels; ++i)
	{
		while (stackPtr >= 0)
		{
			node = stack[stackPtr--];
			//cout << "nodex " << node->aabbMin.x << " nodey " << node->aabbMin.y << " nodez " << node->aabbMin.z
			//	<< " maxx " << node->aabbMax.x << " maxy " << node->aabbMax.y << " maxz " << node->aabbMax.z << " tricount " << node->triCount << endl;
			uint color = greyscale_to_heatmap((float)i / levels);
			Draw3DLine(glm::vec3(node->aabbMin.x, node->aabbMin.y, node->aabbMin.z), glm::vec3(node->aabbMax.x, node->aabbMin.y, node->aabbMin.z),
				color);
			Draw3DLine(glm::vec3(node->aabbMin.x, node->aabbMin.y, node->aabbMin.z), glm::vec3(node->aabbMin.x, node->aabbMin.y, node->aabbMax.z),
				color);
			Draw3DLine(glm::vec3(node->aabbMax.x, node->aabbMin.y, node->aabbMax.z), glm::vec3(node->aabbMax.x, node->aabbMin.y, node->aabbMin.z),
				color);
			Draw3DLine(glm::vec3(node->aabbMax.x, node->aabbMin.y, node->aabbMax.z), glm::vec3(node->aabbMin.x, node->aabbMin.y, node->aabbMax.z),
				color);
			Draw3DLine(glm::vec3(node->aabbMin.x, node->aabbMax.y, node->aabbMin.z), glm::vec3(node->aabbMax.x, node->aabbMax.y, node->aabbMin.z),
				color);
			Draw3DLine(glm::vec3(node->aabbMin.x, node->aabbMax.y, node->aabbMin.z), glm::vec3(node->aabbMin.x, node->aabbMax.y, node->aabbMax.z),
				color);
			Draw3DLine(glm::vec3(node->aabbMax.x, node->aabbMax.y, node->aabbMax.z), glm::vec3(node->aabbMax.x, node->aabbMax.y, node->aabbMin.z),
				color);
			Draw3DLine(glm::vec3(node->aabbMax.x, node->aabbMax.y, node->aabbMax.z), glm::vec3(node->aabbMin.x, node->aabbMax.y, node->aabbMax.z),
				color);
			Draw3DLine(glm::vec3(node->aabbMin.x, node->aabbMin.y, node->aabbMin.z), glm::vec3(node->aabbMin.x, node->aabbMax.y, node->aabbMin.z),
				color);
			Draw3DLine(glm::vec3(node->aabbMax.x, node->aabbMin.y, node->aabbMin.z), glm::vec3(node->aabbMax.x, node->aabbMax.y, node->aabbMin.z),
				color);
			Draw3DLine(glm::vec3(node->aabbMin.x, node->aabbMin.y, node->aabbMax.z), glm::vec3(node->aabbMin.x, node->aabbMax.y, node->aabbMax.z),
				color);
			Draw3DLine(glm::vec3(node->aabbMax.x, node->aabbMin.y, node->aabbMax.z), glm::vec3(node->aabbMax.x, node->aabbMax.y, node->aabbMax.z),
				color);

			if (node->triCount > 0 || newStackPtr >= 3)
				break;
			newstack[newStackPtr++] = &mesh.bvh->bvhNode[node->leftFirst];
			newstack[newStackPtr++] = &mesh.bvh->bvhNode[node->leftFirst + 1];
			if (firsttime)
				cout << "bvhdraw " << node->leftFirst << " stack " << stackPtr << " new " << newStackPtr << endl;
		}
		for (int j = 0; j < 64; ++j)
		{
			stack[j] = newstack[j];
		}
		stackPtr = newStackPtr - 1;
		newStackPtr = 0;
	}
	firsttime = false;
}
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
			glm::vec3 bottompos = glm::vec3(rayTracer.lightPositions[i].position.x, rayTracer.lightHeight, rayTracer.lightPositions[i].position.y);
			Draw3DLine(bottompos,
				bottompos + glm::vec3(0, rayTracer.lightLength, 0),
				userInterface->selectedLightPos == i ? 255 | 170 << 8 | 170 << 16 | 255 << 24
				: 255 | 255 << 8 | 255 << 16 | 255 << 24);
		}

	}
	//cout << "BVH " << mesh.bvh->nodesUsed << " tricount " << mesh.triangleCount << endl;
	//drawBVH();

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
	glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);
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
