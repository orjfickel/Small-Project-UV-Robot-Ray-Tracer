#pragma once

namespace Tmpl8
{
	struct Vertex {
		float posx, posy, posz;
		float normx, normy, normz;
		float2 textureuv;
	};
	struct Texture {
		unsigned int id;
	};

class MyApp : public TheApp
{
public:
	// game flow methods
	void Init();
	void Tick( float deltaTime );
	void Shutdown() { /* implement if you want to do something on exit */ }
	// input handling
	void MouseUp( int button ) { /* implement if you want to detect mouse button presses */ }
	void MouseDown( int button ) { /* implement if you want to detect mouse button presses */ }
	void MouseMove( int x, int y ) { mousePos.x = x, mousePos.y = y; }
	void MouseWheel( float y ) { /* implement if you want to handle the mouse wheel */ }
	void KeyUp( int key ) { /* implement if you want to handle keys */ }
	void KeyDown( int key ) { /* implement if you want to handle keys */ }

	void BindMesh();
	void DrawMesh(Shader& shader);

	// data members
	int2 mousePos;
	RayTracer rayTracer{};
	Camera camera{};

	unsigned int VAO, VBO, EBO;
	vector<Vertex>       vertices;
	vector<unsigned int> indices;
	vector<Texture>      textures;
};

} // namespace Tmpl8