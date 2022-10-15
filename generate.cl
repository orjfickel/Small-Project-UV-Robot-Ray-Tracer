#include "template/common.h"
#include "cl/tools.cl"

static uint SEED;

__kernel void render(__global struct Ray* rays, float2 lightPos, float lightHeight)
{
	const int threadID = get_global_id(0);
	uint seed = WangHash((threadID + 1) * 17 + lightHeight * 13 + lightPos.x * 7 + lightPos.y * 11 + SEED);
	
	struct Ray newray;
	float3 origin = (float3)(lightPos.x, lightHeight /*+ RandomFloat(seed) *lightLength*/, lightPos.y);//TODO: pick random pos on line
	newray.origx = origin.x;
	newray.origy = origin.y;
	newray.origz = origin.z;

	float3 dir = (float3)(RandomFloat(&seed) * 2 - 1, RandomFloat(&seed) * 2 - 1, RandomFloat(&seed) * 2 - 1);//TODO: dependent on light normal? Cosine distribution
	while (dot(dir, dir) > 1) { // Keep generating random cube vectors until we find one within the sphere
		dir = (float3)(RandomFloat(&seed) * 2 - 1, RandomFloat(&seed) * 2 - 1, RandomFloat(&seed) * 2 - 1);
	}
	dir = normalize(dir);
	newray.dirx = dir.x;
	newray.diry = dir.y;
	newray.dirz = dir.z;

	rays[threadID] = newray;
	
	if (threadID == 0) SEED = seed;
}

// EOF