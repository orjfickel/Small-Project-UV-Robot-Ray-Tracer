// random numbers: seed using WangHash((threadidx+1)*17), then use RandomInt / RandomFloat
unsigned int WangHash(unsigned int s ) { s = (s ^ 61) ^ (s >> 16), s *= 9, s = s ^ (s >> 4), s *= 0x27d4eb2d, s = s ^ (s >> 15); return s; }
unsigned int RandomInt(unsigned int* s ) { *s ^= *s << 13, * s ^= *s >> 17, * s ^= *s << 5; return *s; }
float RandomFloat(unsigned int* s ) { return RandomInt( s ) * 2.3283064365387e-10f; /* = 1 / (2^32-1) */ }

unsigned int seed;

struct Ray
{ // 32 Bytes
	float dirx, diry, dirz;
	float origx, origy, origz;
	float dist;
	uint triID;
};

struct TriangleColor
{
	float v0x, v0y, v0z;
	float v1x, v1y, v1z;
	float v2x, v2y, v2z;
};

struct LightPos
{
	float3 position;
	float duration;
};

// Adapted from https://github.com/jbikker/bvh_article ----------------------------------

struct Triangle
{
	float v0x, v0y, v0z, dummy1;
	float v1x, v1y, v1z, dummy2;
	float v2x, v2y, v2z, dummy3;
	float cx, cy, cz, dummy4;
};

struct BVHNode
{
	float minx, miny, minz;
	int leftFirst;
	float maxx, maxy, maxz;
	int triCount;
};

//---------------------------------------------------------------------------------------

// EOF