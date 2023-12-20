#include "precomp.h"
#include "myapp.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

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

	shader3D = new ShaderGL("shaders/shader3D.vert", "shaders/shader3D.frag", false);
	rayTracer.simpleShader = new ShaderGL("shaders/simpleshader.vert", "shaders/simpleshader.frag", false);
	camera.projection = glm::perspective(glm::radians(camera.FOV), (float)SCRWIDTH / (float)SCRHEIGHT, 0.1f, 100.0f);

	shader3D->Bind();
	shader3D->SetInputMatrixGLM("projection", camera.projection);
	shader3D->Unbind();
	rayTracer.simpleShader->Bind();
	rayTracer.simpleShader->SetInt("tex", 0);
	rayTracer.simpleShader->SetInputMatrixGLM("projection", camera.projection);
	rayTracer.simpleShader->Unbind();

	mesh.LoadMesh();
	mesh.loadedMesh = true;

	rayTracer.Init(&mesh);
}

// Convert coordinates to screen space and draw the line
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
// Visualise the BVH for debugging purposes (currently unused)
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
	if (rayTracer.progressTextTimer > 0) rayTracer.progressTextTimer -= deltaTime;

	bool moveLightPos = userInterface->selectedLightPos > -1 && userInterface->selectedLightPos < rayTracer.lightPositions.size();
	if (moveLightPos)
	{
		userInterface->MoveLightPos(keyPresses, deltaTime, camera.view);
	}

	// Update the camera
	camera.UpdateView(keyPresses, deltaTime, !moveLightPos);

	screen->Clear(0);
	if (userInterface->showLights) {
		for (int i = 0; i < rayTracer.lightPositions.size(); ++i)
		{
			glm::vec3 bottompos = glm::vec3(rayTracer.lightPositions[i].position.x, mesh.floorHeight + rayTracer.lightHeight, 
											rayTracer.lightPositions[i].position.y);
			Draw3DLine(bottompos,
				bottompos + glm::vec3(0, rayTracer.lightLength, 0),
				userInterface->selectedLightPos == i ? 255 | 100 << 8 | 100 << 16 | 255 << 24
				: 255 | 255 << 8 | 255 << 16 | 255 << 24);
		}
	}

	// Compute and shade the dosage map
	if (!rayTracer.finishedComputation) { // Only check if we reached the max photon count if we haven't already		
		rayTracer.finishedComputation = rayTracer.currIterations >= rayTracer.maxIterations;
		if (!rayTracer.finishedComputation) {
			rayTracer.ComputeDosageMap();
			rayTracer.Shade();
			if (rayTracer.viewMode == texture) rayTracer.viewMode = dosage;
			rayTracer.currIterations++;
			rayTracer.progress = 100.0f * static_cast<float>(rayTracer.currIterations) / static_cast<float>(rayTracer.maxIterations);

			clFinish(Kernel::GetQueue());// Make sure heatmap computation is finished
			float time = rayTracer.timerClock.elapsed();
			rayTracer.compTime += time;
			cout << "Progress: " << rayTracer.progress << "% photon count: " << rayTracer.photonMapSize << 
				" delta time: " << rayTracer.timerClock.elapsed() * 1000.0f << " total time: " << rayTracer.compTime * 1000.0f << endl;
			rayTracer.timerClock.reset();
		} else { // We have reached the max number of iterations
			// Set the timer to display the "computation done" popup text
			rayTracer.progressTextTimer = 1000;
		}
	}
	
	DrawMesh();
}

void MyApp::DrawMesh()
{
	glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	ShaderGL* shader;
	if (rayTracer.viewMode != texture)
		// Use the shader that displays the uniform solid colors per triangle
		shader = shader3D;
	else {
		// Use the shader that simply displays the texture of each triangle
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mesh.textureBuffer);
		shader = rayTracer.simpleShader;
	}

	shader->Bind();

	shader->SetInputMatrixGLM("view", camera.view);
	glEnable(GL_CULL_FACE);
	glBindVertexArray(mesh.VAO);
	glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);
	glBindVertexArray(0);

	shader->Unbind();
}

void MyApp::KeyDown(int key)
{
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
	glDeleteVertexArrays(1, &mesh.VAO);
	glDeleteBuffers(1, &mesh.VBO);
	glDeleteBuffers(1, &mesh.UVBuffer);
	glDeleteTextures(1, &mesh.textureBuffer);
	glDeleteBuffers(1, &mesh.dosageBufferID);
}
