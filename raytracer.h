#pragma once

namespace Tmpl8
{
	struct LightPos
	{
		float2 position;
		float duration;
	};
	//bool TriangleIntersect(Ray& ray, float3 v1, float3 v2, float3 v3, float& u, float& v);

	class RayTracer
	{
	public:
		void Init();
		void ComputeDosageMap();
		void ResetDosageMap();
		void AddLamp();

		float lightLength = 1.3f;
		float lightHeight = -0.9f; //TODO: make lightHeight not just the ypos but the distance from the ground.
		int photonCount = 1<<21;
		uint maxPhotonCount = (uint)(photonCount * 8);
		float lightIntensity = 180;
		float minDosage = 4;

		float* vertices;
		int vertexCount; // Size of the vertices array. Number of vertices times 3 (1 for every axis) (includes duplicates)
		float4* dosageMap;
		int2 workSize;
		vector<LightPos> lightPositions;
		float timer = 1000000;
		Timer timerClock;

		Kernel* generateKernel = 0, * extendKernel = 0, * shadeKernel = 0, *resetKernel = 0, * timeStepKernel = 0;
		Buffer* dosageBuffer = 0, * photonMapBuffer = 0, * verticesBuffer = 0, * rayBuffer = 0;
			//*lightPosBuffer = 0;
		uint dosageBufferID;
		int photonMapSize = 0;

		int offset = 0;
	};

} // namespace Tmpl8