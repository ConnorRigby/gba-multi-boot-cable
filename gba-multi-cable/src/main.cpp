/* Copyright 2010 Stephen Lynch
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

 
/* A (fairly nasty) Arduino / AVR implementation of 'Matt's Serial Multiboot Cable'
 * 
 * Version 1.0 (2010-03-16) Initial version
 *
 * Warning:
 * - I ran the microcontrollers at 5v and the GBA expects 3v signals. The only
 * problems I experienced were the gameboy failing to power down correctly
 * when shutdown before pressing reset on the microcontroller but I can't
 * guarantee that you wont fry it.
 * 
 * Notes:
 * - Based on code from http://www.axio.ms/projects/GBA/
 * - Works most of the time (80 - 90% by my guestimate)
 * - Tested on ATMega168 at 16mhz(xtal) and 8mhz(internal osc) at 5v.
 * - An ATMega is massive overkill (compiled size is < 2k) but i had some in
 * my parts box.
 * - Should be fairly simple to build without the arduino environment as it
 * doesnt use any arduino functions. I was going to move over to a plain
 * avr-gcc build but I think that this will be much more accessible if its
 * compatible with the arduino
 * - If it doesnt work it may be because the upload tool may reset the arduino
 * when it opens the serial port. Add a sleep to the upload tool 
 */

#include <Arduino.h>

//
// Timing specific
//

//default baud rate of gba is 115.2kbps
#define BAUDRATE 115200
#define BITTIME (F_CPU/BAUDRATE) // 138ish clocks per bit @ 16mhz
#define HALFBITTIME BITTIME / 2

 
//
// Pin mapping
//
 
//PORTB 
//Arduino pin 8
//serial in of the avr (connect to SO on the GBA)
#define SI 0
//Arduino Pin 9
//serial out of the avr (connect to SI on the GBA)
#define SO 1
//Arduino Pin 10
//connect to SD on the gba
#define SD 2
//Arduino Pin 11
//connect to SC on the gba
#define SC 3
//Arduino Pin 13
//LED to indicate status 1 for ready to upload, 0 for passthrough
#define LED 5


//
// Low level Serial IO routines
//

//reset the timer to 0 and clear the overflow flag
#define resettimer() TCNT0 = 0; TIFR0 = 1<<OCF0A;

//set / clear 1 bit in a byte
#define setbit(x,y) x = x | (1<<y);
#define clrbit(x,y) x = x & (~(1<<y));

//wait for the overflow flag to ge set then clear it
#define spinonxfer() while((TIFR0 & 1<<OCF0A )==0){}\
					TIFR0 = 1<<OCF0A;

#define sendbit(x,y)		\
	if((x >> y) & 1){		\
		setbit(PORTB, SD);} \
	else{					\
		clrbit(PORTB, SD);}	\
	spinonxfer();

#define recvbit(x,y)		\
	if ((PINB & (1 << SD)) == 1 << SD)	\
		x = x | (1 << y);	\
	spinonxfer();

#define canreadserial()     (UCSR0A & (1<<7)) > 0
#define canwriteserial()    (UCSR0A & (1<<5)) > 0

//#define __DEBUG

//
// Initialisation
//

//disable timer interrupts because we dont want the interrupts disrupting things
void clearallint(void)
{
	TIMSK0 = 0;
	TIMSK1 = 0;
	TIMSK2 = 0;
}

//setup timer 0 with no interrupts to count up to 
//BITTIME then reset to 0
void setuptmr0(void)
{
	//set the max count
	//baud rate of gba is 115.2kbps
	OCR0A =BITTIME;

	//enable timer0 compare match outputs A (OC0A)
	//set the 2 low bits of WGM
	TCCR0A = 1<<WGM00 | 1<<WGM01 | 1<<COM0A0; 
	
	//set prescaler to none
	TCCR0B = 1<<WGM02 | 1<<CS00 ;

	TIMSK0 = 0;
}

void setupserial()
{
	//set the baud rate
	//Could probably be more accurate at low clock rates.
	UBRR0 = (F_CPU/1000000L);
	
	setbit(UCSR0A,U2X0);
	
	//set the mode to 8bits no parity
	UCSR0C = (1<<UCSZ01) | (1<<UCSZ00);

	//enable both input and output
	UCSR0B = (1<<RXEN0) | (1<<TXEN0);
}


void setup()
{
}



//
// Higher level UART io routines
//
void printstr(const char * c);
void printchar(char c);

void printstr(const char * c)
{
	while(*c != 0)
	{
		printchar(*c);
		c++;
	}
}

void printchar(char c)
{
	while(!canwriteserial())
	{
	}
	UDR0 = c;
}

uint8_t getachar()
{
	while(1)
	{
		while(!canreadserial())
		{
		}
		uint8_t rc = UDR0;
		return rc;
	}
}



//
// GBA serial IO routines
//

