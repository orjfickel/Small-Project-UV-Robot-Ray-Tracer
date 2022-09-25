#include "precomp.h"

// Adapted from https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/moller-trumbore-ray-triangle-intersection
bool Tmpl8::TriangleIntersect(Ray& ray, float3 v1, float3 v2, float3 v3, float& u, float& v)
{
	float3 v1v2 = v2 - v1;
	float3 v1v3 = v3 - v1;
	const float3 pvec = cross(ray.dir, v1v3);
	float det = dot(v1v2, pvec);

	// ray and triangle are parallel if det is close to 0
	if (fabs(det) <= 0.0001f) return false;

	float invDet = 1 / det;

	const float3 tvec = ray.origin - v1;
	u = dot(tvec, pvec) * invDet;
	if (u < 0 || u > 1) return false;

	const float3 qvec = cross(tvec, v1v2);
	v = dot(ray.dir, qvec) * invDet;

	if (v < 0 || u + v > 1) return false;

	ray.dist = dot(v1v3, qvec) * invDet;

	return true;
}