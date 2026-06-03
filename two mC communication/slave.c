// This is the slave: on SPI transfer (initiated by master), receives master's switch states,
#include "ee470avr.h"
ISR(SPI_STC_vect) {
uint8_t received = SPDR; 
// Set LEDs (bit=1 -> LED on)
if (received & (1 << 0)) PORTB |= (1 << PB0); else PORTB &= ~(1 << PB0);
if (received & (1 << 1)) PORTB |= (1 << PB1); else PORTB &= ~(1 << PB1);
uint8_t x = 0;

if (!(PINC & (1 << PC0))) x |= (1 << 0);
if (!(PINC & (1 << PC1))) x |= (1 << 1);
SPDR = x; 
}
int main(void) {
DDRB |= (1 << PB0) | (1 << PB1);
DDRC &= ~((1 << PC0) | (1 << PC1));
PORTC |= (1 << PC0) | (1 << PC1);
DDRB &= ~((1 << PB3) | (1 << PB5) | (1 << PB2));
DDRB |= (1 << PB4);
SPCR = (1 << SPIE) | (1 << SPE);
uint8_t x = 0;
if (!(PINC & (1 << PC0))) x |= (1 << 0);
if (!(PINC & (1 << PC1))) x |= (1 << 1);
SPDR = x;
sei();
while (1) {

}
return 0;
}
