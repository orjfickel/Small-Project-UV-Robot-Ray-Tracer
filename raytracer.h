#pragma once

#define GPU_RAYTRACING 1;

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
		void Init();
		void ComputeDosageMap();

		uint* triangles;
		float* vertices;

		uint vertexCount, // Size of vertices array. Equals the number of vertices times 5
					 triangleCount; // Size of the triangles array. Equals the number of triangles times 3

		float4* dosageMap;


		float2 lightPos = make_float2(0.0f, 0.0f);
		float lightLength = 0.5f, lightHeight = 0.2f; // How long and how high light is positioned. TODO: make lightHeight not just the ypos but the distance from the ground.
		uint photonCount = 2000;
		int dosageMapSize;
		uint maxPhotonCount = 50000;
		float lightIntensity = 180 * 10;

		Kernel* kernel = 0;
		Buffer* dosageBuffer = 0;
		Buffer* rayBuffer = 0;
		unsigned int dosageTexture;

		int offset = 0;
	};

} // namespace Tmpl8