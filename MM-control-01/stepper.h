// @file
// @brief Low level stepper routines

#ifndef STEPPER_H
#define STEPPER_H

#include "config.h"
#include <inttypes.h>

extern int8_t filament_type[EXTRUDERS];
extern float filament_load_speed[EXTRUDERS];
extern float filament_unload_speed[EXTRUDERS];

void home();
bool home_idler();

int get_idler_steps(int current_filament, int next_filament);
int get_selector_steps(int current_filament, int next_filament);

void park_idler(bool _unpark);

void do_pulley_step();
void set_pulley_dir_pull();
void set_pulley_dir_push();
void move_proportional(int _idler, int _selector);

#endif //STEPPER_H

