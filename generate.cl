#include "template/common.h"
#include "cl/tools.cl"

#pragma OPENCL EXTENSION cl_khr_fp64 : enable

static uint SEED;

__kernel void render(__global struct Ray* rays, float3 lightPos, 
	float lightLength)
{
	const int threadID = get_global_id(0);

	uint seed = WangHash((threadID + 1) * 17 + lightPos.x * 13 + lightPos.y * 7 + lightPos.z * 11 + SEED);
	
	struct Ray newray;
	//struct LightPos lightPos = lightPositions[(int)(RandomFloat(&seed) * (lightCount-1))];
	float3 origin = (float3)(lightPos.x, lightPos.y + RandomFloat(&seed) * lightLength, lightPos.z);
	newray.origx = origin.x;
	newray.origy = origin.y;
	newray.origz = origin.z;

	// Generate random y component and random horizontal angle, then determine corresponding x and z components.
	float diry = RandomFloat(&seed) * 2.0f - 1.0f;
	double dirxz = sqrt(1.0 - (double)diry * diry);
	double angle = (double)RandomFloat(&seed) * 2.0 * M_PI;
	float dirx = (float)(cos(angle) * dirxz);
	float dirz = (float)(sin(angle) * dirxz);

	//float3 dir = (float3)(RandomFloat(&seed) * 2 - 1, RandomFloat(&seed) * 2 - 1, RandomFloat(&seed) * 2 - 1);//TODO: dependent on light normal? Cosine distribution
	//while (dot(dir, dir) > 1) { // Keep generating random cube vectors until we find one within the sphere
	//	dir = (float3)(RandomFloat(&seed) * 2 - 1, RandomFloat(&seed) * 2 - 1, RandomFloat(&seed) * 2 - 1);
	//}
	//dir = normalize(dir);
	//float length = sqrt(dirx * dirx + diry * diry + dirz * dirz);
	newray.dirx = dirx;
	newray.diry = diry;
	newray.dirz = dirz;
	newray.dist = 1e30f;
	newray.triID = 0;

	rays[threadID] = newray;
	
	if (threadID == 0) SEED = seed;
}

// EOF