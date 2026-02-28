#pragma once
#include "oct_vars.h"

#ifdef SIM_SINGLE_THREAD
#define CUBEID_CONDITION (OctCubeId == 0)
#else
#define CUBEID_CONDITION 1
#endif

//Pointers to bind an external app logic
void (*EXTERNAL_on_init)();
void (*EXTERNAL_on_tick)();
void (*EXTERNAL_on_pretwisted)(int32_t);
void (*EXTERNAL_on_twisted)(int32_t, uint32_t);
void (*EXTERNAL_on_tap)(int32_t);

//Invoke app's callbacks
void OCT_on_init()                                                  {   if (EXTERNAL_on_init != NULL        && CUBEID_CONDITION)    EXTERNAL_on_init(); }
void OCT_on_tick()                                                  {   if (EXTERNAL_on_tick != NULL        && CUBEID_CONDITION)    EXTERNAL_on_tick(); }
void OCT_on_twisted(int32_t twid, uint32_t disconnected_ms)         {   if (EXTERNAL_on_twisted != NULL     && CUBEID_CONDITION)    EXTERNAL_on_twisted(twid, disconnected_ms); }
void OCT_on_tap(int32_t tapid)                                      {   if (EXTERNAL_on_tap != NULL         && CUBEID_CONDITION)    EXTERNAL_on_tap(tapid); }
void OCT_on_pretwisted(int32_t twid)                                {   if (EXTERNAL_on_pretwisted != NULL  && CUBEID_CONDITION)    EXTERNAL_on_pretwisted(twid); }
