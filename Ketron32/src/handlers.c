/*
 * handlers.c
 *
 * Created: 16.07.2019 11:37:12
 *  Author: Tomek
 */ 
#include <avr/io.h>
#include <avr/eeprom.h>
#include <stdint.h>
#include <string.h>
#include <util/delay.h>
#include "uart.h"
#include "buffer.h"
#include "global.h"
#include "handlers.h"
#include "midi.h"
#include "avrlibtypes.h"
#include "lcd.h"
#include "ff.h"
#include "a2d.h"
#include "timer.h"
#include "lcdstrings.h"

extern FATFS Fatfs;
extern cBuffer uartRxBuffer;
extern volatile unsigned char adcValue;
char indstr[17];
   	
volatile unsigned long timer0_overflow_count = 0;
volatile unsigned long timer0_millis = 0;
static unsigned char timer0_fract = 0;
static char lcdstr[16];

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
	 // timer0: for microseconds
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
	
	// SD chip detect: input, pull-up
	cbi(SD_DETECT_DDR,SD_DETECT);
	sbi(SD_DETECT_PORT,SD_DETECT);
}

INPUT readInputs(unsigned char *inp_new){
	static unsigned char inp_old[11] = {0,0,0,0,0,0,0,0,0,0,0};
	
	INPUT i;
	
	inp_new[POT] = a2dConvert8bit(ADC_CH_ADC0);
	// read buttons
	inp_new[BUTTON0] = !(PINB & (1 << SW0));
	inp_new[BUTTON1] = !(PINB & (1 << SW1));
	inp_new[BUTTON2] = !(PINB & (1 << SW2));
	inp_new[BUTTON3] = !(PINB & (1 << SW3));
	
	// read joystick
	inp_new[JOY_UP] = !(JOY_N_PIN & (1 << JOY_N));
	inp_new[JOY_RIGHT] = !(JOY_E_PIN & (1 << JOY_E));
	inp_new[JOY_DOWN] = !(JOY_S_PIN & (1 << JOY_S));
	inp_new[JOY_LEFT] = !(JOY_W_PIN & (1 << JOY_W));
	inp_new[JOY_PRESS] = !(JOY_T_PIN & (1 << JOY_T));	
	inp_new[SD] = !(SD_DETECT_PIN & (1 << SD_DETECT));
	
	if(inp_new[SD] != inp_old[SD]){
		inp_old[SD] = inp_new[SD];
		return SD;
	}
	
	for(i = BUTTON0; i < POT; i++){
		if(inp_new[i] != inp_old[i]){
			inp_old[i] = inp_new[i];
			if(inp_new[i] == 1)
				return i;			
		}	
	}
	
	if(inp_new[POT] != inp_old[POT]){
		inp_old[POT] = inp_new[POT];
		return POT;
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

unsigned char setMidiFile(struct MD_MIDIFile *mf, char *name){
	
		
	memset(mf,0,sizeof(struct MD_MIDIFile));
	initialise(mf);
	setMidiHandler(mf,midiFun);
	setSysexHandler(mf,sysexFun);
	setMetaHandler(mf,metaFun);
	setFilename(mf,name);	
	
	if(loadMIDIFile(mf) != -1){
		lcdPrintData(getLCDString(OPN_NO,OPN_NO_LEN),OPN_NO_LEN);
		_delay_ms(500);
		return 1;
	}	
	
	mf->_paused = FALSE;
	mf->_looping = FALSE;	
	
	
	// << MIDI
	
	return 0;
}


char * getLCDString(unsigned short offset,unsigned char len){
	memset(lcdstr,0,16);
	eeprom_read_block(lcdstr,(char *)offset,(size_t)len);
	return lcdstr;
}

void checkSD(unsigned char input){
	lcdGotoXY(0,0);
	
	if(input == 1){
		lcdPrintData(getLCDString(SD_REM,SD_REM_LEN),SD_REM_LEN);
		lcdGotoXY(0,1);
		if(f_mount(0,NULL) != FR_OK)
			lcdPrintData(getLCDString(UNM_NO,UNM_NO_LEN),UNM_NO_LEN);
		else
			lcdPrintData(getLCDString(UNM_OK,UNM_OK_LEN),UNM_OK_LEN);
	}
	else{
		lcdPrintData(getLCDString(SD_INS,SD_INS_LEN),SD_INS_LEN);
		lcdGotoXY(0,1);
		if(f_mount(0,&Fatfs) != FR_OK)
			lcdPrintData(getLCDString(MNT_NO,MNT_NO_LEN),MNT_NO_LEN);
		else
			lcdPrintData(getLCDString(MNT_OK,MNT_OK_LEN),MNT_OK_LEN);
	}
}

FRESULT createFileList(char (*tab)[MAX_FNAME],char *type,unsigned char *numfiles)
{
	TCHAR path[9];
	FRESULT res;
	FILINFO fno;
	DIR dir;
		
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
			if(strlen(fno.fname) >= MAX_FNAME)
				continue;
			memset(tab[*numfiles],0,MAX_FNAME);
			if(strstr(fno.fname,type) != NULL || strstr(fno.fname,type) != NULL){
				strncpy(tab[*numfiles],fn,strlen(fn));
				(*numfiles)++;
			}
			if((*numfiles) == MAX_FILES)
				break;
			//res = f_write(&fil,fn,strlen(fn),&i);
			//res = f_write(&fil,"\n",1,&i);	// newline
			
		}
	}

	//res = f_close(&fil);
	
	return res;
}

