#pragma once

namespace Tmpl8
{
	struct LightPos
	{
		float2 position;
		float duration;
	};
	//bool TriangleIntersect(Ray& ray, float3 v1, float3 v2, float3 v3, float& u, float& v);

	enum ViewMode { dosage, maxpower, texture };

	class RayTracer
	{
	public:
		void Init();
		void ComputeDosageMap();
		void Shade();
		void ResetDosageMap();
		void AddLamp();
		void SaveRoute(char fileName[32]);
		void LoadRoute(char fileName[32]);

		float lightLength = 1.3f;
		float lightHeight = -0.9f; //TODO: make lightHeight not just the ypos but the distance from the ground.
		int photonCount = (1<<19);
		int maxIterations = 32;
		int currIterations;
		float lightIntensity = 180;
		float minDosage = 4, minPower = 4;
		char defaultRouteFile[32] = "route";
		char newRouteFile[32] = "nieuwe_route";

		float* vertices;
		int vertexCount; // Size of the vertices array. Number of vertices times 3 (1 for every axis) (includes duplicates)
		float4* dosageMap;
		int2 workSize;
		vector<LightPos> lightPositions;
		float timer = 1000000;
		float compTime = 0;
		float progressTextTimer = 0;
		float progress;
		Timer timerClock;
		bool reachedMaxPhotons = true;
		ViewMode viewMode = texture;
		bool startedComputation = false;

		Kernel* generateKernel = 0, * extendKernel = 0, * shadeKernel = 0, *resetKernel = 0, * accumulateKernel = 0;
		Buffer* colorBuffer = 0, * photonMapBuffer = 0, * verticesBuffer = 0, * rayBuffer = 0, * tempPhotonMapBuffer = 0, *maxPhotonMapBuffer;
			//*lightPosBuffer = 0;
		uint dosageBufferID;
		int photonMapSize = 0;

		ShaderGL* simpleShader;
	};

} // namespace Tmpl8