#pragma once

namespace Tmpl8
{

class Camera
{
public:
	Camera();
	float3	p1, // Upper left corner of the world-space viewport
			p2, // Upper right corner
			p3, // Lower left corner
			position = make_float3(0,0,0);
	mat4 matrix;
	float FOV = 80, focalDistance = 5.0f;

	//glm::vec3 pos = glm::vec3(0,0,0);

	void UpdateView();
	void GetMatrixAxes(float3& x, float3& y, float3& z);
	void TranslateRelative(float3 T); // Translate the camera position
	void TranslateTarget(float3 T); // Move/rotate the target vector for where the camera looks towards
};

} // namespace Tmpl8