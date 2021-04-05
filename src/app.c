#include "gd32vf103.h"
#include "systick.h"
#include "lcd.h"
#include "stdlib.h"

/* This is where most of the interesting stuff happens in this program */
#include "v3de.h"
#include "gd32v_mpu6500_if.h"


#define CUBE_SIZE 1024
#define CUBE_COLOR 0xFF87


void init_accel(void){
    rcu_periph_clock_enable(RCU_I2C0);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_AF);

    /* Initialize the GPIO connected to the I2C0 bus */
    gpio_init(GPIOB, GPIO_MODE_AF_OD, GPIO_OSPEED_50MHZ, GPIO_PIN_6 | GPIO_PIN_7);
    
    /* Configure i2c clock speed and dutycycle */
    i2c_clock_config(I2C0, 100000, I2C_DTCY_2);

    /* Enable i2c */
    i2c_enable(I2C0);

    mpu6500_install(I2C0);

}



int main(void)
{
    /* Initialize vertexes of a unit cube */
    v3de_vertex_t vCube[] = {
        {-CUBE_SIZE,-CUBE_SIZE, CUBE_SIZE},
        {-CUBE_SIZE, CUBE_SIZE, CUBE_SIZE},
        { CUBE_SIZE, CUBE_SIZE, CUBE_SIZE},
        { CUBE_SIZE,-CUBE_SIZE, CUBE_SIZE},
        {-CUBE_SIZE,-CUBE_SIZE,-CUBE_SIZE},
        {-CUBE_SIZE, CUBE_SIZE,-CUBE_SIZE},
        { CUBE_SIZE, CUBE_SIZE,-CUBE_SIZE},
        { CUBE_SIZE,-CUBE_SIZE,-CUBE_SIZE}
    };
    /* Initialize triangles of cube. Back face culling is used so clock wise or anti-clockwise order matters. */
    v3de_triangle_t tris[] = {
        {&vCube[0], &vCube[1], &vCube[3], CUBE_COLOR},
        {&vCube[3], &vCube[1], &vCube[2], CUBE_COLOR},
        {&vCube[1], &vCube[0], &vCube[4], CUBE_COLOR},
        {&vCube[5], &vCube[1], &vCube[4], CUBE_COLOR},
        {&vCube[3], &vCube[4], &vCube[0], CUBE_COLOR},
        {&vCube[3], &vCube[7], &vCube[4], CUBE_COLOR},
        {&vCube[3], &vCube[2], &vCube[7], CUBE_COLOR},
        {&vCube[7], &vCube[2], &vCube[6], CUBE_COLOR},
        {&vCube[2], &vCube[1], &vCube[5], CUBE_COLOR},
        {&vCube[2], &vCube[5], &vCube[6], CUBE_COLOR},
        {&vCube[7], &vCube[5], &vCube[4], CUBE_COLOR},
        {&vCube[7], &vCube[6], &vCube[5], CUBE_COLOR}
    };

    /* Initialize the LCD frame buffer so that pixels can be calculated without waiting for SPI to finish */
    uint16_t framebuffer[80*160];

    /* Cube object */
    v3de_mesh_t cube;

    /* LCD middle offsets */
    uint32_t offset_x = 80;
    uint32_t offset_y = 40;

    /* Timebase for movin the cube (translation) */
    int32_t spatial_counter = 0;

    /* Temporary triangle */
    v3de_triangle_t temptri;
    v3de_vertex_t p1, p2, p3;

    /* Light direction vector */
    v3de_vertex_t light_vector = {1024, 1024, 1024};

    /* Camera direction */
    v3de_vertex_t camera = {0, 0, 1024};

    /* For IMU data */
    mpu_vector_t gyro;

    /* Temp storage */
    int32_t light, backface = 0;

    /* Initialize cube */
    v3de_config_mesh(&cube, tris, 12);

    /* Initialize temporary triangle */
    temptri.p1 = &p1;
    temptri.p2 = &p2;
    temptri.p3 = &p3;

    /* Initialize LCD */
    Lcd_Init();
    
    /* Initialize IMU */
    init_accel();
    
    /* Initialize framebuffer and display */
    v3de_clear_buffer(framebuffer);
    v3de_buffer_display_area(framebuffer, 0, 0, 160, 80);
        
    while(1)
    {
        /* Get current rotation, and scale gyro to match the fixed point number system used */
        mpu6500_getGyro(&gyro);
        cube.rotation.x += (int)gyro.x/128;
        cube.rotation.y += (int)gyro.y/128;
        cube.rotation.z += (int)gyro.z/128;

        /* Apply some movement to the cube, just to make it more visually interesting */
        cube.spatial.x = (ssin(spatial_counter)*5)/7;
        cube.spatial.y = ssin(spatial_counter/7)/2;
        cube.spatial.z = ssin(spatial_counter*2)+4096;
        spatial_counter += 10;

        /* Go through each triangle in the cube */
        for(int i = 0; i < cube.polycount; i++)
        {
            /* Apply current rotation and translation */
            p1 = rotate_vertex(*cube.tris[i].p1, cube.rotation.x, cube.rotation.y, cube.rotation.z);
            p2 = rotate_vertex(*cube.tris[i].p2, cube.rotation.x, cube.rotation.y, cube.rotation.z);
            p3 = rotate_vertex(*cube.tris[i].p3, cube.rotation.x, cube.rotation.y, cube.rotation.z);
            p1 = move_vertex(p1, cube.spatial);
            p2 = move_vertex(p2, cube.spatial);
            p3 = move_vertex(p3, cube.spatial);

            /* Calculate light intensity */
            light = ((sdot(light_vector, snormal(temptri)) + 1024)/2)+100;
            light = light > 1023 ? 1023 : light;
            light = light < 0 ? 0 : light;

            /* Project into 2D coordinates */
            temptri.p1->x = sdiv(temptri.p1->x, temptri.p1->z);
            temptri.p1->y = sdiv(temptri.p1->y, temptri.p1->z);
            temptri.p2->x = sdiv(temptri.p2->x, temptri.p2->z);
            temptri.p2->y = sdiv(temptri.p2->y, temptri.p2->z);
            temptri.p3->x = sdiv(temptri.p3->x, temptri.p3->z);
            temptri.p3->y = sdiv(temptri.p3->y, temptri.p3->z);

            /* Only draw if facing forward */
            backface = sdot(snormal(temptri), camera);

            if(backface > 0)
            {   
                /* Apply additional scaling to fit on LCD */
                temptri.p1->x /= 16;
                temptri.p1->y /= 16;
                temptri.p2->x /= 16;
                temptri.p2->y /= 16;
                temptri.p3->x /= 16;
                temptri.p3->y /= 16;

                /* Offset to middle of LCD */
                temptri.p1->x += offset_x;
                temptri.p1->y += offset_y;
                temptri.p2->x += offset_x;
                temptri.p2->y += offset_y;
                temptri.p3->x += offset_x;
                temptri.p3->y += offset_y;

                /* Calculate color based on light intensity */
                temptri.color = shade(cube.tris[i].color, light);

                /* Draw triangle to fram buffer */
                v3de_draw_triangle(&temptri, framebuffer);
            }
        }
        /* Draw the frame buffer */
        v3de_buffer_display_area(framebuffer, 0, 0, 160, 80);
        /* Clear for next draw */
        v3de_clear_buffer(framebuffer);
    }
}