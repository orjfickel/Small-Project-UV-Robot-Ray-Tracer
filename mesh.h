#pragma once
#include <tiny_gltf.h>

namespace Tmpl8
{
	__declspec(align(64)) struct Tri
	{
		// union each float3 with a 16-byte __m128 for faster BVH construction
		union { float3_strict vertex0; __m128 v0; };
		union { float3_strict vertex1; __m128 v1; };
		union { float3_strict vertex2; __m128 v2; };
		union { float3_strict centroid; __m128 centroid4; }; // total size: 64 bytes
	};

	class Mesh
	{
	public:
		void LoadMesh(string modelFile);
		tinygltf::Model model;
		Tri* triangles;
		int triangleCount;
		float* vertices;
		int vertexCount; // Size of the triangles array. Number of vertices times 3 (1 for every axis) (includes duplicates)
		float* uvcoords;

		BVH* bvh = 0;
	};

};
