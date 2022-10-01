#pragma once


namespace Tmpl8
{
	class Ray
	{
	public:
		float3 dir;
		float3 origin;
		float dist;
		float intensity; // The power transmitted by the UV light
	};

	bool TriangleIntersect(Ray& ray, float3 v1, float3 v2, float3 v3, float& u, float& v);

	class RayTracer
	{
	public:
		const unsigned short* triangles;
		float* vertices;

		int vertexCount, triangleCount;
	};

} // namespace Tmpl8