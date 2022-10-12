#include "template/common.h"
#include "cl/tools.cl"

//// Adapted from https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/moller-trumbore-ray-triangle-intersection
//bool Tmpl8::TriangleIntersect(Ray& ray, float3 v1, float3 v2, float3 v3, float& u, float& v)
//{
//	float3 v1v2 = v2 - v1;
//	float3 v1v3 = v3 - v1;
//	const float3 pvec = cross(ray.dir, v1v3);
//	float det = dot(v1v2, pvec);
//
//	// ray and triangle are parallel if det is close to 0
//	if (fabs(det) <= 0.0001f) return false;
//
//	float invDet = 1 / det;
//
//	const float3 tvec = ray.origin - v1;
//	u = dot(tvec, pvec) * invDet;
//	if (u < 0 || u > 1) return false;
//
//	const float3 qvec = cross(tvec, v1v2);
//	v = dot(ray.dir, qvec) * invDet;
//
//	if (v < 0 || u + v > 1) return false;
//
//	ray.dist = dot(v1v3, qvec) * invDet;
//
//	return true;
//}

__kernel void render(__global float4* photonMap, const int offset, __global float4* rays)
{
//	// plot a pixel to outimg
//	const int i = get_global_id(0);
//
//	float closestDist = 1000000;
//	for (int i = 0; i < triangleCount; i += 3)
//	{
//		float u, v;
//		uint v1 = triangles[i] * 5;
//		uint v2 = triangles[i + 1] * 5;
//		uint v3 = triangles[i + 2] * 5;
//		bool hit = TriangleIntersect(newray, make_float3(vertices[v1], vertices[v1 + 1], vertices[v1 + 2]),
//			make_float3(vertices[v2], vertices[v2 + 1], vertices[v2 + 2]), make_float3(vertices[v3], vertices[v3 + 1], vertices[v3 + 2]), u, v);
//		if (hit && newray.dist > 0 && newray.dist < closestDist)
//		{
//			closestDist = newray.dist;
//		}
//	}
//	//cout << "intensity " << lightIntensity << " distsqr " << (closestDist * closestDist) << endl;
//	float4 
//	dosageMap[i] = (newray.origin + newray.dir * closestDist);
}
//

// EOF