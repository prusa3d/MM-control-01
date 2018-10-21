//! @file
//! @author Alex Wigen

#ifndef _STEPPER_h
#define _STEPPER_h

#include "stdint.h"
#include "Arduino.h"
#include "tmc2130.h"
#include "shr16.h"
#include "AccelStepper.h"

//! @brief Setup and control a stepper
//!
//! 
class Stepper : public AccelStepper
{
    public:
        // Test
        Stepper(volatile uint8_t *step_port, uint8_t step_bit, uint8_t dir_pin, uint8_t axis, bool inverted);
        void wait_move(long);
        void wait_stop();
        bool one_step();
        bool is_stalled();
    private:
        AccelStepper accelstepper;
        bool inverted;
        volatile uint8_t *step_port;
        uint8_t step_bit;
        uint8_t dir_pin;
        uint8_t axis; // Used for TMC chip select
        void step_forward();
        void step_backward();

        // Override AccelStepper:step()
        void step(long);
};

#endif //_STEPPER_h
