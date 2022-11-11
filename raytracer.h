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

		float lightLength = 1.3f;
		float lightHeight = 0.5f;
		int maxPhotonCount = (1 << 26);
		int photonCount = maxPhotonCount;
		int maxIterations = 10;
		int currIterations;
		float lightIntensityDefault = 180;
		float lightIntensity = lightIntensityDefault;
		float minDosage = 4, minPower = 4;
		char defaultRouteFile[32] = "route";
		char newRouteFile[32] = "nieuwe_route";

		Mesh* mesh;
		float* dosageMap = new float[2];
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

		// The calibrated fraction of the default power to use.
		float calibratedPower;

		Kernel* generateKernel = 0, * extendKernel = 0, * shadeDosageKernel = 0, * shadeColorKernel = 0, *resetKernel = 0, * accumulateKernel = 0;
		Buffer* colorBuffer = 0, * dosageBuffer = 0, * photonMapBuffer = 0, * verticesBuffer = 0, * rayBuffer = 0, * tempPhotonMapBuffer = 0, *maxPhotonMapBuffer, * bvhNodesBuffer, *triIdxBuffer;
			//*lightPosBuffer = 0;
		int photonMapSize = 0;

		ShaderGL* simpleShader;
	};

} // namespace Tmpl8