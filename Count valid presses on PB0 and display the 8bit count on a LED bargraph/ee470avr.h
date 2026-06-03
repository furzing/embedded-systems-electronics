// ee470avr.h
#define F_CPU 16000000UL

#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

//
#ifndef getchar
#define getchar	getch
#endif
#ifndef putchar
#define putchar	putch
#endif
// bit manipulation macros 
#define bitset(var,bitno) ((var) |= (1 << (bitno)))		// set a bit
#define bitclr(var,bitno) ((var) &= ~(1 << (bitno)))	// clear a bit
#define bittst(var,bitno) ((var) & (1 << (bitno)))		// test a bit
#define bittgl(var,n) \
(bittst(var,n)==0) ? bitset(var,n) : bitclr(var,n)	// toggle a bit

// copy bit n1 in var1 to bit n2 in var2
#define bit2bit(var1,n1,var2,n2) \
  {(bittst(var1,n1)==0) ? bitclr(var2,n2) : bitset(var2,n2);}
//
void serial_init(unsigned int ubrr) // 103 for 9600bps (F_CPU=16MHz) 
{
// setup Async communication
//	bitset(UCSR0A,U2X0); // double the baud rate
// Set baud rate
UBRR0=ubrr;
UCSR0A=0;
UCSR0B=0b00011000;//enable RX ==> RXD i/p, enable TX ==> TXD o/p
UCSR0C= 6; // 8bit, 1stop, UCSZ0[2:0]=0b011
}

void putch( unsigned char ch )
{
while(bittst(UCSR0A,UDRE0)==0); // Wait for empty transmit buffer
UDR0 = ch; // send character
return;
}

unsigned char getch(void)
{
//wait till data ready (start,data bits,stop)
while(bittst(UCSR0A,RXC0)==0);
return(UDR0); // return received char as the value of the function
}

////////////////////////////
void pcrlf(void)
{
putch(13); // cr
putch(10); // lf
}

//
void putstr(char line[])
{
	unsigned char i=0;
	while(line[i] != '\0')
	{
		putch(line[i]);
		i++;
	}
}

// read an integer number from the keyboard and echo it on the screen
// using getch()/getchar() and putch() functions found in ee470avr.h
// getch()/getchar() return the position of the character in ASCII table
unsigned int get_num(void)
{
   unsigned int num=0;
   unsigned char ch='0';
// Horner method, e.g. decimal 7853=((7*10+8)*10+5)*10+3
   while(ch != '=')     // press '=' to stop reading
   {
      num=num*10+(ch-'0');
      ch=getch();       // ascii byte from kbd
      putch(ch);       // echo the pressed character on the screen
   }
   return num;
}

// map table to convert from hex value to 7seg CC
const unsigned char map[16]=
{0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,
			0x6f,0x77,0x7c,0x58,0x5e,0x79,0x71}; 

void adc_init(void)
{
	ADCSRA= 0x87;	//make ADC enable and select ck/128
	ADMUX= 0x40;	//AVref=Vcc, channel ADC0, right-justified
	bitset(ADCSRA,ADIF);	//reset flag by writing a one!!!
}

void adc_channel(unsigned char ch_num)
{ 
//clear lower 4 bits of ADMUX and select ADC channel according to ch_num
	ADMUX = (ADMUX & 0xF0) | (ch_num & 0x0F); 
}

// take a sample, convert it to digital, and return the digital value
int adc_read()
{
	bitset(ADCSRA,ADSC);	//AD take a sample and Start Conversion
	while(bittst(ADCSRA,ADIF)==0);	//wait for conversion to finish
    bitset(ADCSRA,ADIF);	//reset ADIF flag by writing a '1'
    return ADC;				// read both ADCH and ADCL
}
