//!
//! @file
//! @author Marek Bel

#include "motion.h"
#include "stepper.h"

static uint8_t s_idler = 0;
static uint8_t s_selector = 0;

void motion_set_idler_selector(uint8_t idler_selector)
{
    motion_set_idler_selector(idler_selector, idler_selector);
}

void motion_set_idler_selector(uint8_t idler, uint8_t selector)
{
    int idler_steps = get_idler_steps(s_idler, idler);
    int selector_steps = get_selector_steps(s_selector, selector);

    move_proportional(idler_steps, selector_steps);
    s_idler = idler;
    s_selector = selector;
}

void motion_engage_idler()
{
    park_idler(true);
}

void motion_disengage_idler()
{
    park_idler(false);
}
