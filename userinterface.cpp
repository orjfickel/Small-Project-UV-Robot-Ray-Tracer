#include "precomp.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
//#include <windows.h>
//#include <stdio.h>

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

	strcpy(rayTracer->name, rayTracer->routeFile);
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

	Begin("Parameters", 0);
	SetWindowFontScale(1.5f);

	//ImGui::Text("Vertex count: %u", rayTracer.vertexCount);

	Text("Aantal fotonen"); SameLine();
	InputInt("##photonCount", &rayTracer->maxPhotonCount);

	Text("Lamp sterkte in Watt"); SameLine();
	InputFloat("##power", &rayTracer->lightIntensity);//TODO: should be int probably
	Text("Grens minimale dosering in J/m^2"); SameLine();
	InputFloat("##mindosage", &rayTracer->minDosage);
	Text("Lamp lengte"); SameLine();
	InputFloat("##length", &rayTracer->lightLength);
	Text("Lamp hoogte"); SameLine();
	InputFloat("##height", &rayTracer->lightHeight);
	if (CollapsingHeader("Lamp route")) {
		for (int i = 0; i < rayTracer->lightPositions.size(); ++i)
		{
			Text("Positie %i", i + 1); SameLine();
			InputFloat2(("##position_" + std::to_string(i)).c_str(), rayTracer->lightPositions[i].position.cell);
			Text("Tijdsduur %i", i + 1); SameLine();
			InputFloat(("##duration_" + std::to_string(i)).c_str(), &rayTracer->lightPositions[i].duration);
		}
		if (Button("Voeg nieuwe lamp positie toe"))
		{
			rayTracer->AddLamp();
		}
	}
	
	if (Button("Route opslaan"))
	{
		ImGui::OpenPopup("my_select_popup");
		//TODO: give up on standard dialog, just ask for file name and load it...
	}
	if (ImGui::BeginPopup("my_select_popup"))
	{
		ImGui::Text("Bestand naam:");
		ImGui::InputText("##edit", rayTracer->name, IM_ARRAYSIZE(rayTracer->name));
		if (ImGui::Button("Opslaan")) {
			ImGui::CloseCurrentPopup();
			rayTracer->SaveRoute(rayTracer->name);
		}
		ImGui::EndPopup();
	}

	if (Button("Herbereken UV straling"))
	{
		rayTracer->ResetDosageMap();
	}
	if (Button("Berekening hervatten"))
	{//TODO: move to separate function
		rayTracer->reachedMaxPhotons = rayTracer->photonMapSize + rayTracer->photonCount > rayTracer->maxPhotonCount;
		rayTracer->photonCount = (rayTracer->maxPhotonCount / 4);
	}

	ShowDemoWindow();
	//TODO: configure light length, duration, power, and the min dosage
	//TODO: allow continueing/pauzing computation
	//TODO: show notification when done computing
	//TODO: save the current route automatically and allow saving route to separate file as well
	//TODO: select and move lights with wasd
	//TODO: save heatmap automatically and allow saving to separate file as well. Perhaps save into the gltf model?

	//TODO: for debugging: assign each triangle a color based on its normal

	End();
	Render();
	ImGui_ImplOpenGL3_RenderDrawData(GetDrawData());

	//rayTracer.lightPos = make_float3(lightPosInput[0], lightPosInput[1], lightPosInput[2]);
}