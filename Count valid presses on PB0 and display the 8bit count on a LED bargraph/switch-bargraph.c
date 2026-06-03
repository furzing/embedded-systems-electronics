// switch-bargraph.c
#include "ee470avr.h"
int main(void){
    unsigned char x=0;

    bitclr(DDRB,PB0);   // PB0 as i/p (SW)
    DDRD=0xFF;          // PORTD as o/p (Bargraph)
    PORTD=0;            // clear bargraph
    while(1) {
        while(bittst(PINB,PB0) ==0);	// wait for switch ON
        _delay_ms(20);  // wait 20ms for switch debounce
        x++;
        PORTD=x;        //display the count on bargraph on PORTD
        while(bittst(PINB,PB0) !=0);	// wait for switch OFF
        _delay_ms(20);  // wait 20ms for switch debounce
    }
}
