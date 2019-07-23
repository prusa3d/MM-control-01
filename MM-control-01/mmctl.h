// @brief High level multimaterial switcher control

#ifndef _MMCTL_H
#define _MMCTL_H

#include <inttypes.h>

extern int active_extruder;
extern bool isFilamentLoaded;

void switch_extruder_withSensor(int new_extruder);
void select_extruder(int new_extruder);
bool feed_filament(bool timeout = false);
void load_filament_withSensor(bool disengageIdler = true);
void load_filament_inPrinter();
void unload_filament_withSensor();
void eject_filament(uint8_t filament);
void recover_after_eject();
void mmctl_cut_filament(uint8_t filament);
bool mmctl_IsOk();

#endif //_MMCTL_H
