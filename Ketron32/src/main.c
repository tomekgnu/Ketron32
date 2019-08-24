/*
 * Ketron32.c
 *
 * Created: 15.07.2019 22:39:18
 * Author : Tomek
 */ 
#define __MAIN__

#include <avr/io.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>
#include <avr/eeprom.h>
#include "global.h"
#include "SRAMDriver.h"
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
#include "lcdstrings.h"


#define MIDI_FILE	0
#define SOUND_FILE	1
FATFS Fatfs;

int main(void)
{
    
	FIL soundFile,midiFile;
	FILINFO finf;
	FRESULT res;
	FRESULT openFiles[2] = {FR_INVALID_OBJECT,FR_INVALID_OBJECT};
	struct midi_time_event mtevent;
	char (*files)[MAX_FNAME];
    unsigned char byteValue = 0;
	unsigned char listIndex = 0;
    unsigned char numOfBytes = 0;
	unsigned char numOfItems = 0;
    unsigned char currentMode = SOUND_FAMILY,currentAction = NONE;
	BOOL endRecording = TRUE;
	unsigned char inputs[11];
	
	INPUT input = NONE;
    struct sndfamily fam;
	struct MD_MIDIFile mf;
      
	unsigned long microseconds;
	unsigned long delta;
	//DDRA |= (1 << PA1);		// remove
	files = malloc(MAX_FNAME * MAX_FILES);
	
	configTimers();	
	lcdInit();	
	uartInit();
	midiInit();	
	spiInit();	
	setInputs();
	InitSRAM();
	
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
		
	checkSD(inputs[SD]);
	//if(checkSRAM() == TRUE)
		//lcdPrintData("SRAM OK",7);		
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
			if((input = readInputs(inputs)) >= BUTTON0 && input <= SD){	
				lcdClear();
				if(input >= BUTTON0 && input <= BUTTON3){
					currentMode = input;
					currentAction = NONE;
					listIndex = 0;
					numOfItems = 0;					
				}
				else if(input >= JOY_UP && input <= JOY_PRESS){
					currentAction = input;
				}
				switch(input){						
					case SD:	checkSD(inputs[SD]);
								break; 			
					case BUTTON0:	// select sound family file						
							createFileList(files,".FAM",&numOfItems);
							handleFileList(currentMode,currentAction,listIndex,numOfItems,files);					
							break;						
					case BUTTON1:	//select sound from file						
							break;						
					case BUTTON2:	// play midi
							createFileList(files,".MID",&numOfItems);
							handleFileList(currentMode,currentAction,listIndex,numOfItems,files);
							break;
					case BUTTON3:	// record midi
							endRecording = !endRecording;
									
							if(endRecording == FALSE){
								SRAM_seekWrite(0,SEEK_SET);										
								f_open(&soundFile,"SONG.MID",FA_WRITE | FA_CREATE_ALWAYS);
								writeMidi(&soundFile);
								lcdGotoXY(0,0);
								lcdPrintData("Recording",9);
								microseconds = getMicros();
							}
							else{
								SRAM_seekRead(0,SEEK_SET);
								readSRAM(((unsigned char *)&mtevent),sizeof(struct midi_time_event));
								microseconds = getMicros() + mtevent.delta;
								lcdPrintData("Stopped",7);
								f_write(&soundFile,"\x01\xFF\x2F\x00",4,&numOfBytes);
								f_close(&soundFile);
										
							} 
							break;
					case JOY_UP:	if(listIndex > 0) listIndex--;
									if(currentMode == MIDI_PLAY || currentMode == SOUND_FAMILY)	
										handleFileList(currentMode,currentAction,listIndex,numOfItems,files);
									else if(currentMode == SOUND_SELECT){
										handleSoundList(&soundFile,listIndex,numOfItems,&fam);
										sendProgramChange(fam.bank,fam.prog);
									}
									break;
					case JOY_RIGHT:	break;
					case JOY_DOWN:	if(listIndex < (numOfItems - 1)) listIndex++;
									if(currentMode == MIDI_PLAY || currentMode == SOUND_FAMILY) 
										handleFileList(currentMode,currentAction,listIndex,numOfItems,files);
									else if(currentMode == SOUND_SELECT){
										handleSoundList(&soundFile,listIndex,numOfItems,&fam);
										sendProgramChange(fam.bank,fam.prog);
									}
									break;
					case JOY_LEFT:	break;
					case JOY_PRESS:	if(currentMode == MIDI_PLAY)										
										setMidiFile(&mf,files[listIndex]);									
									else if(currentMode == SOUND_FAMILY){
										if(openFiles[SOUND_FILE] == FR_OK){
											f_close(&soundFile);
											openFiles[SOUND_FILE] = FR_INVALID_OBJECT;
										}
										openFiles[SOUND_FILE] = setSoundFile(&soundFile,&fam,files[listIndex]);	// f_open
										createSoundList(&soundFile,&numOfItems);
										handleSoundList(&soundFile,listIndex,numOfItems,&fam);
										sendProgramChange(fam.bank,fam.prog);
										currentMode = SOUND_SELECT;																				
									}
									else if(currentMode == SOUND_SELECT){
										lcdClear();
										lcdGotoXY(0,0);
										lcdPrintData("Selected: ",10);
										lcdGotoXY(0,1);
										lcdPrintData(fam.name,strlen(fam.name));
										f_close(&soundFile);
										openFiles[SOUND_FILE] = FR_INVALID_OBJECT;
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
			if(currentMode == MIDI_PLAY && currentAction == JOY_PRESS){
				if(!isEOF(&mf)){
					getNextEvent(&mf);
				}
				else{
					closeMIDIFile(&mf);
					lcdPrintData("Finished",8);
					currentAction = NONE;
				}
			}
			else if(currentMode == MIDI_REC && endRecording == TRUE){
				if(getMicros() > microseconds){
					sendMidiBuffer(mtevent.event.data,mtevent.event.size);
					readSRAM(((unsigned char *)&mtevent),sizeof(struct midi_time_event));
					microseconds = getMicros() + mtevent.delta; 
				}
			}
			if(!uartReceiveBufferIsEmpty()){
				byteValue = (unsigned char)uartGetByte();
				if(readMidiMessage(byteValue,&numOfBytes) == TRUE){
					sendMidiMessage(numOfBytes);
					if(currentMode == MIDI_REC && endRecording == FALSE){
						delta = (getMicros() - microseconds);
						//WriteVarLen(&soundFile,delta);
						writeSRAM((unsigned char *)getMidiStruct(delta),sizeof(struct midi_time_event));						
						microseconds = getMicros();
						//f_write(&soundFile,getMidiEvent(),(UINT)numOfBytes,((UINT *)&numOfItems));
					}				
					
				}
				
			}
			
			// << process events
		}
		
		f_close(&soundFile); 
		f_mount(0,NULL);
		free(files);		
}