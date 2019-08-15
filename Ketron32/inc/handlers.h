/*
 * handlers.h
 *
 * Created: 16.07.2019 11:39:24
 *  Author: Tomek
 */ 


#ifndef HANDLERS_H_
#define HANDLERS_H_

#include "avrlibtypes.h"
#include "global.h"

void rx_handler(unsigned char c);
void configTimers();
void toggleLed();
void setInputs();
INPUT readInputs();
void handleFileList(unsigned char currentMode,unsigned char currentAction,unsigned char i,unsigned char n,char (*list)[9]);
unsigned char setMidiFile(char *);
unsigned char midiRecord(int);
unsigned char soundSelect(int);
unsigned char myFunction(int);
FRESULT createFileList(char (*tab)[9],char *type,unsigned char *numfiles);


static unsigned char (*input_handlers[4])(unsigned char) = {setMidiFile,midiRecord,soundSelect,myFunction};
	
#endif /* HANDLERS_H_ */