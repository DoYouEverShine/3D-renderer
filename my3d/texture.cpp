#include"texture.h"	

Texture tex;
void Texture::texture_init()
{
	w = 256; h = 256;
	max_u = w - 1;
	max_v = h - 1;
	texture = new IUINT32[w*h];
	int i, j;
	for (j = 0; j < w; j++) {                 //u ,w
		for (i = 0; i < h; i++){              //v ,h
			int x = i / 32, y = j / 32;
			texture[i*w+j] = ((x + y) & 1) ? 0xffffff : 0xffbcef;
		}
	}
}
int CMID(int x, int min, int max) { return (x < min) ? min : ((x > max) ? max : x); }

IUINT32 Texture::texture_get(float u, float v)
{
	IUINT32 x = u*max_u + 0.5, y = v*max_v + 0.5;
	//x = CMID(x, 0, max_u); y = CMID(y, 0, max_v);
	return texture[y*w + x];
}