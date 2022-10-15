#include "template/common.h"
#include "cl/tools.cl"

// Adapted from https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/moller-trumbore-ray-triangle-intersection
bool TriangleIntersect(struct Ray* ray, float3 v1, float3 v2, float3 v3)
{
	float3 v1v2 = v2 - v1;
	float3 v1v3 = v3 - v1;
	float3 dir = (float3)(ray->dirx, ray->diry, ray->dirz);
	float3 origin = (float3)(ray->origx, ray->origy, ray->origz);
	const float3 pvec = cross(dir, v1v3);
	float det = dot(v1v2, pvec);

	// ray and triangle are parallel if det is close to 0
	if (fabs(det) <= 0.0001f) return false;

	float invDet = 1 / det;

	const float3 tvec = origin - v1;
	float u = dot(tvec, pvec) * invDet;
	if (u < 0 || u > 1) return false;

	const float3 qvec = cross(tvec, v1v2);
	float v = dot(dir, qvec) * invDet;

	if (v < 0 || u + v > 1) return false;

	ray->dist = dot(v1v3, qvec) * invDet;

	return true;
}

bool Test(struct Ray* ray) {
	ray->dist = 23509.2f;
	return true;
}

__kernel void render(__global float4* photonMap, int offset, __global struct Ray* rays,// __global unsigned int* triangles, int triangleCount,
	__global float* vertices, int vertexCount)
{
	const int threadID = get_global_id(0);

	struct Ray* newray = rays + threadID;

	float closestDist = 1000000;
	for (int i = 0; i < vertexCount/3; i++)
	{
		uint v1 = (i) * 3;
		uint v2 = (i + 1) * 3;
		uint v3 = (i + 2) * 3;
		bool hit = TriangleIntersect(newray, (float3)(vertices[v1], vertices[v1 + 1], vertices[v1 + 2]),
			(float3)(vertices[v2], vertices[v2 + 1], vertices[v2 + 2]), (float3)(vertices[v3], vertices[v3 + 1], vertices[v3 + 2]));
		//bool hit = TriangleIntersect(newray, (float3)(0, 1, 2),
		//	(float3)(2, 1, 0), (float3)(2, 3, 1));
		if (hit && newray->dist > 0 && newray->dist < closestDist)
		{
			closestDist = newray->dist;
		}
	}
	//cout << "intensity " << lightIntensity << " distsqr " << (closestDist * closestDist) << endl;
	photonMap[threadID + offset] = (float4)(newray->origx + newray->dirx * closestDist, newray->origy + newray->diry * closestDist, newray->origz + newray->dirz * closestDist, 1);
	//if (photonMap[threadID + offset].x > 0) photonMap[threadID + offset] = (float4)(0, 0, 0, 0);
}
//

// EOF