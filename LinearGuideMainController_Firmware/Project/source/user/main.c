/**
*****************************************************************************
**
**	Author: Lee Chee Cheong. Latest Update 10 September 2013, 11.32p.m.
**	Author: Lee Chee Cheong. Latest Update 15 January 2014, 3.06p.m.
**
**  File        : main.c
**
**  Abstract    : main function.
**
**  Functions   : main
**
**  Environment : Keil MDK ARM(R)
**                STMicroelectronics STM32F4xx Standard Peripherals Library
**
**  Distribution: The file is distributed æ?³s is,ï¿½without any warranty
**                of any kind.
**
**  (c)Copyright CRSST
**  You may use this file as-is or modify it according to the needs of your
**  project.
**
**
**	Project Description:
**	Linear Guide Stepper Motor Control
**  Stepper Motor Specification
**	- 1.8 degree per step. 200 pulses per revolution (Full Step)
**	- 0.9 degree per step. 400 pulses per revolution (Half Step)
**	- Excitation Timing (vextaFeedback) 1 output per 4 pulses (Full Step)
**	- Excitation Timing (vextaFeedback) 1 output per 8 pulses (Half Step)
**	
**	Encoder Specification
**	- single phase 500 pulses per revolution.
**	- AB phase will give you 2000 pulses per revolution
**	- AB phase is used in this program
**
**	Hardware Specification (Based on Half Step and AB Phase Encoder Output)
**	- One revolution = 25mm in translational motion
**	- One revolution = 400 pulses to Stepper, 50 vexta Feedback or 2000 QEI Feedback.
**	- 1mm = 16 pulses to Stepper, 2 vextaFeedback, or 80 QEI Feedback.
**	- 1mm/s = 16 Hz. Minimum Velocity = 50mm/s = 800Hz.
**	- Dr Koo's Spec: max speed 200mm/s (3200Hz), resolution 1mm
**
**	Serial Communication Protocol
**	From PC --------> Microcontroller
**	- "$HOME\r"	- Perform Homing Action
**	- "$STEP,<speed>,<distance>\r" - Perform step action, with user define speed 
**																   and distance
**
**	From Microcontroller ------------> PC
**	- "$LG,<distance from home in mm>,<head sensor>,<tail sensor>\r"
**
**	Sensor and Moving Direction Description
**	One end with TWO supporting point = HEAD
**	The other end with ONLY ONE Supporting point = TAIL
**	Movement from HEAD ----> TAIL  = MOVING FORWARD
**	Movement from TAIL ----> HEAD  = MOVING BACKWARD
*****************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include <stdio.h>
#include "stepper.h"
#include "serialcom.h"

/** @addtogroup STM32F4_Discovery_Peripheral_Examples
  * @{
  */

/** @addtogroup IO_Toggle
  * @{
  */ 

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
SER_MSG serialMsg;

/* Private function prototypes -----------------------------------------------*/
void GPIO_Configuration(void);
void TIM5_Configuration (void);

void Delaynus(vu32 nus);
void delay_ms(uint32_t miliseconds);

#ifdef __GNUC__
  /* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
	/*!< At this stage the microcontroller clock setting is already configured, 
		this is done through SystemInit() function which is called from startup
		file (startup_stm32f4xx.s) before to branch to application main.
		To reconfigure the default setting of SystemInit() function, refer to
		system_stm32f4xx.c file
	*/
	
	Usart2_init(115200);
	GPIO_Configuration ();
	PWM_init();
	QEI1_init ();
	TIM5_Configuration ();
	StepperFeedback_init();
	
	// led indicator
//	GPIO_SetBits(GPIOD, GPIO_Pin_12);
	GPIO_SetBits(GPIOD, GPIO_Pin_14);
	
	printf ("Hello world\r\n");
	
