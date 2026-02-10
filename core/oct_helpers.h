#pragma once
//--------------------------------------------------------------------------------------------------------
//Collection of definitions, helper tables, settings and service functions, used through an entire engine
//--------------------------------------------------------------------------------------------------------
#include "oct_shared.h"
#include "oct_vars.h"
#include "oct_utils.h"

//Prevents compiler reordering as well as ensure CPU completes all writes before continuing.
//Serves as a guarantee that some counter will be changed strictly after some previous buffer writings would be really completed.
//Such guarantee is needed because different optimizations (compiler or hardware) might mess with instructions effective order.
#ifndef OCT_MEM_BARRIER
#define OCT_MEM_BARRIER  { __asm volatile("" ::: "memory"); __asm volatile("dmb" ::: "memory"); }
#endif

#define OCT_LOG()

//Saturating conversions
#define INT8_SAT(i)   ((i >= 127) ? 127     : ((i <= -128)   ? -128   : (int8_t)i))
#define UINT8_SAT(i)  ((i >= 255) ? 255     : ((i <= 0)      ? 0      : (uint8_t)i))
#define INT16_SAT(i)  ((i >= 32767) ? 32767 : ((i <= -32768) ? -32768 : (int16_t)i))
#define UINT16_SAT(i) ((i >= 65535) ? 65535 : ((i <= 0)      ? 0      : (uint16_t)i))

//Helpers to get frequently used fields from an extended object
#define SPRITE_IDX(_i)      (OctMem[_i * OctStride])
#define SPRITE_PARENT(_i)   (OctMem[_i * OctStride + 1])
#define SPRITE_ORDER(_i)    (OctMem[_i * OctStride + 2])
#define SPRITE_PTR(_i)      ((octSprite_t*)(OctMem + _i * OctStride))


uint32_t    OCT_cubeid()        { return OctCubeId; }
bool        OCT_is_leader()     { return (OctCubeId == CUBEID_SERVER); }
bool        OCT_is_follower()   { return (OctCubeId != CUBEID_SERVER) && (OctCubeId < CUBES_COUNT); }


#undef DMP
#define DMP(...) { }



//Set or get actual dev mode
int OCT_dev_mode(int mode)
    {
        //Negative parameter value is a way just to get current mode
        if (mode >= 0) OctDevMode = mode;
        return OctDevMode;
    }

//Returns system time in ms
uint32_t OCT_time()
    {
        ////Clocks are in 1/32768'th of the second
        //uint32_t time; hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_32K, &time);

        ////Convert seconds and milliseconds separately to prevent 32-bits overflow
        //return (time >> 15) * 1000  +  (((time & 0x7FFF) * 1000) >> 15);

        return RTOS_getTimeMs();
    }

//Wrappers
void OCT_set_state(int newstate)    { OctState = newstate;  OctStateStamp = OCT_time(); }
int  OCT_get_state()                { return OctState;      }
bool OCT_is_state(int state)        { return OctState == state;  }




//Current value is inside some cooldown span; safely handles numerical wrap-arounds
bool OCT_is_cooldown(uint32_t stamp, uint32_t cur, int32_t cooldown)
    {
        int32_t d = (cur - stamp);
        return (d >= 0 && d < cooldown);
    }


//Imitate template for pool with free-list: treat any pool as an array of ints.
//0'th slot is reserved and must not be used outside of these helpers.
//
void OCT_pool_init(int* arr, int item_size_in_ints, int cap)
    {
        memset(arr, 0, 4 * item_size_in_ints * cap);
        for (int i = 0; i < cap; i++) arr[i * item_size_in_ints] = i+1;
        arr[(cap-1)*item_size_in_ints] = 0;
    }
//
int OCT_pool_alloc(int* arr, int item_size_in_ints)
    {
        int freeslotidx = arr[0],  freeslotoffset = freeslotidx * item_size_in_ints;
        if (arr[freeslotoffset] == 0) return 0; //No slots left
        arr[0] = arr[freeslotoffset];
        memset(arr+freeslotoffset, 0, 4*item_size_in_ints);
        arr[freeslotoffset] = freeslotidx;
        return freeslotidx;
    }
