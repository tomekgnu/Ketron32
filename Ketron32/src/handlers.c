/*
 * handlers.c
 *
 * Created: 16.07.2019 11:37:12
 *  Author: Tomek
 */ 
#include <avr/io.h>
#include <stdint.h>
#include <string.h>
#include <util/delay.h>
#include "uart.h"
#include "buffer.h"
#include "global.h"
#include "handlers.h"
#include "avr/io.h"
#include "midi.h"
#include "avrlibtypes.h"
#include "lcd.h"
#include "ff.h"
#include "a2d.h"
#include "timer.h"

extern cBuffer uartRxBuffer;
extern volatile unsigned char adcValue;
char indstr[17];
   	
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
	
	// joystick: input, pull-ups	
	cbi(JOY_N_DDR,JOY_N);
	cbi(JOY_E_DDR,JOY_E);
	cbi(JOY_S_DDR,JOY_S);
	cbi(JOY_W_DDR,JOY_W);
	cbi(JOY_T_DDR,JOY_T);
	
	sbi(JOY_N_PORT,JOY_N);
	sbi(JOY_E_PORT,JOY_E);
	sbi(JOY_S_PORT,JOY_S);
	sbi(JOY_W_PORT,JOY_E);
	sbi(JOY_T_PORT,JOY_T);
	
}

INPUT readInputs(){
	static unsigned char adc_new = 0;
	static unsigned char but_new[9];
	static unsigned char adc_old = 0;
	static unsigned char but_old[9] = {0,0,0,0,0,0,0,0,0};
	
	unsigned char i;
	
	adc_new = a2dConvert8bit(ADC_CH_ADC0);
	// read buttons
	but_new[BUTTON0] = !(PINB & (1 << SW0));
	but_new[BUTTON1] = !(PINB & (1 << SW1));
	but_new[BUTTON2] = !(PINB & (1 << SW2));
	but_new[BUTTON3] = !(PINB & (1 << SW3));
	
	// read joystick
	but_new[JOY_UP] = !(JOY_N_PIN & (1 << JOY_N));
	but_new[JOY_RIGHT] = !(JOY_E_PIN & (1 << JOY_E));
	but_new[JOY_DOWN] = !(JOY_S_PIN & (1 << JOY_S));
	but_new[JOY_LEFT] = !(JOY_W_PIN & (1 << JOY_W));
	but_new[JOY_PRESS] = !(JOY_T_PIN & (1 << JOY_T));
	
	if((adc_new) != adc_old){
		adc_old = adc_new;
		return POT;
	}
	
	for(i = 0; i < 9; i++){
		if(but_new[i] != but_old[i]){
			but_old[i] = but_new[i];
			if(but_new[i] == 1)
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

void handleFileList(unsigned char currentMode,unsigned char currentAction,unsigned char index,unsigned char number,char (*list)[9]){
	unsigned char tmp = index - (index % 2);
	char *ch[2] = {"*"," "};
		
	if(currentMode == BUTTON2){		
		lcdGotoXY(0,0);
		if(number == 0){
			lcdPrintData("No files",8);
			return;
		}
		
		lcdGotoXY(0,0);
		lcdPrintData(list[tmp],strlen(list[tmp]));
		lcdGotoXY(9,0);
		lcdPrintData(ch[index % 2],1);
		if(index < number){
			lcdGotoXY(0,1);
			lcdPrintData(list[tmp + 1],strlen(list[tmp + 1]));
			lcdGotoXY(9,1);
			lcdPrintData(ch[(index + 1) % 2],1);
		}
	}
}

unsigned char setMidiFile(char *name){
	
	struct MD_MIDIFile mf;	
	
	memset(&mf,0,sizeof(struct MD_MIDIFile));
	initialise(&mf);
	setMidiHandler(&mf,midiFun);
	setSysexHandler(&mf,sysexFun);
	setMetaHandler(&mf,metaFun);
	setFilename(&mf,name);	
	
	if(loadMIDIFile(&mf) != -1){
		lcdPrintData("Open failed",11);
		_delay_ms(500);
		return 1;
	}	
	
	mf._paused = FALSE;
	mf._looping = FALSE;	
	
	while(!isEOF(&mf)){
		getNextEvent(&mf);		
	}
	
	closeMIDIFile(&mf);
	lcdPrintData("Finished",8);
	// << MIDI
	
	return 0;
}

unsigned char midiRecord(int action){
	itoa((int)action,indstr,10);
	lcdPrintData("Midi rec ",9);
	lcdGotoXY(0,1);
	lcdPrintData(indstr,strlen(indstr));
	return 0;
}

unsigned char soundSelect(int action){
	itoa((int)action,indstr,10);
	lcdPrintData("Sound sel",9);
	lcdGotoXY(0,1);
	lcdPrintData(indstr,strlen(indstr));
	return 0;
	
}

unsigned char myFunction(int action){
	itoa((int)action,indstr,10);
	lcdPrintData("My funct ",9);
	lcdGotoXY(0,1);
	lcdPrintData(indstr,strlen(indstr));
	return 0;
	
}

FRESULT createFileList(char (*tab)[9],char *type,unsigned char *numfiles)
{
	TCHAR path[9];
	FRESULT res;
	FILINFO fno;
	FIL fil;
	DIR dir;
	UINT i;	
	
	char *fn;   /* This function is assuming non-Unicode cfg. */
	#if _USE_LFN
	static char lfn[_MAX_LFN + 1];
	fno.lfname = lfn;
	fno.lfsize = sizeof lfn;
	#endif

	res = f_getcwd(path,8);
	res = f_opendir(&dir, path);                       /* Open the directory */
	if (res == FR_OK) {		
		for (;;) {
			res = f_readdir(&dir, &fno);                   /* Read a directory item */
			if (res != FR_OK || fno.fname[0] == 0) 
				break;  /* Break on error or end of dir */
			if (fno.fname[0] == '.') 
				continue;             /* Ignore dot entry */			
			#if _USE_LFN
			fn = *fno.lfname ? fno.lfname : fno.fname;
			#else
			fn = fno.fname;
			#endif
			if (fno.fattrib & AM_DIR) 
				continue;
			if(strstr(fno.fname,type) != NULL || strstr(fno.fname,type) != NULL){
				strncpy(tab[*numfiles],fn,8);
				(*numfiles)++;
			}
			if((*numfiles) == 10)
				break;
			//res = f_write(&fil,fn,strlen(fn),&i);
			//res = f_write(&fil,"\n",1,&i);	// newline
			
		}
	}

	//res = f_close(&fil);
	
	return res;
}