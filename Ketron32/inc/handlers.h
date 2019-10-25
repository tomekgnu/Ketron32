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
#include "SNDFile.h"

void rx_handler(unsigned char c);
void configTimers();
void toggleLed();
void setInputs();
INPUT readInputs(unsigned char *);
FRESULT createFileList(char (*tab)[MAX_FNAME],char *type,struct file_entry_lookup *filentry);
void handleFileList(unsigned char currentMode,unsigned char currentAction,struct file_entry_lookup *filentry,char (*tab)[MAX_FNAME]);
unsigned char setMidiFile(struct MD_MIDIFile *,char *);

char * getLCDString(unsigned short offset,unsigned char len);
void checkSD(unsigned char input);

void createSoundList(FIL *,unsigned char *);
void writeMidi(FIL *file);
void WriteVarLen(FIL *file,unsigned long value);
void closeFile(FRESULT *);
	
#endif /* HANDLERS_H_ */