
#include "precomp.h"
#include "myapp.h"
#include "glm/glm.hpp"
#include "glm/ext.hpp"

using namespace glm;

Camera::Camera()
{
	view = glm::mat4(1.0f);
	view = lookAt(glm::vec3(1.0f, 0.1f, 0.8f), glm::vec3(0.0f, -0.1f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

void Camera::UpdateView(KeyPresses keyPresses, float deltaTime)// Move all camera view code here & make position change relative while keeping the position itself absolute.
{
	float movement = (keyPresses.shiftPress ? 0.01f : 0.001f) * deltaTime;
	float turning = (keyPresses.shiftPress ? 0.01f : 0.001f) * deltaTime;

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
