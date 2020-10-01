#pragma once
#include "gd32vf103.h"
#define LIO_MOTOR_A     0
#define LIO_MOTOR_B     1

#define LIO_MOTOR_NORMAL 0
#define LIO_MOTOR_INVERT 1

void lio_motor_init(uint32_t motor, uint32_t direction, uint32_t frequency_hz, uint32_t resolution);
void lio_motor_speed(uint32_t motor, int32_t duty);