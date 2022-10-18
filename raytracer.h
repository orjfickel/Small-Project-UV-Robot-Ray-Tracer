#pragma once

namespace Tmpl8
{
	struct LightPos
	{
		float3 position;
		float duration;
	};
	//bool TriangleIntersect(Ray& ray, float3 v1, float3 v2, float3 v3, float& u, float& v);

	class RayTracer
	{
	public:
		void Init();
		void ComputeDosageMap();
		void ResetDosageMap();

		float* vertices;

		int vertexCount; // Size of the vertices array. Number of vertices times 3 (1 for every axis) (includes duplicates)

		float4* dosageMap;
		int2 workSize;

		vector<LightPos> lightPositions;
		float lightLength = 1.3f;
		float floorOffset = -1.5f; //TODO: make lightHeight not just the ypos but the distance from the ground.
		uint maxPhotonCount = 160000;
		int photonCount = (int)(maxPhotonCount / 8);
		int photonMapSize = 0;
		float lightIntensity = 180 * 10;

		Kernel* generateKernel = 0, * extendKernel = 0, * shadeKernel = 0;
		Buffer* dosageBuffer = 0, * photonMapBuffer = 0, * verticesBuffer = 0, * rayBuffer = 0;
			//*lightPosBuffer = 0;
		uint dosageBufferID;

		int offset = 0;
	};

} // namespace Tmpl8