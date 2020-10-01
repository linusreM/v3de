#include "liomatrix.h"
#include "gd32vf103.h"

//Private function declarations
static void configure_timer_interrupt(uint32_t rate_us);
static void init_gpio();
static inline void display_leds_on_row(lio_led_matrix_t *led_data);
static inline uint8_t poll_key();
static inline uint8_t get_pressed_key();

//State variables
static int matrix_on = 0;
static int switches_on = 0;

//Heap allocated resources. Private to the library
static lio_key_t unhandled_key = {LIO_KEY_NONE, LIO_KEY_PRESSED};
static lio_led_matrix_t led_matrix_s = {{{0, 0, 0, 0, 0, 0, 0, 0}}, 0};

//Keypad lookup table
static const uint8_t keypad_translate[17] = {
    0xFF,
	0x01,
	0x02,
	0x03,
	0x0A,
	0x04,
	0x05,
	0x06,
	0x0B,
	0x07,
	0x08,
	0x09,
	0x0C,
	0x0E,
	0x00,
	0x0F,
	0x0D
};


static void configure_timer_interrupt(uint32_t rate_us){

    uint32_t prescaler = ((SystemCoreClock / 1000000) * rate_us) / UINT16_MAX;

    eclic_global_interrupt_enable();

    /* Configuration structs */ 
    timer_oc_parameter_struct timer_ocinitpara;
    timer_parameter_struct timer_initpara;

    /* Enable the TIMER1 interrupt request */
    eclic_irq_enable(TIMER3_IRQn,1,0);
    
    /* Enable the peripheral clock. */
    rcu_periph_clock_enable(RCU_TIMER3);

    /* Reset the timer */
    timer_deinit(TIMER3);

    /* initialize timer configuration struct */
    timer_struct_para_init(&timer_initpara);

    /* TIMER3 configuration */
    timer_initpara.prescaler         = 1 + prescaler;
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = ((SystemCoreClock / 1000000) * (rate_us)) % UINT16_MAX;
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_init(TIMER3, &timer_initpara);

    /* initialize TIMER channel output parameter struct */
    timer_channel_output_struct_para_init(&timer_ocinitpara);

    /* Set the channel configuration */
    timer_ocinitpara.outputstate  = TIMER_CCX_ENABLE;
    timer_ocinitpara.ocpolarity   = TIMER_OC_POLARITY_HIGH;
    timer_ocinitpara.ocidlestate  = TIMER_OC_IDLE_STATE_LOW;
    timer_channel_output_config(TIMER3, TIMER_CH_0, &timer_ocinitpara);
    
    /* CH0 configuration in OC timing mode */
    timer_channel_output_pulse_value_config(TIMER3, TIMER_CH_0, ((SystemCoreClock / 1000000) * rate_us)/2);
    timer_channel_output_mode_config(TIMER3, TIMER_CH_0, TIMER_OC_MODE_TIMING);
    timer_channel_output_shadow_config(TIMER3, TIMER_CH_0, TIMER_OC_SHADOW_DISABLE);

    /* Enable interrupt on the channel */
    timer_interrupt_enable(TIMER3, TIMER_INT_CH0);
    /* Make sure interrupt flag is clear */
    timer_interrupt_flag_clear(TIMER3, TIMER_INT_CH0);

    /* Start the timer */
    timer_enable(TIMER3);
}

/*Keypad functions*/
void lio_init_keypad(uint32_t debounce_time_us){
    switches_on = 1;

    //TODO: debounce_time = debounce_time_us;
    init_gpio();
    //Start the timer interrupt if not started by initializing the led matrix. 
    if(!matrix_on) configure_timer_interrupt(2000);
    
}

lio_key_t lio_read_keypad()
{
    lio_key_t temp_key = unhandled_key;
    unhandled_key.value = LIO_KEY_NONE;
    return temp_key;
}


void lio_register_keypad_event_handler(void (*handler)(lio_key_t key));

/* LED Matrix functions */

void lio_init_led_matrix(uint32_t rate_us)
{
    matrix_on = 1;
    init_gpio();
    led_matrix_s.row = 0;
    configure_timer_interrupt(rate_us);
    
}


void lio_led_show(uint8_t data, uint8_t row);
void lio_write_led_data(lio_row_data_t rows)
{
    for(int i = 0; i < 8; i++) led_matrix_s.data.row[i] = rows.row[i];
    return;
}


static void init_gpio(){

    //GPIO init
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOA);

    if(switches_on)
    {
        gpio_init(GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_0 |
                                                              GPIO_PIN_1 |
                                                              GPIO_PIN_2) ; 

        gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_5 |
                                                                   GPIO_PIN_6 | 
                                                                   GPIO_PIN_7 | 
                                                                   GPIO_PIN_8); 
    }

    if(matrix_on)
    {
        gpio_init(GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_0 |
                                                              GPIO_PIN_1 |
                                                              GPIO_PIN_2 | 
                                                              GPIO_PIN_8 | 
                                                              GPIO_PIN_9 |
                                                              GPIO_PIN_10 |
                                                              GPIO_PIN_11 |
                                                              GPIO_PIN_12 |
                                                              GPIO_PIN_13 |
                                                              GPIO_PIN_14 |
                                                              GPIO_PIN_15); 

        gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_5 |
                                                                   GPIO_PIN_6 | 
                                                                   GPIO_PIN_7 | 
                                                                   GPIO_PIN_8); 
    }
    

    
}

static inline void display_leds_on_row(lio_led_matrix_t *led_data){

    //Save the gpio states that shoudln't be affected
    uint16_t gpio_temp = gpio_output_port_get(GPIOB) & 0x00F8;

    //Put new the next row data in there
    gpio_temp = gpio_temp | (led_data->row & 0x07) | (((led_data->data.row[led_data->row]) << 8) & 0xFF00);

    gpio_port_write(GPIOB, gpio_temp);

}

static inline uint8_t poll_key(){
    return (uint8_t)(gpio_input_port_get(GPIOA) >> 5) & 0x0F;
}


//TODO: Get rid of inefficient ifs (maybe check the disassembly?)
static inline uint8_t get_pressed_key()
{
    uint8_t key = poll_key();

    if(key)
    {
        if(key >= 4)
        {
            if(key >= 8) key = 4;
            else         key = 3;
        }
        return keypad_translate[ led_matrix_s.row + 1 + ((key - 1) * 4) ];
    }
    return keypad_translate[0];
}


//Interrupt handler
//TODO: Find a way to redirect interrupts dynamically, or at least a bit more elegantly
//      Maybe an optional library for dynamic redirection via a vector table in ram or some linker trickery
void TIMER3_IRQHandler()
{
    if(timer_interrupt_flag_get(TIMER3, TIMER_INT_CH0) == SET){
        uint8_t key;
        if(switches_on)
        {
        //TODO: debounce
            key = get_pressed_key();
            if(key != LIO_KEY_NONE) unhandled_key.value = key;
        }
    
        if(matrix_on)
        {
            led_matrix_s.row = (led_matrix_s.row + 1) & 0x07;
            display_leds_on_row(&led_matrix_s);
        }
        else
        {
            led_matrix_s.row = (led_matrix_s.row + 1) & 0x03;
            gpio_port_write(GPIOB, (gpio_input_port_get(GPIOB) & 0xFFF8) |
                                    (led_matrix_s.row & 0x07));
        }
        timer_interrupt_flag_clear(TIMER3, TIMER_INT_CH0);
    }
}