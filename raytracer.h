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

		const unsigned short* triangles;
		float* vertices;

		int vertexCount, // Size of vertices array. Equals the number of vertices times 5
			triangleCount; // Size of the triangles array. Equals the number of triangles

		vector<float4> dosageMap;
		float3 lightPos = make_float3(0.5f, 0.5f, 0.5f);
	};

} // namespace Tmpl8