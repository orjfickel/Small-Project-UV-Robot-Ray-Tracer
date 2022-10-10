#include "template/common.h"
#include "raytracer.h"
#include "cl/tools.cl"

__kernel void render(__global float* dosageMap, const int offset, __global Ray* rays)
{
	// plot a pixel to outimg
	const int i = get_global_id(0);

	Ray newray{};
	newray.origin = make_float3(lightPos.x, lightHeight /*+ RandomFloat() *lightLength*/, lightPos.y);//TODO: pick random pos on line
	newray.dir = make_float3(RandomFloat() * 2 - 1, RandomFloat() * 2 - 1, RandomFloat() * 2 - 1);//TODO: dependent on light normal? Cosine distribution
	while (sqrLength(newray.dir) > 1) { // Keep generating random cube vectors until we find one within the sphere
		newray.dir = make_float3(RandomFloat() * 2 - 1, RandomFloat() * 2 - 1, RandomFloat() * 2 - 1);
	}
	newray.dir = normalize(newray.dir);

	rays[i] = newray;
}

// EOF