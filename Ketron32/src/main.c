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
#include "SNDFile.h"
#include "lcdstrings.h"


#define MIDI_FILE	0
#define SOUND_FILE	1
FATFS Fatfs;

int main(void)
{
    
	FIL open_files[2];
	
	struct midi_time_event mtevent;
	struct SNDFile sf = {0};
	struct file_entry_lookup file_entry = {0};
	char (*file_list)[MAX_FNAME];
    unsigned char byteValue = 0;
	unsigned char numOfBytes = 0;
	unsigned char currentMode = SOUND_FAMILY,currentAction = NONE;
	BOOL endRecording = TRUE;
	unsigned char inputs[11];
	
	INPUT input = NONE;
    struct family_entry fam;
	struct sound_entry snd;
	struct MD_MIDIFile mf;
      
	unsigned long microseconds = 0;
	unsigned long delta = 0;
	//DDRA |= (1 << PA1);		// remove
	file_list = malloc(MAX_FNAME * MAX_FILES);
	
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
	
	
	midiPlayVolume(inputs[POT]);
	readInputs(inputs);	
	
	while(1){						
			if((input = readInputs(inputs)) >= BUTTON0 && input <= SD){	
				lcdClear();
				if(input >= BUTTON0 && input <= BUTTON3){
					currentMode = input;
					currentAction = NONE;
					file_entry.current_index = 0;
					file_entry.current_items = 0;					
				}
				else if(input >= JOY_UP && input <= JOY_PRESS){
					currentAction = input;
				}
				switch(input){	
					case NONE:	break;
					case POT:	break;					
					case SD:	delay_ms(1000);
								checkSD(inputs[SD]);								
								break; 			
					case BUTTON0:	// select sound family file						
							createFileList(file_list,".FAM",&file_entry);
							handleFileList(currentMode,currentAction,&file_entry,file_list);					
							break;						
					case BUTTON1:	//select sound from file						
							break;						
					case BUTTON2:	// play midi
							createFileList(file_list,".MID",&file_entry);
							handleFileList(currentMode,currentAction,&file_entry,file_list);
							break;
					case BUTTON3:	// record midi
							endRecording = !endRecording;
									
							if(endRecording == FALSE){
								SRAM_seekWrite(0,SEEK_SET);										
								f_open(&open_files[MIDI_FILE],"SONG.MID",FA_WRITE | FA_CREATE_ALWAYS);
								writeMidi(&open_files[MIDI_FILE]);
								lcdGotoXY(0,0);
								lcdPrintData(getLCDString(RECORD,RECORD_LEN),RECORD_LEN);
								microseconds = getMicros();
							}
							else{
								SRAM_seekRead(0,SEEK_SET);
								readSRAM(((unsigned char *)&mtevent),sizeof(struct midi_time_event));
								microseconds = getMicros() + mtevent.delta;
								lcdPrintData(getLCDString(STOPPED,STOPPED_LEN),STOPPED_LEN);
								f_write(&open_files[MIDI_FILE],"\x01\xFF\x2F\x00",4,(UINT *)&numOfBytes);
								f_close(&open_files[MIDI_FILE]);
										
							} 
							break;
					case JOY_UP:	
								if(file_entry.current_index > 0) file_entry.current_index--;
								if(currentMode == MIDI_PLAY || currentMode == SOUND_FAMILY)	
									handleFileList(currentMode,currentAction,&file_entry,file_list);
								else if(currentMode == SOUND_SELECTED){
									scrollSoundList(&sf,JOY_UP,&fam,&snd);
									sendProgramChange(snd.bank,snd.prog);
								}
								break;
					case JOY_RIGHT:	
								scrollSoundList(&sf,JOY_RIGHT, &fam,&snd);
								sendProgramChange(snd.bank,snd.prog);
								break;
					case JOY_DOWN:	
								if(file_entry.current_index < (file_entry.current_items - 1)) file_entry.current_index++;
								if(currentMode == MIDI_PLAY || currentMode == SOUND_FAMILY) 
									handleFileList(currentMode,currentAction,&file_entry,file_list);
								else if(currentMode == SOUND_SELECTED){
									scrollSoundList(&sf,JOY_DOWN, &fam,&snd);
									sendProgramChange(snd.bank,snd.prog);
								}
								break;
					case JOY_LEFT:	
								scrollSoundList(&sf,JOY_LEFT,&fam,&snd);
								sendProgramChange(snd.bank,snd.prog);
								break;
					case JOY_PRESS:	
							if(currentMode == MIDI_PLAY)										
								setMidiFile(&mf,file_list[file_entry.current_index]);									
							else if(currentMode == SOUND_FAMILY){
								if(sf._fileOpen == TRUE){
									closeSNDFile(&sf);
								}
								memset(&sf,0,sizeof(struct SNDFile));
								// open selected file and get first family name and first sound of that family (fam/snd params)
								// f_open
								setSNDFile(&sf,file_list[file_entry.current_index],&fam,&snd);
								//createSoundList(&open_files[SOUND_FILE],&numOfItems);
								lcdClear();
								lcdPrintData(fam.name,strlen(fam.name));
								lcdGotoXY(0,1);
								lcdPrintData(snd.name,strlen(snd.name));
								sendProgramChange(snd.bank,snd.prog);
								currentMode = SOUND_SELECTED;																				
							}
							else if(currentMode == SOUND_SELECTED){
								lcdClear();
								lcdGotoXY(0,0);
								lcdPrintData(getLCDString(SELECTED,SELECTED_LEN),SELECTED_LEN);
								lcdGotoXY(0,1);
								lcdPrintData(fam.name,strlen(fam.name));
								closeSNDFile(&sf);
										
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
				midiPlayVolume(inputs[POT]);
			}
			
			
			// >> process events
			if(currentMode == MIDI_PLAY && currentAction == JOY_PRESS){
				if(!isEOF(&mf)){
					getNextEvent(&mf);
				}
				else{
					closeMIDIFile(&mf);
					lcdPrintData(getLCDString(FINISHED,FINISHED_LEN),FINISHED_LEN);
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
		
		f_close(&open_files[SOUND_FILE]); 
		f_mount(0,NULL);
		free(file_list);		
}