//Go and back test program	
/*	linearGuideHome ();														// home the linear guide
 	delay_ms(1000);
		
	MOTOR_COIL_ON;												// on motor coil
	while (1){
		runStepper (MOTOR_FORWARD, 500);
		while (TAIL_SENSOR != Bit_RESET);
		runStepper (MOTOR_STOP, 500);
	delay_ms(1000);
		runStepper (MOTOR_BACKWARD, 500);
		while (HEAD_SENSOR != Bit_RESET);
		runStepper (MOTOR_STOP, 500);
	delay_ms(1000);
	} */
	
 	// Home linear guide
	linearGuideHome ();														// home the linear guide
// 	delay_ms(1000);
//  	linearGuideStep (MOTOR_FORWARD, 50, 1000);		// move motor forward, 100mm/s, 1000mm distance
// 	delay_ms(1000);
// 	linearGuideHome ();														// home the linear guide
	
	while (1){
		// receive and process message from usart2
		if (parseSerialMessage(&serialMsg) == 1)
			continue;
 		// perform homing action here
		if(serialMsg.command == COMMAND_HOME){
			linearGuideHome ();														// home the linear guide
			delay_ms(1000);
			// feedback to PC on machine status
			printf ("$LG,%.2f,%d,%d\r", getPosition(), HEAD_SENSOR, TAIL_SENSOR);
		}
 		// perform STEP to right action here
		else if (serialMsg.command == COMMAND_STEP){
			linearGuideStep (MOTOR_FORWARD, serialMsg.speed, serialMsg.distance);		// move motor forward, 50mm/s, 25mm distance
			// feedback to PC on machine status
			printf ("$LG,%.2f,%d,%d\r", getPosition(), HEAD_SENSOR, TAIL_SENSOR);
		}
		// perform go to Left Edge
		else if (serialMsg.command == COMMAND_LEFT){
				MOTOR_COIL_ON;

				while (HEAD_SENSOR != Bit_RESET){
					//linearGuideStep (MOTOR_BACKWARD, 50, 1000);
					runStepper (MOTOR_BACKWARD, 1000);
					delay_ms(100);
				}
				MOTOR_COIL_OFF;
 			// feedback to PC on machine status
 			printf ("$LG,%.2f,%d,%d\r", getPosition(), HEAD_SENSOR, TAIL_SENSOR);				
		}
		// perform go to Right Edge
		else if (serialMsg.command == COMMAND_RIGHT){
			MOTOR_COIL_ON;

			while (TAIL_SENSOR != Bit_RESET){
				//linearGuideStep (MOTOR_FORWARD, 50, 1000);
				runStepper (MOTOR_FORWARD, 1000);
				delay_ms(100);
			}
			MOTOR_COIL_OFF;
 			// feedback to PC on machine status
 			printf ("$LG,%.2f,%d,%d\r", getPosition(), HEAD_SENSOR, TAIL_SENSOR);				
		}
 		// perform STEP to right action here
 		else if (serialMsg.command == COMMAND_STRG){
 			linearGuideStep (MOTOR_FORWARD, serialMsg.speed, serialMsg.distance);		// move motor forward, ?? mm/s,  ?? mm distance
 			// feedback to PC on machine status
 			printf ("$LG,%.2f,%d,%d\r", getPosition(), HEAD_SENSOR, TAIL_SENSOR);
 		}
 		// perform STEP to left action here
 		else if (serialMsg.command == COMMAND_STLF){
 			linearGuideStep (MOTOR_BACKWARD, serialMsg.speed, serialMsg.distance);		// move motor forward, ?? mm/s, ?? mm distance
 			// feedback to PC on machine status
 			printf ("$LG,%.2f,%d,%d\r", getPosition(), HEAD_SENSOR, TAIL_SENSOR);
 		}
		// forform STOP to motion
		else if (serialMsg.command == COMMAND_STATUS){
 			// feedback to PC on machine status
 			printf ("$LG,%.2f,%d,%d\r", getPosition(), HEAD_SENSOR, TAIL_SENSOR);
		}
	}

	while (1);				// program shouldn't reach here
}

/**
  * @brief  GPIO Configuration. Configure 4 LEDs as output, head and tail sensor, motor coil on/off pin
  * @param  None
  * @retval None
  */
