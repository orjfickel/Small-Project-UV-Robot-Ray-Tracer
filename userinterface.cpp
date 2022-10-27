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
	Text("Aantal fotonen per 1000"); SameLine();
	int numPhotons = rayTracer->maxPhotonCount / 1024;
	InputInt("##photonCount", &numPhotons, 10,1000);
	rayTracer->maxPhotonCount = numPhotons * 1024;

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

	if (!rayTracer->reachedMaxPhotons || rayTracer->progressTextTimer > 0) {
		SetNextWindowPos(ImVec2(10, SCRHEIGHT - 40), 0);
		SetNextWindowSize(ImVec2(150, 0), 0);
		Begin("progress", 0, ImGuiCond_FirstUseEver | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs);
		if (!rayTracer->reachedMaxPhotons) {
			Text("Vooruitgang: %.2f%%", rayTracer->progress);
		} else {
			Text("Berekening klaar");
		}
		End();
	}
	// UI is unresponsive during computation so this does not work
	//if (Button("Berekening stoppen"))
	//{
	//	rayTracer->reachedMaxPhotons = true;
	//}

	//TODO: For some reason computation becomes significantly slower
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
	//TODO: show progress/notification when done computing
	//TODO: button to show regularly shaded scene, maybe depth per triangle? So that the user can still understand what they are looking at if everything is red.
	//TODO: select and move lights with wasd. base height off the ground by creating histogram of vertex heights (below half of model) and taking the lowest max bucket
	//TODO: Dosage to color legend
	//TODO: max dosage map
	//TODO: BVH
	//TODO: light movement interpolate
	//TODO: save heatmap automatically and allow saving to separate file as well. Perhaps save into the gltf model?

	//TODO: for debugging: assign each triangle a color based on its normal

	//TODO: allow continueing/pauzing computation (not a priority)
	End();
	Render();
	ImGui_ImplOpenGL3_RenderDrawData(GetDrawData());

	//rayTracer.lightPos = make_float3(lightPosInput[0], lightPosInput[1], lightPosInput[2]);
}