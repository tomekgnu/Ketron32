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
INPUT readInputs(unsigned char *,unsigned char[]);

#endif /* HANDLERS_H_ */