#pragma once

namespace Tmpl8
{
	struct Vertex {
		float posx, posy, posz;
		float2 textureuv;
	};
	struct Texture {
		unsigned int id;
	};
    struct KeyPresses
    {
        bool wPress, aPress, sPress, dPress, qPress, ePress, upPress, leftPress, downPress, rightPress, shiftPress;
        float moveTimer = 0, timeTillMove = 2;
    };

class MyApp : public TheApp
{
public:
	// game flow methods
	void Init(GLFWwindow* window);
	void Tick( float deltaTime );
    void Shutdown();
	// input handling
	void MouseUp( int button ) { /* implement if you want to detect mouse button presses */ }
	void MouseDown( int button ) { /* implement if you want to detect mouse button presses */ }
	void MouseMove( int x, int y ) { mousePos.x = x, mousePos.y = y; }
	void MouseWheel( float y ) { /* implement if you want to handle the mouse wheel */ }
    void KeyUp(int key);
    void KeyDown(int key);

	void BindMesh();
	void UpdateDosageMap();
	void DrawMesh();
	void DrawUI();

    string modelFile = "assets/testroomopt.glb";
    ShaderGL* shader3D;

    KeyPresses keyPresses;
	int2 mousePos;
	RayTracer rayTracer{};
	Camera camera{};

	unsigned int texture;
	unsigned int VAO, VBO, EBO;

	uint texSize = 0, texWidth, texHeight;

	float timer = 1000000;
};

} // namespace Tmpl8