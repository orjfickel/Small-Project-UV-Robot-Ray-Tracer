#include "template/common.h"
#include "cl/tools.cl"

float3 greyscale_to_heatmap(float intensity) {
    float minDosageColor = 0.5f;
    float upperHalfColor = minDosageColor + (1.0 - minDosageColor) / 2;
    float lowerHalfColor = minDosageColor / 2.0f;
    if (intensity > minDosageColor) {
        if (intensity > upperHalfColor)
            return (float3)(1.0f, (1.0f - intensity) / (1.0f - upperHalfColor), 0);
        else
            return (float3)((intensity - minDosageColor) / (upperHalfColor - minDosageColor), 1.0f, 0);
    }
    else {
        if (intensity > lowerHalfColor)
            return (float3)(0, 1.0f, (minDosageColor - intensity) / (minDosageColor - lowerHalfColor));
        else
            return (float3)(0, (intensity) / (lowerHalfColor), 1.0f);
    }
    return intensity;
}

__kernel void computeDosage(__global double* photonMap, __global float* dosageMap,
    __global struct Triangle* vertices, int photonsPerLight, float scaledPower)
{
	const int threadID = get_global_id(0);

    struct Triangle tri = vertices[threadID];
    float3 v0 = (float3)(tri.v0x, tri.v0y, tri.v0z);
    float3 v1 = (float3)(tri.v1x, tri.v1y, tri.v1z);
    float3 v2 = (float3)(tri.v2x, tri.v2y, tri.v2z);
    float3 centerPos = (v0 + v1 + v2) / 3;

    float3 v0v1 = v0 - v1;
    float3 v0v2 = v0 - v2;
    float area = (length(cross(v0v1, v0v2)) / 2.0f);

    // Compute the normalised irradiance/dosis. 
    float dose = (scaledPower * photonMap[threadID]) / (area * photonsPerLight);
    dosageMap[threadID] = scaledPower;
}

__kernel void dosageToColor(__global float* dosageMap, __global struct TriangleColor* colorMap, float minValue) {

    const int threadID = get_global_id(0);

    float maxValue = minValue * 2;
    // Compute the normalised irradiance/dosis. 
    float normValue = (dosageMap[threadID]) / (maxValue);

    float3 color = greyscale_to_heatmap(normValue);

    //TODO: Might be better to use float3 for better cache aligned accesses?
    struct TriangleColor* triColor = &colorMap[threadID];
    triColor->v0x = color.x;
    triColor->v0y = color.y;
    triColor->v0z = color.z;
    triColor->v1x = color.x;
    triColor->v1y = color.y;
    triColor->v1z = color.z;
    triColor->v2x = color.x;
    triColor->v2y = color.y;
    triColor->v2z = color.z;
}

// EOF