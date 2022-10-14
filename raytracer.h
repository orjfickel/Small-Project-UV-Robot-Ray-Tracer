#pragma once

namespace Tmpl8
{

	struct ALIGN(32) Ray
	{ // 32 Bytes
		float dirx, diry, dirz;
		float origx,origy,origz;
		float dist;
		float intensity; // The power transmitted by the UV light
	};

	class Photon
	{
	public:// 24 Bytes
		float posx, posy, posz;//Can be compressed, is only relevant for quick search in kd-tree
		float timeStep;
		int triangleID, lampID;// lampID refers to a lamp position, to group for computing max power
	};

	bool TriangleIntersect(Ray& ray, float3 v1, float3 v2, float3 v3, float& u, float& v);

	class RayTracer
	{
	public:
		void Init();
		void ComputeDosageMap();

		uint* triangles;
		float* vertices;

		int vertexCount, // Size of vertices array. Equals the number of vertices times 3
			triangleCount; // Size of the triangles array. Equals the number of triangles times 3

		float4* dosageMap;
		int2 workSize;

		float2 lightPos = make_float2(0.0f, 0.0f);
		float lightLength = 0.5f, lightHeight = 0.2f; // How long and how high light is positioned. TODO: make lightHeight not just the ypos but the distance from the ground.
		int photonCount = 2000;
		int photonMapSize = 0;
		uint maxPhotonCount = 50000;
		float lightIntensity = 180 * 10;

		Kernel* generateKernel = 0, * extendKernel = 0, * shadeKernel = 0;
		Buffer* dosageBuffer = 0, *photonMapBuffer = 0, *triangleBuffer = 0, *verticesBuffer = 0, *rayBuffer = 0;
		uint dosageBufferID;

		int offset = 0;
	};

} // namespace Tmpl8