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

	PushItemWidth(-90);
	//ImGui::Text("Vertex count: %u", rayTracer.vertexCount);
	Text("Aantal fotonen"); SameLine();
	int numPhotons = rayTracer->maxPhotonCount / 1024;
	InputInt("##photonCount", &numPhotons, 0,0);
	rayTracer->maxPhotonCount = (numPhotons > MAXINT / 1024) ? MAXINT : numPhotons * 1024;
	SameLine(); Text("duizend");

	bool actuallyReachedMaxPhotons = rayTracer->reachedMaxPhotons && rayTracer->photonMapSize + rayTracer->photonCount > rayTracer->maxPhotonCount;
	if (rayTracer->startedComputation && rayTracer->reachedMaxPhotons && !actuallyReachedMaxPhotons)
	{
		if (Button("Berekening hervatten")) {
			rayTracer->reachedMaxPhotons = false;
		}
	}

	PushItemWidth(-2);
	Text("Lamp sterkte in Watt"); SameLine();
	InputFloat("##power", &rayTracer->lightIntensity,0,0,"%.2f");//TODO: should be int probably
	Text("Minimale dosering in J/m^2"); SameLine();
	InputFloat("##mindosage", &rayTracer->minDosage, 0, 0, "%.2f");
	Text("Lamp lengte"); SameLine();
	InputFloat("##length", &rayTracer->lightLength, 0, 0, "%.2f");
	Text("Lamp hoogte"); SameLine();
	InputFloat("##height", &rayTracer->lightHeight, 0, 0, "%.2f");
	if (addedLamp) {
		SetNextTreeNodeOpen(true);
		addedLamp = false;
	}
	if (CollapsingHeader("Lamp route")) {
		BeginChild("lightpositions", ImVec2(0,200));
		for (int i = 0; i < rayTracer->lightPositions.size(); ++i)
		{
			char buf[32];
			sprintf(buf, "##Positie %d", i + 1);
			if (Selectable(buf, selectedLightPos == i,ImGuiSelectableFlags_AllowItemOverlap,ImVec2(0,50)))
			{
				selectedLightPos = i;
			}
			SetItemAllowOverlap();
			SameLine();
			BeginGroup();
			Text("Positie %i", i + 1); SameLine();
			InputFloat2(("##position_" + std::to_string(i)).c_str(), rayTracer->lightPositions[i].position.cell, "%.2f");
			Text("Tijdsduur"); SameLine();
			PushItemWidth(-120);
			InputFloat(("##duration_" + std::to_string(i)).c_str(), &rayTracer->lightPositions[i].duration, 0, 0, "%.2f");
			SameLine();
			PushItemWidth(-2);
			if (Button(("Verwijder###delete_" + std::to_string(i)).c_str()))
			{
				rayTracer->lightPositions.erase(rayTracer->lightPositions.begin()+i);
			}
			EndGroup();
		}
		EndChild();
	}
	if (Button("Voeg nieuwe lamp positie toe"))
	{
		rayTracer->AddLamp();
		addedLamp = true;
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

	if ((rayTracer->startedComputation && Button("Herbereken UV straling")) || (!rayTracer->startedComputation && Button("Bereken UV straling")))
	{
		rayTracer->ResetDosageMap();
	}

	//Progress popup
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

	if (!rayTracer->heatmapView && Button("Toon heatmap"))
		rayTracer->heatmapView = true;
	else if (rayTracer->heatmapView && Button("Toon diepte"))
		rayTracer->heatmapView = false;

	ShowDemoWindow();
	//TODO: add button to hide / toon lights
	//TODO: explain camera controls
	//TODO: button to show regularly shaded scene, maybe depth per triangle? So that the user can still understand what they are looking at if everything is red.
	//TODO: select and move lights with wasd. base height off the ground by creating histogram of vertex heights (below half of model) and taking the lowest max bucket
	//TODO: Dosage to color legend
	//TODO: max dosage map
	//TODO: BVH
	//TODO: save heatmap automatically and allow saving to separate file as well. Perhaps save into the gltf model?

	//TODO: for debugging: assign each triangle a color based on its normal

	//TODO: light movement interpolate
	//TODO: allow continueing/pauzing computation (not a priority)
	End();
	Render();
	ImGui_ImplOpenGL3_RenderDrawData(GetDrawData());

	//rayTracer.lightPos = make_float3(lightPosInput[0], lightPosInput[1], lightPosInput[2]);
}

void UserInterface::MoveLightPos(KeyPresses keyPresses, float deltaTime, glm::mat4 cameraView)
{
	if (keyPresses.leftClick)
		selectedLightPos = -1;

	float movement = (keyPresses.shiftPress ? 0.015f : 0.005f) * deltaTime;

	float2 moveVec = make_float2(0);
	if (glm::abs(cameraView[2][2]) < glm::abs(cameraView[0][2])) {// w press should move forward in the dominant axis of the view direction vector
		if (keyPresses.wPress) { moveVec.x -= movement * glm::sign(cameraView[0][2]); }
		if (keyPresses.aPress) { moveVec.y += movement * glm::sign(cameraView[0][2]); }
		if (keyPresses.sPress) { moveVec.x += movement * glm::sign(cameraView[0][2]); }
		if (keyPresses.dPress) { moveVec.y -= movement * glm::sign(cameraView[0][2]); }
	} else
	{
		if (keyPresses.wPress) { moveVec.y -= movement * glm::sign(cameraView[2][2]); }
		if (keyPresses.aPress) { moveVec.x -= movement * glm::sign(cameraView[2][2]); }
		if (keyPresses.sPress) { moveVec.y += movement * glm::sign(cameraView[2][2]); }
		if (keyPresses.dPress) { moveVec.x += movement * glm::sign(cameraView[2][2]); }
	}
	rayTracer->lightPositions[selectedLightPos].position += moveVec;
}