void txdata(uint8_t high, uint8_t low)
{
	//set SD  to input
	clrbit(DDRB,SD);
	clrbit(DDRB,SC);
	clrbit(PORTB,SD);
	clrbit(PORTB,SC);
	
	//tell the gba we're sending
	setbit(PORTB,SO);
	
	//wait for SD and SC to be high
	while((PINB & (1 << SD | 1 << SC)) != (1 << SD | 1<< SC))
	{
	}

	resettimer();
	spinonxfer();
	spinonxfer();
	spinonxfer();
	spinonxfer();
	spinonxfer();
	spinonxfer();
	spinonxfer();
	spinonxfer();

	//configure SD and SC as output, and make them low
	DDRB = DDRB |(1<<SD |1<<SC);
	PORTB = PORTB & (~(1<<SD | 1<<SC));

	//wait 1 bit time (start bit)
	spinonxfer();
	
	//low byte
	sendbit(low,0);
	sendbit(low,1);
	sendbit(low,2);
	sendbit(low,3);
	sendbit(low,4);
	sendbit(low,5);
	sendbit(low,6);
	sendbit(low,7);

	//high byte
	sendbit(high,0);
	sendbit(high,1);
	sendbit(high,2);
	sendbit(high,3);
	sendbit(high,4);
	sendbit(high,5);
	sendbit(high,6);
	sendbit(high,7);

	//stop bit
	setbit(PORTB,SD);
	spinonxfer();
	//im not xfering any more
}

void rxdata(uint8_t *_high, uint8_t *_low)
{
	//reset timer to 0
	uint8_t low = 0;
	uint8_t high = 0;
	
	//set SD to input
	//clrbit(PORTB,SD);
	clrbit(DDRB,SD);

	//tell the gba we're rxing
	clrbit(PORTB,SO);
	//wait for SD to go low
	while((PINB & (1<<SD)) == (1 << SD))
	{
	}
	
	//Reset the counter to a half a bit time
	//reset the timeout flag
	//wait until the middle of the start bit
	resettimer();
	TCNT0 = HALFBITTIME;
	spinonxfer();
	
	//should be in the middle of the start bit
	uint8_t start = 0;
	recvbit(start,0);
	
	//low byte
	recvbit(low,0);
	recvbit(low,1);
	recvbit(low,2);
	recvbit(low,3);
	recvbit(low,4);
	recvbit(low,5);
	recvbit(low,6);
	recvbit(low,7);

	//high byte
	recvbit(high,0);
	recvbit(high,1);
	recvbit(high,2);
	recvbit(high,3);
	recvbit(high,4);
	recvbit(high,5);
	recvbit(high,6);
	recvbit(high,7);
	
	uint8_t stop = 0;
	//stop bit
	recvbit(stop,0);

	//done recieving
	setbit(PORTB,SO);

	if (start == 1)
	{
#ifdef __DEBUG
		setbit(PORTB,LED);
		printstr("error recving start of data");	
#endif
	}
	
	if (stop == 0)
	{
#ifdef __DEBUG
		setbit(PORTB,LED);
		printstr("error recving data");
#endif
	}
	
	//wait for other GBAs ?!?
	spinonxfer();
	spinonxfer();
	spinonxfer();
	spinonxfer();
	spinonxfer();

	setbit(PORTB,SC);

	(*_high) = high;
	(*_low) = low;
}

//
// Program logic
//

void passthroughserial()
{
	//disable serial hardware
	clrbit(UCSR0B,RXEN0);
	clrbit(UCSR0B,TXEN0);
	clrbit(UCSR0B,RXCIE0);
	
	//turn off ready led
	clrbit(PORTB,LED);
	//set SC and SD to output and make them low
	clrbit(DDRB,SD);
	clrbit(DDRB,SC);
	clrbit(PORTB,SD);
	clrbit(PORTB,SC);
	
	//go into passthrough
	clrbit(DDRD,0);
	setbit(DDRD,1);
	clrbit(DDRB,SI);
	setbit(DDRB,SO);
	clrbit(PORTB,SI);//ensure the pull up is disabled
	
	//follow the input and act as pass through
	while(1)
	{
		if(PIND & 1)
		{
			setbit(PORTB,SO);
		}
		else
		{
			clrbit(PORTB,SO);
		}
		
		if(PINB & (1<<SI))
		{
			setbit(PORTD,1);
		}
		else
		{
			clrbit(PORTD,1);
		}
	}
}

//should just have to convert this to main to get it to build as plain c
void loop()
{
	clearallint();
	//set serial out +clock to output others to in
	setbit(DDRB,SO);
	clrbit(DDRB,SC);
	clrbit(DDRB,SI);
	clrbit(DDRB,SD);
	//setup the led as an output and turn it off
	setbit(DDRB,LED);
	clrbit(PORTB,LED);
	setuptmr0();
	resettimer();

	uint8_t datalow;
	uint8_t datahigh;
	bool ishigh = false;
	setupserial();
	
	//we are pretty much ready for input
	setbit(PORTB,LED);
	while(1)
	{
		uint8_t in = getachar();
		
		if(in == 255)
		{
			//start command mode
			while(1)
			{
				uint8_t in2 = getachar();
				if (in2 == 0)
				{
					//just an escaped 255
					break;
				}
				else if(in2 == 1)
				{
					//command to get version string
					printstr("[GBA:MBA RS232 cable v0.1, 15/5/2002 me]");
				}
				else if(in2 == 2)
				{
					//command to go back to regular input
					in = getachar();
					ishigh = false;
					break;
				}
				else if(in2 == 3)
				{
					//do passthrough
					passthroughserial();
				}
			}
		}
		
		if(!ishigh)
		{
			datalow = in; 
			ishigh = true;
		}
		else
		{
			datahigh = in;

			txdata(datahigh, datalow);
			rxdata(&datahigh,&datalow);
			printchar(datalow);
			printchar(datahigh);
			ishigh = false;
		}
	}
}
