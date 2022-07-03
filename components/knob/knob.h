#ifndef _KNOB_H
#define _KNOB_H

#include <stdio.h>

typedef enum knob_state
{
    knob_still,
    knob_left,
    knob_right,
    
};


void knob_init();

enum knob_state knob_get_state();

#endif