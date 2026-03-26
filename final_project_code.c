#define F_CPU 14745600UL // External 14.7456 MHz crystal

#include <stdio.h>
#include "lcd.h"
#include <stdlib.h>
#include <avr/io.h>
#include <util/delay.h>
#include <stdbool.h>
#include "hd44780.h"
#include <avr/interrupt.h>

uint16_t sampling_rate = 1;        // Initial sampling rate in Hz
uint16_t saved_voltage[3][2] = {0}; // Saved voltage for channels: [0]=whole, [1]=decimal
#define DEBOUNCE_DELAY_MS 100       
uint16_t lcd_update_counter = 10; 

FILE lcd_str = FDEV_SETUP_STREAM(lcd_putchar, NULL, _FDEV_SETUP_WRITE); // LCD stream

// UART Initialization
void uart_init(void) {
    UBRR0L = (F_CPU / (16UL * 76800)) - 1; // Set baud rate for 76800
    UCSR0B = (1 << TXEN0) | (1 << RXEN0);  // Enable transmitter and receiver
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // Set 8 data bits, no parity, 1 stop bit
}

// UART Transmit a single character
void uart_transmit(char data) {
    while (!(UCSR0A & (1 << UDRE0))); 
    UDR0 = data; 
}

// UART Transmit a string
void uart_transmit_string(const char* str) {
    while (*str) {
        uart_transmit(*str++);
    }
}

// Send voltage data via UART
void send_voltage_data(void) {
    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%u.%01u,%u.%01u,%u.%01u\r\n",
             saved_voltage[0][0], saved_voltage[0][1],
             saved_voltage[1][0], saved_voltage[1][1],
             saved_voltage[2][0], saved_voltage[2][1]);
    uart_transmit_string(buffer);
}

// Clear LCD screen
void lcd_clear(void) {
    hd44780_outcmd(0x01); // Clear display
    hd44780_wait_ready(true); // Wait until the display is ready
}

// Custom delay function for variable delays
void custom_delay_ms(uint16_t delay_ms) {
    while (delay_ms--) {
        _delay_ms(1); // Delay by 1 ms in a loop
    }
}

// ADC Initialization
void adc_init() {
    DDRC &= ~((1 << PC2) | (1 << PC3) | (1 << PC4)); // Set PC2, PC3, PC4 as inputs
    DIDR0 = (1 << ADC2D) | (1 << ADC3D) | (1 << ADC4D); 
    ADMUX = (1 << REFS0); 
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); 
}

// Read ADC value from a specific channel
uint16_t read_adc(uint8_t channel) {
    ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);
        ADCSRA |= (1 << ADSC); 
    while (ADCSRA & (1 << ADSC)); 
    return ADC; // Return ADC value
}

// Adjust sampling rate
void adjust_rate(bool increase) {
   if (increase) {
        if (sampling_rate == 0) {
            sampling_rate = 1;
        } else if (sampling_rate < 10) {
            sampling_rate++;
        } else if (sampling_rate < 100) {
            sampling_rate += 10;
        } else if (sampling_rate < 1000) {
            sampling_rate += 100;
        }
        if (sampling_rate > 1000) sampling_rate = 1000; // Cap at 1000 Hz
    } else {
        if (sampling_rate <= 10 && sampling_rate > 1) {
            sampling_rate--;
        } else if (sampling_rate <= 100 && sampling_rate > 1) {
            sampling_rate -= 10;
        } else if (sampling_rate <= 1000 && sampling_rate > 1) {
            sampling_rate -= 100;
        }
        if (sampling_rate == 1) {
            sampling_rate = 0; // Pause sampling
        }
    }
}

// Interrupt Service Routine for rate adjustment
ISR(PCINT0_vect) {
    if (!(PINB & (1 << PB1))) { // Check if PB1 is pressed
        _delay_ms(DEBOUNCE_DELAY_MS); 
        if (!(PINB & (1 << PB1))){
            adjust_rate(true);
            lcd_update_counter = 10;
        }
    }
    if (!(PINB & (1 << PB2))) { // Check if PB2 is pressed
        _delay_ms(DEBOUNCE_DELAY_MS); 
        if (!(PINB & (1 << PB2))){
            adjust_rate(false);
        lcd_update_counter = 10;
        }
    }
}

int main() {
    lcd_init(); // Initialize LCD
    stdout = &lcd_str; 
    adc_init(); // Initialize ADC
    uart_init(); // Initialize UART

    DDRB &= ~((1 << PB1) | (1 << PB2));
    PORTB |= (1 << PB1) | (1 << PB2); 

    // Enable Pin Change Interrupts for PB1 and PB2
    PCICR |= (1 << PCIE0);  
    PCMSK0 |= (1 << PCINT1) | (1 << PCINT2);

    sei(); // Enable global interrupts



    while (1) {
        if (sampling_rate == 0) { // Pause sampling
            lcd_clear();
            fprintf(&lcd_str, "%u.%01u %u.%01u %u.%01u",
                    saved_voltage[0][0], saved_voltage[0][1],
                    saved_voltage[1][0], saved_voltage[1][1],
                    saved_voltage[2][0], saved_voltage[2][1]);
            fprintf(&lcd_str, "\x1b\xc0Paused");
            custom_delay_ms(200);
            continue;
        }

        // Read ADC values for all channels
        for (uint8_t channel = 0; channel < 3; channel++) {
            uint16_t adc_value = read_adc(channel + 2); // Channels: PC2, PC3, PC4
            saved_voltage[channel][0] = (adc_value * 5) / 1023;     
            saved_voltage[channel][1] = ((adc_value * 50) / 1023) % 10; 
        }

        // Update LCD every 3 sampling cycles
        lcd_update_counter++;
        if (lcd_update_counter >= 10) {
            lcd_update_counter = 0; // Reset counter for next update cycle

            // Clear and update LCD
            lcd_clear();
            fprintf(&lcd_str, "%u.%01u %u.%01u %u.%01u",
                    saved_voltage[0][0], saved_voltage[0][1],
                    saved_voltage[1][0], saved_voltage[1][1],
                    saved_voltage[2][0], saved_voltage[2][1]);
            fprintf(&lcd_str, "\x1b\xc0Rate: %u Hz", sampling_rate);
        }


        send_voltage_data();

    
        custom_delay_ms(1000 / sampling_rate);
    }

    return 0;
}

