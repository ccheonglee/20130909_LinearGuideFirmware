#include "stepper.h"
#include <stdio.h>
#include <limits.h>

extern void Delaynus(vu32 nus);
extern void delay_ms(uint32_t miliseconds);

int32_t qeiFeedback = 0;
uint32_t vextaFeedback = 0;
float lgPosition = 0.0;
int32_t posToGo = 0;

uint8_t TimOverflow = 0;

/**
  * @brief  This function is to initialize pwm output for stepper motor.
						In this code, pwm always 50%, ONLY frequency changes
  * @param  None
  * @retval None
  */
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

/**
  * @brief  This function is to initialize vexta excitation timing output
						to be captured using external interrupt
  * @param  None
  * @retval None
  */
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
  * @brief  This function initialises quadrature encoder input to capture
						AB phase output from ENB encoder. 
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

/**
  * @brief  This function returns qei feedback to user. No conversion to mm is done here
  * @param  None
  * @retval integer value of encoder reading
  */
int32_t getQeiFeedback (void)
{
	int32_t result;
	result = TIM3->CNT + qeiFeedback;
	return result;
}

/**
  * @brief  This function resets qei feedback reading, and restart to count from 0
  * @param  None
  * @retval None
  */
void resetQeiFeedback (void)
{
	// record travalled distance
	lgPosition += (float)(getQeiFeedback())/80.0f;		// 1 mm = 80 QEI Feedback
	
	// reset the timer
	TIM_Cmd(TIM3, DISABLE);
	TIM3->CNT = 0;
	qeiFeedback = 0;
	TIM_Cmd(TIM3, ENABLE);
}

/**
  * @brief  This function returns excitation timing to user.
						No conversion to mm is done here
  * @param  None
  * @retval integer value of timing input.
  */
uint32_t getVextaFeedback (void)
{
	return vextaFeedback;
}

/**
  * @brief  This function resets timing feedback reading, and restart to count from 0
  * @param  None
  * @retval None
  */
void resetVextaFeedback (void)
{
	vextaFeedback = 0;
}

/**
  * @brief  This function returns current position to user, which has been converted to mm
  * @param  None
  * @retval floating point value in unit mm
  */
float getPosition (void)
{
	float currentPosition;

	currentPosition = (float)(getQeiFeedback())/80.0f;		// 1 mm = 80 QEI Feedback
	currentPosition += lgPosition;											// lgPosition is recorded travelled distance where
																											// user has already reset the QEI Feedback
	return currentPosition;
}

/**
  * @brief  This function resets current position to the value you wish.
            Usually ONLY Step Left, Step Right, Home will use this.
  * @param  The current position value you would like to reset to
  * @retval None
  */
void resetCurrentPosition (float position)
{
	//1520.25
	//1520.31
	lgPosition = position;
}

/**
  * @brief  This function reset "position to go" to the value you wish.
            Usually ONLY Step Left, Step Right, Home will use this.
            This function different from resetCurrentPosition as it reset the "position to go"
            Current Position will keep update while the motor is moving. Position to go is the
            position you set where you want to go. It is not current position and will not update
            itself. Current Position will be in float, while posToGo is in int as smallest step is 1mm.
            reset position is in multiplier of 5. 1mm = 5. if wish to reset to 100mm. value of postogo is 500.
  * @param  The "position to go" value you would like to reset to
  * @retval None
  */
void resetPositionToGo (int position)
{
	//1520.25
	//1520.31
	//1520.25
	posToGo = position;
}

/**
  * @brief  This function output a signal with user define frequency and duty cycle to CH1
  * @param  frequency: user define frequency. Minimum 500Hz
	* @param  dutyCycle: user define dutyCycle. 0-100
  * @retval None
  */
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

/**
  * @brief  This function output a signal with user define frequency and duty cycle to CH2
  * @param  frequency: user define frequency. Minimum 500Hz
	* @param  dutyCycle: user define dutyCycle. 0-100
  * @retval None
  */
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

/**
  * @brief  This function output a signal with user define frequency and duty cycle to CH1
  * @param  direction: MOTOR_FORWARD, MOTOR_BACKWARD, or MOTOR_STOP
	* @param  speed: speed in Hertz. Minimum 500
  * @retval None
  */
