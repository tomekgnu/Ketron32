/*
 * handlers.c
 *
 * Created: 16.07.2019 11:37:12
 *  Author: Tomek
 */ 
#include <avr/io.h>
#include "uart.h"
#include "buffer.h"
#include "global.h"
#include "avr/io.h"
#include "midi.h"
#include "avrlibtypes.h"
#include "ff.h"
#include "a2d.h"

extern cBuffer uartRxBuffer;
extern volatile unsigned char adcValue;

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