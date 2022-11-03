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

static void HelpMarker(const char* desc)
{
	ImGui::SameLine();
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		SetWindowFontScale(1.5f);
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

void UserInterface::DrawUI()
{
	//TODO: ensure it is drawn in front of the lights
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	NewFrame();
	Begin("Statistieken", 0);
	SetWindowFontScale(1.5f);
	Text("Aantal driehoeken: %u", rayTracer->mesh->triangleCount);
	Text("Aantal fotonen: %i", rayTracer->photonMapSize);
	End();

	if (showControls) {
		Begin("Uitleg", &showControls, ImGuiWindowFlags_NoNavInputs);
		SetWindowFontScale(1.5f);
		PushTextWrapPos(GetFontSize() * 35.0f);
		Text("Gebruik de WASDQE toetsen om de camera te bewegen");
		Text("Gebruik de pijltjes toetsen om de camera te draaien");
		Text("Om een een route te maken, klik op \"Lamp positie toevoegen\"");
		Text("Selecteer een van de lampen uit de lijst, en gebruik WASD om de lamp te verplaatsen");
		Text("Pas de overige parameters aan en klik op \"Bereken UV straling\" om de heatmap van de straling te tonen");
		Text("De heatmap toont de cumulatieve of maximale UV straling die elke driehoek in het 3D model heeft ontvangen, vanuit de discrete lamp posities");
		Text("Om de kleur van de heatmap te schalen, open het kopje \"Geavanceerd\" en pas de drempelwaarde(s) aan");
		PopTextWrapPos();
		End();
	}

	Begin("Parameters", 0, ImGuiWindowFlags_NoNavInputs);
	SetWindowFontScale(1.5f);
	PushItemWidth(-2);

	if (Button("Handleiding"))
	{
		showControls = !showControls;
	}
	
	Text("Lamp sterkte (W)");
	HelpMarker("Hoe veel energie de lamp uitstraalt"); SameLine();
	InputFloat("##power", &rayTracer->lightIntensity,0,0,"%.2f");//TODO: should be int probably
	
	Text("Lamp lengte (m)"); SameLine();
	InputFloat("##length", &rayTracer->lightLength, 0, 0, "%.2f");
	Text("Lamp hoogte (m)"); SameLine();
	InputFloat("##height", &rayTracer->lightHeight, 0, 0, "%.2f");
	if (addedLamp) {
		SetNextTreeNodeOpen(true);
		addedLamp = false;
	}
	bool tempOpen = CollapsingHeader("Lamp posities");
	HelpMarker("Discrete plekken waar de lamp voor bepaalde tijdsduren staat \nSelecteer een positie uit de lijst om met WASD te verplaatsen");
	if (tempOpen) {
		BeginChild("lightpositions", ImVec2(0,220));
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
			Text("Positie %i (m)", i + 1); SameLine();
			InputFloat2(("##position_" + std::to_string(i)).c_str(), rayTracer->lightPositions[i].position.cell, "%.2f");
			Text("Tijdsduur (s)"); SameLine();
			PushItemWidth(-120);
			InputFloat(("##duration_" + std::to_string(i)).c_str(), &rayTracer->lightPositions[i].duration, 0, 0, "%.2f");
			SameLine();
			PushItemWidth(-2);
			if (Button(("Verwijder###delete_" + std::to_string(i)).c_str()))
			{
				rayTracer->lightPositions.erase(rayTracer->lightPositions.begin()+i);
			}
			EndGroup();
			ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 5));
		}
		EndChild();
	}
	if (Button("Lamp positie toevoegen"))
	{
		rayTracer->AddLamp();
		addedLamp = true;
		selectedLightPos = rayTracer->lightPositions.size() - 1;
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
		SetNextWindowPos(ImVec2(10, SCRHEIGHT - 50), 0);
		SetNextWindowSize(ImVec2(220, 0), 0);
		Begin("progress", 0, ImGuiCond_FirstUseEver | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs);
		SetWindowFontScale(1.5f);
		if (!rayTracer->reachedMaxPhotons) {
			Text("Vooruitgang: %.2f%%", rayTracer->progress);
		} else {
			Text("Berekening klaar");
		}
		End();
	}
	Text("");

	tempOpen = Selectable("Toon fotoscan", rayTracer->viewMode == texture);
	HelpMarker("De kamer gekleurd zoals met de camera is ingescand");
	if (tempOpen)
	{
		rayTracer->viewMode = texture;
	}
	tempOpen = Selectable("Toon totale dosis", rayTracer->viewMode == dosage);
	HelpMarker("De cumulatieve dosis UV straling dat elk oppervlak heeft ontvangen");
	if (tempOpen)
	{
		rayTracer->viewMode = dosage;
		if (!rayTracer->startedComputation)
			rayTracer->ResetDosageMap();
		rayTracer->Shade();
	}
	tempOpen = Selectable("Toon max bestralingssterkte", rayTracer->viewMode == maxpower);
	HelpMarker("De maximale UV straling dat elk oppervlak heeft ontvangen");
	if (tempOpen)
	{
		rayTracer->viewMode = maxpower;
		if (!rayTracer->startedComputation)
			rayTracer->ResetDosageMap();
		rayTracer->Shade();
	}

	if (!showLights && Button("Toon lamp posities"))
		showLights = true;
	else if (showLights && Button("Verberg lamp posities"))
		showLights = false;

	if (CollapsingHeader("Geavanceerd")) {
		Text("Legenda drempel \ndosis (mJ/cm^2)");
		HelpMarker("De heatmap wordt zo geschaald dat deze waarde groen is"); SameLine();
		float tempminValue = rayTracer->minDosage;
		InputFloat("##mindosage", &rayTracer->minDosage, 0, 0, "%.2f");
		// Update the shading if the scaling dosage is changed
		if (rayTracer->startedComputation && tempminValue != rayTracer->minDosage && rayTracer->viewMode == dosage)
			rayTracer->Shade();

		//PushTextWrapPos(GetFontSize() * 15.0f);
		Text("Drempel bestralings-\nsterkte (\xC2\xB5W/cm^2 W)");
		HelpMarker("De heatmap wordt zo geschaald dat deze waarde groen is"); SameLine();
		tempminValue = rayTracer->minPower;
		//ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 10));
		InputFloat("##minpower", &rayTracer->minPower, 0, 0, "%.2f");
		//ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - 5));
		// Update the shading if the scaling dosage is changed
		if (rayTracer->startedComputation && tempminValue != rayTracer->minPower && rayTracer->viewMode == maxpower)
			rayTracer->Shade();

		Text("Fotonen per iteratie");
		HelpMarker("Meer fotonen zorgen voor minder ruis in de berekening");
		SameLine();
		InputInt("##photonCount", &rayTracer->photonCount, 0, 0);
		rayTracer->photonCount = min(1 << 26, rayTracer->photonCount);
		if (rayTracer->photonCount > 1 << 26 || rayTracer->photonCount < 1)
		{
			rayTracer->photonCount = 1 << 26;
		}

		Text("Aantal iteraties"); SameLine();
		InputInt("##iterations", &rayTracer->maxIterations, 1, 0);

		if (rayTracer->maxIterations > 1)
		{
			Text("Note: de max bestralingssterkte \nmap werkt slechts voor 1 iteratie ");
		}
		else if (rayTracer->maxIterations < 1)
		{
			rayTracer->maxIterations = 1;
		}

		bool actuallyReachedMaxPhotons = rayTracer->reachedMaxPhotons && rayTracer->currIterations >= rayTracer->maxIterations;
		if (rayTracer->startedComputation && rayTracer->reachedMaxPhotons && !actuallyReachedMaxPhotons)
		{
			if (Button("Berekening hervatten")) {
				rayTracer->reachedMaxPhotons = false;
			}
		}
	}

	// Draw the heatmap color legend
	static float pickerSize = 420;
	static int halfnumberwidth = 25;
	float rainbowHeight = 60, lineHeight = 50, numberHeight = 30;
	SetNextWindowPos(ImVec2(SCRWIDTH - pickerSize - 2 * halfnumberwidth - 20, SCRHEIGHT - rainbowHeight - 45), 0);
	SetNextWindowSize(ImVec2(pickerSize + 2 * halfnumberwidth + 10, rainbowHeight + 35), 0);
	Begin(rayTracer->viewMode == maxpower ? "Maximale bestralingssterkte (\xC2\xB5W/cm^2)" : "Cumulatieve dosis (mJ/cm^2)", 0, ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs);
	SetWindowFontScale(1.5f);

	ImDrawList* draw_list = GetWindowDrawList();
	ImVec2 panelPos = GetWindowPos();
	const ImU32 col_hues[4 + 1] = { IM_COL32(0,0,255,255), IM_COL32(0,255,255,255), IM_COL32(0,255,0,255), IM_COL32(255,255,0,255), IM_COL32(255,0,0,255)};

	for (int i = 0; i < 4; ++i)
		draw_list->AddRectFilledMultiColor(ImVec2(panelPos.x + i * (pickerSize / 4) + halfnumberwidth, panelPos.y+ rainbowHeight), ImVec2(panelPos.x + (i + 1) * (pickerSize / 4) + halfnumberwidth, panelPos.y + rainbowHeight + 25), col_hues[i], col_hues[i+1], col_hues[i + 1], col_hues[i]);

	static int numberCount = 4;
	for (int i = 0; i <= numberCount; i++)
	{
		char buf[16];
		sprintf(buf, "%.2f", i * (rayTracer->viewMode == maxpower ? rayTracer->minPower : rayTracer->minDosage) * 2.0f / numberCount);
		draw_list->AddText(ImVec2(panelPos.x + i * ((pickerSize) / numberCount) + 5, panelPos.y + numberHeight), 0xFFFFFFFF,buf);
	}

	int lineCount = numberCount * 2;
	for (int i = 0; i <= lineCount; i++)
	{
		float posx = panelPos.x + i * ((pickerSize) / lineCount) + halfnumberwidth;
		draw_list->AddLine(ImVec2(posx, panelPos.y + lineHeight), ImVec2(posx, panelPos.y + lineHeight + 10), 0xFFFFFFFF);
	}
	End();

	//ShowDemoWindow();
	//TODO: explain camera controls
	//TODO: base height off the ground by creating histogram of vertex heights (below half of model) and taking the lowest max bucket

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