#include "template/common.h"
#include "cl/tools.cl"

__kernel void render(__global int* photonMap, __global struct Triangle* dosageMap)
{
    const int threadID = get_global_id(0);

    photonMap[threadID] = 0;

    float testColor = 0;
    struct Triangle* triColor = &dosageMap[threadID]; 
    triColor->v1x = testColor;//f.x;
    triColor->v1y = testColor;//f.y;
    triColor->v1z = testColor;//f.z;
    triColor->v2x = testColor;//f.x;
    triColor->v2y = testColor;//f.y;
    triColor->v2z = testColor;//f.z;
    triColor->v3x = testColor;//f.x;
    triColor->v3y = testColor;//f.y;
    triColor->v3z = testColor;//f.z;
}

// EOF