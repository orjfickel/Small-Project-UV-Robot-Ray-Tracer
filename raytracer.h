#pragma once

namespace Tmpl8
{

	//bool TriangleIntersect(Ray& ray, float3 v1, float3 v2, float3 v3, float& u, float& v);

	class RayTracer
	{
	public:
		void Init();
		void ComputeDosageMap();

		float* vertices;

		int vertexCount; // Size of the vertices array. Number of vertices times 3 (1 for every axis) (includes duplicates)

		float4* dosageMap;
		int2 workSize;

		float2 lightPos = make_float2(0.4f, 0.5f);
		float lightLength = 1.3f, lightHeight = -0.9f; // How long and how high light is positioned. TODO: make lightHeight not just the ypos but the distance from the ground.
		uint maxPhotonCount = 160000;
		int photonCount = (int)(maxPhotonCount / 8);
		int photonMapSize = 0;
		float lightIntensity = 180 * 10;

		Kernel* generateKernel = 0, * extendKernel = 0, * shadeKernel = 0;
		Buffer* dosageBuffer = 0, *photonMapBuffer = 0, *verticesBuffer = 0, *rayBuffer = 0;
		uint dosageBufferID;

		int offset = 0;
	};

} // namespace Tmpl8