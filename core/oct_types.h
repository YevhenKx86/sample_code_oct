#pragma once
#include <stdint.h>
#include "oct_shared.h"
#include "oct_consts.h"

#ifdef OCTSIM
#include "sim_hal_windows.h"    //SemaphoreHandle_t
#else
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#endif

//Data from static map to spawn runtime object
typedef struct
{
    float       X, Y;       //Coordinates in side space \ parent space
    uint32_t    Tags;       //Custom set of bits
    int16_t     W, H;       //Used in non-trivial cases
    int16_t     BmpIdx;     //Assigned bitmap; can be zero for helper objects
    int16_t     Number;     //Custom number

    uint16_t    Twistable:1, Looped:1, Hidden:1, Paused:1, PingPong:1, Label:2, FlipH:1, FlipV:1, Rot:2;
    int8_t      Side;       //Side of the cube or -1 when object is attached to another one
    int8_t      Rate;       //Frame time for animated sprites: 0 = default (taken from bmp itself), 1.. = ticks (50ms..)

    uint8_t     Name;       //Custom and unique for this map place ID
    uint8_t     Group;      //Custom and non unique ID; can be used to mark scene subset
    uint8_t     Parent;     //ID (same as name) of the parent place, or 0
    uint8_t     Type;       //Custom and non unique ID
} octPlace_t;


//Container to hold text, used by label objects
typedef struct
{
    int         Idx;
    char        Text[OCT_LABEL_LEN];
} octLabel_t;


//Virtual viewport links scene and display: it maps to scene plane ('layout' part), and it binds with screen ('topology' part)
typedef struct
{
    octTm_t     Tm;                     //View transform, kind of camera looking into 6-planes-space scene and projecting the result to whatever screen is currently attached via Sid
    uint32_t    Sid;                    //Render outputs pixels to this screen id
    int         Mode;                   //Normal, Disabled, Grayscale
} octViewport_t;


//[TODO]: Absolute time replace with delta
typedef struct
{
    int     ValueMin, ValueMax, ValueAutoRange;
    int     TimePerPixel;
    int     X, Y, W, H;
    uint16_t Color, Enabled, ShowNumbers;

    int     Offset;
    int     TimeMs[OCT_DEBUGRAPH_CAP];
    int8_t  Values[OCT_DEBUGRAPH_CAP];  //Normalized to +-100%
    //uint8_t   Colors[OCT_DEBUGRAPH_CAP];
} octGraph_t;



//Index in resource cache
typedef int octBmpId_t;


//Bitmap descriptor
typedef struct
{
    ///int         PackerSizes;            //[NOTE]: Container W and H, unused by engine
    int         NumPixels;              //Effective pixels count
    float       PivotX, PivotY;         //Origin of the bitmap in texture coordinates
    float       Bx, By, Bw, Bh;         //Bounding volume center (relative to pivot) and it's half-size
    uint32_t    Tags;
    uint32_t    Compression;            //Reserved | Reserved | Offset Bitness | Symbol Bitness
    int16_t     W, H;                   //Effective bitmap W and H
    int16_t     Number;
    uint8_t     Group;
    uint8_t     Type;
    uint8_t     Flags;                  //Rendering properties
    uint8_t     Pidx;                   //Palette index
    int8_t      Seq;
    int8_t      Rate;
} octBmp_t;


//Palette descriptor
typedef struct
{
    uint8_t     Id, Blend, K, Anims;    //K is palette size minus one. Other fields are not used yet.
    uint32_t    Flags;
    int         ColorsBufferIndex;      //Index in buffer containing all used palette colors (OctPalsData)
} octPal_t;

//Wrapper to collect stats about some value
typedef struct
{
    uint32_t    Stamp, Counter, CounterSaved;
    int32_t     Avg, Min, Max;
    int32_t     AvgSaved, MinSaved, MaxSaved;
} octStat_t;

typedef struct
{
    uint32_t    Stamp, Counter, CounterSaved;
} octStatPerSec_t;


//Following structs are tightly packed
#ifdef __GNUC__
    #define ATTR_PACKED __attribute__((packed))
#else 
    //Instead of __attribute__((packed))
    #define ATTR_PACKED
    #pragma pack(push, 1)
#endif



#ifndef __GNUC__
#pragma pack(pop)
#endif


typedef struct
{
    int                     Way[6];                             //Unpacked data from W field
    int                     WayLen;
    int                     Disconnected;

    uint32_t                Seq;
    uint32_t                SenderStampMs;
    uint32_t                ReceiverStampMs;            //[TODO]: Set this stamp right on packet arrival, and then probably mutex wrapping is needed (both stamps must be actual)
    uint32_t                W;
    uint32_t                LatestUnprocessedCmdSeq;
    uint32_t                LatestProcessedCmdSeq;

    int32_t                 ReportingResult;            //[NOTE]: Always write this field before the next field
    uint32_t                ReportingCmdSeq;            //Read and check before using previous field

    uint32_t                BtSeq;
    uint32_t                ProcessedBlobSeq;
    uint32_t                UnprocessedBlobSeq;

    octStat_t               StatInfoDeliveryLag;

} octInfo_t;

