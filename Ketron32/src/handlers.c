/*
 * handlers.c
 *
 * Created: 16.07.2019 11:37:12
 *  Author: Tomek
 */ 
#include <avr/io.h>
#include <stdint.h>
#include "uart.h"
#include "buffer.h"
#include "global.h"
#include "avr/io.h"
#include "midi.h"
#include "avrlibtypes.h"

#include "ff.h"
#include "a2d.h"
#include "timer.h"

extern cBuffer uartRxBuffer;
extern volatile unsigned char adcValue;

static volatile uint32_t _micros = 0;
static volatile uint32_t _millis = 0;		// counter for microseconds
static volatile uint32_t _seconds = 0;

void rx_handler(unsigned char byte){
	
	if(byte == MIDI_SENSE)
		return;	
	if( !bufferAddToEnd(&uartRxBuffer, byte) )
		uartFlushReceiveBuffer();	
	
}

DWORD get_fattime(){
	return 0;
}

void configTimers()
{
	 // Set up timer 1 to overflow once a millisecond
	 // WGM = 4, OCRA = 2000
	 TCCR1B |= (1 << WGM12) | ( 1 << CS11); // WGM=4, Clear on Timer Match | Set prescaler to 1/8th
	 OCR1A = 2000; // Overflow once per 1000 microseconds
	 // Enable interrupt
	 TIMSK |= (1 << OCIE1A);
	
	// timer2: interrupt every 10ms	
	OCR2 = 156;
	TCCR2 |= (1 << WGM21); //Wlaczenie trybu CTC (Clear on compare match)
	TIMSK |= (1 << OCIE2); //Wystapienie przerwania gdy zliczy do podanej wartosci
	TCCR2 |= (1 << CS22) | (1 << CS21) | (1 << CS20); // presc. 1024
	
}

void toggleLed(){
	PORTA ^= (1 << 0);
}

void setInputs(){
	// adc
	cbi(PORTA,ADC_CH_ADC0);
	cbi(DDRA,ADC_CH_ADC0);
	// switches: input, pull-ups
	cbi(DDRB,SW0);
	cbi(DDRB,SW1);
	cbi(DDRB,SW2);
	cbi(DDRB,SW3);
	sbi(PORTB,SW0);
	sbi(PORTB,SW1);
	sbi(PORTB,SW2);
	sbi(PORTB,SW3);
	
}

INPUT readInputs(unsigned char *pot,unsigned char but[]){
	static unsigned char adc_old = 0;
	static unsigned char but_old[4] = {0,0,0,0};	
	unsigned char i;
	
	*pot = a2dConvert8bit(ADC_CH_ADC0);
	but[SW0] = !(PINB & (1 << SW0));
	but[SW1] = !(PINB & (1 << SW1));
	but[SW2] = !(PINB & (1 << SW2));
	but[SW3] = !(PINB & (1 << SW3));
	
	if((*pot) != adc_old){
		adc_old = *pot;
		return POT;
	}
	
	for(i = 0; i < 4; i++){
		if(but[i] != but_old[i]){
			but_old[i] = but[i];
			if(but[i] == 1)
				return i;			
		}
		
	
	}
	
	return NONE;
	
}

void resetTime(){
	uint8_t oldSREG = SREG;
	cli();
	TCNT1 = 0;
	SREG = oldSREG;
}

uint32_t getMicros(){	
	uint8_t oldSREG = SREG, t;
	cli();
	t = TCNT1;
	SREG = oldSREG;
	
	
	return 1000 * _millis + (t >> 1);
}

uint32_t getMillis(){	
	
	return _millis;	
	
}

void addMillis(){
	
	unsigned char statusReg = SREG; // Save the status register
	++_millis; // Add one millisecond, cause we interrupt once per millisecond
	_seconds += (0 == (_millis % 1000))?1:0; // Increase seconds each thousand milliseconds
	SREG = statusReg; // Restore the status register
}