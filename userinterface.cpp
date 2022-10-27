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
	PushItemWidth(-2);

	//ImGui::Text("Vertex count: %u", rayTracer.vertexCount);
	Text("Aantal fotonen"); SameLine();
	InputInt("##photonCount", &rayTracer->maxPhotonCount);

	Text("Lamp sterkte in Watt"); SameLine();
	InputFloat("##power", &rayTracer->lightIntensity);//TODO: should be int probably
	Text("Minimale dosering in J/m^2"); SameLine();
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
			PushItemWidth(-120);
			InputFloat(("##duration_" + std::to_string(i)).c_str(), &rayTracer->lightPositions[i].duration);
			SameLine();
			PushItemWidth(-2);
			if (Button(("Verwijder###delete_" + std::to_string(i)).c_str()))
			{
				rayTracer->lightPositions.erase(rayTracer->lightPositions.begin()+i);
			}
		}
	}
	if (Button("Voeg nieuwe lamp positie toe"))
	{
		rayTracer->AddLamp();
	}

	if (Button("Route opslaan"))
	{
		OpenPopup("savePopup");
	}
	if (BeginPopup("savePopup"))
	{
		Text("Bestand naam:");
		InputText("##edit", rayTracer->newRouteFile, IM_ARRAYSIZE(rayTracer->newRouteFile));
		if (Button("Opslaan")) {
			CloseCurrentPopup();
			rayTracer->SaveRoute(rayTracer->newRouteFile);
		}
		EndPopup();
	}
	SameLine();
	if (Button("Route laden"))
	{
		OpenPopup("loadPopup");
	}
	if (BeginPopup("loadPopup"))
	{
		Text("Bestand naam:");
		InputText("##edit2", rayTracer->newRouteFile, IM_ARRAYSIZE(rayTracer->newRouteFile));
		if (Button("Laden")) {
			CloseCurrentPopup();
			rayTracer->LoadRoute(rayTracer->newRouteFile);
		}
		EndPopup();
	}

	if (Button("Herbereken UV straling"))
	{
		rayTracer->ResetDosageMap();
	}
	// UI is unresponsive during computation so this does not work
	//if (Button("Berekening stoppen"))
	//{
	//	rayTracer->reachedMaxPhotons = true;
	//}

	// For some reason computation becomes significantly slower
	//if (Button("Berekening hervatten"))
	//{//TODO: move to separate function
	//	rayTracer->reachedMaxPhotons = rayTracer->photonMapSize + 100 > rayTracer->maxPhotonCount;
	//	if (!rayTracer->reachedMaxPhotons) {
	//		rayTracer->photonCount = min(rayTracer->photonCount,((rayTracer->maxPhotonCount - rayTracer->photonMapSize) / 4));
	//		//delete rayTracer->rayBuffer;
	//		//rayTracer->rayBuffer = new Buffer(8 * rayTracer->photonCount, Buffer::DEFAULT);
	//		//rayTracer->generateKernel->SetArgument(0, rayTracer->rayBuffer);
	//		//rayTracer->extendKernel->SetArgument(2, rayTracer->rayBuffer);
	//	}
	//}

	ShowDemoWindow();
	//TODO: allow continueing/pauzing computation
	//TODO: show notification when done computing
	//TODO: save the current route automatically
	//TODO: allow deleting light positions
	//TODO: select and move lights with wasd
	//TODO: save heatmap automatically and allow saving to separate file as well. Perhaps save into the gltf model?

	//TODO: for debugging: assign each triangle a color based on its normal

	End();
	Render();
	ImGui_ImplOpenGL3_RenderDrawData(GetDrawData());

	//rayTracer.lightPos = make_float3(lightPosInput[0], lightPosInput[1], lightPosInput[2]);
}