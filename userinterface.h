#pragma once

namespace Tmpl8
{
	struct KeyPresses
	{
		bool wPress, aPress, sPress, dPress, qPress, ePress, upPress, leftPress, downPress, rightPress, shiftPress, leftClick;
		float moveTimer = 0, timeTillMove = 2;
	};

	class UserInterface
	{
	public:
		void Init(GLFWwindow* window, RayTracer* rayTracer);
		void DrawUI();
		void MoveLightPos(KeyPresses keyPresses, float deltaTime, glm::mat4 cameraView);

		RayTracer* rayTracer;
		int selectedLightPos = -1;
		bool addedLamp = false;
		bool showLights = true;
		bool showControls = false;
	};
}