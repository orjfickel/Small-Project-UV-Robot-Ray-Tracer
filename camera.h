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
	glm::vec3 position = glm::vec3(1, 0, 0),
		      rotation = glm::vec3(0, 0, 0);
	glm::mat4 view;
	float FOV = 40;

	void UpdateView(KeyPresses keyPresses, float deltaTime);
};

} // namespace Tmpl8