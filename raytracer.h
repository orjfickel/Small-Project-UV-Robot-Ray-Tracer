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

	/*class Photon
	{
	public:
		Photon(float3 pos, float dosage) : pos{ pos }, dosage{ dosage }{}
		float3 pos;
		float dosage;
	};*/

	bool TriangleIntersect(Ray& ray, float3 v1, float3 v2, float3 v3, float& u, float& v);

	class RayTracer
	{
	public:
		void computeDosageMap();

		uint* triangles;
		float* vertices;

		uint vertexCount, // Size of vertices array. Equals the number of vertices times 5
					 triangleCount; // Size of the triangles array. Equals the number of triangles times 3

		vector<float4> dosageMap;
		float2 lightPos = make_float2(0.0f, 0.0f);
		float lightLength = 0.1f, lightHeight = 0.1f; // How long and how high light is positioned. TODO: make lightHeight not just the ypos but the distance from the ground.
		uint photonCount = 100;
		float lightIntensity = 180 * 10;
	};

} // namespace Tmpl8