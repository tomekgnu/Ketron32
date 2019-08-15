/*! \file global.h \brief AVRlib project global include. */
//*****************************************************************************
//
// File Name	: 'global.h'
// Title		: AVRlib project global include 
// Author		: Pascal Stang - Copyright (C) 2001-2002
// Created		: 7/12/2001
// Revised		: 9/30/2002
// Version		: 1.1
// Target MCU	: Atmel AVR series
// Editor Tabs	: 4
//
//	Description : This include file is designed to contain items useful to all
//					code files and projects.
//
// This code is distributed under the GNU Public License
//		which can be found at http://www.gnu.org/licenses/gpl.txt
//
//*****************************************************************************

#ifndef GLOBAL_H
#define GLOBAL_H

#include <stdint.h>
// global AVRLIB defines
#include "avrlibdefs.h"
// global AVRLIB types definitions
#include "avrlibtypes.h"
#include "ff.h"
// project/system dependent defines

// CPU clock speed
//#define F_CPU        16000000               		// 16MHz processor
//#define F_CPU        14745000               		// 14.745MHz processor
//#define F_CPU        8000000               		// 8MHz processor
//#define F_CPU        7372800               		// 7.37MHz processor
//#define F_CPU        4000000               		// 4MHz processor
//#define F_CPU        3686400               		// 3.69MHz processor
#define CYCLES_PER_US ((F_CPU+500000)/1000000) 	// cpu cycles per microsecond

#define MAX_FILES		10

#define SCK PB7
#define MOSI PB5
#define CS PB4
#define MISO PB6

#define SW0		PB0
#define SW1		PB1
#define SW2		PB2
#define SW3		PB3 

// PC2,PC3,PC4,PC5: JTAG - don't use
#define	JOY_N	PC6
#define JOY_E	PC7
#define JOY_S	PA1
#define JOY_W	PA2
#define JOY_T	PA3

#define	JOY_N_PORT	PORTC
#define JOY_E_PORT	PORTC
#define JOY_S_PORT	PORTA
#define JOY_W_PORT	PORTA
#define JOY_T_PORT	PORTA

#define	JOY_N_PIN	PINC
#define JOY_E_PIN	PINC
#define JOY_S_PIN	PINA
#define JOY_W_PIN	PINA
#define JOY_T_PIN	PINA

#define	JOY_N_DDR	DDRC
#define JOY_E_DDR	DDRC
#define JOY_S_DDR	DDRA
#define JOY_W_DDR	DDRA
#define JOY_T_DDR	DDRA

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))

struct sndfamily{
	unsigned char bank;
	unsigned char prog;
	char name[50];
};

typedef enum{NONE=-1,BUTTON0,BUTTON1,BUTTON2,BUTTON3,JOY_UP,JOY_RIGHT,JOY_DOWN,JOY_LEFT,JOY_PRESS,POT}  INPUT;
	
uint32_t getMicros();
uint32_t getMillis();
void addMillis();


// arduino defines
#define clockCyclesPerMicrosecond() ( F_CPU / 1000000L )
#define clockCyclesToMicroseconds(a) ( (a) / clockCyclesPerMicrosecond() )
#define microsecondsToClockCycles(a) ( (a) * clockCyclesPerMicrosecond() )
#define MICROSECONDS_PER_TIMER0_OVERFLOW (clockCyclesToMicroseconds(64 * 256))
#define MILLIS_INC (MICROSECONDS_PER_TIMER0_OVERFLOW / 1000)
#define FRACT_INC ((MICROSECONDS_PER_TIMER0_OVERFLOW % 1000) >> 3)
#define FRACT_MAX (1000 >> 3)

#define MIDI_PLAY	0
#define MIDI_REC	1
#define SOUND_SEL	2

#endif
