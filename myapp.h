#pragma once
#include <tiny_gltf.h>

namespace Tmpl8
{
	struct Vertex {
		float posx, posy, posz;
		float2 textureuv;
	};
	struct Texture {
		unsigned int id;
	};

class MyApp : public TheApp
{
public:
	// game flow methods
	void Init(GLFWwindow* window, UserInterface* userInterface);
	void Tick( float deltaTime );
    void Shutdown();
	// input handling
	void MouseUp( int button ) { if (button == GLFW_MOUSE_BUTTON_LEFT) keyPresses.leftClick = false; }
	void MouseDown(int button) { if (button == GLFW_MOUSE_BUTTON_LEFT) keyPresses.leftClick = true; }
	void MouseMove( int x, int y ) { mousePos.x = x, mousePos.y = y; }
	void MouseWheel( float y ) { /* implement if you want to handle the mouse wheel */ }
    void KeyUp(int key);
    void KeyDown(int key);
	bool CameraKeyPressed();

	void DrawMesh();
	void Draw3DLine(glm::vec3 bottom, glm::vec3 top, uint color);
	uint greyscale_to_heatmap(float intensity);
	void drawBVH();
	//void UpdateDosageMap();

    ShaderGL* shader3D;
	Mesh mesh;

    KeyPresses keyPresses;
	int2 mousePos;
	RayTracer rayTracer{};
	UserInterface* userInterface;
	Camera camera{};


	//uint texSize = 0, texWidth, texHeight;

	//float timerStart = 0;
	//bool bufferSwapDraw = false;
};

} // namespace Tmpl8