#ifndef DEVICE_H
#define DEVICE_H
#include<cstdlib>
#include<cassert>
#include<cstdio>
#include<fstream>
#include"mathlib.h"
#include"transform.h"
#include"light.h"
#include"texture.h"
//extern point_t cam;

//typedef struct 
//{
//	transform_t transform;
//	int width;
//	int height;
//	IUINT32 **framebuffer;      //framebuffer[j] 指向第j行
//	float **zbuffer;
//	int render_state;           // 渲染状态
//	IUINT32 foreground;
//	IUINT32 background;
//}device_t;



#define PLOY_NUM					12
#define MESH_NUM					8
#define RENDER_STATE_WIREFRAME      1		// 渲染线框
#define RENDER_STATE_TEXTURE        2		// 渲染纹理
#define RENDER_STATE_COLOR          4		// 渲染颜色
class Device
{
public:
	Device(int w, int h, void* fb)
	{
		int need = 8 * h + h*w * 4;
		char *ptr = (char*)malloc(need);
		char *framebuf, *zbuf;
		int j;
		assert(ptr);
		framebuffer = (IUINT32**)ptr;
		ptr += sizeof(IUINT32**)*h;
		zbuffer = (float**)ptr;
		framebuf = (char*)fb;
		ptr += sizeof(float**)*h;
		zbuf = (char*)ptr;
		for (j = 0; j < h; ++j)
		{
			framebuffer[j] = (IUINT32*)(framebuf + j*w * 4);
			zbuffer[j] = (float*)(zbuf + j*w * 4);
		}
		Transform::transform_init(&transform, w, h);
		render_state = RENDER_STATE_COLOR;
		foreground = 0xc0c0c0;
		background = 0.0f;
		width = w;
		height = h;
		light = new Light;
		tex = new Texture;
		vlist = mesh;
		init_object();
	}

	void device_init_vertex_norm(ploy_t *ploy)
	{
		for (int i = 0; i < ploygon_num; ++i)
		{
			vector_t n;
			auto tmp = (ploy + i);
			Vector::vector_get_norm(&n,vlist + tmp->vert[0], vlist + tmp->vert[1], vlist + tmp->vert[2]);
			Vector::vector_add(&(vlist + tmp->vert[0])->n,&( vlist + tmp->vert[0])->n, &n);
			Vector::vector_add(&(vlist + tmp->vert[1])->n, &(vlist + tmp->vert[1])->n, &n);
			Vector::vector_add(&(vlist + tmp->vert[2])->n, &(vlist + tmp->vert[2])->n, &n);
		}
		for (int i = 0; i < vertex_num; ++i)
		{
			Vector::vector_normalize(&mesh[i].n, &mesh[i].n);
			/*auto tmp = (ploy + i);
			Vector::vector_normalize(&(vlist + tmp->vert[0])->n, &(vlist + tmp->vert[0])->n);
			Vector::vector_normalize(&(vlist + tmp->vert[1])->n, &(vlist + tmp->vert[1])->n);
			Vector::vector_normalize(&(vlist + tmp->vert[2])->n, &(vlist + tmp->vert[2])->n);*/
		}
	}

	~Device()
	{
		delete framebuffer;
		delete zbuffer;
		delete light;
		delete tex;
		framebuffer = NULL;
		zbuffer = NULL;
		light = NULL;
		tex = NULL;
	}
	void device_pixel( int x, int y, IUINT32 color) {
		if (((IUINT32)x) < (IUINT32)width && ((IUINT32)y) < (IUINT32)height && x >= 0 && y >= 0) {
			framebuffer[y][x] = color;
		}
	}
	void device_clear()
	{
		for (int i = 0; i < height; ++i)
		for (int j = 0; j < width; ++j)
		{
			framebuffer[i][j] = background;
			zbuffer[i][j] = 0.0f;
		}
	}

	void device_interp(vertex_t *y, vertex_t *x1, vertex_t *x2, float factor)
	{
		float ifactor = 1 - factor;
		y->pos.x = x2->pos.x * factor + x1->pos.x * ifactor;
		y->pos.y = x2->pos.y * factor + x1->pos.y * ifactor;
		y->pos.z = x2->pos.z * factor + x1->pos.z * ifactor;
		y->light_color.r = x2->light_color.r * factor + x1->light_color.r * ifactor;
		y->light_color.g = x2->light_color.g * factor + x1->light_color.g * ifactor;
		y->light_color.b = x2->light_color.b * factor + x1->light_color.b * ifactor;
		y->texture.u = x2->texture.u *  factor + x1->texture.u * ifactor;
		y->texture.v = x2->texture.v *  factor + x1->texture.v * ifactor;
		y->rhw = x2->rhw * factor + x1->rhw * ifactor;
	}

