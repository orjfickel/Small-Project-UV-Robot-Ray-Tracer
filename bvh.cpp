#include "precomp.h"
//
//BVH::BVH(Mesh* mesh)
//{
//	//mesh = triMesh;
//	this->mesh = mesh;
//	bvhNode = (BVHNode*)_aligned_malloc(sizeof(BVHNode) * mesh->triangleCount * 2 + 64, 64);
//	triIdx = new uint[mesh->triangleCount];
//	Build();
//}
//
//void BVH::Build()
//{
//	// reset node pool
//	nodesUsed = 2;
//	memset(bvhNode, 0, mesh->triangleCount * 2 * sizeof(BVHNode));
//	// populate triangle index array
//	for (int i = 0; i < mesh->triangleCount; i++) triIdx[i] = i;
//	// calculate triangle centroids for partitioning
//	Tri* tri = mesh->triangles;
//	for (int i = 0; i < mesh->triangleCount; i++)
//		mesh->triangles[i].centroid = (tri[i].vertex0 + tri[i].vertex1 + tri[i].vertex2) * 0.3333f;
//	// assign all triangles to root node
//	BVHNode& root = bvhNode[0];
//	root.leftFirst = 0, root.triCount = mesh->triangleCount;
//	float3 centroidMin, centroidMax;
//	UpdateNodeBounds(0, centroidMin, centroidMax);
//	// subdivide recursively
//	buildStackPtr = 0;
//	Subdivide(0, 0, nodesUsed, centroidMin, centroidMax);
//	// do the parallel tasks, if any
//	uint nodePtr[64];
//	int N = buildStackPtr;
//	nodePtr[0] = nodesUsed;
//	for (int i = 1; i < N; i++) nodePtr[i] = nodePtr[i - 1] + bvhNode[buildStack[i - 1].nodeIdx].triCount * 2;
//#pragma omp parallel for schedule(dynamic,1)
//	for (int i = 0; i < N; i++)
//	{
//		float3 cmin = buildStack[i].centroidMin, cmax = buildStack[i].centroidMax;
//		Subdivide(buildStack[i].nodeIdx, 99, nodePtr[i], cmin, cmax);
//	}
//	nodesUsed = mesh->triangleCount * 2 + 64;
//}
//
//void BVH::Subdivide(uint nodeIdx, uint depth, uint& nodePtr, float3& centroidMin, float3& centroidMax)
//{
//	BVHNode& node = bvhNode[nodeIdx];
//	// determine split axis using SAH
//	int axis, splitPos;
//	float splitCost = FindBestSplitPlane(node, axis, splitPos, centroidMin, centroidMax);
//	// terminate recursion
//	if (subdivToOnePrim)
//	{
//		if (node.triCount == 1) return;
//	}
//	else
//	{
//		float nosplitCost = node.CalculateNodeCost();
//		if (splitCost >= nosplitCost) return;
//	}
//	// in-place partition
//	int i = node.leftFirst;
//	int j = i + node.triCount - 1;
//	float scale = BINS / (centroidMax[axis] - centroidMin[axis]);
//	while (i <= j)
//	{
//		// use the exact calculation we used for binning to prevent rare inaccuracies
//		int binIdx = min(BINS - 1, (int)((mesh->triangles[triIdx[i]].centroid[axis] - centroidMin[axis]) * scale));
//		if (binIdx < splitPos) i++; else swap(triIdx[i], triIdx[j--]);
//	}
//	// abort split if one of the sides is empty
//	int leftCount = i - node.leftFirst;
//	if (leftCount == 0 || leftCount == node.triCount) return; // never happens for dragon mesh, nice
//	// create child nodes
//	int leftChildIdx = nodePtr++;
//	int rightChildIdx = nodePtr++;
//	bvhNode[leftChildIdx].leftFirst = node.leftFirst;
//	bvhNode[leftChildIdx].triCount = leftCount;
//	bvhNode[rightChildIdx].leftFirst = i;
//	bvhNode[rightChildIdx].triCount = node.triCount - leftCount;
//	node.leftFirst = leftChildIdx;
//	node.triCount = 0;
//	// recurse
//	UpdateNodeBounds(leftChildIdx, centroidMin, centroidMax);
//	if (depth == 3)
//	{
//		// postpone the work, we'll do this in parallel later
//		buildStack[buildStackPtr].nodeIdx = leftChildIdx;
//		buildStack[buildStackPtr].centroidMin = centroidMin;
//		buildStack[buildStackPtr++].centroidMax = centroidMax;
//	}
//	else Subdivide(leftChildIdx, depth + 1, nodePtr, centroidMin, centroidMax);
//	UpdateNodeBounds(rightChildIdx, centroidMin, centroidMax);
//	if (depth == 3)
//	{
//		// postpone the work, we'll do this in parallel later
//		buildStack[buildStackPtr].nodeIdx = rightChildIdx;
//		buildStack[buildStackPtr].centroidMin = centroidMin;
//		buildStack[buildStackPtr++].centroidMax = centroidMax;
//	}
//	else Subdivide(rightChildIdx, depth + 1, nodePtr, centroidMin, centroidMax);
//}
//
//float BVH::FindBestSplitPlane(BVHNode& node, int& axis, int& splitPos, float3& centroidMin, float3& centroidMax)
//{
//	float bestCost = 1e30f;
//	for (int a = 0; a < 3; a++)
//	{
//		float boundsMin = centroidMin[a], boundsMax = centroidMax[a];
//		if (boundsMin == boundsMax) continue;
//		// populate the bins
//		float scale = BINS / (boundsMax - boundsMin);
//		float leftCountArea[BINS - 1], rightCountArea[BINS - 1];
//		int leftSum = 0, rightSum = 0;
//#ifdef USE_SSE
//		__m128 min4[BINS], max4[BINS];
//		uint count[BINS];
//		for (uint i = 0; i < BINS; i++)
//			min4[i] = _mm_set_ps1(1e30f),
//			max4[i] = _mm_set_ps1(-1e30f),
//			count[i] = 0;
//		for (uint i = 0; i < node.triCount; i++)
//		{
//			Tri& triangle = mesh->triangles[triIdx[node.leftFirst + i]];
//			int binIdx = min(BINS - 1, (int)((triangle.centroid[a] - boundsMin) * scale));
//			count[binIdx]++;
//			min4[binIdx] = _mm_min_ps(min4[binIdx], triangle.v0);
//			max4[binIdx] = _mm_max_ps(max4[binIdx], triangle.v0);
//			min4[binIdx] = _mm_min_ps(min4[binIdx], triangle.v1);
//			max4[binIdx] = _mm_max_ps(max4[binIdx], triangle.v1);
//			min4[binIdx] = _mm_min_ps(min4[binIdx], triangle.v2);
//			max4[binIdx] = _mm_max_ps(max4[binIdx], triangle.v2);
//		}
//		// gather data for the 7 planes between the 8 bins
//		__m128 leftMin4 = _mm_set_ps1(1e30f), rightMin4 = leftMin4;
//		__m128 leftMax4 = _mm_set_ps1(-1e30f), rightMax4 = leftMax4;
//		for (int i = 0; i < BINS - 1; i++)
//		{
//			leftSum += count[i];
//			rightSum += count[BINS - 1 - i];
//			leftMin4 = _mm_min_ps(leftMin4, min4[i]);
//			rightMin4 = _mm_min_ps(rightMin4, min4[BINS - 2 - i]);
//			leftMax4 = _mm_max_ps(leftMax4, max4[i]);
//			rightMax4 = _mm_max_ps(rightMax4, max4[BINS - 2 - i]);
//			const __m128 le = _mm_sub_ps(leftMax4, leftMin4);
//			const __m128 re = _mm_sub_ps(rightMax4, rightMin4);
//			leftCountArea[i] = leftSum * (le.m128_f32[0] * le.m128_f32[1] + le.m128_f32[1] * le.m128_f32[2] + le.m128_f32[2] * le.m128_f32[0]);
//			rightCountArea[BINS - 2 - i] = rightSum * (re.m128_f32[0] * re.m128_f32[1] + re.m128_f32[1] * re.m128_f32[2] + re.m128_f32[2] * re.m128_f32[0]);
//		}
//#else
//		struct Bin { aabb bounds; int triCount = 0; } bin[BINS];
//		for (uint i = 0; i < node.triCount; i++)
//		{
//			Tri& triangle = mesh->triangles[triIdx[node.leftFirst + i]];
//			int binIdx = min(BINS - 1, (int)((triangle.centroid[a] - boundsMin) * scale));
//			bin[binIdx].triCount++;
//			bin[binIdx].bounds.grow(triangle.vertex0);
//			bin[binIdx].bounds.grow(triangle.vertex1);
//			bin[binIdx].bounds.grow(triangle.vertex2);
//		}
//		// gather data for the 7 planes between the 8 bins
//		aabb leftBox, rightBox;
//		for (int i = 0; i < BINS - 1; i++)
//		{
//			leftSum += bin[i].triCount;
//			leftCount[i] = leftSum;
//			leftBox.grow(bin[i].bounds);
//			leftArea[i] = leftBox.area();
//			rightSum += bin[BINS - 1 - i].triCount;
//			rightCount[BINS - 2 - i] = rightSum;
//			rightBox.grow(bin[BINS - 1 - i].bounds);
//			rightArea[BINS - 2 - i] = rightBox.area();
//		}
//#endif
//		// calculate SAH cost for the 7 planes
//		scale = (boundsMax - boundsMin) / BINS;
//		for (int i = 0; i < BINS - 1; i++)
//		{
//			const float planeCost = leftCountArea[i] + rightCountArea[i];
//			if (planeCost < bestCost)
//				axis = a, splitPos = i + 1, bestCost = planeCost;
//		}
//	}
//	return bestCost;
//}
//
//void BVH::UpdateNodeBounds(uint nodeIdx, float3& centroidMin, float3& centroidMax)
//{
//	BVHNode& node = bvhNode[nodeIdx];
//#ifdef USE_SSE
//	__m128 min4 = _mm_set_ps1(1e30f), max4 = _mm_set_ps1(-1e30f);
//	__m128 cmin4 = _mm_set_ps1(1e30f), cmax4 = _mm_set_ps1(-1e30f);
//	for (uint first = node.leftFirst, i = 0; i < node.triCount; i++)
//	{
//		Tri& leafTri = mesh->triangles[triIdx[first + i]];
//		min4 = _mm_min_ps(min4, leafTri.v0), max4 = _mm_max_ps(max4, leafTri.v0);
//		min4 = _mm_min_ps(min4, leafTri.v1), max4 = _mm_max_ps(max4, leafTri.v1);
//		min4 = _mm_min_ps(min4, leafTri.v2), max4 = _mm_max_ps(max4, leafTri.v2);
//		cmin4 = _mm_min_ps(cmin4, leafTri.centroid4);
//		cmax4 = _mm_max_ps(cmax4, leafTri.centroid4);
//	}
//	__m128 mask4 = _mm_cmpeq_ps(_mm_setzero_ps(), _mm_set_ps(1, 0, 0, 0));
//	node.aabbMin4 = _mm_blendv_ps(node.aabbMin4, min4, mask4);
//	node.aabbMax4 = _mm_blendv_ps(node.aabbMax4, max4, mask4);
//	centroidMin = *(float3*)&cmin4;
//	centroidMax = *(float3*)&cmax4;
//#else
//	node.aabbMin = float3(1e30f);
//	node.aabbMax = float3(-1e30f);
//	centroidMin = float3(1e30f);
//	centroidMax = float3(-1e30f);
//	for (uint first = node.leftFirst, i = 0; i < node.triCount; i++)
//	{
//		uint leafTriIdx = triIdx[first + i];
//		Tri& leafTri = mesh->triangles[leafTriIdx];
//		node.aabbMin = fminf(node.aabbMin, leafTri.vertex0);
//		node.aabbMin = fminf(node.aabbMin, leafTri.vertex1);
//		node.aabbMin = fminf(node.aabbMin, leafTri.vertex2);
//		node.aabbMax = fmaxf(node.aabbMax, leafTri.vertex0);
//		node.aabbMax = fmaxf(node.aabbMax, leafTri.vertex1);
//		node.aabbMax = fmaxf(node.aabbMax, leafTri.vertex2);
//		centroidMin = fminf(centroidMin, leafTri.centroid);
//		centroidMax = fmaxf(centroidMax, leafTri.centroid);
//	}
//#endif
//}
