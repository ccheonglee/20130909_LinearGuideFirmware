#include "stepper.h"
#include <stdio.h>
#include <limits.h>

extern void Delaynus(vu32 nus);
extern void delay_ms(uint32_t miliseconds);

int32_t qeiFeedback = 0;
uint32_t vextaFeedback = 0;

void PWM_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	
	/* TIMx clock enable */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM12, ENABLE);

	/* GPIOB clock enable */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

	/* GPIOB Configuration: TIM12 CH1 (PB14) and TIM12 CH2 (PB15) */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14 | GPIO_Pin_15 ;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* Connect TIMx pins to AF */
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource14, GPIO_AF_TIM12);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource15, GPIO_AF_TIM12);

	//PWM
	//84M/1000=84000  (timer f/f needed)
	//84000=16800*5 (period*prescaler)
	//16800*0.5=8400 (if for duty cycle 50% and 1000Hz)(4200 for CCRx_Val)(8400 for Period_Val)

	//84M/100=840000  (timer f/f needed)
	//840000=16800*50 (period*prescaler)
	//16800*0.5=8400 (if for duty cycle 50% and 100Hz)(8400 for CCRx_Val)(16800 for Period_Val)

	//(Prescaler - 1)
	//(Period - 1)

	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = 0;
	TIM_TimeBaseStructure.TIM_Prescaler = TIM12_PRESCALER;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

	TIM_TimeBaseInit(TIM12, &TIM_TimeBaseStructure);

	TIM_ARRPreloadConfig(TIM12, ENABLE);

	/* TIM12 disable counter */
	TIM_Cmd(TIM12, DISABLE);
}

void StepperFeedback_init (void)
{
	EXTI_InitTypeDef   EXTI_InitStructure;
	GPIO_InitTypeDef   GPIO_InitStructure;
	NVIC_InitTypeDef   NVIC_InitStructure;

	/* Enable GPIOB clock */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	
	/* Enable SYSCFG clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

	/* Configure PB12 pin as input pull up */
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* Connect MOTOR feedback to PB13, use EXTI to capture feedback */
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOB, EXTI_PinSource13);

	/* Configure EXTI Line13 */
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_InitStructure.EXTI_Line = EXTI_Line13;
	EXTI_Init(&EXTI_InitStructure);

	/* Enable and set EXTI Line13 Interrupt to the lowest priority */
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
	NVIC_Init(&NVIC_InitStructure);
}


/**
  * @brief  Encoder1 timer initialization
  * @param  void
  * @retval void
  * @brief
  * PB4  --> TIM3 CH1
  * PB5  --> TIM3 CH2
  */
void QEI1_init (void)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_ICInitTypeDef TIM_ICInitStructure;

	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	/* TIM3 clock source enable */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

	/* Enable GPIOB, clock */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

	GPIO_StructInit(&GPIO_InitStructure);
	/* Configure PB.4,5 as encoder alternate function */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP ;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* Connect TIM3 pins to AF2 */
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource4, GPIO_AF_TIM3);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_TIM3);

	/* Enable the TIM3 Update Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/* Timer configuration in Encoder mode */
	TIM_DeInit(TIM3);

	TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);

	TIM_TimeBaseStructure.TIM_Prescaler = 0x0;  // No prescaling
	TIM_TimeBaseStructure.TIM_Period = (4*4000)-1;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

	TIM_EncoderInterfaceConfig(TIM3, TIM_EncoderMode_TI12,
	TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);

	TIM_ICStructInit(&TIM_ICInitStructure);
	TIM_ICInitStructure.TIM_ICFilter = 6;
	TIM_ICInit(TIM3, &TIM_ICInitStructure);

	// Clear all pending interrupts
	TIM_ClearFlag(TIM3, TIM_FLAG_Update);
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

	//Reset counter
	TIM3->CNT = 0;      //encoder value

	TIM_Cmd(TIM3, ENABLE);
}

int32_t getQeiFeedback (void)
{
	int32_t result;
	result = TIM3->CNT + qeiFeedback;
	return result;
}

void TIM12_CH1_PWM_OUT (uint16_t frequency, uint8_t dutyCycle)
{
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	uint16_t pwmDutyCycleCounter;
	uint32_t period;

	// calculate the period and duty cycle needed in instruction cycles
	period = ((uint32_t)(84000000)/(TIM12_PRESCALER+1)/frequency);
	pwmDutyCycleCounter = (dutyCycle*period)/100;		// resolution 1%
	
	// if > 100% duty cycle, just keet it at 100%
	if (dutyCycle >= 100)
		pwmDutyCycleCounter = period + 1;
	
	// disable timer before changing register value
	TIM_Cmd(TIM12, DISABLE);
	
	// update duty cycle
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = pwmDutyCycleCounter;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC1Init(TIM12, &TIM_OCInitStructure);
	TIM_OC1PreloadConfig(TIM12, TIM_OCPreload_Enable);
	
	// update period
	TIM_TimeBaseStructure.TIM_Period = period;
	TIM_TimeBaseStructure.TIM_Prescaler = TIM12_PRESCALER;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM12, &TIM_TimeBaseStructure);
	
	TIM_ARRPreloadConfig(TIM12, ENABLE);
	
	// reset timer counter to restart from 0
	TIM12->CNT = 0;
	
	// enable timer to continue generate pulse
	TIM_Cmd(TIM12, ENABLE);
}

