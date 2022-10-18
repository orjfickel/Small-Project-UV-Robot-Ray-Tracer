#pragma once

namespace Tmpl8
{
	class UserInterface
	{
	public:
		void Init(GLFWwindow* window, RayTracer* rayTracer);
		void DrawUI();

		RayTracer* rayTracer;
	};
}