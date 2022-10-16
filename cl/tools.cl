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
	float intensity; // The power transmitted by the UV light
};

struct Photon
{ // 24 Bytes
	float posx, posy, posz;//Can be compressed, is only relevant for quick search in kd-tree
	float timeStep, timePoint;// Delta time and the point in time. timeStep is relevant for cumulative dosis, while timePoint is relevant for grouping for max power.
	int triangleID;
};

struct Triangle
{ // 36 Bytes
	float v1x, v1y, v1z;
	float v2x, v2y, v2z;
	float v3x, v3y, v3z;
};

// EOF