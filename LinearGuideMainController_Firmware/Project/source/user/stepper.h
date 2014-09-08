/**
  ******************************************************************************
  * @file    stepper.h
  * @author  LEE CHEE CHEONG
  * @version V1.0.0
  * @date    10 September 2013
  * @brief   This file contains all the functions prototypes for the stepper 
  *          motor control
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT CRSST</center></h2>
  ******************************************************************************  
  */ 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STEPPER_H
#define __STEPPER_H

#include "stm32f4xx.h"

#define MOTOR_COIL_ON GPIO_SetBits(GPIOB, GPIO_Pin_12)
#define MOTOR_COIL_OFF GPIO_ResetBits(GPIOB, GPIO_Pin_12)

#define MOTOR_FORWARD 	0
#define MOTOR_BACKWARD	1
#define MOTOR_STOP			2

#define TIM12_PRESCALER 3

#define HEAD_SENSOR GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11)
#define TAIL_SENSOR GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_5)

#define GET_QEI_FEEDBACK (TIM3->CNT + qeiFeedback)

extern int32_t qeiFeedback;
extern uint32_t vextaFeedback;

void PWM_init(void);
void StepperFeedback_init (void);
void QEI1_init (void);

void TIM12_CH1_PWM_OUT (uint16_t frequency, uint8_t dutyCycle);
void TIM12_CH2_PWM_OUT (uint16_t frequency, uint8_t dutyCycle);
void runStepper (uint8_t direction, uint16_t speed);

int32_t getQeiFeedback (void);
void resetQeiFeedback (void);
uint32_t getVextaFeedback (void);
void resetVextaFeedback (void);
float getPosition (void);
void resetCurrentPosition (float position);
void resetPositionToGo (int position);

void linearGuideHomeLeft (void);
void linearGuideHomeRight (void);
void linearGuideStep (uint8_t direction, uint8_t speed, uint16_t stepDistance);

#endif /*__STEPPER_H */

/******************* (C) COPYRIGHT CRSST *****END OF FILE**********************/
