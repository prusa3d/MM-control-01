//! @file
//! @author Alex Wigen

#include "stepper.h"

Stepper::Stepper(volatile uint8_t *sport, uint8_t sbit, uint8_t dpin, uint8_t ax, bool inv)
    : AccelStepper(AccelStepper::FUNCTION, 0, 0, 0, 0, false)
{
    step_port = sport;
    step_bit = sbit;
    dir_pin = dpin;
    axis = ax;
    inverted = inv;
}

void Stepper::step(long step)
{
    // Set direction pin
    if ((inverted && speed() > 0) ||
        (!inverted && speed() < 0))
            shr16_set_dir(shr16_get_dir() & ~dir_pin);
    else
            shr16_set_dir(shr16_get_dir() | dir_pin);

    // Pulse one step
    *step_port |= step_bit;
    asm("nop");
    *step_port &= ~step_bit;
    asm("nop");
}

bool Stepper::one_step() {
    // Block until one step is made
    // Returns false when no steps are needed
    if (distanceToGo() == 0)
        return false;

    while (!runSpeed() && distanceToGo() != 0) {}
    // Run AccelStepper::computeNewSpeed() as we are calling runSpeed()
    // directly to block until next step.
    computeNewSpeed();

    return true;
}

void Stepper::wait_move(long steps) {
    // Block until move is complete
    move(steps);
    runToPosition();
}

void Stepper::wait_stop() {
    // Stop as quickly as possible while staying within the deaccelleration limit
    // Blocks until move is complete
    stop();
    runToPosition();
}

bool Stepper::is_stalled() {
    return tmc2130_read_sg(axis) < 6;
}
