#ifndef TEXTURE_H
#define TEXTURE_H
#include"mathlib.h"
class Texture
{
private:
	IUINT32 *texture;
	int w, h, max_u, max_v;
public:
	Texture() = default;
	~Texture(){ delete[]texture; }
	void texture_init();
	IUINT32 texture_get(float, float);

};
extern Texture tex;

#endif