#pragma once

namespace Tmpl8
{
	struct LightPos
	{
		float2 position;
		float duration;
	};
	
	enum ViewMode { dosage, maxpower, texture };

	class RayTracer
	{
	public:
		void Init(Mesh* mesh);
		void ComputeDosageMap(vector<LightPos> lightPositions, int photonCount);
		void ComputeDosageMap(LightPos lightPos, int photonsPerLight, int triangleCount);
		void Shade();
		void ResetDosageMap();
		void ClearBuffers(bool resetColor);
		void AddLamp();
		void CalibratePower(float measurePower, float measureHeight, float measureDist);
		void SaveRoute(char fileName[32]);
		void LoadRoute(char fileName[32]);

		float lightLength = 1.0f;
		float lightHeight = 0.8f;
		int maxPhotonCount = (1 << 26);
		int photonCount = (1 << 25);
		int maxIterations = 10;
		int currIterations; // The number of computed iterations
		float lightIntensity = 450;
		float minDosage = 100, minPower = 1500;
		char defaultRouteFile[32] = "route";
		char newRouteFile[32] = "new_route";

		Mesh* mesh;
		float* dosageMap = new float[2];
		vector<LightPos> lightPositions;
		float compTime = 0;
		float progressTextTimer = 0;
		float progress;
		Timer timerClock;
		bool finishedComputation = true;
		ViewMode viewMode = texture;
		bool thresholdView = false;
		bool startedComputation = false;
		float calibratedPower;

		Kernel* generateKernel = 0, * extendKernel = 0, * shadeDosageKernel = 0, * shadeColorKernel = 0, *resetKernel = 0, * accumulateKernel = 0;
		Buffer* colorBuffer = 0, * dosageBuffer = 0, * photonMapBuffer = 0, * verticesBuffer = 0, * rayBuffer = 0, * tempPhotonMapBuffer = 0,
			*maxPhotonMapBuffer, * bvhNodesBuffer, *triIdxBuffer;
		int photonMapSize = 0;

		ShaderGL* simpleShader;
	};

} // namespace Tmpl8