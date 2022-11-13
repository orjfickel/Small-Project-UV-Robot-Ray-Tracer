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
	StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 130");
}

// Puts a "(?)" symbol on the same line that displays a popup when hovering over it, adapted from the imgui_demo.cpp file
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
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	NewFrame();
	Begin(language == dutch ? "Statistieken###Statistics" : "Statistics###Statistics", 0);
	SetWindowFontScale(1.5f);
	Text(language == dutch ? "Aantal driehoeken: %u" : "Number of triangles: %u", rayTracer->mesh->triangleCount);
	Text(language == dutch ? "Aantal fotonen: %i" : "Number of photons: %i", rayTracer->photonMapSize);
	End();

	if (showControls) {
		Begin(language == dutch ? "Uitleg###Guide" : "Guide###Guide", &showControls, ImGuiWindowFlags_NoNavInputs);
		SetWindowFontScale(1.5f);
		PushTextWrapPos(GetFontSize() * 35.0f);
		if (language == dutch) {
			Text("Gebruik de WASDQE toetsen om de camera te bewegen.");
			Text("Gebruik de pijltjes toetsen om de camera te draaien.");
			Text("Klik op \"Lamp positie toevoegen\" en selecteer de lamp positie in de lijst onder \"Lamp posities\". Gebruik WASD om de lamp te verplaatsen.");
			Text("Pas de overige parameters aan en klik op \"Bereken UV straling\" om de heatmap van de straling te tonen.");
			Text("De heatmap toont de cumulatieve of maximale UV straling die elke driehoek in het 3D model heeft ontvangen, vanuit de discrete lamp posities.");
			Text("Om de kleur van de heatmap te schalen, open het kopje \"Geavanceerd\" en pas de drempelwaarde(s) aan.");
			Text("Tip: beweeg de muis over de \"(?)\" symbolen voor extra uitleg of informatie.");
		} else if (language == english)
		{
			Text("Use the WASDQE keys to move the camera.");
			Text("Use the arrow keys to turn the camera.");
			Text("Click \"Add lamp position\" and select the lamp position from the list under \"Lamp positions\". Use WASD to move the lamp.");
			Text("Edit the remaining parameters and click \"Compute UV radiation\" to compute and show the dosage heatmap.");
			Text("The heatmap shows the cumulative or maximal UV radiation that every mesh triangle in the 3D model has received, emitted from the discrete lamp positions.");
			Text("To scale the heatmap color, click \"Advanced\" and adjust the threshold value(s).");
			Text("Tip: move the cursor over the \"(?)\" symbols for additional explanation or information.");
		}
		PopTextWrapPos();
		End();
	}

	Begin("Parameters", 0, ImGuiWindowFlags_NoNavInputs);
	SetWindowFontScale(1.5f);
	PushItemWidth(-2);

	if (Button(language == dutch ? "Handleiding" : "Guide"))
	{
		showControls = !showControls;
	}
	SameLine();
	if(language == dutch && Button("Switch naar Engels"))
	{
		language = english;
	} else if (language == english && Button("Switch to Dutch"))
	{
		language = dutch;
	}
	
	Text(language == dutch ? "Lamp sterkte (W)" : "Lamp power (W)");
	HelpMarker(language == dutch ? "Hoe veel energie de lamp uitstraalt" : "How much energie is emitted by the lamp"); SameLine();
	InputFloat("##power", &rayTracer->lightIntensity,0,0,"%.2f");

	if (Button(language == dutch ? "Calibreren" : "Calibrate"))
	{
		OpenPopup(language == dutch ? "Calibreren###Calibrate" : "Calibrate###Calibrate");
	}
	if (BeginPopupModal(language == dutch ? "Calibreren###Calibrate" : "Calibrate###Calibrate"))
	{
		// The irradiance that is manually measured and used to calibrate the simulation.
		static float measurePower = 2909;
		static float measureHeight = 0.8f;
		static float measureDist = 1;

		PushTextWrapPos(GetFontSize() * 35.0f);
		Text(language == dutch ? "Vul de gemeten waardes in en druk op \"Calibreren\"." : "Enter the measured values and click \"Calibrate\".");
		Text(language == dutch ? "De lamp sterkte wordt lineair aangepast op basis van de hoe gemeten bestralingsterkte vergelijkt met de gesimuleerde bestralingssterkte." :
			"The lamp power is linearly adjusted based on how the measured irradiance compares to the simulated irradiance.");
		Text(language == dutch ? "De calibraties van meerdere metingen zullen handmatig moeten worden samengevoegd (bijvoorbeeld door de gemiddelde lamp sterkte te nemen)." :
			"The calibrations of multiple measurements will need to be manually merged (for example by taking the average lamp power).");
		PopTextWrapPos();

		float offset = 400;
		Text(language == dutch ? "Meting bestralingssterkte (\xC2\xB5W/cm^2 W)" : "Measured irradiance (\xC2\xB5W/cm^2 W)"); SameLine(offset);
		InputFloat("##measurepower", &measurePower, 0, 0, "%.2f");

		Text(language == dutch ? "Meting hoogte (m)" : "Measurement height (m)");
		HelpMarker(language == dutch ? "Hoe hoog boven de vloer de meting is genomen" : "How high above the floor the measurement was taken"); SameLine(offset);
		InputFloat("##measureheight", &measureHeight, 0, 0, "%.2f");

		Text(language == dutch ? "Meting lamp afstand (m)" : "Measurement lamp distance (m)"); SameLine(offset);
		InputFloat("##measuredist", &measureDist, 0, 0, "%.2f");

		if (Button(language == dutch ? "Calibreren##calib2" : "Calibrate##calib2"))
		{
			rayTracer->CalibratePower(measurePower, measureHeight, measureDist);
		}
		if (Button(language == dutch ? "Sluiten" : "Close"))
		{
			CloseCurrentPopup();
		}
		EndPopup();
	}
	
	Text(language == dutch ? "Lamp lengte (m)" : "Lamp length (m)"); SameLine();
	InputFloat("##length", &rayTracer->lightLength, 0, 0, "%.2f");
	Text(language == dutch ? "Lamp hoogte boven vloer (m)" : "Lamp height above floor (m)"); SameLine();
	InputFloat("##height", &rayTracer->lightHeight, 0, 0, "%.2f");
	HelpMarker(language == dutch ? "Hoe hoog de lamp boven de vloer staat. De hoogte van de vloer wordt automatisch bepaald." : 
		"How high above the floor the lamp is positioned. The height of the floor is automatically detemined.");

	if (addedLamp) {
		SetNextTreeNodeOpen(true);
		addedLamp = false;
	}
	bool tempOpen = CollapsingHeader(language == dutch ? "Lamp posities###Lamp positions" : "Lamp positions###Lamp positions");
	HelpMarker(language == dutch ? "Discrete plekken waar de lamp voor bepaalde tijdsduren staat. Selecteer een positie uit de lijst om met WASD te verplaatsen" :
		"Discrete places where the lamp remains stationary for certain amounts of time. Select a position from the list to move it with WASD");
	if (tempOpen) {
		BeginChild("lightpositions", ImVec2(0,220), false, ImGuiWindowFlags_NoNavInputs);
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
			Text(language == dutch ? "Positie %i (m)" : "Position %i (m)", i + 1); SameLine();
			InputFloat2(("##position_" + std::to_string(i)).c_str(), rayTracer->lightPositions[i].position.cell, "%.2f");
			Text(language == dutch ? "Tijdsduur (s)" : "Duration (s)  "); SameLine();
			PushItemWidth(language == dutch ? -110 : -80);
			InputFloat(("##duration_" + std::to_string(i)).c_str(), &rayTracer->lightPositions[i].duration, 0, 0, "%.2f");
			SameLine();
			PushItemWidth(-2);
			if (Button(((language == dutch ? "Verwijder###delete_" : "Delete###delete_") + std::to_string(i)).c_str()))
			{
				rayTracer->lightPositions.erase(rayTracer->lightPositions.begin()+i);
			}
			EndGroup();
			ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 5));
		}
		EndChild();
	}

	if (Button(language == dutch ? "Lamp positie toevoegen" : "Add lamp position"))
	{
		rayTracer->AddLamp();
		addedLamp = true;
		selectedLightPos = rayTracer->lightPositions.size() - 1;
	}

	if (Button(language == dutch ? "Parameters opslaan" : "Save parameters"))
	{
		OpenPopup("savePopup");
	}
	if (BeginPopup("savePopup"))
	{
		Text(language == dutch ? "Bestand naam:" : "File name:");
		InputText("##edit", rayTracer->newRouteFile, IM_ARRAYSIZE(rayTracer->newRouteFile));
		if (Button(language == dutch ? "Opslaan" : "Save")) {
			CloseCurrentPopup();
			rayTracer->SaveRoute(rayTracer->newRouteFile);
		}
		EndPopup();
	}
	SameLine();
	if (Button(language == dutch ? "Parameters laden" : "Load parameters"))
	{
		OpenPopup("loadPopup");
	}
	if (BeginPopup("loadPopup"))
	{
		Text(language == dutch ? "Bestand naam:" : "File name:");
		InputText("##edit2", rayTracer->newRouteFile, IM_ARRAYSIZE(rayTracer->newRouteFile));
		if (Button(language == dutch ? "Laden" : "Load")) {
			CloseCurrentPopup();
			rayTracer->LoadRoute(rayTracer->newRouteFile);
		}
		EndPopup();
	}

	if (Button(language == dutch ? "Kamer model laden" : "Load room model"))
	{
		OpenPopup("loadModelPopup");
	}
	if (BeginPopup("loadModelPopup"))
	{
		Text(language == dutch ? "Bestand naam:" : "File name:");
		HelpMarker(language == dutch ? "Model bestanden moeten in .glb formaat in de \"rooms\" folder staan" : "Model files should be in .glb format, in the \"rooms\" folder");
		InputText("##editmodelname", rayTracer->mesh->modelFile, 32);
		if (Button(language == dutch ? "Laden" : "Load")) {
			CloseCurrentPopup();
			glDeleteVertexArrays(1, &rayTracer->mesh->VAO);
			glDeleteBuffers(1, &rayTracer->mesh->VBO);
			glDeleteBuffers(1, &rayTracer->mesh->UVBuffer);
			glDeleteTextures(1, &rayTracer->mesh->textureBuffer);
			glDeleteBuffers(1, &rayTracer->mesh->dosageBufferID);
			rayTracer->mesh->LoadMesh();
			rayTracer->Init(rayTracer->mesh);
			rayTracer->viewMode = texture;
			rayTracer->startedComputation = false;
		}
		EndPopup();
	}

	if ((rayTracer->startedComputation && Button(language == dutch ? "Herbereken UV straling" : "Recompute UV radiation")) 
		|| (!rayTracer->startedComputation && Button(language == dutch ? "Bereken UV straling" : "Compute UV radiation")))
	{
		rayTracer->ResetDosageMap();
	}

	Text("");

	tempOpen = Selectable(language == dutch ? "Toon foto scan" : "Show photo scan", rayTracer->viewMode == texture);
	HelpMarker(language == dutch ? "De kamer gekleurd zoals met de camera is ingescand" : "The room colored as it was scanned using the phone camera");
	if (tempOpen)
	{
		rayTracer->viewMode = texture;
	}
	tempOpen = Selectable(language == dutch ? "Toon cumulatieve dosis" : "Show cumulative dosage", rayTracer->viewMode == dosage);
	HelpMarker(language == dutch ? "De cumulatieve dosis UV straling dat elk oppervlak heeft ontvangen" : "The cumulative dose of UV radiation that each surface has received");
	if (tempOpen)
	{
		rayTracer->viewMode = dosage;
		if (!rayTracer->startedComputation)
			rayTracer->ResetDosageMap();
		rayTracer->Shade();
	}
	tempOpen = Selectable(language == dutch ? "Toon max bestralingssterkte" : "Show max irradiance", rayTracer->viewMode == maxpower);
	HelpMarker(language == dutch ? "De maximale UV straling dat elk oppervlak heeft ontvangen" : "The maximal UV radiation that each surface has received");
	if (tempOpen)
	{
		rayTracer->viewMode = maxpower;
		if (!rayTracer->startedComputation)
			rayTracer->ResetDosageMap();
		rayTracer->Shade();
	}

	Text("");

	if (!showLights && Button(language == dutch ? "Toon lamp posities" : "Show lamp positions"))
		showLights = true;
	else if (showLights && Button(language == dutch ? "Verberg lamp posities" : "Hide lamp positions"))
		showLights = false;

	if (!rayTracer->thresholdView && Button(language == dutch ? "Toon drempelwaarde" : "Show threshold view"))
	{
		rayTracer->thresholdView = true;
		if (rayTracer->viewMode != texture) rayTracer->Shade();
	}
	else if (rayTracer->thresholdView && Button(language == dutch ? "Verberg drempelwaarde" : "Hide threshold view"))
	{
		rayTracer->thresholdView = false;
		if (rayTracer->viewMode != texture) rayTracer->Shade();
	}
	HelpMarker(language == dutch ? "Alle waardes onder de drempelwaarde (zie kopje \"Geavanceerd\") worden donkerder gemaakt" :
		"All values under the threshold value (see under \"Advanced\") are darkened");

	if (CollapsingHeader(language == dutch ? "Geavanceerd###Advanced" : "Advanced###Advanced")) {
		Text(language == dutch ? "Legenda drempel \ndosis (mJ/cm^2)" : "Legend threshold \ndose (mJ/cm^2)");
		HelpMarker(language == dutch ? "De heatmap wordt zo geschaald dat deze waarde groen is" : "The heatmap is scaled such that this value is green"); SameLine();
		float tempminValue = rayTracer->minDosage;
		InputFloat("##mindosage", &rayTracer->minDosage, 0, 0, "%.2f");
		// Update the shading if the scaling dosage is changed
		if (rayTracer->startedComputation && tempminValue != rayTracer->minDosage && rayTracer->viewMode == dosage)
			rayTracer->Shade();

		Text(language == dutch ? "Drempel bestralings-\nsterkte (\xC2\xB5W/cm^2)" : "Threshold irradiation\n (\xC2\xB5W/cm^2)");
		HelpMarker(language == dutch ? "De heatmap wordt zo geschaald dat deze waarde groen is" : "The heatmap is scaled such that this value is green"); SameLine();
		tempminValue = rayTracer->minPower;
		InputFloat("##minpower", &rayTracer->minPower, 0, 0, "%.2f");
		// Update the shading if the scaling dosage is changed
		if (rayTracer->startedComputation && tempminValue != rayTracer->minPower && rayTracer->viewMode == maxpower)
			rayTracer->Shade();

		Text(language == dutch ? "Fotonen per iteratie" : "Photons per iteration");
		HelpMarker(language == dutch ? "Meer fotonen zorgen voor minder ruis in de berekening" : "More photons mean less noise in the computation");
		SameLine();
		InputInt("##photonCount", &rayTracer->photonCount, 0, 0);
		rayTracer->photonCount = min(rayTracer->maxPhotonCount, rayTracer->photonCount);
		if (rayTracer->photonCount > rayTracer->maxPhotonCount || rayTracer->photonCount < 1)
		{
			rayTracer->photonCount = rayTracer->maxPhotonCount;
		}

		if (rayTracer->maxIterations > 1)
		{
			Text(language == dutch ? "Note: de max bestralingssterkte \nmap werkt slechts voor 1 iteratie " : "Note: the max irradiation \nmap only works for 1 iteration");
		}
		else if (rayTracer->maxIterations < 1)
		{
			rayTracer->maxIterations = 1;
		}
		bool actuallyReachedMaxPhotons = rayTracer->finishedComputation && rayTracer->currIterations >= rayTracer->maxIterations;
		if (rayTracer->startedComputation && rayTracer->finishedComputation && !actuallyReachedMaxPhotons)
		{
			if (Button(language == dutch ? "Berekening hervatten" : "Resume computation")) {
				rayTracer->finishedComputation = false;
			}
		}

		Text(language == dutch ? "Aantal iteraties" : "Number of iterations"); SameLine();
		InputInt("##iterations", &rayTracer->maxIterations, 1, 0);
	}

	// Progress popup
	if (!rayTracer->finishedComputation || rayTracer->progressTextTimer > 0) {
		SetNextWindowPos(ImVec2(SCRWIDTH - 230, 10), 0);
		SetNextWindowSize(ImVec2(220, 0), 0);
		Begin("progress", 0, ImGuiCond_FirstUseEver | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs);
		SetWindowFontScale(1.5f);
		if (!rayTracer->finishedComputation) {
			Text(language == dutch ? "Vooruitgang: %.2f%%" : "Progress: %.2f%%", rayTracer->progress);
		}
		else {
			Text(language == dutch ? "Berekening klaar" : "Computation done");
		}
		End();
	}

	// Draw the heatmap color legend
	static float pickerSize = 420;
	static int halfnumberwidth = 25;
	float rainbowHeight = 60, lineHeight = 50, numberHeight = 30;
	SetNextWindowPos(ImVec2(SCRWIDTH - pickerSize - 2 * halfnumberwidth - 20, SCRHEIGHT - rainbowHeight - 45), 0);
	SetNextWindowSize(ImVec2(pickerSize + 2 * halfnumberwidth + 10, rainbowHeight + 35), 0);
	Begin(rayTracer->viewMode == maxpower ? (language == dutch ? "Maximale bestralingssterkte (\xC2\xB5W/cm^2)" : "Maximal irradiation (\xC2\xB5W/cm^2)") :
		(language == dutch ? "Cumulatieve dosis (mJ/cm^2)" : "Cumulative dose (mJ/cm^2)"), 0,
		ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs);
	SetWindowFontScale(1.5f);

	ImDrawList* draw_list = GetWindowDrawList();
	ImVec2 panelPos = GetWindowPos();
	const ImU32 col_hues[4 + 1] = { IM_COL32(0,0,255,255), IM_COL32(0,255,255,255), IM_COL32(0,255,0,255), IM_COL32(255,255,0,255), IM_COL32(255,0,0,255) };
	const ImU32 col_hues_threshold[4 + 1] = { IM_COL32(0,0,0,255), IM_COL32(0,0,255/2,255), IM_COL32(0,0,255,255), IM_COL32(255,255,0,255), IM_COL32(255,0,0,255) };

	for (int i = 0; i < 4; ++i)
	{
		draw_list->AddRectFilledMultiColor(ImVec2(panelPos.x + i * (pickerSize / 4) + halfnumberwidth, panelPos.y+ rainbowHeight), 
			ImVec2(panelPos.x + (i + 1) * (pickerSize / 4) + halfnumberwidth, panelPos.y + rainbowHeight + 25), 
			i < 2 && rayTracer->thresholdView ? col_hues_threshold[i] : col_hues[i], i < 2 && rayTracer->thresholdView ? col_hues_threshold[i+1] : col_hues[i+1],
			i < 2 && rayTracer->thresholdView ? col_hues_threshold[i+1] : col_hues[i + 1], i < 2 && rayTracer->thresholdView ? col_hues_threshold[i] : col_hues[i]);
	}

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
	
	End();
	Render();
	ImGui_ImplOpenGL3_RenderDrawData(GetDrawData());
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