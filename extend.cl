#include "template/common.h"
#include "cl/tools.cl"

__kernel void render(__global float* dosageMap, const int offset, __global float4* rays)
{
	// plot a pixel to outimg
	const int i = get_global_id(0);

	float closestDist = 1000000;
	for (int i = 0; i < triangleCount; i += 3)
	{
		float u, v;
		uint v1 = triangles[i] * 5;
		uint v2 = triangles[i + 1] * 5;
		uint v3 = triangles[i + 2] * 5;
		bool hit = TriangleIntersect(newray, make_float3(vertices[v1], vertices[v1 + 1], vertices[v1 + 2]),
			make_float3(vertices[v2], vertices[v2 + 1], vertices[v2 + 2]), make_float3(vertices[v3], vertices[v3 + 1], vertices[v3 + 2]), u, v);
		if (hit && newray.dist > 0 && newray.dist < closestDist)
		{
			closestDist = newray.dist;
		}
	}
	//cout << "intensity " << lightIntensity << " distsqr " << (closestDist * closestDist) << endl;
	dosageMap[i] = (newray.origin + newray.dir * closestDist);
}

// EOF