#include "template/common.h"
#include "cl/tools.cl"

__kernel void render(__global double* photonMap, __global double* maxPhotonMap, __global int* tempPhotonMap, float timeStep)
{
    const int threadID = get_global_id(0);

    photonMap[threadID] = photonMap[threadID] + (double)tempPhotonMap[threadID] * timeStep;
    maxPhotonMap[threadID] = max(maxPhotonMap[threadID], (double)tempPhotonMap[threadID]);
    tempPhotonMap[threadID] = 0;
}

// EOF