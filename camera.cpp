
#include "precomp.h"
#include "myapp.h"
#include "glm/glm.hpp"
#include "glm/ext.hpp"
#include "tinyxml2.h"
#include <glm/gtx/string_cast.hpp>

using namespace glm;
using namespace tinyxml2;

Camera::Camera()
{
	view = lookAt(vec3(1.0f, 0.1f, 0.8f), vec3(0.0f, -0.1f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
	Load();
}

void Camera::UpdateView(KeyPresses keyPresses, float deltaTime)// Move all camera view code here & make position change relative while keeping the position itself absolute.
{
	float movement = (keyPresses.shiftPress ? 0.015f : 0.005f) * deltaTime;
	float turning = (keyPresses.shiftPress ? 0.006f : 0.002f) * deltaTime;

	vec3 turnVec = vec3(0,0,0), moveVec = vec3(0, 0, 0);
	if (keyPresses.wPress) { moveVec.z += movement; }
	if (keyPresses.aPress) { moveVec.x += movement; }
	if (keyPresses.sPress) { moveVec.z -= movement; }
	if (keyPresses.dPress) { moveVec.x -= movement; }
	if (keyPresses.qPress) { moveVec.y -= movement; }
	if (keyPresses.ePress) { moveVec.y += movement; }
	if (keyPresses.upPress) { turnVec.x += turning; }
	if (keyPresses.leftPress) { turnVec.y += turning; }
	if (keyPresses.downPress) { turnVec.x -= turning; }
	if (keyPresses.rightPress) { turnVec.y -= turning; }
		
	view = rotate(view, turnVec.y, vec3(vec4(0, 1, 0, 1)));
	// Only turn up or down if doing so doesn't turn the room upside down
	if(view[1][1] > 0 || (view[1][2] > 0 != turnVec.x > 0)) 
		view = rotate(view, turnVec.x, vec3(vec4(view[0][0], view[1][0], view[2][0], 1)));
	view = translate(view, transpose(mat3(view)) * moveVec);	
}

void Camera::Save()
{
	XMLDocument doc;
	XMLNode* root = doc.NewElement("camera");
	doc.InsertFirstChild(root);
	XMLElement* viewElem = doc.NewElement("view");
	viewElem->SetAttribute("m00", view[0][0]);
	viewElem->SetAttribute("m01", view[0][1]);
	viewElem->SetAttribute("m02", view[0][2]);
	viewElem->SetAttribute("m03", view[0][3]);
	viewElem->SetAttribute("m10", view[1][0]);
	viewElem->SetAttribute("m11", view[1][1]);
	viewElem->SetAttribute("m12", view[1][2]);
	viewElem->SetAttribute("m13", view[1][3]);
	viewElem->SetAttribute("m20", view[2][0]);
	viewElem->SetAttribute("m21", view[2][1]);
	viewElem->SetAttribute("m22", view[2][2]);
	viewElem->SetAttribute("m23", view[2][3]);
	viewElem->SetAttribute("m30", view[3][0]);
	viewElem->SetAttribute("m31", view[3][1]);
	viewElem->SetAttribute("m32", view[3][2]);
	viewElem->SetAttribute("m33", view[3][3]);
	root->InsertEndChild(viewElem);
	((XMLElement*)root->InsertEndChild(doc.NewElement("FOV")))->SetText(FOV);
	doc.SaveFile(cameraFile);
}

void Camera::Load()
{
	XMLDocument doc;
	XMLError result = doc.LoadFile(cameraFile);
	if (result != XML_SUCCESS) return;
	XMLNode* root = doc.FirstChild();
	if (root == nullptr) return;
	XMLElement* docElem;
	if ((docElem = root->FirstChildElement("view"))) {
		docElem->QueryFloatAttribute("m00", &view[0][0]);
		docElem->QueryFloatAttribute("m01", &view[0][1]);
		docElem->QueryFloatAttribute("m02", &view[0][2]);
		docElem->QueryFloatAttribute("m03", &view[0][3]);
		docElem->QueryFloatAttribute("m10", &view[1][0]);
		docElem->QueryFloatAttribute("m11", &view[1][1]);
		docElem->QueryFloatAttribute("m12", &view[1][2]);
		docElem->QueryFloatAttribute("m13", &view[1][3]);
		docElem->QueryFloatAttribute("m20", &view[2][0]);
		docElem->QueryFloatAttribute("m21", &view[2][1]);
		docElem->QueryFloatAttribute("m22", &view[2][2]);
		docElem->QueryFloatAttribute("m23", &view[2][3]);
		docElem->QueryFloatAttribute("m30", &view[3][0]);
		docElem->QueryFloatAttribute("m31", &view[3][1]);
		docElem->QueryFloatAttribute("m32", &view[3][2]);
		docElem->QueryFloatAttribute("m33", &view[3][3]);
	}
	if ((docElem = root->FirstChildElement("FOV"))) docElem->QueryFloatText(&FOV);
}
