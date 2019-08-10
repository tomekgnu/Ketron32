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

volatile unsigned long timer0_overflow_count = 0;
volatile unsigned long timer0_millis = 0;
static unsigned char timer0_fract = 0;

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
	 
	 #if defined(TCCR0A) && defined(WGM01)
	 sbi(TCCR0A, WGM01);
	 sbi(TCCR0A, WGM00);
	 #endif

	 // set timer 0 prescale factor to 64
	 #if defined(__AVR_ATmega128__)
	 // CPU specific: different values for the ATmega128
	 sbi(TCCR0, CS02);
	 #elif defined(TCCR0) && defined(CS01) && defined(CS00)
	 // this combination is for the standard atmega8
	 sbi(TCCR0, CS01);
	 sbi(TCCR0, CS00);
	 #elif defined(TCCR0B) && defined(CS01) && defined(CS00)
	 // this combination is for the standard 168/328/1280/2560
	 sbi(TCCR0B, CS01);
	 sbi(TCCR0B, CS00);
	 #elif defined(TCCR0A) && defined(CS01) && defined(CS00)
	 // this combination is for the __AVR_ATmega645__ series
	 sbi(TCCR0A, CS01);
	 sbi(TCCR0A, CS00);
	 #else
	 #error Timer 0 prescale factor 64 not set correctly
	 #endif

	 // enable timer 0 overflow interrupt
	 #if defined(TIMSK) && defined(TOIE0)
	 sbi(TIMSK, TOIE0);
	 #elif defined(TIMSK0) && defined(TOIE0)
	 sbi(TIMSK0, TOIE0);
	 #else
	 #error	Timer 0 overflow interrupt not set correctly
	 #endif 
	 
	
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

void addMillis()
{
	// copy these to local variables so they can be stored in registers
	// (volatile variables must be read from memory on every access)
	unsigned long m = timer0_millis;
	unsigned char f = timer0_fract;

	m += MILLIS_INC;
	f += FRACT_INC;
	if (f >= FRACT_MAX) {
		f -= FRACT_MAX;
		m += 1;
	}

	timer0_fract = f;
	timer0_millis = m;
	timer0_overflow_count++;
}

unsigned long getMillis()
{
	unsigned long m;
	uint8_t oldSREG = SREG;

	// disable interrupts while we read timer0_millis or we might get an
	// inconsistent value (e.g. in the middle of a write to timer0_millis)
	cli();
	m = timer0_millis;
	SREG = oldSREG;

	return m;
}

unsigned long getMicros() {
	unsigned long m;
	uint8_t oldSREG = SREG, t;
	
	cli();
	m = timer0_overflow_count;
	#if defined(TCNT0)
	t = TCNT0;
	#elif defined(TCNT0L)
	t = TCNT0L;
	#else
	#error TIMER 0 not defined
	#endif

	#ifdef TIFR0
	if ((TIFR0 & _BV(TOV0)) && (t < 255))
	m++;
	#else
	if ((TIFR & _BV(TOV0)) && (t < 255))
	m++;
	#endif

	SREG = oldSREG;
	
	return ((m << 8) + t) * (64 / clockCyclesPerMicrosecond());
}