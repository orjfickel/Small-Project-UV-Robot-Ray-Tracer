#include "template/common.h"
#include "cl/tools.cl"

__kernel void render(__global double* photonMap, float timeStep)
{
    const int threadID = get_global_id(0);

    photonMap[threadID] = photonMap[threadID] * timeStep;
}

// EOF