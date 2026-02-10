#pragma once
#include "oct_vars.h"
#include "oct_render.h"

void OCT_print_debug_text(uint16_t* framebuf)
    {
        for (int i = 0; i < DEBUG_STRINGS; i++)
            OCT_RENDER_text(framebuf, OctText[i], 0, 220-i*10, COLOR_WHITE);
    }


//
void OCT_print_debug_stats(uint16_t* framebuf, uint32_t vid)
    {
        //FPS
        char dbgstr[100] = {};
        sprintf(dbgstr, "%3ld %3ld %3ld", StatFrametime.MinSaved, StatFrametime.AvgSaved, StatFrametime.MaxSaved);
        OCT_RENDER_text(framebuf, dbgstr, 140, 229, COLOR_LIGHT_GREEN);
    }

