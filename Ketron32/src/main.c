/*
 * Ketron32.c
 *
 * Created: 15.07.2019 22:39:18
 * Author : Tomek
 */ 

#include <avr/io.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>
#include "global.h"
#include "uart.h"
#include "midi.h"
#include "handlers.h"
#include "buffer.h"
#include "avrlibdefs.h"
#include "lcd.h"
#include "spi.h"
#include "ff.h"
#include "timer.h"
#include "diskio.h"
#include "a2d.h"
#include "MD_MIDIFile.h"

FATFS Fatfs;

int main(void)
{
    unsigned char byteValue = 0;
	struct MD_MIDIFile mf;
	char indstr[17];
    unsigned int ind = 0;
    unsigned char numOfBytes = 0;
    unsigned char pot = 0;
    unsigned char buttons[4];
    INPUT input = NONE;
    struct sndfamily fam;
    FIL file;
    FILINFO finfo;
    DIR directory;
    FRESULT res;    
	
	DDRA |= (1 << PA1);		// remove
			
	configTimers();	
	lcdInit();	
	uartInit();
	midiInit();	
	spiInit();	
	
	// >> ADC
	a2dInit();
	a2dSetPrescaler(ADC_PRESCALE_DIV32);
	a2dSetReference(ADC_REFERENCE_AVCC);
	a2dSetChannel(ADC_CH_ADC0);
	a2dStartConvert();		
	// << ADC
	 
	timerAttach(TIMER2OUTCOMPARE_INT,disk_timerproc);
	timerAttach(TIMER1OUTCOMPAREA_INT,addMillis);
	
	
	uartSetBaudRate(MIDI_BAUD_RATE);
	uartSetRxHandler(rx_handler);
	uartFlushReceiveBuffer();
	
	lcdGotoXY(0,0);	
		
	if(f_mount(0,&Fatfs) != FR_OK)
		lcdPrintData("Mount failed",12);
	else
		lcdPrintData("Mount OK",8);
	lcdGotoXY(0,1);		
	
	if((res = f_open(&file,"Plik.mid",FA_READ)) != FR_OK)
		lcdPrintData("Open failed",11);
	else
		lcdPrintData("Open OK",7);		
	f_close(&file);
	
	// >> MIDI
	memset(&mf,0,sizeof(struct MD_MIDIFile));
	//initialise(&mf);
	setFilename(&mf,"Plik.mid");
	
	if(loadMIDIFile(&mf) != -1){
		lcdPrintData("Open failed",11);
		while(1){}
	}
	setMidiHandler(&mf,midiFun);
	setSysexHandler(&mf,sysexFun);
	setMetaHandler(&mf,metaFun);
	
	mf._paused = FALSE;
	mf._looping = FALSE;
	mf._tickTime = 2083;
	//synchTracks(&mf);
	resetTime();
	while(!isEOF(&mf)){
		getNextEvent(&mf);
		if(!uartReceiveBufferIsEmpty()){
				byteValue = (unsigned char)uartGetByte();
				if(readMidiMessage(byteValue,&numOfBytes) == TRUE)
					sendMidiMessage(numOfBytes);
			
			}	
	}
	
	closeMIDIFile(&mf);
	lcdPrintData("Finished",8);
	while(1){}
	// << MIDI
	
	readInputs(&pot,buttons);
	
	f_read(&file,&fam,sizeof(fam),&numOfBytes);
	f_lseek(&file,0);
	
	setInputs();
	while(1){
			get_input:			
			if((input = readInputs(&pot,buttons)) != NONE){				
				switch(input){
					case POT:	 goto get_input;
					case BUTTON0: 
							if(ind > 0)
								ind--;																					
							break;
					case BUTTON1: 
							if(!f_eof(&file))
								ind++;
							break;
					case BUTTON2:
							lcdPrintData("Button 3",8); 
							break;
					case BUTTON3:
							lcdPrintData("Button 4",8);
							break;
				}
				
				lcdClear();
				lcdGotoXY(0,0);
				itoa((ind + 1),indstr,10);
				lcdPrintData(indstr,2);
				f_lseek(&file,ind * sizeof(struct sndfamily));
				f_read(&file,&fam,sizeof(fam),&numOfBytes);
				lcdGotoXY(0,1);
				lcdPrintData(fam.name,strlen(fam.name));
				sendProgramChange(fam.bank,fam.prog);
				_delay_ms(250);
			}
			
			if(!uartReceiveBufferIsEmpty()){
				byteValue = (unsigned char)uartGetByte();
				if(readMidiMessage(byteValue,&numOfBytes) == TRUE)
					sendMidiMessage(numOfBytes);
			
			}
		}
		
		f_close(&file);
		f_mount(0,NULL);
		
}


void tickMetronome(struct MD_MIDIFile *m)
// flash a LED to the beat
{
  static uint32_t	lastBeatTime = 0;
  static BOOL	inBeat = FALSE;
  uint16_t	beatTime;

  beatTime = 60000/m->_tempo;		// msec/beat = ((60sec/min)*(1000 ms/sec))/(beats/min)
  if (!inBeat)
  {
    if ((getMillis() - lastBeatTime) >= beatTime)
    {
      lastBeatTime = getMillis();
      inBeat = TRUE;
    }
  }
  else
  {
    if ((getMillis() - lastBeatTime) >= 100)	// keep the flash on for 100ms only    {
      
      inBeat = FALSE;
    }
  }