void runStepper (uint8_t direction, uint16_t speed)
{
	// due to prescaler setting, minimum speed = 500Hz
	if (speed < 321)
		speed = 321;
	
	// the output is inverted, thus 100% duty cycle = 0% duty in actual.
	if (direction == MOTOR_FORWARD){
		TIM12_CH1_PWM_OUT(speed, 50);
		TIM12_CH2_PWM_OUT(speed, 100);
		//printf ("$LG4,%d\r", counter);
	}
	else if (direction == MOTOR_BACKWARD){
		TIM12_CH2_PWM_OUT(speed, 50);
		TIM12_CH1_PWM_OUT(speed, 100);
		//printf ("$LG8,%d\r", counter);	
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
	* @param  stepDistance: Distance to move in mm * 5. 1mm will get 5 step distance
  * @retval None
  */
void linearGuideStep (uint8_t direction, uint8_t speed, uint16_t stepDistance)
{
	int32_t distanceInPulse, overshoot = 3;
	uint16_t speedInHz, currSpeedHz = 321;
	uint16_t remainDistance;
	
	speedInHz = speed*16;							// Convert speed into Hertz. 1mm/s = 16Hz
																		// 1mm = 16 pulses
	
	if (direction == MOTOR_FORWARD){
		
		// If the tail sensor is on, mean the it reaches the maximum, do nothing and return
		if (TAIL_SENSOR == Bit_RESET)
			return;
		
		posToGo += (int32_t)stepDistance;
		distanceInPulse = posToGo*16 - overshoot;							// Convert Distance into equivalent QEI Feedback.
		stepDistance = stepDistance * 16 - overshoot;					// Convert Step Distance into equivalent QEI Feedback.
		remainDistance = distanceInPulse - getQeiFeedback();	//  Convert remaining Distance into equivalent QEI Feedback.
		
		// run the stepper
		MOTOR_COIL_ON;												// on motor coil
		runStepper (direction, currSpeedHz);	// run the motor
		
		// perform ramping if only step distance > 10mm. else skip ramping as the distance is not enough for ramping
		if (stepDistance > 700){
			// ramp up every 20ms. TimOverflow is the flag keep the 20ms time.
			while (currSpeedHz < speedInHz){
				if (TimOverflow == 1){
					// clear overflow flag
					TimOverflow = 0;
					
					//increase 180hz per ramp
					currSpeedHz = currSpeedHz + 40;
					
					// limit the ramping. if hit maximum speed, quit the ramping
					if (currSpeedHz > speedInHz)
						currSpeedHz = speedInHz;

					runStepper (direction, currSpeedHz);		// run the motor
				}
				
				// ifthe tail sensor is on
				if (TAIL_SENSOR == Bit_RESET)
					break;
			}
		
			// wait until last 1/3 distance for ramp down. while waiting, keep the speed
			while (1){
				remainDistance = distanceInPulse - getQeiFeedback();		//  Convert remaining Distance into equivalent QEI Feedback.
				if (remainDistance < (stepDistance/3))
					break;
				// or the tail sensor is on
				if (TAIL_SENSOR == Bit_RESET)
					break;
			}
		}
		
		while (1){
			//printf ("$LG,%u,%d,%d,%d\r", getVextaFeedback() ,getQeiFeedback(), HEAD_SENSOR, TAIL_SENSOR);	
			// ramp down. ramp until speed 321
			while (currSpeedHz > 321){
				// if reach the end, direct break the loop
				if (getQeiFeedback() >= distanceInPulse)
					break;
				
				if (TimOverflow == 1){
					currSpeedHz -= 40;
					runStepper (direction, currSpeedHz);		// run the motor
					TimOverflow = 0;
				}
			}

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
		
		posToGo -= (int32_t)stepDistance;
		distanceInPulse = posToGo*16 + overshoot;								// Convert Distance into equivalent QEI Feedback.
		stepDistance = stepDistance * 16 - overshoot;						// Convert Step Distance into equivalent QEI Feedback.
		remainDistance = getQeiFeedback() - distanceInPulse;		//  Convert remaining Distance into equivalent QEI Feedback.
		
		MOTOR_COIL_ON;												// on motor coil
		runStepper (direction, speedInHz);		// run the motor
		
		// perform ramping if only step distance > 10mm. else skip ramping as the distance is not enough for ramping
		if (stepDistance > 700){
			// ramp up every 20ms. TimOverflow is the flag keep the 20ms time.
			while (currSpeedHz < speedInHz){
				if (TimOverflow == 1){
					// clear overflow flag
					TimOverflow = 0;
					
					//increase 180hz per ramp
					currSpeedHz = currSpeedHz + 40;
					
					// limit the ramping. if hit maximum speed, quit the ramping
					if (currSpeedHz > speedInHz)
						currSpeedHz = speedInHz;

					runStepper (direction, currSpeedHz);		// run the motor
				}
				// or the tail sensor is on
				if (HEAD_SENSOR == Bit_RESET)
					break;
			}
		
			// wait until last 1/3 distance for ramp down. while waiting, keep the speed
			// wait until last 1/3 distance for ramp down. while waiting, keep the speed
			while (1){
				remainDistance = getQeiFeedback() - distanceInPulse;		//  Convert remaining Distance into equivalent QEI Feedback.
				if (remainDistance < (stepDistance/3))
					break;
				// or the tail sensor is on
				if (HEAD_SENSOR == Bit_RESET)
					break;
			}
		}

		while (1){
			//printf ("$LG,%u,%d,%d,%d\r", getVextaFeedback() ,getQeiFeedback(), HEAD_SENSOR, TAIL_SENSOR);	
			// ramp down. ramp until speed 321
			while (currSpeedHz > 321){
				// if reach the end, direct break the loop
				if (getQeiFeedback() <= distanceInPulse)
					break;
				
				if (TimOverflow == 1){
					currSpeedHz -= 40;
					runStepper (direction, currSpeedHz);		// run the motor
					TimOverflow = 0;
				}
			}

			// wait for feedback overshoot
			if (getQeiFeedback() <= distanceInPulse)
				break;
			
			// or the tail sensor is on
			if (HEAD_SENSOR == Bit_RESET)
				break;
		}
	}
	
	runStepper (MOTOR_STOP, speedInHz);		// stop the motor
	delay_ms(1000);
	MOTOR_COIL_OFF;
}

/**
  * @brief  This function is to home the linear guide to the left
  * @param  None
  * @retval None
  */
void linearGuideHomeLeft (void)
{
	MOTOR_COIL_ON;

	while (HEAD_SENSOR != Bit_RESET){
		runStepper (MOTOR_BACKWARD, 1000);
		delay_ms(100);
	}
	resetQeiFeedback();														// reset QEI feedback to 0
	resetVextaFeedback();													// reset Vexta feedback to 0
	resetCurrentPosition (0.0);										// reset position to 0.0mm.
	resetPositionToGo (0);                        // reset position to go to 0mm.
	
	runStepper (MOTOR_STOP, 1000);
	delay_ms(1000);
	linearGuideStep (MOTOR_FORWARD, 50, 500);
	MOTOR_COIL_ON;

	while (HEAD_SENSOR != Bit_RESET){
		runStepper (MOTOR_BACKWARD, 1000);
		delay_ms(100);
	}
	runStepper (MOTOR_STOP, 1000);		// stop the motor
	delay_ms(1000);
	MOTOR_COIL_OFF;
	
	resetQeiFeedback();														// reset QEI feedback to 0
	resetVextaFeedback();													// reset Vexta feedback to 0
	resetCurrentPosition (0.0);										// reset position to 0.0mm.
	resetPositionToGo (0);                        // reset position to go to 0mm.
}

/**
  * @brief  This function is to home the linear guide to the right
  * @param  None
  * @retval None
  */
void linearGuideHomeRight (void)
{
	MOTOR_COIL_ON;

	while (TAIL_SENSOR != Bit_RESET){
		runStepper (MOTOR_FORWARD, 1000);
		delay_ms(100);
	}
	resetQeiFeedback();
	qeiFeedback = 121620;												// reset QEI feedback to 121620 = 1520.25mm
	resetVextaFeedback();												// reset Vexta feedback to 0
	resetCurrentPosition (0);										// reset position to 0.0mm.
	resetPositionToGo (7601);                   // reset position to go to 1520.25mm.
	
	
	runStepper (MOTOR_STOP, 1000);
	delay_ms(1000);
	linearGuideStep (MOTOR_BACKWARD, 50, 500);
	MOTOR_COIL_ON;

	while (TAIL_SENSOR != Bit_RESET){
		runStepper (MOTOR_FORWARD, 1000);
		delay_ms(100);
	}
	runStepper (MOTOR_STOP, 1000);		// stop the motor
	delay_ms(1000);
	MOTOR_COIL_OFF;
	
	resetQeiFeedback();
	qeiFeedback = 121620;												// reset QEI feedback to 121620 = 1520.25mm
	resetVextaFeedback();												// reset Vexta feedback to 0
	resetCurrentPosition (0);										// reset position to 0.0mm.
	resetPositionToGo (7601);                   // reset position to go to 1520.25mm.
}
