#include "template/common.h"
#include "cl/tools.cl"

__kernel void render(__global float4* photonMap, int photonCount, __global float* dosageMap)
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



    //float minDistSqr = 10000000.0;
    //float minVal = 0.0;
    //float maxRangeSqr = 0.1f;
    //int nearbyPhotons = 0;
    //for (int i = 0; i < photonCount; ++i) {
    //    float4 photon = photonMap[i];

    //}    
    //float maxVal = 1800;
    //float colorDist = (9.0f * nearbyPhotons) / (PI * maxRangeSqr * pointCount);//TODO: replace 9.0 with the power stored in the photon, and divide by maxVal to scale color.
    ////    float maxDistSqr = 1;
    ////    float colorDist = 0.05f / (minDistSqr / maxDistSqr);
    //f = vec4(0.1f + colorDist * 0.3f, colorDist * 0.9f, colorDist, 1);

    dosageMap[threadID * 3 + 0] = 1;
    dosageMap[threadID * 3 + 1] = 1;
    dosageMap[threadID * 3 + 2] = 1;

    //if (threadID < 1000)
    //    photonMap[threadID] = (float4)(8, 8, 8, 8);
    //write_imagef(dosageMap, (int2)(get_global_id(0), get_global_id(1)), (float4)(1,1,1,1))
}

// EOF