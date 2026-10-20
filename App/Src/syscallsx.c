/*
 * syscallsx.c
 *
 *  Created on: 2026年8月15日
 *      Author: bitstream
 */
#include "main.h"
#include "usart.h"


int __io_putchar(int ch)
{
	HAL_UART_Transmit(&huart1, (const uint8_t *)&ch, 1, 2);
	return ch;
}
