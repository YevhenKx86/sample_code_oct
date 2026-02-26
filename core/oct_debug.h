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

        //UARTs
        for (int i = 0; i < NET_LINES_MAX; i++)
        {
            sprintf(dbgstr, "RX%5lu %2lu %3lu TX%5lu %3lu",  OctUarts[i].RxStatBandwidth.CounterSaved, OctUarts[i].RxStatPackets.CounterSaved, OctUarts[i].RxStatSkipped%1000,   OctUarts[i].TxStatBandwidth.CounterSaved, OctUarts[i].TxStatDropped%1000);
            OCT_RENDER_text(framebuf, dbgstr, 4, 10 * i, COLOR_YELLOW);
        }
    }

