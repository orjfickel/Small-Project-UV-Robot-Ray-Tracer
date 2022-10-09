#include "precomp.h"

void RayTracer::computeDosageMap()
{
	//Shoot rays
	//dosageMap.push_back(make_float4(0, 0, 0, 900));
	//dosageMap.push_back(make_float4(1.8, 0, 0, 100));
	//dosageMap.push_back(make_float4(-1.5f, 0.4, -1.5f, 400));
	//dosageMap.push_back(make_float4(0, 0, 1.8, 1));

	for (uint i = 0; i < photonCount; ++i)
	{
		Ray newray{};
		newray.origin = make_float3(lightPos.x, lightHeight /*+ RandomFloat() *lightLength*/, lightPos.y);//TODO: pick random pos on line
		newray.dir = make_float3(RandomFloat() * 2 - 1, RandomFloat() * 2 - 1, RandomFloat() * 2 - 1);//TODO: dependent on light normal? Cosine distribution
		while (sqrLength(newray.dir) > 1) { // Keep generating random cube vectors until we find one within the sphere
			newray.dir = make_float3(RandomFloat() * 2 - 1, RandomFloat() * 2 - 1, RandomFloat() * 2 - 1);
		}
		newray.dir = normalize(newray.dir);
		float closestDist = 1000000;
		for (int i = 0; i < triangleCount; i += 3)
		{
			float u, v;
			uint v1 = triangles[i] * 5;
			uint v2 = triangles[i+1] * 5;
			uint v3 = triangles[i+2] * 5;
			bool hit = TriangleIntersect(newray, make_float3(vertices[v1], vertices[v1 + 1], vertices[v1 + 2]), 
				make_float3(vertices[v2], vertices[v2 + 1], vertices[v2 + 2]), make_float3(vertices[v3], vertices[v3 + 1], vertices[v3 + 2]), u, v);
			if (hit && newray.dist > 0 && newray.dist < closestDist)
			{
				closestDist = newray.dist;
			}
		}
		//cout << "intensity " << lightIntensity << " distsqr " << (closestDist * closestDist) << endl;
		dosageMap.push_back(newray.origin + newray.dir * closestDist);
	}

#if 0
	float3 color = make_float3(0, 0, 0);
	Ray newray{};
	for (int u = 0; u < screenWidth; u++) {
		for (int v = 0; v < screenHeight; v++) {
			float closestdist = 0;//TODO: find closest triangle
			color = make_float3(0, 0, 0);
			float3 umult = (camera.p2 - camera.p1) / screenWidth;
			float3 vmult = (camera.p3 - camera.p1) / screenHeight;
			newray.origin = camera.p1 + u * umult + v * vmult;
			newray.dir = normalize(newray.origin - camera.position);
			for (int i = 0; i < rayTracer.triangleCount; i += 3)
			{
				float u, v;
				bool hit = TriangleIntersect(newray, rayTracer.vertices[rayTracer.triangles[i]], rayTracer.vertices[rayTracer.triangles[i + 1]], rayTracer.vertices[rayTracer.triangles[i + 2]], u, v);
				if (hit)
				{
					//printf("ray hit %f p1: %f  %f %f \n", newray.dist * newray.dist * 0.008F, camera.p1.x, camera.p1.y, camera.p1.z);
					color = make_float3(newray.dist * newray.dist * 0.006F, newray.dist * newray.dist * 0.006F, newray.dist * newray.dist * 0.006F);
					break;
				}
			}

			//color = raytracer.hostcolorBuffer[u + v * (screenWidth)];
			screen->Plot(u, v, ((int)(min(color.z, 1.0f) * 255.0f) << 16) +
				((int)(min(color.y, 1.0f) * 255.0f) << 8) + (int)(min(color.x, 1.0f) * 255.0f));
		}
	}
#endif
}

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