/**
 * This class allows for control of a 3 phase BLDC motor.
 * It uses trapezoidal commutation to start up the motor, and once it has moved a bit, it will switch to sinusoidal control.
 * The sinusoidal control runs between 0 and motor->pwm % duty cycle, and each phase switches to a new pwm% duty cycle when the duty cycle hits 0%.
 */

#ifndef __MOTOR__H
#define __MOTOR__H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"
#include "config.h"
#include "constants.h"

#define PWM_MOTOR 31250			//PWM frequency in Hertz
#define MIN_SPEED 11			//rotations per minute
#define MAX_SPEED 360
#define DUTY_STEPS 384

//state machine
enum MOTOR_STATE {
  STARTING, SETTING_UP, READY_TO_TRANSITION, TRANSITIONING, GOING, STOPPED
};

//non volatile stuff, constant things
struct Motor_setup {
	// just L or R to help with debugging
	char side;

	// related to timer running pwm
	TIM_HandleTypeDef htim_pwm;
	int8_t TIM_PWM_IRQn;

	// timer determining the duty cycle
	TIM_HandleTypeDef htim_duty;
	int8_t TIM_DUTY_IRQn;

	// timer determining the speed
	TIM_HandleTypeDef htim_speed;
	int8_t TIM_SPEED_IRQn;
	int8_t TS_bitmask;

	// hall pins
	GPIO_TypeDef* HALL_PORT;
	int16_t HALL_PINS[3];
	int8_t EXTI_IRQn;

	uint8_t OFFSET_POS_HALL;
	uint8_t OFFSET_NEG_HALL;
	int8_t OFFSET_DIR;

	// pull ups, active when BSRR is reset / low
	GPIO_TypeDef* GPIO_LOW_PORTS[3];
	uint16_t GPIO_LOW_CH_PINS[3];

	// pull downs, active when BSRR is set / high
	GPIO_TypeDef* GPIO_HIGH_PORT;
	uint16_t GPIO_HIGH_CH_PINS;
};

// volatile variables that change on interrupts
struct Motor {
	struct Motor_setup setup;
	struct Motor* other_motor;
	volatile uint32_t uwPeriodValue;
	volatile float pwm_percent_period;

	volatile enum MOTOR_STATE state;
	volatile uint16_t period_count; //hall
	volatile uint16_t position; //hall

	volatile float pwm;
	volatile float new_pwm;
	volatile float ratio;

	volatile int64_t delta;
	volatile float last_hall_count;
	volatile float this_hall_count;
	volatile float total_hall_count;
	volatile uint64_t hall_limit;

	volatile uint16_t speed;
	volatile uint16_t target_speed;
	volatile int8_t direction; //+1 or -1

	//this is to keep track of an "old" set of duty cycles and a "new set", so that all three channels of the motor can transition smoothly
	//check out in_range_duty function for more details
	volatile uint16_t DUTY_LOOKUP_1[DUTY_STEPS];
	volatile uint16_t DUTY_LOOKUP_2[DUTY_STEPS];
	volatile uint16_t *DUTY_LOOKUP_POINTER_OLD;
	volatile uint16_t *DUTY_LOOKUP_POINTER_NEW;

	volatile int16_t timer_duty_cnt;
};

void motors_setup_and_init(void);
void motors_speeds(int16_t l_rpm, int16_t r_rpm);
void motors_pwms();
void motors_stop();
void motors_calibrate();

void HALL_ISR_Callback(struct Motor *motor);
void Duty_ISR_Callback(struct Motor *motor);
void Speed_ISR_Callback(struct Motor *motor);

extern void error_handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __STM32F1xx_IT_H */
