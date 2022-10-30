#include "template/common.h"
#include "cl/tools.cl"

__kernel void render(__global double* photonMap, __global double* maxPhotonMap, __global struct TriangleColor* dosageMap)
{
    const int threadID = get_global_id(0);

    photonMap[threadID] = 0;
    maxPhotonMap[threadID] = 0;

    double testColor = 0;
    struct TriangleColor* triColor = &dosageMap[threadID];
    triColor->v0x = testColor;
    triColor->v0y = testColor;
    triColor->v0z = testColor;
    triColor->v1x = testColor;
    triColor->v1y = testColor;
    triColor->v1z = testColor;
    triColor->v2x = testColor;
    triColor->v2y = testColor;
    triColor->v2z = testColor;
}

// EOF