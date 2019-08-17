//  Midi.c
//
//  Midi output routines for the atmel atmega163 (and others)
//  

//#include <avr/io.h>
//#include <avr/pgmspace.h>

#include <util/delay.h>
#include <string.h>
#include "midi.h"
#include "uart.h"
#include "midi.h"

// midi state machine
unsigned char midiState;
// buffer for currently read midi event
unsigned char midiEvent[3];
// value 0-2 of where to write the next input byte 
unsigned char midiReadIndex;
unsigned char midiBytesToIgnore;
BOOL noteEvent = FALSE;

unsigned char playVolume,fileVolume;

/* simple map of midi messages
      first byte
          |
----------------------
|                    |
high nibble  low nibble bytes following (& description)
   /            /       /
  / ------------       /
 / /   ----------------
| /   /
8 ch  2 (note number, velocity=64) NOTE OFF
9 *                                NOTE ON
A *                                AFTERTOUCH
B ch  2 (cc number, value)         CC
C ch  1 (prog number)              PROG CHANGE
D ch  1 (pressure amt)             PRESSURE
E ch  2 (pitch wheel as 14-bit #)  PITCHBEND
F 0   n (bytes until F7)           SYSEX (first byte following F0 = manufacture id)
F 1   1 (timecode)                 MTC
F 2   2 (SPP)                      SONG POSITION POINTER
F 3   1 (song #)                   SONG SELECT
F 6   0                            TUNE REQUEST
F 8   0                            MIDI CLOCK (24ppqn)
F 9   0                            MIDI SYNC (10ms)
F A   0                            MIDI START
F C   0                            MIDI STOP
F B   0                            MIDI CONTINUE
F E   0                            MIDI SENSE (300ms if exists)
F F   0                            Reset
*/

void midiInit(){
	midiState = MIDI_WAIT;
}

unsigned char * getMidiEvent(){
	return midiEvent;
}

BOOL readMidiMessage(unsigned char byte,unsigned char *len){
	
   unsigned char tmp;

   // state machine for parsing the byte
   switch(midiState)
   {
         // we are currently stateless, waiting to start reading an event we care about.
         case MIDI_WAIT:
            if (byte == 0xF0)
            {
               // start of sysex
               // call sysex handler, which will return the state we should be in.
               //midiState = handleSysex();
               break;
            }
            // store length of midi command
            tmp = commandLen(byte);
            // is the message one byte long?
            if (tmp == 1)
            {
               // send event out!
/*               if (byte == MIDI_CLOCK && midiClockFunc)
               {
                  // it's a clock event and we have a registered clock handler
                  midiClockFunc();
               } else {*/
            	 *len = 1;
            	 midiEvent[0] = byte;
            	 noteEvent = FALSE;
                 return TRUE;
            } else if(tmp == 0){
            	if(noteEvent == TRUE) {
            		midiEvent[1] = byte;
            		midiReadIndex = 2;
            	}
            }
			else {
				   // save first byte of event, position pointer..
				   midiEvent[0] = byte;
				   midiReadIndex = 1;
				}
            midiState = MIDI_READING;
            break;
         case MIDI_READING:
        	if(commandLen(byte) > 0){
        		midiState = MIDI_WAIT;
        		noteEvent = FALSE;
        		return FALSE;
        	}
        	midiEvent[midiReadIndex++] = byte;
            if (midiReadIndex == commandLen(midiEvent[0]&0xF0))
            {
               midiState = MIDI_WAIT;
               *len = midiReadIndex;
               if(midiEvent[0] & (MIDI_NOTE_ON|MIDI_NOTE_OFF)){
            	   noteEvent = TRUE;
               }
               return TRUE;
            }
            break;
         case MIDI_IGNORING:
            if (byte == 0xF7)
            {
               midiState = MIDI_WAIT;
            }
            break;
      }

   return FALSE;
}

void sendMidiMessage(unsigned char num){
	unsigned char i;
	for(i = 0; i < num; i++)
		uartSendByte(midiEvent[i]);
	
}

void sendMidiBuffer(unsigned char *buf,unsigned char num){
	unsigned char i;
	for(i = 0; i < num; i++)
	uartSendByte(buf[i]);
}

void sendProgramChange(unsigned char bank,unsigned char program){
	midiEvent[0] = MIDI_CONTROL_CHANGE;
	midiEvent[1] = 0;			// MSB
	midiEvent[2] = bank;		// LSB
	sendMidiMessage(3);
	midiEvent[0] = MIDI_PROGRAM_CHANGE;
	midiEvent[1] = program;
	sendMidiMessage(2);
}

unsigned char commandLen(unsigned char cmd)
{
	
	if ((cmd & 0xF0) != 0xF0)
	cmd = cmd & 0xF0;
	
	switch(cmd){
		case	MIDI_TUNE_REQ:
		case	MIDI_CLOCK:
		case	MIDI_SYNC:
		case	MIDI_START:
		case	MIDI_STOP:
		case	MIDI_CONT:
		case	MIDI_SENSE:
		case	MIDI_RESET:	
					return 1;
		case	MIDI_PROGRAM_CHANGE:
		case	MIDI_CHANNEL_TOUCH:
		case	MIDI_MTC:
		case	MIDI_SONG_SEL:	
					return 2;
		case	MIDI_NOTE_OFF:
		case	MIDI_NOTE_ON:
		case	MIDI_POLY_TOUCH:
		case	MIDI_CONTROL_CHANGE:
		case	MIDI_PITCH_BEND:
		case	MIDI_SPP:	
					return 3;
		
	}
	
	return 0;
}

void metaFun(meta_event *ev){
	
}

void sysexFun(sysex_event *ev){
	
}

void midiFun(midi_event *ev){	
	if((ev->data[0] & 0xF0) == 0x90)
		ev->data[2] = fileVolume;
	else if((ev->data[0] & 0xF0) == 0x80)
		ev->data[2] = 0;
	ev->data[0] = ev->data[0] | ev->channel;
	sendMidiBuffer(ev->data,ev->size);
}

void midiFileVolume(unsigned char vol){
	fileVolume = vol;
}

void midiPlayVolume(unsigned char vol){	
	playVolume = vol;
}