	void device_draw_line( int x0, int y0, int x1, int y1, IUINT32 color)
	{
		if (abs(y1 - y0) <= abs(x1 - x0)){
			int x = x0;
			int increm = (x0 < x1) ? 1 : -1;
			float y = y0, det_y, det = x1 - x0;
			det_y = 1.0 * (y1 - y0) / det;
			for (x = x0;; x += increm){
				if (x == x1) break;
				device_pixel( x, 0.5 + y, color);
				y += increm * det_y;
			}
		}
		else {
			int y = y0;
			int increm = (y0 < y1) ? 1 : -1;
			float x = x0, det_x, det = y1 - y0;
			det_x = 1.0 * (x1 - x0) / det;
			for (y = y0;; y += increm){
				if (y == y1) break;
				device_pixel( x, 0.5 + y, color);
				x += increm * det_x;
			}
		}
	}
	void device_draw_Scanline( vertex_t *s, vertex_t *e, int y)
	{
		if (((IUINT32)y) > (IUINT32)height || y < 0) return;
		float *zb = zbuffer[y];
		vertex_t *tmp = NULL;
		if (s->pos.x > e->pos.x){ tmp = s; s = e; e = tmp; }
		int startX = ceil(s->pos.x) , endX = ceil(e->pos.x)-1;
		float lenX = s->pos.x - e->pos.x;
		for (int i = startX; i <= endX; ++i)
		{
			float factor = 0;
			if (!FCMP(lenX,0))  factor = (float)(i - e->pos.x) / lenX;
			vertex_t point;
			device_interp(&point, e, s, factor);  
			float invz = point.rhw,w = 1.0f / invz;
			if (zb[i] > invz) continue;
			zb[i] = invz;
			if (render_state == RENDER_STATE_COLOR)
			{
				IUINT32 R = point.light_color.r *255.0f *w;
				IUINT32 G = point.light_color.g *255.0f *w;
				IUINT32 B = point.light_color.b *255.0f *w;
				IUINT32 c = (R << 16) | (G << 8) | B;
				device_pixel( i, y, c);
			}
			else
			{
				IUINT32 c_text = tex->texture_get(point.texture.u *w, point.texture.v *w),
					    R = point.light_color.r *( c_text >> 16),
						G = point.light_color.g *((c_text & 0x00ff00) >> 8),
						B = point.light_color.b *(c_text & 0x0000ff);
				IUINT32 c = (R << 16) | (G << 8) | B;
				device_pixel( i, y, c);
			}
		}
	}
	void device_draw_triangle( vertex_t *a, vertex_t *b, vertex_t *c)           //    a
	{                      
		static float max;// b   
		vertex_t *tmp = NULL;                                                                           //      c     
		if ((FCMP(a->pos.y, b->pos.y) && FCMP(a->pos.y, c->pos.y)) ||
			(FCMP(a->pos.x, b->pos.x) && FCMP(a->pos.x, c->pos.x))) return;
		if (a->pos.y < b->pos.y) { tmp = a; a = b; b = tmp; }
		if (a->pos.y < c->pos.y) { tmp = a; a = c; c = tmp; }
		if (b->pos.y < c->pos.y) { tmp = b; b = c; c = tmp; }
		vertex_t middle;
		float mid = (b->pos.y - c->pos.y) / (a->pos.y - c->pos.y), lenY = (a->pos.y - b->pos.y);
		device_interp(&middle, c, a, mid);
		int startY = ceil(a->pos.y)-1, endY = ceil(b->pos.y);
		for (int i = endY; i <=startY ; ++i)
		{
			float factor = 0;
			if (!FCMP(lenY,0)) factor = (float)(i - b->pos.y) / lenY;

			vertex_t xa, xb;
			device_interp(&xa, b, a, factor);
			device_interp(&xb, &middle, a, factor);
			device_draw_Scanline( &xa, &xb, i);
		}
		startY = ceil(b->pos.y)-1;
		endY = ceil(c->pos.y);
		lenY = (b->pos.y - c->pos.y);
		for (int i = endY; i <= startY; ++i)
		{
			float factor = 0;
			if (!FCMP(lenY, 0))  factor = (float)(i - c->pos.y) / lenY;
			vertex_t xa, xb;
			device_interp(&xa, c, b, factor);
			device_interp(&xb, c, &middle, factor);
			device_draw_Scanline( &xa, &xb, i);
		}
	}
	bool device_backface_judge(point_t *a, point_t *b, point_t *c, point_t *n)
	{
		point_t p1, p2, n1;
		Vector::vector_sub(&p1, b, a);
		Vector::vector_sub(&p2, c, b);
		Vector::vector_crossproduct(&n1, &p1, &p2);
		float res = Vector::vector_dotproduct(&n1, n);
		return res < 0 ? 1 : 0;
	}

