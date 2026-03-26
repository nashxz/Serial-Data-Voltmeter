/* 
 * File:   Lab3.c
 * Author: syedn
 * Student Number : 1773218
 *
 * Created on November 20, 2024, 4:27 PM
 */

#define F_CPU 14745600UL // External xx MHz crystal

#include <stdio.h>
#include "lcd.h"
#include <stdlib.h>
#include <avr/io.h>
#include <util/delay.h>
#include <stdbool.h>
#include "hd44780.h"
#include <avr/fuse.h>

#define VRX PC2
#define VRY PC3
#define SW PB0

// Define fuse settings
FUSES = {
    .low = 0xFF,       
    .high = 0xD9,      
    .extended = 0xFF  
};

// Lock bits
LOCKBITS = 0xFF;  

FILE lcd_str = FDEV_SETUP_STREAM(lcd_putchar, NULL, _FDEV_SETUP_WRITE); 
void uart_init(void) {
    // Set baud rate (UBRR = 23 for 9600 baud with F_CPU = 14.7456 MHz)
    //UBRR0H = (23 >> 8); // Set high byte of baud rate
    UBRR0L = 23;        // Set low byte of baud rate

    // Enable transmitter (TXEN0) and receiver (RXEN0)
    UCSR0B = (1 << TXEN0) | (1 << RXEN0);

    // Set frame format: 8 data bits, no parity, 1 stop bit
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); 
}

void USART_Transmit(unsigned char data) {
    /* Wait for empty transmit buffer */
    while (!(UCSR0A & (1 << UDRE0)));

    /* Put data into buffer, sends the data */
    UDR0 = data;
}

unsigned char USART_Receive(void) {
    while (!(UCSR0A & (1 << RXC0)))
        ;

    return UDR0;
}


void lcd_clear(void) {
    hd44780_outcmd(0x01); 
    hd44780_wait_ready(true);
}

void adc_init() {
    // Set VRX and VRY as inputs
    DDRC &= ~(1 << VRX); // PC2 (VRX) as input
    DDRC &= ~(1 << VRY); // PC3 (VRY) as input

    DIDR0 = (1 << ADC2D) | (1 << ADC3D);
    ADMUX = (1 << REFS0); 
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t read_adc(uint8_t channel) {
    // Select the ADC channel by updating only the MUX[3:0] bits
    ADMUX = (ADMUX & 0xF0) | (channel & 0x0F); 

    // Start the ADC conversion
    ADCSRA |= (1 << ADSC);

    // Wait for the conversion to complete
    while (ADCSRA & (1 << ADSC)); 

    return ADC;           

int main() {
    lcd_init();        // Initialize LCD
    stdout = &lcd_str; // Redirect stdout to LCD stream
    adc_init();        // Initialize ADC
    uart_init();
    
    // Configure switch input with pull-up resistor
    DDRB &= ~(1 << SW);  
    PORTB |= (1 << SW);  

    uint8_t channel = 0; 
    uint8_t last_state = PINB & (1 << SW); 

    while (1) {
        // Read button state
        uint8_t button_state = PINB & (1 << SW);

    
        if (button_state == 0 && last_state != 0) {
            _delay_ms(50); // Debounce delay
            channel = (channel == 0 ? 1 : 0); // Toggle between X (ADC2) and Y (ADC3)
        }
        last_state = button_state; 
        
        lcd_clear();

        // Write to Line 1 (Row 0)
        hd44780_wait_ready(false);
        hd44780_outcmd(0x80); 
        fprintf(&lcd_str, "Y: %2d", read_adc(2)); 
        
        hd44780_wait_ready(false);
        hd44780_outcmd(0x88); 
        fprintf(&lcd_str, "SW: %d", channel);

        hd44780_wait_ready(false);
        hd44780_outcmd(0xC0); 
        fprintf(&lcd_str, "X: %2d", read_adc(3)); 
        
        if (channel == 0) {
            USART_Transmit((read_adc(2) >> 8)); 
            USART_Transmit(read_adc(2) & 0xFF); 
        } else {
            USART_Transmit((read_adc(3) >> 8)); 
            USART_Transmit(read_adc(3) & 0xFF); 
        }


        
        _delay_ms(50);
    }

    return 0;
}
