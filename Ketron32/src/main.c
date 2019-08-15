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
static char files[MAX_FILES][9];

int main(void)
{
    unsigned char byteValue = 0;
	unsigned char idx = 0;
    unsigned char num = 0;
    unsigned char currentMode = SOUND_SEL,currentAction = NONE;
	INPUT input = NONE;
    struct sndfamily fam;
    FIL file;
    FILINFO finfo;
    DIR directory;
    FRESULT res;    
	
	//DDRA |= (1 << PA1);		// remove
	
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
	//timerAttach(TIMER1OUTCOMPAREA_INT,addMillis);
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
	
	readInputs();	
	
	while(1){						
			if((input = readInputs()) != NONE){	
				lcdClear();
				if(input >= BUTTON0 && input <= BUTTON3){
					currentMode = input;
					currentAction = NONE;
					idx = 0;
					num = 0;					
				}
				else if(input >= JOY_UP && input <= JOY_PRESS){
					currentAction = input;
				}
				switch(input){
					case BUTTON0:	// select sound						
						createFileList(files,".FAM",&num);						
						break;						
					case BUTTON1:
						break;						
					case BUTTON2:	// play midi
						createFileList(files,".MID",&num);
						handleFileList(currentMode,currentAction,idx,num,files);
						break;
					case BUTTON3:	// record midi
						break;
					case JOY_UP:	if(idx > 0) idx--;	
									handleFileList(currentMode,currentAction,idx,num,files);
									break;
					case JOY_RIGHT:	break;
					case JOY_DOWN:	if(idx < (num - 1)) idx++; 
									handleFileList(currentMode,currentAction,idx,num,files);
									break;
					case JOY_LEFT:	break;
					case JOY_PRESS:	if(currentMode == BUTTON2)
										setMidiFile(files[idx]);
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
				//sendProgramChange(fam.bank,fam.prog);
				//_delay_ms(250);
			}
			
			if(!uartReceiveBufferIsEmpty()){
				byteValue = (unsigned char)uartGetByte();
				if(readMidiMessage(byteValue,&num) == TRUE)
					sendMidiMessage(num);
			
			}
		}
		
		//f_close(&file); 
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

