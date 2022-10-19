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

__kernel void render(__global struct Ray* rays,// __global unsigned int* triangles, int triangleCount,
	__global struct Triangle* vertices, int vertexCount)
{
	const int threadID = get_global_id(0);

	struct Ray* newray = rays + threadID;

	float closestDist = 1000000;
	for (int i = 0; i < vertexCount/9; i++)
	{
		struct Triangle tri = vertices[i];
		bool hit = TriangleIntersect(newray, (float3)(tri.v1x, tri.v1y, tri.v1z),
			(float3)(tri.v2x, tri.v2y, tri.v2z), (float3)(tri.v3x, tri.v3y, tri.v3z));
		//bool hit = TriangleIntersect(newray, (float3)(0, 1, 2),
		//	(float3)(2, 1, 0), (float3)(2, 3, 1));
		if (hit && newray->dist > 0 && newray->dist < closestDist)
		{
			closestDist = newray->dist;
			newray->triangleID = i;
		}
	}
	//TODO: either increment photon count for the triangle as a whole for cumulative dosis, or increment a bucket in a histogram of time points for the triangle for the max dosis (largest bucket wins)
	
	// 
	//cout << "intensity " << lightIntensity << " distsqr " << (closestDist * closestDist) << endl;]
	//struct Photon newPhoton;
	//newPhoton.posx = newray->origx + newray->dirx * closestDist;
	//newPhoton.posy = newray->origy + newray->diry * closestDist;
	//newPhoton.posz = newray->origz + newray->dirz * closestDist;
	//newPhoton.timeStep = 1;
	//newPhoton.timePoint = 0;
	//newPhoton.triangleID = /*(vertexCount/3) - */triID;
	//photonMap[threadID + offset] = newPhoton;
	//if (photonMap[threadID + offset].x > 0) photonMap[threadID + offset] = (float4)(0, 0, 0, 0);
}

__kernel void count(__global int* triangleDosage, __global struct Ray* rays) {
	const int threadID = get_global_id(0);
	volatile __global int* triPtr = triangleDosage + rays[threadID].triangleID;
	atomic_inc(triPtr);
}
//

// EOF