	//=====================================================================
	// 渲染实现
	//=====================================================================

	void device_draw_primitive( const ploy_t *index)
	{
		vertex_t p1, p2, p3, c1, c2, c3;
		vector_t n1, n2, n3;
		//int render_state = render_state;
		if (!light->mark_v[index->vert[0]])
		{
			Transform::transform_apply_world_v(&transform, &n1, &(vlist + index->vert[0])->n);
			light->Light_Renderer_vertex(vlist + index->vert[0], &n1, &cam);
			light->mark_v[index->vert[0]] = 1;
		}
		if (!light->mark_v[index->vert[1]])
		{
			Transform::transform_apply_world_v(&transform, &n2, &(vlist + index->vert[1])->n);
			light->Light_Renderer_vertex(vlist + index->vert[1], &n2, &cam);
			light->mark_v[index->vert[1]] = 1;
		}
		if (!light->mark_v[index->vert[2]])
		{
			Transform::transform_apply_world_v(&transform, &n3, &(vlist + index->vert[2])->n);                  //乘以视图矩阵 v * world * view * projection
			light->Light_Renderer_vertex(vlist + index->vert[2], &n3, &cam);
			light->mark_v[index->vert[2]] = 1;
		}
		Transform::transform_apply_world(&transform, &c1, vlist + index->vert[0]);
		Transform::transform_apply_world(&transform, &c2, vlist + index->vert[1]);
		Transform::transform_apply_world(&transform, &c3, vlist + index->vert[2]);                  //乘以视图矩阵 v * world * view * projection

		//light->Light_Renderer(&c1, &c2, &c3,&n1,&n2,&n3, &cam);
		Transform::transform_apply_view_projection(&transform, &c1, &c1);
		Transform::transform_apply_view_projection(&transform, &c2, &c2);
		Transform::transform_apply_view_projection(&transform, &c3, &c3);                  //乘以视图矩阵 v * world * view * projection

		Transform::transform_homogenize(&transform, &c1, &c1);                 //除以齐次坐标项并转换为屏幕坐标
		Transform::transform_homogenize(&transform, &c2, &c2);
		Transform::transform_homogenize(&transform, &c3, &c3);
		point_t n = { 0, 0, -1, 1 };
		if (device_backface_judge(&p1.pos, &p2.pos, &p3.pos, &n)) return;
		if (render_state &( RENDER_STATE_COLOR | RENDER_STATE_TEXTURE))
		{
			device_draw_triangle( &c1, &c2, &c3);
		}
		if (render_state == RENDER_STATE_WIREFRAME)
		{
			device_draw_line( (int)c1.pos.x, (int)c1.pos.y, (int)c2.pos.x, (int)c2.pos.y, foreground);
			device_draw_line( (int)c1.pos.x, (int)c1.pos.y, (int)c3.pos.x, (int)c3.pos.y, foreground);
			device_draw_line( (int)c3.pos.x, (int)c3.pos.y, (int)c2.pos.x, (int)c2.pos.y, foreground);
		}
	}
	void init_object()
	{
		std::ifstream in("vertex.txt");
		float a, b, c;
		vertex_num = 0;
		while (in >> a >> b >> c)
		{
			mesh[vertex_num++] = { { a, b, c, 1 }, { 0.0f, 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }, { 1 } };
		}
		in.close();
		std::ifstream in1("ploygon.txt");
		int x, y, z; ploygon_num = 0;
		while (in1 >> x >> y >> z)
		{
			ploygon[ploygon_num].vert[0] = z; ploygon[ploygon_num].vert[1] = y; ploygon[ploygon_num++].vert[2] = x;
		}
		in1.close();
		//mesh[0] = { { 1, -1, 1, 1 }, { 0.0f, 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }, {1 }};         //0
		//mesh[1] = { { -1, -1, 1, 1 }, { 0.0f, 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1 } };      //1
		//mesh[2] = { { -1, 1, 1, 1 }, { 0.0f, 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f }, { 1 } };       //2
		//mesh[3] = { { 1, 1, 1, 1 }, { 0.0f, 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f }, { 1 } };       //3
		//mesh[4] = { { 1, -1, -1, 1 }, { 0.0f, 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f }, { 1 } };      //4
		//mesh[5] = { { -1, -1, -1, 1 }, { 0.0f, 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f }, { 1 } };    //5
		//mesh[6] = { { -1, 1, -1, 1 }, { 0.0f, 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }, { 1 } };      //6
		//mesh[7] = { { 1, 1, -1, 1 }, { 0.0f, 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1 } };      //7
		//ploygon[0].vlist = mesh; ploygon[0].vert[0] = 0; ploygon[0].vert[1] = 1; ploygon[0].vert[2] = 2;
		//ploygon[1].vlist = mesh; ploygon[1].vert[0] = 2; ploygon[1].vert[1] = 3; ploygon[1].vert[2] = 0;
		//ploygon[2].vlist = mesh; ploygon[2].vert[0] = 7; ploygon[2].vert[1] = 6; ploygon[2].vert[2] = 5;
		//ploygon[3].vlist = mesh; ploygon[3].vert[0] = 5; ploygon[3].vert[1] = 4; ploygon[3].vert[2] = 7;
		//ploygon[4].vlist = mesh; ploygon[4].vert[0] = 0; ploygon[4].vert[1] = 4; ploygon[4].vert[2] = 5;
		//ploygon[5].vlist = mesh; ploygon[5].vert[0] = 5; ploygon[5].vert[1] = 1; ploygon[5].vert[2] = 0;
		//ploygon[6].vlist = mesh; ploygon[6].vert[0] = 1; ploygon[6].vert[1] = 5; ploygon[6].vert[2] = 6;
		//ploygon[7].vlist = mesh; ploygon[7].vert[0] = 6; ploygon[7].vert[1] = 2; ploygon[7].vert[2] = 1;
		//ploygon[8].vlist = mesh; ploygon[8].vert[0] = 2; ploygon[8].vert[1] = 6; ploygon[8].vert[2] = 7;
		//ploygon[9].vlist = mesh; ploygon[9].vert[0] = 7; ploygon[9].vert[1] = 3; ploygon[9].vert[2] = 2;
		//ploygon[10].vlist = mesh; ploygon[10].vert[0] = 3; ploygon[10].vert[1] = 7; ploygon[10].vert[2] = 4;
		//ploygon[11].vlist = mesh; ploygon[11].vert[0] = 4; ploygon[11].vert[1] = 0; ploygon[11].vert[2] = 3; ploygon_num = 12; vertex_num = 8;
		device_init_vertex_norm(ploygon);
	}
	void camera_at_zero(float x, float y, float z)
	{
		point_t eye = { x, y, z, 1 }, at = { 0, 0, 0, 1 }, up = { 0, 0, 1, 1 };
		cam = eye;
		Matrix::matrix_set_lookat(&transform.view, &eye, &at, &up);
		Transform::transform_update(&transform);
	}

