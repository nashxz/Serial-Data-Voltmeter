/* 
 * File:   defines.h
 * Author: syedn
 *
 * Created on November 20, 2024, 4:20 PM
 */

/* CPU frequency */
#define F_CPU 14745600UL 

/* UART baud rate */
#define UART_BAUD  9600

/* HD44780 LCD port connections */
#define HD44780_RS C, 0
#define HD44780_RW C, 1
#define HD44780_E  D, 2
/* The data bits have to be not only in ascending order but also consecutive. */
#define HD44780_D4 D, 4

/* Whether to read the busy flag, or fall back to
   worst-time delays. */
#define USE_BUSY_BIT 1