#pragma once
//----------------------------------------------------------------------------------------------------
// Everything in this header can be included from any .c:
// - "static" makes it's own internal copy for each .c
// - "unused" silence warnings when some module isn't really using all functions
//----------------------------------------------------------------------------------------------------
#include <stdint.h>
#include <limits.h>

#ifdef __GNUC__
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif

UNUSED static uint32_t OCT_min_u32(uint32_t a, uint32_t b)
    {
        return (a < b) ? a : b;
    }


//Safe againts long uptime wrap-arounds
UNUSED static int32_t OCT_difference(uint32_t prev, uint32_t cur)
    {
        //[NOTE]: Signed difference is well defined only when delta is less than half of 32-bit range; such extreme cases should probably be processed before this function
        return (cur - prev);
    }


