#include "liomotor.h"
#include "gd32vf103.h"

static uint32_t timer_initialized = 0;
static uint32_t motor_a_direction = 0;
static uint32_t motor_b_direction = 0;

//Private functions
static void init_motor_timer(uint32_t frequency_hz, uint32_t resolution)
{
    uint32_t period = (1U << resolution) - 1;
    float prescaler = ((float)SystemCoreClock*(1.0/(float)frequency_hz))/(float)period;

    timer_parameter_struct timer_initpara;

    rcu_periph_clock_enable(RCU_TIMER4);

    timer_deinit(TIMER4);

    timer_struct_para_init(&timer_initpara);

    timer_initpara.prescaler         = (int)prescaler;                  
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;  
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;    
    timer_initpara.period            = period; 
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;    
    timer_initpara.repetitioncounter = 0;                   // Runs continiously
    timer_init(TIMER4, &timer_initpara);                    

    /* auto-reload preload enable */
    timer_auto_reload_shadow_enable(TIMER4);

    /* start the timer */
    timer_enable(TIMER4);
}

static void configure_motor_timer_channel(uint32_t timer, uint16_t channel)
{
    timer_oc_parameter_struct timer_ocinitpara;
    timer_channel_output_struct_para_init(&timer_ocinitpara);

    /* PWM configuration */
    timer_ocinitpara.outputstate  = TIMER_CCX_ENABLE;                   // Channel enable
    timer_ocinitpara.outputnstate = TIMER_CCXN_DISABLE;                 // Disable complementary channel
    timer_ocinitpara.ocpolarity   = TIMER_OC_POLARITY_HIGH;             // Active state is high
    timer_ocinitpara.ocnpolarity  = TIMER_OCN_POLARITY_HIGH;    
    timer_ocinitpara.ocidlestate  = TIMER_OC_IDLE_STATE_LOW;            // Idle state is low
    timer_ocinitpara.ocnidlestate = TIMER_OCN_IDLE_STATE_LOW;
    timer_channel_output_config(timer,channel,&timer_ocinitpara);   // Apply settings to channel

    timer_channel_output_pulse_value_config(timer,channel,0);                   // Set pulse width
    timer_channel_output_mode_config(timer,channel,TIMER_OC_MODE_PWM0);         // Set pwm-mode
    timer_channel_output_shadow_config(timer,channel,TIMER_OC_SHADOW_DISABLE);
}





//Public interface

//The frequency selection is very coarse at higher frequencies
void lio_motor_init(uint32_t motor, uint32_t direction, uint32_t frequency_hz, uint32_t resolution)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_AF);
    if(!timer_initialized)
    {
        init_motor_timer(frequency_hz, resolution);
        timer_initialized = 1;
    } 
    
    if(motor == LIO_MOTOR_A)
    {
        motor_a_direction = direction;
        gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_0);
        gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_1);
        configure_motor_timer_channel(TIMER4, TIMER_CH_0);
        configure_motor_timer_channel(TIMER4, TIMER_CH_1);
    }

    if(motor == LIO_MOTOR_B)
    {
        motor_b_direction = direction;
        gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_2);
        gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_3);
        configure_motor_timer_channel(TIMER4, TIMER_CH_2);
        configure_motor_timer_channel(TIMER4, TIMER_CH_3);
    }
}

void lio_motor_speed(uint32_t motor, int32_t duty)
{
    int32_t dir = duty > 0 ? 1 : 0;
     
    duty = duty < 0 ? -duty : duty;
    if(motor == LIO_MOTOR_A)
    {
        dir ^= motor_a_direction;
        timer_channel_output_pulse_value_config(TIMER4,TIMER_CH_0, duty * dir);
        timer_channel_output_pulse_value_config(TIMER4,TIMER_CH_1, duty * !dir);
    }
    if(motor == LIO_MOTOR_B)
    {
        dir ^= motor_b_direction;
        timer_channel_output_pulse_value_config(TIMER4,TIMER_CH_2, duty * dir);
        timer_channel_output_pulse_value_config(TIMER4,TIMER_CH_3, duty * !dir);
    }
}