
#include "precomp.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

using namespace ImGui;

void UserInterface::Init(GLFWwindow* window, RayTracer* rayTracer)
{
	this->rayTracer = rayTracer;
	// Initialise ImGui
	IMGUI_CHECKVERSION();
	if (!CreateContext())
	{
		printf("ImGui::CreateContext failed.\n");
		exit(EXIT_FAILURE);
	}
	ImGuiIO& io = GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	StyleColorsDark(); // or ImGui::StyleColorsClassic();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 130");
	
}

void UserInterface::DrawUI()
{
	//TODO: ensure it is drawn in front of the lights
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	NewFrame();
	Begin("Statistieken", 0);
	SetWindowFontScale(1.5f);
	Text("Aantal driehoeken: %u", rayTracer->vertexCount / 3);
	Text("Aantal fotonen: %i", rayTracer->photonMapSize);
	End();

	Begin("Lamp posities", 0);
	SetWindowFontScale(1.5f);

	//ImGui::Text("Vertex count: %u", rayTracer.vertexCount);
	for (int i = 0; i < rayTracer->lightPositions.size(); ++i)
	{
		std::string str = "Position " + std::to_string(i);
		const char* chars = str.c_str();
		InputFloat3(chars, rayTracer->lightPositions[i].position.cell);
	}
	if (Button("Herbereken UV straling"))
	{
		rayTracer->ResetDosageMap();
		rayTracer->ComputeDosageMap();
	}
	//TODO: configure light length, duration, power, and the min dosage
	//TODO: add new lights
	//TODO: save route
	//TODO: select and move lights with wasd

	End();
	Render();
	ImGui_ImplOpenGL3_RenderDrawData(GetDrawData());

	//rayTracer.lightPos = make_float3(lightPosInput[0], lightPosInput[1], lightPosInput[2]);
}