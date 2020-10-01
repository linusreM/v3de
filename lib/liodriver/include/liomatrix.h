#pragma once

#include "stdint.h"


typedef struct 
{
    uint8_t row[8];
} lio_row_data_t;


//Private?
typedef struct
{
    lio_row_data_t data;
    uint8_t row;

} lio_led_matrix_t;


typedef enum
{
    LIO_KEY_ONE = 0x01,
    LIO_KEY_TWO = 0x02,
    LIO_KEY_THREE = 0x03,
    LIO_KEY_FOUR = 0x04,
    LIO_KEY_FIVE = 0x05,
    LIO_KEY_SIX = 0x06,
    LIO_KEY_SEVEN = 0x07,
    LIO_KEY_EIGHT = 0x08,
    LIO_KEY_NINE = 0x09,
    LIO_KEY_A = 0x0A,
    LIO_KEY_B = 0x0B,
    LIO_KEY_C = 0x0C,
    LIO_KEY_D = 0x0D,
    LIO_KEY_STAR = 0x0E,
    LIO_KEY_SQUARE = 0x0F,
    LIO_KEY_NONE = 0xFF

} lio_key_value_t;


typedef enum
{
    LIO_KEY_RELEASE,
    LIO_KEY_PRESSED

} lio_key_state_t;



typedef struct
{
    lio_key_value_t value;
    lio_key_state_t state;

} lio_key_t;


/*Keypad functions*/
void lio_init_keypad(uint32_t debounce_time_us);
lio_key_t lio_read_keypad();


void lio_register_keypad_event_handler(void (*handler)(lio_key_t key));

/* LED Matrix functions */

void lio_init_led_matrix(uint32_t rate_us);
void lio_led_show(uint8_t data, uint8_t row);
void lio_write_led_data(lio_row_data_t rows);

/* Interrupt handler */

void TIMER3_IRQHandler();
