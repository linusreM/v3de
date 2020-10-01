#pragma once

#include "stdint.h"

typedef struct {
    int32_t x, y, z;
}v3de_vertex_t;

typedef struct{
    v3de_vertex_t* p1;
    v3de_vertex_t* p2;
    v3de_vertex_t* p3;
    uint16_t color;
}v3de_triangle_t;

typedef struct{
    v3de_triangle_t *tris;
    uint32_t polycount;
    v3de_vertex_t spatial;
    v3de_vertex_t rotation;
}v3de_mesh_t;


void v3de_draw_triangle(v3de_triangle_t* tri, uint16_t* fb);
void v3de_draw_mesh(v3de_mesh_t mesh, uint16_t* fb);
void v3de_buffer_display(uint16_t* fb);
void v3de_buffer_display_area(uint16_t* fb, uint32_t xsta, uint32_t ysta, uint32_t xend, uint32_t yend);
void v3de_config_mesh(v3de_mesh_t* p_mesh, v3de_triangle_t* p_tris, uint32_t polycount);


int32_t smul(int32_t a, int32_t b);
int32_t sdiv(int32_t a_over, int32_t b);
int32_t ssqrt(int32_t n);
int32_t sdot(v3de_vertex_t a, v3de_vertex_t b);
int32_t ssin(int32_t n);
v3de_vertex_t snormal(v3de_triangle_t tri);
int32_t backface(v3de_triangle_t tri);
v3de_vertex_t snormalize(v3de_vertex_t v);
uint32_t slength(v3de_vertex_t v);

v3de_vertex_t rotate_vertex(v3de_vertex_t v, int32_t rotx, int32_t roty, int32_t rotz);
v3de_vertex_t move_vertex(v3de_vertex_t v, v3de_vertex_t move);
uint16_t shade(uint16_t color, int32_t shade);
void v3de_clear_buffer(uint16_t *fb);
