#version 330 core
out vec4 f;

in vec2 uv;
in vec4 worldPos;

uniform int pointCount;
uniform sampler2D tex;


vec4 getPoint(in sampler2D tex, in float index) {
    ivec2 size = textureSize(tex, 0);
    ivec2 uv = ivec2(mod(index, size.x), index / size.x);
    return texelFetch(tex, uv, 0);
}

void main()
{
	//f = texture(tex, uv);
    
    // Find the closest point
    float minDistSqr = 10000000.0; 
    float minVal = 0.0;
    int i = 0;
    for (int i = 0; i < pointCount; ++i) {
        vec4 a = getPoint(tex, float(i));
        if (a.w >= 0) {
            vec3 diff = worldPos.xyz - a.xyz;
            float distSqr = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
            if (distSqr < minDistSqr) {
                minDistSqr = distSqr;
                minVal = a.w;
            }
        }//TODO: try break when getting to the -1 values
    }
    float maxVal = 1800;
    float maxRangeSqr = 0.1f;
    float colorDist = minDistSqr < maxRangeSqr ? (minVal / maxVal) : 0;
//    float maxDistSqr = 1;
//    float colorDist = 0.05f / (minDistSqr / maxDistSqr);
    f = vec4(colorDist, colorDist, colorDist, 1);
}