#include "template/common.h"
#include "cl/tools.cl"

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
	float tx1 = (node->minx - ray->origx) / ray->dirx, tx2 = (node->maxx - ray->origx) / ray->dirx;
	float tmin = min(tx1, tx2), tmax = max(tx1, tx2);
	float ty1 = (node->miny - ray->origy) / ray->diry, ty2 = (node->maxy - ray->origy) / ray->diry;
	tmin = max(tmin, min(ty1, ty2)), tmax = min(tmax, max(ty1, ty2));
	float tz1 = (node->minz - ray->origz) / ray->dirz, tz2 = (node->maxz - ray->origz) / ray->dirz;
	tmin = max(tmin, min(tz1, tz2)), tmax = min(tmax, max(tz1, tz2));
	if (tmax >= tmin && tmin < ray->dist && tmax > 0) return tmin; else return 1e30f;
}


void BVHIntersect(struct Ray* ray,
	struct Triangle* tri, struct BVHNode* bvhNode, uint* triIdx)
{
	struct BVHNode* node = &bvhNode[0], * stack[32];
	uint stackPtr = 0;
	int level = 0;
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
			if (dist2 != 1e30f) {
				stack[stackPtr++] = child2; 
			}
		}
		level++;
	}
}

__kernel void render(__global int* tempPhotonMap, __global struct Triangle* triangles, __global struct Ray* rays,
	__global struct BVHNode* bvhNodes, __global uint* idxData, int triangleCount)
{
	const int threadID = get_global_id(0);

	struct Ray* newray = rays + threadID;

	BVHIntersect(newray, triangles, bvhNodes, idxData);

	//float closestDist = 1000000;
	//int triID = -1;
	//for (int i = 0; i < triangleCount; i++)
	//{
	//	//struct Triangle tri = &triangles[i];
	//	/*bool hit = */IntersectTri(newray, &triangles[i], i);
	//	//bool hit = TriangleIntersect(newray, (float3)(0, 1, 2),
	//	//	(float3)(2, 1, 0), (float3)(2, 3, 1));
	//	//if (hit && newray->dist > 0 && newray->dist < closestDist)
	//	//{
	//	//	closestDist = newray->dist;
	//	//	newray->triID = i;
	//	//}
	//}

	// If the ray hit something, increment that triangle's photon count
	if (newray->dist != 1e30f) {
		volatile __global int* triPtr = tempPhotonMap + newray->triID;
		atomic_inc(triPtr);
	}
}
//

// EOF