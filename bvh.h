#pragma once

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

///*__declspec(align(64))*/ class BVH
//{
//	struct BuildJob
//	{
//		uint nodeIdx;
//		float3 centroidMin, centroidMax;
//	};
//public:
//	BVH() = default;
//	BVH(int triCount);
//	void Build();
//	void Refit();
//	void Intersect(Ray& ray, uint instanceIdx);
//private:
//	void Subdivide(uint nodeIdx, uint depth, uint& nodePtr, float3& centroidMin, float3& centroidMax);
//	void UpdateNodeBounds(uint nodeIdx, float3& centroidMin, float3& centroidMax);
//	float FindBestSplitPlane(BVHNode& node, int& axis, int& splitPos, float3& centroidMin, float3& centroidMax);
//	Tri* triangles = 0;
//public:
//	uint* triIdx = 0;
//	uint nodesUsed;
//	BVHNode* bvhNode = 0;
//	bool subdivToOnePrim = false; // for TLAS experiment
//	BuildJob buildStack[64];
//	int buildStackPtr;
//};