#include "template/common.h"
#include "cl/tools.cl"

static uint SEED;

__kernel void render(__global struct Ray* rays, float2 lightPos, float lightHeight, float lightLength)
{
	const int threadID = get_global_id(0);
	uint seed = WangHash((threadID + 1) * 17 + lightHeight * 13 + lightPos.x * 7 + lightPos.y * 11 + SEED);
	
	struct Ray newray;
	float3 origin = (float3)(lightPos.x, lightHeight + RandomFloat(&seed) * lightLength, lightPos.y);
	newray.origx = origin.x;
	newray.origy = origin.y;
	newray.origz = origin.z;

	// Generate random y component and random horizontal angle, then determine corresponding x and z components.
	float diry = RandomFloat(&seed) * 2.0f - 1.0f;
	float dirxz = sqrt(1.0f - diry * diry);
	float angle = RandomFloat(&seed) * 2 * M_PI_F;
	float dirx = cos(angle) * dirxz;
	float dirz = sin(angle) * dirxz;

	//float3 dir = (float3)(RandomFloat(&seed) * 2 - 1, RandomFloat(&seed) * 2 - 1, RandomFloat(&seed) * 2 - 1);//TODO: dependent on light normal? Cosine distribution
	//while (dot(dir, dir) > 1) { // Keep generating random cube vectors until we find one within the sphere
	//	dir = (float3)(RandomFloat(&seed) * 2 - 1, RandomFloat(&seed) * 2 - 1, RandomFloat(&seed) * 2 - 1);
	//}
	//dir = normalize(dir);
	newray.dirx = dirx;
	newray.diry = diry;
	newray.dirz = dirz;

	rays[threadID] = newray;
	
	if (threadID == 0) SEED = seed;
}

// EOF