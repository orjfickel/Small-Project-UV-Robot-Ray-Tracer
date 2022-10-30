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

__kernel void render(__global double* photonMap, __global struct Triangle* colorMap,
    __global struct Triangle* vertices, int photonsPerLight, float power, float minValue)
{
	const int threadID = get_global_id(0);

    struct Triangle tri = vertices[threadID];
    float3 v1 = (float3)(tri.v1x, tri.v1y, tri.v1z);
    float3 v2 = (float3)(tri.v2x, tri.v2y, tri.v2z);
    float3 v3 = (float3)(tri.v3x, tri.v3y, tri.v3z);
    float3 centerPos = (v1 + v2 + v3) / 3;

    float3 v1v2 = v1 - v2;
    float3 v1v3 = v1 - v3;
    float area = (length(cross(v1v2, v1v3)) / 2.0f);
    
    int triID = 0;
    float maxValue = minValue * 2;
    float normValue = (power * photonMap[threadID]) / (area * photonsPerLight * maxValue);

    float3 color = greyscale_to_heatmap(normValue);

    //TODO: better to use float3 for better cache aligned accesses?
    struct Triangle* triColor = &colorMap[threadID];
    triColor->v1x = color.x;
    triColor->v1y = color.y;
    triColor->v1z = color.z;
    triColor->v2x = color.x;
    triColor->v2y = color.y;
    triColor->v2z = color.z;
    triColor->v3x = color.x;
    triColor->v3y = color.y;
    triColor->v3z = color.z;
}

// EOF