void GPIO_Configuration(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	/* GPIOD Periph clock enable */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	
	/* Configure PD12, 13, 14, 15, in output pushpull mode */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	
	/* CONFIGURE HEAD AND TAIL SENSOR HERE */
	/* GPIOE, B Periph clock enable */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	
	//HEAD_SENSOR = (GPIOB, GPIO_Pin_11)
	//TAIL_SENSOR = (GPIOE, GPIO_Pin_5)
	/* Configure PE5, input pull up */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	
	/* Configure PB11, input pull up */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	
	/* CONFIGURE STEPPER MOTOR CONTROL SIGNAL HERE, COIL ON/OFF */
	/* Configure 12, in output open drain mode */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}

/**
  * @brief  TIM Configuration.
  * @param  None
  * @retval None
  */
void TIM5_Configuration(void)
{
	/*
	In this example TIM3 input clock (TIM3CLK) is set to 2 * APB1 clock (PCLK1), 
	since APB1 prescaler is different from 1.   
		TIM3CLK = 2 * PCLK1  
		PCLK1 = HCLK / 4 
		=> TIM3CLK = HCLK / 2 = SystemCoreClock /2
				
	To get TIM3 counter clock at 21 MHz, the prescaler is computed as follows:
		 Prescaler = (TIM3CLK / TIM3 counter clock) - 1
		 Prescaler = ((SystemCoreClock /2) /1 MHz) - 1
																						
	CC1 update rate = TIM3 counter clock / 20000 = 50.00 Hz
	*/
	uint16_t PrescalerValue = 0;
	
	NVIC_InitTypeDef NVIC_InitStructure;
	
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	
	/* TIM3 clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);
	
	/* Enable the TIM3 gloabal Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = TIM5_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
	
	/* Compute the prescaler value */
  PrescalerValue = (uint16_t) ((SystemCoreClock / 2) / 1000000) - 1;

  /* Time base configuration */
  TIM_TimeBaseStructure.TIM_Period = 65535;
  TIM_TimeBaseStructure.TIM_Prescaler = 0;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

  TIM_TimeBaseInit(TIM5, &TIM_TimeBaseStructure);

  /* Prescaler configuration */
  TIM_PrescalerConfig(TIM5, PrescalerValue, TIM_PSCReloadMode_Immediate);

  /* Output Compare Timing Mode configuration: Channel1 */
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Timing;
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_Pulse = 20000;
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

  TIM_OC1Init(TIM5, &TIM_OCInitStructure);

  TIM_OC1PreloadConfig(TIM5, TIM_OCPreload_Disable);
	
	/* TIM Interrupts enable */
  TIM_ITConfig(TIM5, TIM_IT_CC1, ENABLE);

  /* TIM5 Disable counter */
  TIM_Cmd(TIM5, ENABLE);
}

/**
* @brief  Inserts a estimated delay time in uS. CPU FREQ = 168MHz. This is not an accurate delay
					Do not use this function for high accuracy application
* @param  nus: specifies the delay time in micro second.
* @retval None
*/
void Delaynus(vu32 nus)
{
    uint8_t nCount;
    while (nus--)
        for (nCount = 6; nCount != 0; nCount--);
}

/**
* @brief  Inserts a estimated delay time in mS. CPU FREQ = 168MHz. This is not an accurate delay
					Do not use this function for high accuracy application
* @param  miliseconds: specifies the delay time in mili second.
* @retval None
*/
void delay_ms(uint32_t miliseconds)
{
	uint32_t i;

	// one loop = 1ms. so loop for x loop to hit the delay.
	for(i=0; i < miliseconds; i++)
		Delaynus(1000);
}

/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
PUTCHAR_PROTOTYPE
{
  USART_SendData(USART2, (uint8_t) ch);
  while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);

  return ch;
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */ 

/**
  * @}
  */ 

/******************* (C) COPYRIGHT CRSST 2013s *****END OF FILE*****************/
