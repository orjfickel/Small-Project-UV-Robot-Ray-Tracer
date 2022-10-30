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

void IntersectTri(struct Ray* ray, struct Triangle* tri, const uint triID)
{
	float3 v0 = (float3)(tri->v0x, tri->v0y, tri->v0z);
	float3 v1 = (float3)(tri->v1x, tri->v1y, tri->v1z);
	float3 v2 = (float3)(tri->v2x, tri->v2y, tri->v2z);
	float3 dir = (float3)(ray->dirx, ray->diry, ray->dirz);
	float3 origin = (float3)(ray->origx, ray->origy, ray->origz);
	float3 edge1 = v1 - v0, edge2 = v2 - v0;
	float3 h = cross(dir, edge2);
	float a = dot(edge1, h);
	if (fabs(a) < 0.00001f) return; // ray parallel to triangle
	float f = 1 / a;
	float3 s = origin - v0;
	float u = f * dot(s, h);
	if (u < 0 | u > 1) return;
	const float3 q = cross(s, edge1);
	const float v = f * dot(dir, q);
	if (v < 0 | u + v > 1) return;
	const float t = f * dot(edge2, q);
	if (t > 0.0001f && t < ray->dist)
		ray->dist = t, ray->triID = triID;
}

float IntersectAABB(struct Ray* ray, struct BVHNode* node)
{
	float tx1 = (node->minx - ray->origx) * ray->dirx, tx2 = (node->maxx - ray->origx) * ray->dirx;
	float tmin = min(tx1, tx2), tmax = max(tx1, tx2);
	float ty1 = (node->miny - ray->origy) * ray->diry, ty2 = (node->maxy - ray->origy) * ray->diry;
	tmin = max(tmin, min(ty1, ty2)), tmax = min(tmax, max(ty1, ty2));
	float tz1 = (node->minz - ray->origz) * ray->dirz, tz2 = (node->maxz - ray->origz) * ray->dirz;
	tmin = max(tmin, min(tz1, tz2)), tmax = min(tmax, max(tz1, tz2));
	if (tmax >= tmin && tmin < ray->dist && tmax > 0) return tmin; else return 1e30f;
}

void BVHIntersect(struct Ray* ray, /*uint instanceIdx,*/
	struct Triangle* tri, struct BVHNode* bvhNode, uint* triIdx)
{
	struct BVHNode* node = &bvhNode[0], * stack[32];
	uint stackPtr = 0;
	while (1)
	{
		if (node->triCount > 0) // isLeaf()
		{
			for (uint i = 0; i < node->triCount; i++)
			{
				uint triID = triIdx[node->leftFirst + i];
				IntersectTri(ray, &tri[triID], triID);
			}
			if (stackPtr == 0) break; else node = stack[--stackPtr];
			continue;
		}
		struct BVHNode* child1 = &bvhNode[node->leftFirst];
		struct BVHNode* child2 = &bvhNode[node->leftFirst + 1];
		float dist1 = IntersectAABB(ray, child1);
		float dist2 = IntersectAABB(ray, child2);
		if (dist1 > dist2)
		{
			float d = dist1; dist1 = dist2; dist2 = d;
			struct BVHNode* c = child1; child1 = child2; child2 = c;
		}
		if (dist1 == 1e30f)
		{
			if (stackPtr == 0) break; else node = stack[--stackPtr];
		}
		else
		{
			node = child1;
			if (dist2 != 1e30f) stack[stackPtr++] = child2;
		}
	}
}

__kernel void render(__global int* tempPhotonMap, int offset, __global struct Ray* rays,// __global unsigned int* triangles, int triangleCount,
	__global struct Triangle* vertices, int triangleCount)
{
	const int threadID = get_global_id(0);

	struct Ray* newray = rays + threadID;

	float closestDist = 1000000;
	int triID = -1;
	for (int i = 0; i < triangleCount; i++)
	{
		struct Triangle tri = vertices[i];
		bool hit = TriangleIntersect(newray, (float3)(tri.v0x, tri.v0y, tri.v0z),
			(float3)(tri.v1x, tri.v1y, tri.v1z), (float3)(tri.v2x, tri.v2y, tri.v2z));
		//bool hit = TriangleIntersect(newray, (float3)(0, 1, 2),
		//	(float3)(2, 1, 0), (float3)(2, 3, 1));
		if (hit && newray->dist > 0 && newray->dist < closestDist)
		{
			closestDist = newray->dist;
			triID = i;
		}
	}
	//TODO: either increment photon count for the triangle as a whole for cumulative dosis, or increment a bucket in a histogram of time points for the triangle for the max dosis (largest bucket wins)
	if (triID >= 0) {
		volatile __global int* triPtr = tempPhotonMap + triID;
		atomic_inc(triPtr);
	}
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
//

// EOF