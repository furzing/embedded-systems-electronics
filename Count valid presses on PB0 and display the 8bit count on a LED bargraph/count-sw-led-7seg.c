/* count-sw-led-7seg.c
   Count switch pressings and directly display on 7-segment 
   display-Common cathode connected to PORTD. 
   If the count reaches 15, the next count should be 0 and repeats.
   Copy SW on PB0 to LED on PB5.
   */
#include "ee470avr.h"
unsigned char b;
// unsigned char map[16]={0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,
//   0x07,0x7f,0x6f,0x77,0x7c,0x58,0x5e,0x79,0x71};//0..0xF
int main() {
    bitclr(DDRB,PB0);   // SW
    bitset(DDRB,PB5);   // LED
    DDRD=0xFF;          // pins 0 to 6 of PORTD are o/p for 7seg CC
    b=0; PORTD=map[b];  // put 0 on 7seg CC on port D
    while(1) {
        while(bittst(PINB,PB0)==0) {
            bitclr(PORTB,PB5);// wait until PB0==1
        } 

        b++;
        if (b==16) b=0;
        PORTD=map[b];   // write to 7-segment display
        _delay_ms(50);  // wait for 50 msec

        while (bittst(PINB,PB0)!=0) {
            bitset(PORTB,PB5);
        } // wait until PB0==0
    }
}
