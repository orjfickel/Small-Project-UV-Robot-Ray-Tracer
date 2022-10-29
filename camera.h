#pragma once
#include "glm/glm.hpp"
#include "glm/ext.hpp"

namespace Tmpl8
{
	struct KeyPresses;

	class Camera
{
public:
	Camera();
	void UpdateView(KeyPresses keyPresses, float deltaTime, bool allowMovement);
	void Save();
	void Load();

	glm::mat4 view, projection;
	float FOV = 40;
	const char* cameraFile = "camera.xml";
};

} // namespace Tmpl8