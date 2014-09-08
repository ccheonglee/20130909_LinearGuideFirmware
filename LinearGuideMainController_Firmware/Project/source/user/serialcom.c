#include <string.h>
#include <stdlib.h>
#include "serialcom.h"

static char usartRawMessage[200] = {0};

static char *my_strtok(char *s1, char *s2);

/**
  * @brief  USART2 peripheral initialization
  * @param  baudrate --> the baudrate at which the USART is supposed to operate
  * @retval void
  */
void Usart2_init(uint32_t baudrate)
{
	USART_InitTypeDef USART_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	
	NVIC_InitTypeDef NVIC_InitStructure;
	
	/* Enable the USARTx Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	/* Enable GPIOA clock */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	
	/* GPIOA Configuration: PA2, TX */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	/* GPIOA Configuration: PA3, RX */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	/* Connect PA2 to COMM_PORT_Tx*/
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);
	
	/* Connect PA3 to COMM_PORT_Rx*/
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2);

	/* Enable USART2 clock */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	USART_InitStructure.USART_BaudRate = baudrate;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No ;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	/* Configure the USARTx */
	USART_Init(USART2, &USART_InitStructure);
	
	/* Enable the USART2 Receive interrupt: this interrupt is generated when the 
     USART2 receive data register is not empty */
  USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
	
	/* Enable the USARTx */
	USART_Cmd(USART2, ENABLE);
}

/**
  * @brief  this function will be called every receive of a character of usart1.
  * @param  Data receive via USART2 interface
  * @retval None
  */
void UsartRxCallback (uint8_t data)
{			
	uint8_t dummyData;
	static uint16_t counter = 1;
	// always receive from counter = 1, counter = 0 is reserved for busy flag indication
	// if buffer is not free, just read the data, and return from this function
	if (usartRawMessage[0] == 1){
		dummyData = data;
		return;
	}
	
	// if current counter = 1, always wait for first character to come in, which is '$'
	if (counter == 1){
		if (data == '$'){
			usartRawMessage[counter++] = data;
		}
	}
	// if counter != 1, mean receiving in progress, keep receive data
	else{
		usartRawMessage[counter++] = data;
		if (data == '\n'){
			usartRawMessage[0] = 1;
			counter = 1;
		}
	}
}

/**
  * @brief  This function parse the raw usart message into details information.
  * @param  message:  pointer to serialMessage structure. where it holds the extracted data
											from raw usart message.
  * @retval 0 or 1. 0 = success, 1 = nothing has been received, or data parsing error.
  */
uint8_t parseSerialMessage (PSER_MSG message)
{
	char *tempPtr, *csPtr;
	uint8_t checksum, rcvChecksum;
	
	// if there is no message, return 1.
	if (usartRawMessage[0] == 0)
		return 1;
	
	// if there is message received, check the message integrity using NMEA Checksum
	// invalid checksum return 2
	csPtr = strchr(&usartRawMessage[1], '*');				//find checksum of this string
	checksum = htoi (csPtr+1, 2);
	rcvChecksum = getNmeaChecksum (&usartRawMessage[1], strlen(&usartRawMessage[1]));
	if (checksum != rcvChecksum){
		// clear the usart buffer for next receive
		memset (usartRawMessage, 0, 200);
		return 2;
	}
	
	if(strncmp(&usartRawMessage[1], "$HOMEL", 6) == 0)					// it is home LEFT command
		message->command = COMMAND_HOME_LEFT;
	
	else if(strncmp(&usartRawMessage[1], "$HOMER", 6) == 0)			// it is home RIGHT command
		message->command = COMMAND_HOME_RIGHT;
	else if(strncmp(&usartRawMessage[1], "$STLF", 5) == 0){		// it is home command
		my_strtok (&usartRawMessage[1], ","); 									//get $STEP
		message->command = COMMAND_STLF;
		
		tempPtr = my_strtok (0, ",");					// pointer to speed string
		message->speed = atoi(tempPtr);				// convert string to integer
		
		tempPtr = my_strtok (0, "\r");					// pointer to speed string
		message->distance = atoi(tempPtr);				// convert string to integer
	}
	
	else if(strncmp(&usartRawMessage[1], "$STRG", 5) == 0){		// it is home command
		my_strtok (&usartRawMessage[1], ","); 									//get $STEP
		message->command = COMMAND_STRG;
		
		tempPtr = my_strtok (0, ",");					// pointer to speed string
		message->speed = atoi(tempPtr);				// convert string to integer
		
		tempPtr = my_strtok (0, "\r");					// pointer to speed string
		message->distance = atoi(tempPtr);				// convert string to integer
	}
	
	else if (strncmp(&usartRawMessage[1], "$STAT", 5) == 0){
		message->command = COMMAND_STATUS;
	}
	
	else{	
		// clear the usart buffer for next receive
		message->command = COMMAND_UNKNOWN;
	}
	
	// clear the usart buffer for next receive
	memset (usartRawMessage, 0, 200);
	return 0;
}

/*****************************************************************************************
 unsigned char *my_strtok(*ptr, unsigned char term)
 param : ptr to a string, term to token, with 0 as first argument, starts searching from the saved pointer.
 return pointer of the string token. null if end of string. and term if there is no char btw 2 delims
*****************************************************************************************/
static char *my_strtok(char *s1, char *s2)
{
	char *beg, *end;
	static char *save;
	
	beg = (s1)? s1: save;
	if (*beg == '\0'){
		*save = ' ';
		return(0);
	}
	end = strpbrk(beg, s2);
	if (*end != '\0'){
		*end = '\0';
		end++;
	}
	save = end;
	if (end-beg == 1)
		return s2;
	return(beg);
}

uint8_t getNmeaChecksum (char *nmeaStr, uint16_t length)
{
	uint8_t checksum = 0;
	uint16_t i;
	length = length - 5;		//last 4 char not count in nmea checksum, *cc\r\n"
	for(i=1; i<length; i++)		//1st character not count in nmea checksum '$'
		checksum ^= nmeaStr[i];
	return checksum;
}

uint8_t getXorChecksum (char *str, uint16_t length)
{
	uint8_t checksum = 0;
	uint16_t i;
	for(i=0; i<length; i++)		//1st character not count in nmea checksum '$'
		checksum ^= str[i];
	return checksum;
}

int32_t htoi(char *s, uint8_t length)
{
	int32_t checksum = 0, t;
	uint8_t i;
	for(i=0; i<length; i++){
	 	if ((s[i] >= 'A') && (s[i] <= 'F'))
            t = s[i] - 'A' + 10;
        else if ((s[i] >= 'a') && (s[i] <= 'f'))
            t = s[i] - 'a' + 10;
        else if ((s[i] >= '0') && (s[i] <= '9'))
            t = s[i] - '0';
        else
            return -1;					//not belongs to hex, hex must better '0'-'9', 'a'-'f', 'A'-'F'
		checksum = (checksum<<4) + t;
	}
	return checksum;
}
