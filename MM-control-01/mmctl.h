// @brief High level multimaterial switcher control

#ifndef _MMCTL_H
#define _MMCTL_H

#include <inttypes.h>

extern int active_extruder;
extern bool isFilamentLoaded;

void switch_extruder_withSensor(int new_extruder);
void select_extruder(int new_extruder);
void load_filament_withoutSensor(bool disengageIdler = true);
void load_filament_inPrinter();
void unload_filament_withoutSensor();
void eject_filament(uint8_t filament);
void recover_after_eject();
bool mmctl_IsOk();
void motion_feed_into_mmu(uint16_t steps);


#endif //_MMCTL_H
