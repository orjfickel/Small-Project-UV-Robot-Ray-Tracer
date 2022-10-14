#include "template/common.h"
#include "cl/tools.cl"

__kernel void render(__global float4* photonMap, int photonCount, __global float* dosageMap, __global unsigned int* triangles, int triangleCount,
    __global float* vertices)
{
	const int threadID = get_global_id(0);

    //float minDistSqr = 10000000.0;
    //float minVal = 0.0;
    //float maxRangeSqr = 0.1f;
    //int nearbyPhotons = 0;
    //for (int i = 0; i < photonCount; ++i) {
    //    vec4 a = getPoint(tex, float(i));
    //    vec3 diff = worldPos.xyz - a.xyz;
    //    float distSqr = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
    //    if (distSqr < maxRangeSqr) {
    //        //minDistSqr = distSqr;
    //        //minVal = a.w;
    //        nearbyPhotons++;
    //    }//TODO: try break when getting to the -1 values
    //}
    //float maxVal = 1800;
    //float colorDist = (9.0f * nearbyPhotons) / (PI * maxRangeSqr * pointCount);//TODO: replace 9.0 with the power stored in the photon, and divide by maxVal to scale color.
    ////    float maxDistSqr = 1;
    ////    float colorDist = 0.05f / (minDistSqr / maxDistSqr);
    //f = vec4(0.1f + colorDist * 0.3f, colorDist * 0.9f, colorDist, 1);

    uint v1ID = triangles[threadID] * 3;
    uint v2ID = triangles[threadID + 1] * 3;
    uint v3ID = triangles[threadID + 2] * 3;
    float3 v1 = (float3)(vertices[v1ID + 0], vertices[v1ID + 1], vertices[v1ID + 2]);
    float3 v2 = (float3)(vertices[v2ID + 0], vertices[v2ID + 1], vertices[v2ID + 2]);
    float3 v3 = (float3)(vertices[v3ID + 0], vertices[v3ID + 1], vertices[v3ID + 2]);
    float3 centerPos = (v1 + v2 + v3) / 3;

    float minDistSqr = 10000000.0;
    float minVal = 0.0;
    float maxRangeSqr = 0.3f;
    int nearbyPhotons = 0;
    for (int i = 0; i < photonCount; ++i) {
        float4 photon = photonMap[i];

        float3 diff = centerPos - (float3)(photon.x, photon.y, photon.z);
        float distSqr = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
        if (distSqr < maxRangeSqr) {
            //minDistSqr = distSqr;
            //minVal = a.w;
            nearbyPhotons++;
        }//TODO: try break when getting to the -1 values
    }    
    float maxVal = 1800;
    float colorDist = (9.0f * nearbyPhotons) / (PI * maxRangeSqr * photonCount);//TODO: replace 9.0 with the power stored in the photon, and divide by maxVal to scale color.
    //    float maxDistSqr = 1;
    //    float colorDist = 0.05f / (minDistSqr / maxDistSqr);
    float3 f = (float3)(0.1f + colorDist * 0.3f, colorDist * 0.9f, colorDist);

    //TODO: better to use float3 for better cache aligned accesses?
    dosageMap[threadID * 3 + 0] = f.x;
    dosageMap[threadID * 3 + 1] = f.y;
    dosageMap[threadID * 3 + 2] = f.z;//TODO: are we now computing this multiple times for duplicate vertices -_-. 
    //                                        Indices array already had duplicates though, so somehow should be merged

    //if (threadID < 1000)
    //    photonMap[threadID] = (float4)(8, 8, 8, 8);
    //write_imagef(dosageMap, (int2)(get_global_id(0), get_global_id(1)), (float4)(1,1,1,1))
}

// EOF