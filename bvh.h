#pragma once

namespace Tmpl8
{//Forward declaration
	class Mesh;
}

struct Ray
{ // 32 Bytes
	float dirx, diry, dirz;
	float origx, origy, origz;
	float dist;
	uint triID;
};

// 32-byte BVH node struct
struct BVHNode
{
	union { struct { float3 aabbMin; uint leftFirst; }; __m128 aabbMin4; };
	union { struct { float3 aabbMax; uint triCount; }; __m128 aabbMax4; };
	bool isLeaf() const { return triCount > 0; } // empty BVH leaves do not exist
	float CalculateNodeCost()
	{
		float3 e = aabbMax - aabbMin; // extent of the node
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
		float3 centroidMin, centroidMax;
	};
public:
	BVH() = default;
	BVH(Mesh* mesh);
	void Build();
private:
	void Subdivide(uint nodeIdx, uint depth, uint& nodePtr, float3& centroidMin, float3& centroidMax);
	void UpdateNodeBounds(uint nodeIdx, float3& centroidMin, float3& centroidMax);
	float FindBestSplitPlane(BVHNode& node, int& axis, int& splitPos, float3& centroidMin, float3& centroidMax);
	Mesh* mesh;
public:
	uint* triIdx = 0;
	uint nodesUsed;
	BVHNode* bvhNode = 0;
	bool subdivToOnePrim = false; // for TLAS experiment
	BuildJob buildStack[64];
	int buildStackPtr;
};