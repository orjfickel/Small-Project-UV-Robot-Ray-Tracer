#include "template/common.h"
#include "cl/tools.cl"

__kernel void render(__global struct Photon* photonMap, int photonCount, __global struct Triangle* dosageMap,
    __global struct Triangle* vertices, int photonsPerLight)// float power
{
	const int threadID = get_global_id(0);

    struct Triangle tri = vertices[threadID];
    float3 v1 = (float3)(tri.v1x, tri.v1y, tri.v1z);
    float3 v2 = (float3)(tri.v2x, tri.v2y, tri.v2z);
    float3 v3 = (float3)(tri.v3x, tri.v3y, tri.v3z);
    float3 centerPos = (v1 + v2 + v3) / 3;

    int triID = 0;
    
    float minDistSqr = 10000000.0;
    float minVal = 0.0;
    float maxRangeSqr = 0.25f;
    int nearbyPhotons = 0;
    for (int i = 0; i < photonCount; ++i) {
        struct Photon photon = photonMap[i];

        float3 diff = centerPos - (float3)(photon.posx, photon.posy, photon.posz);
        float distSqr = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
        if (distSqr < maxRangeSqr) {
            nearbyPhotons++;
            //if (distSqr < minDistSqr) {
            //    minDistSqr = distSqr;
            //    triID = photon.triangleID;
            //}
        }
    }    
    float maxVal = 1800;
    float colorDist = (16.0f * nearbyPhotons) / (PI * maxRangeSqr * photonsPerLight);//TODO: replace 9.0 with the power stored in the photon, and divide by maxVal to scale color.

    //    0.1f + colorDist * 0.3f, colorDist * 0.9f, colorDist
    float3 f;
    float minDosageColor = 0.3f;
    float upperHalfColor = minDosageColor + (1.0 - minDosageColor) / 2;
    float lowerHalfColor = minDosageColor / 2.0f;
    if (colorDist > minDosageColor) {
        if (colorDist > upperHalfColor)
            f = (float3)(1.0f, (1.0f - colorDist) / (1.0f - upperHalfColor), 0);
        else
            f = (float3)((colorDist - minDosageColor) / (upperHalfColor - minDosageColor), 1.0f, 0);
    }
    else {
        if (colorDist > lowerHalfColor)
            f = (float3)(0, 1.0f, (minDosageColor - colorDist) / (minDosageColor - lowerHalfColor));
        else
            f = (float3)(0, (colorDist) / (lowerHalfColor), 1.0f);
    }

    //TODO: better to use float3 for better cache aligned accesses?
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

}

// EOF