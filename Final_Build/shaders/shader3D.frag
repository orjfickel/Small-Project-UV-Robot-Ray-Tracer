#version 330 core
//#define PI 3.1415926535897932384626433832795

out vec4 f;

in vec4 f_color;

//vec4 getPoint(in sampler2D tex, in float index) {
//    ivec2 size = textureSize(tex, 0);
//    ivec2 uv = ivec2(mod(index, size.x), index / size.x);
//    return texelFetch(tex, uv, 0);
//}

void main()
{
	f = f_color;
    
//    // Find the closest point
//    float minDistSqr = 10000000.0; 
//    float minVal = 0.0;
//    float maxRangeSqr = 0.1f;
//    int nearbyPhotons = 0;
//    for (int i = 0; i < pointCount; ++i) {
//        vec4 a = getPoint(tex, float(i));
//        vec3 diff = worldPos.xyz - a.xyz;
//        float distSqr = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
//        if (distSqr < maxRangeSqr) {
//            //minDistSqr = distSqr;
//            //minVal = a.w;
//            nearbyPhotons++;
//        }//TODO: try break when getting to the -1 values
//    }
//    float maxVal = 1800;
//    float colorDist = (9.0f * nearbyPhotons) / (PI * maxRangeSqr * pointCount);//TODO: replace 9.0 with the power stored in the photon, and divide by maxVal to scale color.
////    float maxDistSqr = 1;
////    float colorDist = 0.05f / (minDistSqr / maxDistSqr);
//    f = vec4(0.1f + colorDist * 0.3f, colorDist * 0.9f, colorDist, 1);
    
}