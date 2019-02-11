#ifndef _MAIN_H
#define _MAIN_H


#include <inttypes.h>
#include <stdio.h>

void manual_extruder_selector();
void unrecoverable_error();
void drive_error();
void check_filament_not_present();
void filament_presence_signaler();

extern uint8_t tmc2130_mode;
extern FILE* uart_com;

#endif //_MAIN_H
