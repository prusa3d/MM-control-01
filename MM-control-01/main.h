#ifndef _MAIN_H
#define _MAIN_H


#include <inttypes.h>
#include <stdio.h>

void manual_extruder_selector();
void unrecoverable_error();

extern uint8_t tmc2130_mode;
extern FILE* uart_com;

#endif //_MAIN_H