void TIM12_CH2_PWM_OUT (uint16_t frequency, uint8_t dutyCycle)
{
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	uint16_t pwmDutyCycleCounter;
	uint32_t period;

	// calculate the period and duty cycle needed in instruction cycles
	period = ((uint32_t)(84000000)/(TIM12_PRESCALER+1)/frequency);
	pwmDutyCycleCounter = (dutyCycle*period)/100;		// resolution 1%
	
	// if > 100% duty cycle, just keet it at 100%
	if (dutyCycle >= 100)
		pwmDutyCycleCounter = period + 1;
	
	// disable timer before changing register value
	TIM_Cmd(TIM12, DISABLE);
	
	// update duty cycle
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = pwmDutyCycleCounter;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC2Init(TIM12, &TIM_OCInitStructure);
	TIM_OC2PreloadConfig(TIM12, TIM_OCPreload_Enable);
	
	// update period
	TIM_TimeBaseStructure.TIM_Period = period;
	TIM_TimeBaseStructure.TIM_Prescaler = TIM12_PRESCALER;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM12, &TIM_TimeBaseStructure);
	
	TIM_ARRPreloadConfig(TIM12, ENABLE);
	
	// reset timer counter to restart from 0
	TIM12->CNT = 0;
	
	// enable timer to continue generate pulse
	TIM_Cmd(TIM12, ENABLE);
}

void runStepper (uint8_t direction, uint16_t speed)
{
	// the output is inverted, thus 100% duty cycle = 0% duty in actual.
	if (direction == MOTOR_FORWARD){
		TIM12_CH1_PWM_OUT(speed, 50);
		TIM12_CH2_PWM_OUT(speed, 100);
	}
	else if (direction == MOTOR_BACKWARD){
		TIM12_CH2_PWM_OUT(speed, 50);
		TIM12_CH1_PWM_OUT(speed, 100);
	}
	else{
		TIM12_CH1_PWM_OUT(speed, 100);
		TIM12_CH2_PWM_OUT(speed, 100);
	}
}

/**
  * @brief  This function is to step the linear guide to a direction with
						user define speed and distance
  * @param  direction: MOTOR_FORWARD or MOTOR_BACKWARD
  * @param  speed: speed of linear guide in mm/s
	* @param  stepDistance: Distance to move in mm
  * @retval None
  */
void linearGuideStep (uint8_t direction, uint8_t speed, uint16_t stepDistance)
{
	int32_t distanceInPulse;
	uint16_t speedInHz;
	speedInHz = speed*16;							// Convert speed into Hertz. 1mm/s = 16Hz
																				// 1mm = 16 pulses
	RESET_QEI_FEEDBACK;										// reset QEI feedback
	
	if (direction == MOTOR_FORWARD){
		// If the tail sensor is on, mean the it reaches the maximum, do nothing and return
		if (TAIL_SENSOR == Bit_RESET)
			return;
		
		MOTOR_COIL_ON;												// on motor coil
		runStepper (direction, speedInHz);		// run the motor
		distanceInPulse = stepDistance*16;		// Convert Distance into equivalent QEI Feedback.
		while (1){
			// wait for feedback overshoot
			if (getQeiFeedback() >= distanceInPulse)
				break;
			// or the tail sensor is on
			if (TAIL_SENSOR == Bit_RESET)
				break;
		}
	}
	else{
		// If the head sensor is on, mean the it reaches the maximum, do nothing and return
		if (HEAD_SENSOR == Bit_RESET)
			return;
		MOTOR_COIL_ON;												// on motor coil
		runStepper (direction, speedInHz);		// run the motor
		distanceInPulse = stepDistance*(-16);		// Convert Distance into equivalent QEI Feedback.
		while (distanceInPulse < getQeiFeedback()){
			// wait for feedback overshoot
			if (distanceInPulse >= getQeiFeedback())
				break;
			// or the head sensor is on
			if (HEAD_SENSOR == Bit_RESET)
				break;
		}
	}
	
	runStepper (MOTOR_STOP, speedInHz);		// stop the motor
	MOTOR_COIL_OFF;
	delay_ms(10);
}

/**
  * @brief  This function is to home the linear guide
  * @param  None
  * @retval None
  */
void linearGuideHome (void)
{
	MOTOR_COIL_ON;
	runStepper (MOTOR_BACKWARD, 700);
	while (HEAD_SENSOR != Bit_RESET){
		printf ("Vexta feedback = %u, QEI feedback = %d\r\n", vextaFeedback, GET_QEI_FEEDBACK);
		delay_ms(100);
	}
	delay_ms(500);
	MOTOR_COIL_OFF;
}
