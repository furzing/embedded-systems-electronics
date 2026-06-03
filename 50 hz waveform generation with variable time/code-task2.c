/* gradeA.c */

// Read two potentiometers on PC0 (ADC0) and PC1 (ADC1) every 20ms using Timer1 overflow interrupt.
// Generate two 50Hz PWM waveforms on PB1 (OC1A) and PB2 (OC1B) with ON time 1-2ms based on ADC values.
// Start with 1.5ms ON time.
// Push button on PD2 resets ON time to 1.5ms using external interrupt INT0 on falling edge.
// No blocking delays.

#include "ee470avr.h"

volatile unsigned int on_ticks1 = 3000; 
volatile unsigned int on_ticks2 = 3000;

ISR(TIMER1_OVF_vect) {
    unsigned int adc;

    ADMUX = (1 << REFS0) | 0;  
    ADCSRA |= (1 << ADSC);  
    while (ADCSRA & (1 << ADSC));  
    adc = ADC;
    on_ticks1 = 2000 + (adc * 2000UL / 1023);

    
    ADMUX = (1 << REFS0) | 1;  
    ADCSRA |= (1 << ADSC);  
    while (ADCSRA & (1 << ADSC));  
    adc = ADC;
    on_ticks2 = 2000 + (adc * 2000UL / 1023);

    
    OCR1A = on_ticks1;
    OCR1B = on_ticks2;
}

ISR(INT0_vect) {
    
    on_ticks1 = 3000;
    on_ticks2 = 3000;
    OCR1A = on_ticks1;
    OCR1B = on_ticks2;
}

int main(void) {
    
    DDRC &= ~((1 << PC0) | (1 << PC1));  

    
    ADMUX = (1 << REFS0);  
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

    
    DDRB |= (1 << PB1) | (1 << PB2);  

    
    ICR1 = 39999;  // For 20ms period at 16MHz/8 = 2MHz clock (0.5us tick)
    OCR1A = 3000;  // Initial 1.5ms
    OCR1B = 3000;
    TCCR1A = (1 << COM1A1) | (1 << COM1B1) | (1 << WGM11);  // Noninverting on A and B, WGM11=1
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);  // WGM=14, CS=010 for /8
    TIMSK1 = (1 << TOIE1);  
    
    DDRD &= ~(1 << PD2);  
    PORTD |= (1 << PD2);  
    EICRA = (1 << ISC01);  
    EIMSK = (1 << INT0);  

    sei();  

    while (1) {
        
    }
    return 0;
}
