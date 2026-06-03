// system1-spi-nonblocking.c
// replacing UART with SPI for two AVRs communication (Exp 7.3).
// this is the master
#include "ee470avr.h"
volatile uint8_t spi_busy = 0; 
ISR(TIMER0_COMPA_vect) {
if (!spi_busy) {
uint8_t x = 0;

if (!(PINC & (1 << PC0))) x |= (1 << 0);
if (!(PINC & (1 << PC1))) x |= (1 << 1);
PORTB &= ~(1 << PB2); 
SPDR = x; 
spi_busy = 1;
}
}
ISR(SPI_STC_vect) {
uint8_t received = SPDR; 
if (received & (1 << 0)) PORTB |= (1 << PB0); else PORTB &= ~(1 << PB0);
if (received & (1 << 1)) PORTB |= (1 << PB1); else PORTB &= ~(1 << PB1);
PORTB |= (1 << PB2); 
spi_busy = 0; 
}
int main(void) {
DDRB |= (1 << PB0) | (1 << PB1);
DDRC &= ~((1 << PC0) | (1 << PC1));
PORTC |= (1 << PC0) | (1 << PC1);
DDRB |= (1 << PB3) | (1 << PB5) | (1 << PB2);
DDRB &= ~(1 << PB4);
PORTB |= (1 << PB2);
SPCR = (1 << SPIE) | (1 << SPE) | (1 << MSTR);
// Timer0 CTC for ~10ms period at 16MHz (prescaler 1024, OCR0A=155 -> 15625Hz / 156 = 100Hz)
TCCR0A = (1 << WGM01);
TCCR0B = (1 << CS02) | (1 << CS01) | (1 << CS00);
OCR0A = 155;
TIMSK0 = (1 << OCIE0A);
sei(); 
while (1) {

}
return 0;
}
