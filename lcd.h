/* 
 * File:   lcd.h
 * Author: syedn
 *
 * Created on November 20, 2024, 4:22 PM
 */

/*
 * Initialize LCD controller.  Performs a software reset.
 */
void	lcd_init(void);

/*
 * Send one character to the LCD.
 */
int	lcd_putchar(char c, FILE *stream);