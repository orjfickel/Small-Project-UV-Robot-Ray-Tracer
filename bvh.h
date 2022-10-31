#pragma once

namespace Tmpl8
{//Forward declaration
	class Mesh;
}

// 32-byte BVH node struct
struct BVHNode //TODO: float3 should be actually only 3 floats instead of taking up 16 bytes -____-
{
	union { struct { float3_strict aabbMin; uint leftFirst; }; __m128 aabbMin4; };
	union { struct { float3_strict aabbMax; uint triCount; }; __m128 aabbMax4; };
	bool isLeaf() const { return triCount > 0; } // empty BVH leaves do not exist
	float CalculateNodeCost()
	{
		float3_strict e = aabbMax - aabbMin; // extent of the node
		return (e.x * e.y + e.y * e.z + e.z * e.x) * triCount;
	}
};

// enable the use of SSE in the AABB intersection function
#define USE_SSE
// bin count for binned BVH building
#define BINS 8

/*__declspec(align(64))*/ class BVH
{
	struct BuildJob
	{
		uint nodeIdx;
		float3_strict centroidMin, centroidMax;
	};
public:
	BVH() = default;
	BVH(Mesh* mesh);
	void Build();
private:
	void Subdivide(uint nodeIdx, uint depth, uint& nodePtr, float3_strict& centroidMin, float3_strict& centroidMax);
	void UpdateNodeBounds(uint nodeIdx, float3_strict& centroidMin, float3_strict& centroidMax);
	float FindBestSplitPlane(BVHNode& node, int& axis, int& splitPos, float3_strict& centroidMin, float3_strict& centroidMax);
	Mesh* mesh;
public:
	uint* triIdx = 0;
	uint nodesUsed;
	BVHNode* bvhNode = 0;
	BuildJob buildStack[64];
	int buildStackPtr;
};