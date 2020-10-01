#include "v3de.h"
#include "lcd.h"
#define SIN_INTERP 4
#define TABLE_SIZE 256

#define RMASK 0xF800
#define GMASK 0x07E0
#define BMASK 0x001F

uint16_t const sin_table[TABLE_SIZE] = {
    0,     402,   804,   1206,  1608,  2010,  2412,  2813, 
    3215,  3617,  4018,  4419,  4820,  5221,  5622,  6023, 
    6423,  6823,  7223,  7622,  8022,  8420,  8819,  9217, 
    9615,  10013, 10410, 10807, 11203, 11599, 11995, 12390, 
    12785, 13179, 13572, 13965, 14358, 14750, 15142, 15533, 
    15923, 16313, 16702, 17090, 17478, 17865, 18252, 18638, 
    19023, 19407, 19791, 20174, 20556, 20938, 21318, 21698, 
    22077, 22455, 22833, 23209, 23585, 23960, 24333, 24706, 
    25078, 25449, 25819, 26188, 26557, 26924, 27290, 27655, 
    28019, 28382, 28744, 29105, 29464, 29823, 30180, 30537, 
    30892, 31246, 31599, 31951, 32301, 32650, 32998, 33345, 
    33691, 34035, 34378, 34720, 35060, 35399, 35737, 36073, 
    36408, 36742, 37074, 37405, 37735, 38063, 38389, 38714, 
    39038, 39360, 39681, 40000, 40318, 40634, 40949, 41262, 
    41574, 41884, 42192, 42499, 42805, 43108, 43410, 43711, 
    44009, 44307, 44602, 44896, 45188, 45478, 45767, 46054, 
    46339, 46622, 46904, 47184, 47462, 47739, 48013, 48286, 
    48557, 48826, 49093, 49359, 49622, 49884, 50144, 50402, 
    50658, 50912, 51164, 51415, 51663, 51909, 52154, 52396, 
    52637, 52875, 53112, 53347, 53579, 53810, 54038, 54265, 
    54489, 54711, 54932, 55150, 55366, 55580, 55792, 56002, 
    56210, 56416, 56619, 56821, 57020, 57217, 57412, 57605, 
    57795, 57984, 58170, 58354, 58536, 58716, 58893, 59068, 
    59242, 59412, 59581, 59747, 59911, 60073, 60233, 60390, 
    60545, 60698, 60848, 60996, 61142, 61286, 61427, 61566, 
    61703, 61837, 61969, 62099, 62226, 62351, 62473, 62594, 
    62712, 62827, 62940, 63051, 63160, 63266, 63369, 63471, 
    63570, 63666, 63760, 63852, 63941, 64028, 64113, 64195, 
    64274, 64352, 64426, 64499, 64569, 64636, 64701, 64764, 
    64824, 64882, 64937, 64990, 65041, 65089, 65134, 65177, 
    65218, 65256, 65292, 65325, 65356, 65384, 65410, 65434, 
    65455, 65473, 65489, 65503, 65514, 65522, 65529, 65532
};

void v3de_draw_triangle(v3de_triangle_t* tri, uint16_t* fb)
{ 
    int p12, p13, p23;
    uint16_t color = tri->color;

    int p1x = tri->p1->x;
    int p1y = tri->p1->y;
    int p2x = tri->p2->x;
    int p2y = tri->p2->y;
    int p3x = tri->p3->x;
    int p3y = tri->p3->y;
    int minx = p1x;
    int maxx = p1x;
    int miny = p1y;
    int maxy = p1y;

    minx = minx < p2x ? minx : p2x;
    minx = minx < p3x ? minx : p3x;
    miny = miny < p2y ? miny : p2y;
    miny = miny < p3y ? miny : p3y;
    maxx = maxx > p2x ? maxx : p2x;
    maxx = maxx > p3x ? maxx : p3x;
    maxy = maxy > p2y ? maxy : p2y;
    maxy = maxy > p3y ? maxy : p3y;

    for(int y = miny < 0 ? 0 : miny; y <= maxy && y < 80; y++)
    {
        for(int x = minx < 0 ? 0 : minx; x <= maxx && x < 160; x++)
        {
            /* Is the current point inside the triangle? 
            Uses cross product to determine which side of the walls of the
            triangle the point is. */
            p12 = (p2x - p1x) * (y - p1y) - (p2y - p1y) * (x - p1x) > 0;
            p13 = (p3x - p1x) * (y - p1y) - (p3y - p1y) * (x - p1x) > 0;
            p23 = (p3x - p2x) * (y - p2y) - (p3y - p2y) * (x - p2x) > 0;

            if((p12 == p23) && (p13 != p12)) fb[(y*160)+x] = color;   
        }
    }
}

void v3de_draw_mesh(v3de_mesh_t mesh, uint16_t* fb);

void v3de_buffer_display(uint16_t* fb)
{
    for(int y = 0; y < 80; y++){
        for(int x = 0; x < 160; x++) LCD_DrawPoint(x, y, fb[(y*160)+x]);
    }
}

