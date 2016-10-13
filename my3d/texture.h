#ifndef TEXTURE_H
#define TEXTURE_H
#include"mathlib.h"
#include<cstdlib>
class Texture
{
private:
	IUINT32 *texture = NULL;
	int w, h, max_u, max_v;
public:
	Texture();
	~Texture(){ delete[]texture; }
	IUINT32 texture_get(float, float);

};
//extern Texture tex;

#endif