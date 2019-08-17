#ifndef _MIDI_H
#define _MIDI_H

#include "avrlibtypes.h"
#include "MD_MIDIFile.h"

// midi states
#define MIDI_WAIT 1
#define MIDI_READING 2
#define MIDI_IGNORING 3

// midi events
#define MIDI_NOTE_OFF 	0x80
#define MIDI_NOTE_ON 	0x90
#define MIDI_POLY_TOUCH 	0xA0
#define MIDI_CONTROL_CHANGE	0xB0
#define MIDI_PROGRAM_CHANGE	0xC0
#define MIDI_CHANNEL_TOUCH	0xD0
#define MIDI_PITCH_BEND	        0xE0

// midi system events
#define MIDI_SYSEX_START        0xF0
#define MIDI_SYSEX_END          0xF7
#define MIDI_MTC                0xF1
#define MIDI_SPP                0xF2
#define MIDI_SONG_SEL           0xF3
#define MIDI_TUNE_REQ           0xF6
#define MIDI_CLOCK              0xF8
#define MIDI_SYNC               0xF9
#define MIDI_START              0xFA
#define MIDI_STOP               0xFC
#define MIDI_CONT               0xFB
#define MIDI_SENSE              0xFE
#define MIDI_RESET              0xFF


#define MIDI_DATA_MASK			0x7F
#define MIDI_STATUS_MASK		0xF0
#define MIDI_CHANNEL_MASK		0x0F

#define MIDI_BAUD_RATE			31250

unsigned char * getMidiEvent();
BOOL readMidiMessage(unsigned char c,unsigned char *len);
void sendMidiMessage(unsigned char num);
void sendMidiBuffer(unsigned char *buf,unsigned char num);
void sendProgramChange(unsigned char bank,unsigned char program);
void midiFileVolume(unsigned char vol);
void midiPlayVolume(unsigned char vol);
void midiFun(midi_event *ev);
void metaFun(meta_event *ev);
void sysexFun(sysex_event *ev);
void midiInit(void);
void midiPoll(unsigned char byte);
unsigned char commandLen(unsigned char cmd);
#endif
