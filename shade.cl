#include "template/common.h"
#include "cl/tools.cl"

__kernel void render(__global struct Photon* photonMap, int photonCount, __global struct Triangle* dosageMap,// __global unsigned int* triangles,
    __global struct Triangle* vertices)
{
	const int threadID = get_global_id(0);

    struct Triangle tri = vertices[threadID];
    float3 v1 = (float3)(tri.v1x, tri.v1y, tri.v1z);
    float3 v2 = (float3)(tri.v2x, tri.v2y, tri.v2z);
    float3 v3 = (float3)(tri.v3x, tri.v3y, tri.v3z);
    float3 centerPos = (v1 + v2 + v3) / 3;

    float3 v1v2 = v1 - v2;
    float3 v1v3 = v1 - v3;
    float area = length(cross(v1v2, v1v3)) / 2.0f;

    //float3 diff = v1 = centerpos;
    //float distSqr = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
    //diff = v1 = centerpos;
    //distSqr = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
    int triID = 0;
    
    float minDistSqr = 10000000.0;
    float minVal = 0.0;
    float maxRangeSqr = 10.3f;//TODO: make this the max dist between the centerpos and the other vertices
    int nearbyPhotons = 0;
    for (int i = 0; i < photonCount; ++i) {
        struct Photon photon = photonMap[i];
        //if (photon.timeStep != 0)
        //    triID = i;

        float3 diff = centerPos - (float3)(photon.posx, photon.posy, photon.posz);
        float distSqr = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
        if (distSqr < maxRangeSqr && threadID == photon.triangleID/* && abs(photon.triangleID - threadID) < 10*/) {//wtf
            //minDistSqr = distSqr;
            //minVal = a.w;
            nearbyPhotons++;
            if (distSqr < minDistSqr) {
                minDistSqr = distSqr;
                triID = photon.triangleID;
            }
        }
    }    
    float maxVal = 1800;
    float colorDist = (9.0f * nearbyPhotons) / (area * photonCount);//TODO: replace 9.0 with the power stored in the photon, and divide by maxVal to scale color.
    //    float maxDistSqr = 1;
    //    float colorDist = 0.05f / (minDistSqr / maxDistSqr);
    float3 f = (float3)(0.1f + colorDist * 0.3f, colorDist * 0.9f, colorDist);

    //TODO: better to use float3 for better cache aligned accesses?//photonCount / 9000.0f;//
    struct Triangle* triColor = &dosageMap[threadID];
    triColor->v1x = f.x;
    triColor->v1y = f.y;
    triColor->v1z = f.z;
    triColor->v2x = f.x;
    triColor->v2y = f.y;
    triColor->v2z = f.z;
    triColor->v3x = f.x;
    triColor->v3y = f.y;
    triColor->v3z = f.z;

    //float testColor = triID == threadID ? 0.9f : 0.1f;// triID / 100000.0f;
    //triColor->v1x = testColor;//f.x;
    //triColor->v1y = testColor;//f.y;
    //triColor->v1z = testColor;//f.z;
    //triColor->v2x = testColor;//f.x;
    //triColor->v2y = testColor;//f.y;
    //triColor->v2z = testColor;//f.z;
    //triColor->v3x = testColor;//f.x;
    //triColor->v3y = testColor;//f.y;
    //triColor->v3z = testColor;//f.z;

    //if (threadID < 1000)
    //    photonMap[threadID] = (float4)(8, 8, 8, 8);
    //write_imagef(dosageMap, (int2)(get_global_id(0), get_global_id(1)), (float4)(1,1,1,1))
}

// EOF