//
void OCT_pool_dealloc(int* arr, int item_size_in_ints, int delidx)
    {
        int slotidx = arr[0], closure = delidx;
        arr[closure*item_size_in_ints] = slotidx,  arr[0] = closure;
    }

//Prevents any overflows
void OCT_strcpy(char* dest, int dest_cap, const char* src)
    {
        //[TODO]: invalid call, report
        if (src == NULL) { dest[0] = 0; return; }

        //Copy all chars while can
        for (int n = 0; n < dest_cap; n++)
        {
            dest[n] = src[n];
            if (src[n] == 0) return;
        }

        //End of src wasn't reached
        dest[dest_cap-1] = 0;
    }

//Shows any debug data from app on screen
///void OCT_text(const char* text)                 { OCT_strcpy(OctText, 127, text); }


inline uint32_t OCT_crc(const uint8_t* buf, int len) {
    uint32_t crc = 0xFFFFFFFF;
    while (len--) crc = (crc >> 8) ^ OCT_CRC_TABLE[(crc ^ *buf++) & 0xFF];
    return crc ^ 0xFFFFFFFF;
}

inline uint32_t OCT_crc_continue(uint32_t init_crc, const uint8_t* buf, int len) {
    uint32_t crc = init_crc ^ 0xFFFFFFFF;
    while (len--)
        crc = (crc >> 8) ^ OCT_CRC_TABLE[(crc ^ *buf++) & 0xFF];
    return crc ^ 0xFFFFFFFF;
}

inline uint32_t OCT_crc_byte(uint32_t init_crc, const uint8_t databyte)
{
    uint32_t crc = init_crc ^ 0xFFFFFFFF;
    crc = (crc >> 8) ^ OCT_CRC_TABLE[(crc ^ databyte) & 0xFF];
    return crc ^ 0xFFFFFFFF;
}

void OCT_stat(uint32_t cur_time, int32_t cur_value, octStat_t* s)
    {
        //[TODO]: recover from random corrupted stamp
        //Finish old and begin new timespan
        if (OCT_difference(s->Stamp, cur_time) > 1000 || s->Stamp == 0)
        {
            //Save stats
            s->CounterSaved = s->Counter;
            if (s->Counter == 0) s->Counter = 1;
            s->AvgSaved = s->Avg / s->Counter,  s->MaxSaved = s->Max,  s->MinSaved = s->Min;
            s->Counter = 0,  s->Avg = 0,  s->Max = INT_MIN,  s->Min = INT_MAX;
            s->Stamp = cur_time;
        }

        //Continue collecting stats
        s->Counter++;
        s->Avg += cur_value;
        if (s->Min > cur_value) s->Min = cur_value;
        if (s->Max < cur_value) s->Max = cur_value;
    }

void OCT_per_second(uint32_t cur_time, octStatPerSec_t* s)
    {
        //[TODO]: recover from random corrupted stamp
        //Update stored counter value and start anew counting for another second
        if (OCT_difference(s->Stamp, cur_time) > 1000)
            s->CounterSaved = s->Counter,  s->Stamp = cur_time,  s->Counter = 0;
    }



void OCT_text(int string_index, const char* format, ...)
    {
        //When index is not in real range, just insert new string erasing the latest from array
        if (string_index < 0 || string_index >= DEBUG_STRINGS)
        {
            //Offset other strings
            for (int i = DEBUG_STRINGS-1; i > 0; i--)
                memcpy(OctText[i], OctText[i-1], DEBUG_STRING_CAP);

            //Now there is a space
            string_index = 0;
        }

        //[TODO]: check len, make sure there is a 0

        //
        va_list args;
        va_start(args, format);
        vsnprintf(OctText[string_index], DEBUG_STRING_CAP, format, args);
        va_end(args);
    }