FRESULT setSoundFile(FIL *file, struct family_entry *fam,struct sound_entry *snd,char *filename){
	FRESULT res;
	UINT numOfBytes;
	
	if((res = f_open(file,filename,FA_READ)) != FR_OK)
		lcdPrintData(getLCDString(OPN_NO,OPN_NO_LEN),OPN_NO_LEN);
	else{
		lcdPrintData(getLCDString(OPN_OK,OPN_OK_LEN),OPN_OK_LEN);
		f_lseek(file,0);
		f_read(file,fam,sizeof(struct family_entry),&numOfBytes);
		f_read(file,snd,sizeof(struct sound_entry),&numOfBytes);				
	}
	
	f_lseek(file,0);
	return res;
}

void createSoundList(FIL *file,unsigned char *num){
	(*num) = file->fsize / sizeof(struct sound_entry); 
}

void scrollSoundList(FIL *file,INPUT joy, struct family_entry *fam, struct sound_entry *snd,struct sound_file_ptr *sptr){	
	UINT numOfBytes;
	sptr->next_family = sptr->current_family + sizeof(struct family_entry) + fam->current_sounds * sizeof(struct sound_entry);
	if(fam->previous_sounds != 0)
	sptr->previous_family = sptr->current_family - fam->previous_sounds * sizeof(struct sound_entry) - sizeof(struct family_entry);
	lcdClear();	
	switch(joy){
		case BUTTON0:
		case BUTTON1:
		case BUTTON2:
		case BUTTON3:
		case SD:
		case POT:
				break;
		case JOY_PRESS:
				break;
		case NONE: break;
		case JOY_LEFT:
			sptr->current_sound = 0;
			f_lseek(file,sptr->previous_family);
			break;
		case JOY_RIGHT:
			sptr->current_sound = 0;
			if(sptr->next_family < file->fsize)
				f_lseek(file,sptr->next_family);
			break;
		case JOY_UP:
			if(sptr->current_sound > 0)
				sptr->current_sound--;
			break;
		case JOY_DOWN:
			if(sptr->current_sound < (fam->current_sounds - 1))
				sptr->current_sound++;
			break;

	}

		sptr->current_family = f_tell(file);
		f_read(file,fam,sizeof(struct family_entry),&numOfBytes);		
		lcdGotoXY(0,0);
		lcdPrintData(fam->name,strlen(fam->name));
		f_lseek(file,sptr->current_family + sizeof(struct family_entry) + sptr->current_sound * sizeof(struct sound_entry));
		f_read(file,snd,sizeof(struct sound_entry),&numOfBytes);
		f_lseek(file,sptr->current_family);
		lcdGotoXY(0,1);
		lcdPrintData(snd->name,strlen(snd->name));
		
}

void handleFileList(unsigned char currentMode,unsigned char currentAction,unsigned char index,unsigned char number,char (*list)[MAX_FNAME]){
	unsigned char tmp = index - (index % 2);
	char *ch[2] = {"*"," "};
	
	lcdGotoXY(0,0);
	if(number == 0){
		lcdPrintData(getLCDString(NO_SND,NO_SND_LEN),NO_SND_LEN);
		return;
	}
	
	lcdGotoXY(1,0);
	lcdPrintData(list[tmp],strlen(list[tmp]));
	lcdGotoXY(0,0);
	lcdPrintData(ch[index % 2],1);
	if(index < number){
		lcdGotoXY(1,1);
		lcdPrintData(list[tmp + 1],strlen(list[tmp + 1]));
		lcdGotoXY(0,1);
		lcdPrintData(ch[(index + 1) % 2],1);
	}
}

void writeMidi(FIL *file){
	//UINT br;
	/*
	f_write(file,"MThd",4,&br);		// marker
	f_write(file,"\x00\x00\x00\x06",4,&br);	// length of data
	f_write(file,"\x00\x01",2,&br);		// SMF type
	f_write(file,"\x00\x02",2,&br);		// number of tracks
	f_write(file,"\x01\xE0",2,&br);	// PPQ
	
	// midi tempo track
	f_write(file,"MTrk",4,&br);			// marker
	f_write(file,"\x00\x00\x00\x19",4,&br);	// length of following data
	f_write(file,"\x00\xFF\x58\x04\x04\x02\x18\x08",8,&br);		// meta event: time signature
	f_write(file,"\x00\xFF\x59\x02\x02\x00",6,&br);		// meta event: key signature
	f_write(file,"\x00\xFF\x51\x03\x0F\x42\x40",7,&br);	// meta event: set tempo
	f_write(file,"\x01\xFF\x2F\x00",4,&br);	// meta event:	end of track
	
	f_write(file,"MTrk",4,&br);			// marker
	f_write(file,"\x00\x00\x00\x71",4,&br);	// length of following data
	*/
}

void WriteVarLen(FIL *file,unsigned long value)
{
	unsigned long buffer;
	unsigned char c;
	UINT n;
	
	buffer = value & 0x7f;
	while((value >>= 7) > 0)
	{
		buffer <<= 8;
		buffer |= 0x80;
		buffer += (value & 0x7f);
	}
	while(1){
		c = ((unsigned)(buffer & 0xff));
		f_write(file,&c,1,&n);
		if(buffer & 0x80)
			buffer >>= 8;
		else
			return;
	}
}

void closeFile(FRESULT *fr){
	
}