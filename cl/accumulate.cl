#include "template/common.h"
#include "cl/tools.cl"

__kernel void render(__global double* photonMap, __global double* maxPhotonMap, __global int* tempPhotonMap, float timeStep)
{
    const int threadID = get_global_id(0);

    // Sum the timeSteps times photon counts together for the cumulative dose
    photonMap[threadID] = photonMap[threadID] + (double)tempPhotonMap[threadID] * timeStep;
    // Simply take the max photon count
    maxPhotonMap[threadID] = max(maxPhotonMap[threadID], (double)tempPhotonMap[threadID]);
    // Reset the temp photon map
    tempPhotonMap[threadID] = 0;
}

// EOF