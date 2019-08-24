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
#include "MD_MIDIFile.h"

void rx_handler(unsigned char c);
void configTimers();
void toggleLed();
void setInputs();
INPUT readInputs(unsigned char *);
FRESULT createFileList(char (*tab)[MAX_FNAME],char *type,unsigned char *numfiles);
void handleFileList(unsigned char currentMode,unsigned char currentAction,unsigned char i,unsigned char n,char (*tab)[MAX_FNAME]);
unsigned char setMidiFile(struct MD_MIDIFile *,char *);
unsigned char midiRecord(int);
unsigned char soundSelect(int);
unsigned char myFunction(int);
char * getLCDString(unsigned char offset,unsigned char len);
void checkSD(unsigned char input);

FRESULT setSoundFile(FIL *,struct sndfamily *,unsigned char *);
void createSoundList(FIL *,unsigned char *);
void handleSoundList(FIL *,unsigned char,unsigned char,struct sndfamily *);
void writeMidi(FIL *file);
void WriteVarLen(FIL *file,unsigned long value);
static unsigned char (*input_handlers[4])(unsigned char) = {setMidiFile,midiRecord,soundSelect,myFunction};
	
#endif /* HANDLERS_H_ */