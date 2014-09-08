/**
  ******************************************************************************
  * @file    serialcom.h
  * @author  LEE CHEE CHEONG
  * @version V1.0.0
  * @date    10 September 2013
  * @brief   This file contains all the functions prototypes for serial comm 
  *          with pc to enable user to control via PC
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
#ifndef __SERIALCOM_H
#define __SERIALCOM_H

#include "stm32f4xx.h"

#define COMMAND_HOME_LEFT 2
#define COMMAND_HOME_RIGHT 3
#define COMMAND_STLF 4
#define COMMAND_STRG 5
#define COMMAND_STATUS 6
#define COMMAND_UNKNOWN 255

typedef struct {
	uint8_t	command;
	uint16_t speed;
	uint16_t distance;
} SER_MSG, *PSER_MSG;

void Usart2_init(uint32_t baudrate);
void UsartRxCallback (uint8_t data);
uint8_t parseSerialMessage (PSER_MSG message);
uint8_t getNmeaChecksum (char *nmeaStr, uint16_t length);
uint8_t getXorChecksum (char *str, uint16_t length);
int32_t htoi(char *s, uint8_t length);

#endif /*__SERIALCOM_H */

/******************* (C) COPYRIGHT CRSST *****END OF FILE**********************/