	int run(float angle,float pos)
	{
		memset(light->mark_v, 0, sizeof(light->mark_v));
		device_clear();
		camera_at_zero(pos, angle, 0.0f);
		Matrix::matrix_set_rotation(&transform.world, 1, 1, 1, 0.0f);
		Transform::transform_update(&transform);
		for (int i = 0; i < ploygon_num; ++i)
		{
			auto tmp = (ploygon + i);
			(vlist + tmp->vert[0])->texture.u = 0; (vlist + tmp->vert[0])->texture.v = 0;
			(vlist + tmp->vert[1])->texture.u = 1, (vlist + tmp->vert[1])->texture.v = 0;
			(vlist + tmp->vert[2])->texture.u = 1; (vlist + tmp->vert[2])->texture.v = 1;
			device_draw_primitive(tmp);

		}
		return 1;
	}
			transform_t transform;
			int width;
			int height;
			int vertex_num;
			int ploygon_num;
			IUINT32 **framebuffer;      //framebuffer[j] 指向第j行
			float **zbuffer;
			int render_state;           // 渲染状态
			IUINT32 foreground;
			IUINT32 background;
			point_t cam;
			Light *light;
			Texture *tex;
			ploy_t ploygon[17000];
			vertex_t mesh[8200];
			vertex_t_ptr vlist;
};

#endif