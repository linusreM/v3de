#include "gd32vf103.h"
#include "systick.h"
#include "cltool.h"
#include "usb_serial_if.h"
#include "liomatrix.h"
#include "liomotor.h"
#include "liobt.h"
#include "lcd.h"
#include "string.h"
#include "float.h"
#include "stdlib.h"
#include "math.h"
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

    delay_1ms(1000);
/*
    while(1){
        delay_1ms(100);
        mpu6500_getAccel(&vec);
        if (vec.x < 0.0)LCD_ShowChar(8*6, 0, '-', OPAQUE, YELLOW);
        else LCD_ShowChar(8*6, 0, ' ', OPAQUE, YELLOW);
        LCD_ShowNum(8*7, 0, (int16_t)abs(vec.x), 5, YELLOW);
    }*/
}



int main(void)
{
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
    uint16_t framebuffer[80*160];

    v3de_mesh_t cube;
    uint32_t offset_x = 80;
    uint32_t offset_y = 40;
    int32_t spatial_counter = 0;
    v3de_triangle_t temptri;
    v3de_vertex_t p1, p2, p3;
    v3de_vertex_t light_vector = {1024, 1024, 1024};
    v3de_vertex_t camera = {0, 0, 1024};
    mpu_vector_t gyro;
    int32_t light, backface = 0;

    v3de_config_mesh(&cube, tris, 12);
    Lcd_Init();
    init_accel();
    
    temptri.p1 = &p1;
    temptri.p2 = &p2;
    temptri.p3 = &p3;

    v3de_clear_buffer(framebuffer);
    v3de_buffer_display_area(framebuffer, 0, 0, 160, 80);
        
    while(1)
    {
        for(int i = 0; i < cube.polycount; i++)
        {
            p1 = rotate_vertex(*cube.tris[i].p1, cube.rotation.x, cube.rotation.y, cube.rotation.z);
            p2 = rotate_vertex(*cube.tris[i].p2, cube.rotation.x, cube.rotation.y, cube.rotation.z);
            p3 = rotate_vertex(*cube.tris[i].p3, cube.rotation.x, cube.rotation.y, cube.rotation.z);
            p1 = move_vertex(p1, cube.spatial);
            p2 = move_vertex(p2, cube.spatial);
            p3 = move_vertex(p3, cube.spatial);

            light = ((sdot(light_vector, snormal(temptri)) + 1024)/2)+100;
            light = light > 1023 ? 1023 : light;
            light = light < 0 ? 0 : light;
            temptri.p1->x = sdiv(temptri.p1->x, temptri.p1->z);
            temptri.p1->y = sdiv(temptri.p1->y, temptri.p1->z);
            temptri.p2->x = sdiv(temptri.p2->x, temptri.p2->z);
            temptri.p2->y = sdiv(temptri.p2->y, temptri.p2->z);
            temptri.p3->x = sdiv(temptri.p3->x, temptri.p3->z);
            temptri.p3->y = sdiv(temptri.p3->y, temptri.p3->z);

            backface = sdot(snormal(temptri), camera);

            if(backface > 0)
            {
                temptri.p1->x /= 16;
                temptri.p1->y /= 16;
                temptri.p2->x /= 16;
                temptri.p2->y /= 16;
                temptri.p3->x /= 16;
                temptri.p3->y /= 16;

                temptri.p1->x += offset_x;
                temptri.p1->y += offset_y;
                temptri.p2->x += offset_x;
                temptri.p2->y += offset_y;
                temptri.p3->x += offset_x;
                temptri.p3->y += offset_y;
                temptri.color = shade(cube.tris[i].color, light);
                v3de_draw_triangle(&temptri, framebuffer);
            }
        }

        mpu6500_getGyro(&gyro);
        cube.rotation.x += (int)gyro.x/128;
        cube.rotation.y += (int)gyro.y/128;
        cube.rotation.z += (int)gyro.z/128;

        cube.spatial.x = (ssin(spatial_counter)*5)/7;
        cube.spatial.y = ssin(spatial_counter/7)/2;
        cube.spatial.z = ssin(spatial_counter*2)+4096;
        spatial_counter += 10;
        v3de_buffer_display_area(framebuffer, 0, 0, 160, 80);
        v3de_clear_buffer(framebuffer);
    }
}