#ifndef _KNOB_H
#define _KNOB_H

#include <stdio.h>

typedef enum knob_state
{
    knob_still,
    knob_right,
    knob_left,
    
};


void knob_init();

enum knob_state knob_get_state();

#endif