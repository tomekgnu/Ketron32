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


int main(void)
{
    FATFS Fatfs;
	char (*files)[MAX_FNAME];
    unsigned char byteValue = 0;
	unsigned char idx = 0;
    unsigned char numOfBytes = 0;
	unsigned char numOfItems = 0;
    unsigned char currentMode = SOUND_FAMILY,currentAction = NONE;
	unsigned char inputs[10];
	INPUT input = NONE;
    struct sndfamily fam;
	struct MD_MIDIFile mf;
    FIL file;
    FILINFO finfo;
    DIR directory;
    FRESULT res;    
	
	//DDRA |= (1 << PA1);		// remove
	files = malloc(MAX_FNAME * MAX_FILES);
	
	configTimers();	
	lcdInit();	
	uartInit();
	midiInit();	
	spiInit();	
	setInputs();
	
	// >> ADC
	a2dInit();
	a2dSetPrescaler(ADC_PRESCALE_DIV32);
	a2dSetReference(ADC_REFERENCE_AVCC);
	a2dSetChannel(ADC_CH_ADC0);
	a2dStartConvert();		
	// << ADC
	 
	timerAttach(TIMER2OUTCOMPARE_INT,disk_timerproc);
	timerAttach(TIMER0OVERFLOW_INT,addMillis);
	
	uartSetBaudRate(MIDI_BAUD_RATE);
	uartSetRxHandler(rx_handler);
	uartFlushReceiveBuffer();
	
	lcdGotoXY(0,0);	
		
	if(f_mount(0,&Fatfs) != FR_OK)
		lcdPrintData("Mount failed",12);
	else
		lcdPrintData("Mount OK",8);
	lcdGotoXY(0,1);		
	/*
	res = disk_initialize(0);		
	if((res = f_open(&file,"Piano.fam",FA_READ)) != FR_OK)
		lcdPrintData("Open failed",11);
	else{
		lcdPrintData("Open OK",7);		
		f_close(&file);
	}
	*/
	
	midiFileVolume(inputs[POT] / 2);
	readInputs(inputs);	
	
	while(1){						
			if((input = readInputs(inputs)) >= BUTTON0 && input <= JOY_PRESS){	
				lcdClear();
				if(input >= BUTTON0 && input <= BUTTON3){
					currentMode = input;
					currentAction = NONE;
					idx = 0;
					numOfItems = 0;					
				}
				else if(input >= JOY_UP && input <= JOY_PRESS){
					currentAction = input;
				}
				switch(input){					
					case BUTTON0:	// select sound family file						
						createFileList(files,".FAM",&numOfItems);
						handleFileList(currentMode,currentAction,idx,numOfItems,files);					
						break;						
					case BUTTON1:	//select sound from file
						createSoundList(&file,&numOfItems);
						handleSoundList(&file,idx,numOfItems,&fam);
						break;						
					case BUTTON2:	// play midi
						createFileList(files,".MID",&numOfItems);
						handleFileList(currentMode,currentAction,idx,numOfItems,files);
						break;
					case BUTTON3:	// record midi
						break;
					case JOY_UP:	if(idx > 0) idx--;
									if(currentMode == MIDI_PLAY || currentMode == SOUND_FAMILY)	
										handleFileList(currentMode,currentAction,idx,numOfItems,files);
									else if(currentMode == SOUND_SELECT){
										handleSoundList(&file,idx,numOfItems,&fam);
										sendProgramChange(fam.bank,fam.prog);
									}
									break;
					case JOY_RIGHT:	break;
					case JOY_DOWN:	if(idx < (numOfItems - 1)) idx++;
									if(currentMode == MIDI_PLAY || currentMode == SOUND_FAMILY) 
										handleFileList(currentMode,currentAction,idx,numOfItems,files);
									else if(currentMode == SOUND_SELECT){
										handleSoundList(&file,idx,numOfItems,&fam);
										sendProgramChange(fam.bank,fam.prog);
									}
									break;
					case JOY_LEFT:	break;
					case JOY_PRESS:	if(currentMode == MIDI_PLAY)										
										setMidiFile(&mf,files[idx]);									
									else if(currentMode == SOUND_FAMILY){
										setSoundFile(&file,&fam,files[idx]);
										sendProgramChange(fam.bank,fam.prog);
										lcdGotoXY(0,1);
										lcdPrintData(fam.name,strlen(fam.name));										
									}
									break;							
				}
				
					
				//lcdClear();
				//lcdGotoXY(0,0);
				//itoa((ind + 1),indstr,10);
				//lcdPrintData(indstr,2);
				//f_lseek(&file,ind * sizeof(struct sndfamily));
				//f_read(&file,&fam,sizeof(fam),&numOfBytes);
				//lcdGotoXY(0,1);
				//lcdPrintData(fam.name,strlen(fam.name));
				
				//_delay_ms(250);
			}
			else if(input == POT){
				midiFileVolume(inputs[POT] / 2);
			}
			
			
			// >> process events
			if(currentMode == BUTTON2 && currentAction == JOY_PRESS){
				if(!isEOF(&mf)){
					getNextEvent(&mf);
				}
				else{
					closeMIDIFile(&mf);
					lcdPrintData("Finished",8);
					currentAction = NONE;
				}
			}
			if(!uartReceiveBufferIsEmpty()){
				byteValue = (unsigned char)uartGetByte();
				if(readMidiMessage(byteValue,&numOfBytes) == TRUE)
					sendMidiMessage(numOfBytes);			
			}
			// << process events
		}
		
		f_close(&file); 
		f_mount(0,NULL);
		free(files);
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

