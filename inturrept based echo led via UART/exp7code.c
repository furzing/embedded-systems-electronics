// echo-uart-interrupt.c: Using PD1=TXD pin and PD0=RXD pin using the USART with interrupts
// If '1' received, echo '*1' and LED ON, else echo char and LED OFF

#include "ee470avr.h"


volatile char tx_second = 0;

ISR(USART_RX_vect) {
    unsigned char c = UDR0;
    unsigned char tx_first;
    if (c == '1') {
        PORTC |= (1 << PC0);  
        tx_first = '*';
        tx_second = '1';
    } else {
        PORTC &= ~(1 << PC0); 
        tx_first = c;
        tx_second = 0;
    }
    // poll briefly
    while (!(UCSR0A & (1 << UDRE0)));
    //  starts sending
    UDR0 = tx_first;
    
    if (tx_second != 0) {
        UCSR0B |= (1 << UDRIE0);
    }
}

ISR(USART_UDRE_vect) {
     
    UDR0 = tx_second;
    
    tx_second = 0;
    // disabling UDRE interrupt
    UCSR0B &= ~(1 << UDRIE0);
}

int main(void) {
    UCSR0A = 0;
    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
    UBRR0 = 51;
    DDRD &= ~(1 << PD0);   
    DDRD |= (1 << PD1);   
    DDRC |= (1 << PC0);   
    PORTC &= ~(1 << PC0); 
    sei();
    while (1) {
        
    }
    return 0;
}