void v3de_buffer_display_area(uint16_t* fb, uint32_t xsta, uint32_t ysta, uint32_t xend, uint32_t yend)
{
    LCD_Address_Set(xsta,ysta,xend-1,yend-1); // Set cursor position
    for(int y = ysta; y < yend; y++){
        for(int x = xsta; x < xend; x++) LCD_WR_DATA(fb[(y*160)+x]);;
    }
}

void v3de_clear_buffer(uint16_t *fb)
{
    uint16_t color = shade(BLUE, 256);
    for(int y = 0; y < 80; y++){
        for(int x = 0; x < 160; x++) fb[(y*160)+x] = color;
    }
}

void v3de_config_mesh(v3de_mesh_t* p_mesh, v3de_triangle_t* p_tris, uint32_t polycount)
{
    p_mesh->polycount = polycount;
    p_mesh->tris = p_tris;
    p_mesh->spatial.x = 0;
    p_mesh->spatial.y = 0;
    p_mesh->spatial.z = 0;
    p_mesh->rotation.x = 0;
    p_mesh->rotation.y = 0;
    p_mesh->rotation.z = 0;
}

int32_t smul(int32_t a, int32_t b)
{
    return (a * b) >> 10;
}

int32_t sdiv(int32_t a_over, int32_t b)
{
    return (a_over << 10)/b;
}

int32_t ssqrt(int32_t n) 
{
    int64_t val = n << 10;
    unsigned long temp, g=0, b = 0x8000, bshft = 15;
    do {
        if (val >= (temp = (((g << 1) + b)<<bshft--))) {
           g += b;
           val -= temp;
        }
    } while (b >>= 1);
    return g;
}

int32_t sdot(v3de_vertex_t a, v3de_vertex_t b)
{
    return smul(a.x, b.x) + smul(a.y, b.y) + smul(a.z, b.z);
}

int32_t ssin(int32_t index)
{
    index %= (TABLE_SIZE*4);
    index = index < 0 ? (TABLE_SIZE*4) - index : index; 
    uint32_t invert = (index >> 8)%2;
    uint32_t sign = (index >> 9);
    index %= TABLE_SIZE;
    index = invert ? TABLE_SIZE - index - 1 : index;
    int32_t ret = sin_table[index] >> 6;
    return sign ? -ret : ret;
}

v3de_vertex_t snormal(v3de_triangle_t tri)
{
    v3de_vertex_t p1, p2, ret;

    p1.x = tri.p2->x - tri.p1->x;
    p1.y = tri.p2->y - tri.p1->y;
    p1.z = tri.p2->z - tri.p1->z;

    p2.x = tri.p3->x - tri.p1->x;
    p2.y = tri.p3->y - tri.p1->y;
    p2.z = tri.p3->z - tri.p1->z;

    ret.x = smul(p1.y, p2.z) - smul(p1.z, p2.y);
    ret.y = smul(p1.z, p2.x) - smul(p1.x, p2.z);
    ret.z = smul(p1.x, p2.y) - smul(p1.y, p2.x);
    return snormalize(ret);
}

v3de_vertex_t snormalize(v3de_vertex_t v)
{
    uint32_t mag = slength(v);
    v.x = sdiv(v.x, mag);
    v.y = sdiv(v.y, mag);
    v.z = sdiv(v.z, mag);
    return v;
}

uint32_t slength(v3de_vertex_t v)
{
    return ssqrt((smul(v.x,v.x))+(smul(v.y,v.y))+(smul(v.z,v.z)));
}

v3de_vertex_t rotate_vertex(v3de_vertex_t v, int32_t rotx, int32_t roty, int32_t rotz)
{
    v3de_vertex_t rv;

	//rotate x
    int32_t cos = ssin(rotx + (1 << 8));
    int32_t sin = ssin(rotx);
    rv.y = smul(v.y, cos) + smul(v.z, sin);
    rv.z = -smul(v.y, sin) + smul(v.z, cos);
    v.y = rv.y;
    v.z = rv.z;

	//rotate y
    cos = ssin(roty + (1 << 8));
    sin = ssin(roty);
    rv.x = smul(v.x, cos) - smul(v.z, sin);
    rv.z = smul(v.x, sin) + smul(v.z, cos);
    v.x = rv.x;
    v.z = rv.z;

	//rotate z
    cos = ssin(rotz + (1 << 8));
    sin = ssin(rotz);
    rv.x = smul(v.x, cos) + smul(v.y, sin);
    rv.y = -smul(v.x, sin) + smul(v.y, cos);

    return rv;
}

uint16_t shade(uint16_t color, int32_t shade)
{
    uint32_t r = ((shade * (color & RMASK))>>10) & RMASK;
    uint32_t g = ((shade * (color & GMASK))>>10) & GMASK;
    uint32_t b = ((shade * (color & BMASK))>>10) & BMASK;
    return r | g | b;
}

v3de_vertex_t move_vertex(v3de_vertex_t v, v3de_vertex_t move)
{
    v.x += move.x;
    v.y += move.y;
    v.z += move.z;

    return v;
}