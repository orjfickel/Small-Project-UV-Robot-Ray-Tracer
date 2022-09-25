
#include "precomp.h"

Camera::Camera()
{
	float3 pos = make_float3(0, 0, 0);
	matrix = mat4::LookAt(pos, make_float3(0, 0, -1), make_float3(0, 1, 0));// .Inverted();
	TranslateRelative(make_float3(0, 0, 1));

}

void Camera::UpdateView()
{
	float3 x, y, z;
	GetMatrixAxes(x, y, z);
	std::cout << "updateview " << y.x << y.y << y.z << "\n";
	position = matrix.GetTranslation();
	std::cout << "updateviewpos " << position.x << " y " << position.y << " z " << position.z << "\n";
	const float viewSize = tanf(FOV / 2 / (180 / PI));
	const float3 center = position + focalDistance * z;
	p1 = center - viewSize * x * focalDistance + viewSize * y * focalDistance;
	p2 = center + viewSize * x * focalDistance + viewSize * y * focalDistance;
	p3 = center - viewSize * x * focalDistance - viewSize * y * focalDistance;
	std::cout << "updateviewp1 " << p1.x << " y " << p1.y << " z " << p1.z << "\n";
	std::cout << "updateviewp2 " << p2.x << " y " << p2.y << " z " << p2.z << "\n";
	std::cout << "updateviewp3 " << p3.x << " y " << p3.y << " z " << p3.z << "\n";
}

void Camera::GetMatrixAxes(float3& x, float3& y, float3& z)
{
	x = make_float3(matrix.cell[0], matrix.cell[4], matrix.cell[8]);
	y = make_float3(matrix.cell[1], matrix.cell[5], matrix.cell[9]);
	z = make_float3(matrix.cell[2], matrix.cell[6], matrix.cell[10]);
}

void Camera::TranslateRelative(float3 T)
{
	float3 x, y, z;
	GetMatrixAxes(x, y, z);
	float3 delta = T.x * x + T.y * y + T.z * z;
	matrix.SetTranslation(matrix.GetTranslation() + delta);
}

void Camera::TranslateTarget(float3 T)
{
	float3 x, y, z;
	GetMatrixAxes(x, y, z);
	float3 delta = T.x * x + T.y * y + T.z * z;
	z = normalize(z + delta);
	x = normalize(cross(z, normalize(make_float3(0, 1, 0))));
	y = cross(x, z);
	matrix[0] = x.x, matrix[4] = x.y, matrix[8] = x.z;
	matrix[1] = y.x, matrix[5] = y.y, matrix[9] = y.z;
	matrix[2] = z.x, matrix[6] = z.y, matrix[10] = z.z;
}
