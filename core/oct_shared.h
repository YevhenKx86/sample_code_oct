#pragma once
//----------------------------------------------------------------------------------------------------
// Everything in this header is used both in engine and on app side.
//----------------------------------------------------------------------------------------------------

#ifndef NULL
#define NULL 0
#endif

#include <stdint.h>

//Renderer transparency
#define OCT_TRANSP_MAX 31
const int OCT_TRANSP_HALF = OCT_TRANSP_MAX/2;


//Animator
enum ANIM_FUNC  { FUNC_NONE = 0, FUNC_ACC, FUNC_DEC, FUNC_LINEAR, FUNC_QUAD, FUNC_FALL, FUNC_BOUNCEOUT, FUNC_JELLY, FUNC_SPAWN  }; //FUNC_FORCED4B = 0xF03CED4B
enum ANIM_FIELD { ANIM_TM = 1, ANIM_TM_REL };       //position, rotation, tm, float, color, alpha, time

//
enum SCHEME { SCHEME_CUBE = 0 };

//Engine on simulator is multithreaded so it's global vars must use this attribute
#ifdef OCTSIM
    #define TL thread_local
#else
    #define TL
#endif


//Flags to control different debugging features from app
const int OCT_DEV_IMU_MINMAX    = 1<<0;
const int OCT_DEV_STAT          = 1<<1;
const int OCT_DEV_TEXT          = 1<<2;
const int OCT_DEV_FPS           = 1<<3;
const int OCT_DEV_NO_RASTER     = 1<<4;
const int OCT_DEV_COLLIDERS     = 1<<5;
const int OCT_DEV_IMU_ACC       = 1<<6;
const int OCT_DEV_BENCH         = 1<<7;
const int OCT_DEV_NETSTAT       = 1<<8;
const int OCT_DEV_CLEAR_SCREEN  = 1<<9;
const int OCT_DEV_QUAD_IDS      = 1<<10;

//Coordinate planes of the scene
const int8_t OCT_PLANE_TOP    = 0;
const int8_t OCT_PLANE_FRONT  = 1;
const int8_t OCT_PLANE_RIGHT  = 2;
const int8_t OCT_PLANE_BACK   = 3;
const int8_t OCT_PLANE_LEFT   = 4;
const int8_t OCT_PLANE_BOTTOM = 5;
const int8_t OCT_PLANE_WITH_BORDER = 10;

#define SIDES_COUNT 6
#define SECTORS_COUNT 4
#define VIEWPORTS_COUNT (SIDES_COUNT*SECTORS_COUNT)
const int FACE_TOP = 0, FACE_FRONT = 1, FACE_RIGHT = 2, FACE_BACK = 3, FACE_LEFT = 4, FACE_BOTTOM = 5;
const int SIDE_TOP = 0, SIDE_FRONT = 1, SIDE_RIGHT = 2, SIDE_BACK = 3, SIDE_LEFT = 4, SIDE_BOTTOM = 5;


//Display
#define HALFSIDE 120
const int       SIDE                = 240;
const int       BORDER              = SIDE - 1;

//Logic works with fixed frametime
const int       OCT_FRAME_MS        = 50;
const int       OCT_1SEC_TICKS      = 1000 / OCT_FRAME_MS;
const float     OCT_DT              = OCT_FRAME_MS / 1000.0f;

//Quadrants or angle mappings
const int XMOVE[4] = {1, 0, -1, 0};
const int YMOVE[4] = {0, 1, 0, -1};
const int XSIGN[4] = {+1, -1, -1, +1};
const int YSIGN[4] = {+1, +1, -1, -1};


const int ALIGN_CENTER  = 0;
const int ALIGN_LEFT    = 1;
const int ALIGN_RIGHT   = 2;
const int FONT_1  = 1;
const int FONT_2  = 2;
const int FONT_3  = 3;


//___________________________________________________________________________________________________________________________________________
//Just an id, but also index into OCT_TWISTS table
typedef int octTwistId_t;

enum TWIST_ID { TOP_CCW  = 0, TOP_CW  = 1,  FRONT_CCW = 2, FRONT_CW = 3,  RIGHT_CCW  = 4,  RIGHT_CW  = 5, //CCW or CW is determined looking from outside of the cube
                BACK_CCW = 6, BACK_CW = 7,  LEFT_CCW  = 8, LEFT_CW  = 9,  BOTTOM_CCW = 10, BOTTOM_CW = 11 };

enum AXIS_ID { AXIS_NONE = 0, AXIS_X = 1, AXIS_Y = 2, AXIS_Z = 3};

const octTwistId_t OCT_TWIST_HALF = 12, OCT_TWIST_ALL = 24, OCT_TWIST_NONE = -1;
//0..11 - usual twists
//12..23 - half twists
//24 - consider everything changed
//-1 - no changes


//OCT_anim restart parameter []
typedef int octSeqRestart_t;

const octSeqRestart_t OCT_SEQ_RESTART = 1, OCT_SEQ_REVERSE = -1, OCT_SEQ_REFRESH = 0;

//Size MUST be multiple of 4.
#pragma pack(push, 4)
typedef struct
{
    float   X, Y;
    int16_t A;
    int8_t  Plane;
    int8_t  XFlip:4, YFlip:4;
} octTm_t;
#pragma pack(pop)


//Only used to pass bitmap info to the wasm side
#pragma pack(push, 4)
typedef struct
{
    char        Name[24];               //Sprite name in pack
    float       PivotX, PivotY;         //Pivot point relative to top left corner
    float       Bx, By, Bw, Bh;         //Bounding geometry
    int         NumPixels;              //Can be used as visual mass estimation
    uint32_t    Tags;                   //Additional data about image
    int16_t     W, H;                   //Width and height in screen pixels
    int16_t     Number;
    uint8_t     Group;
    uint8_t     Type;
} octBmpInfo_t;
#pragma pack(pop)


//Scene object, can be extended on the application's side. Size MUST be multiple of 4.
#pragma pack(push, 4)
typedef struct
{
    int         Idx;                            //MUST be the first 4 bytes. Unique key.
    int         Parent;                         //MUST be the second 4 bytes. Index of parent object, or zero.
    int         Order;                          //MUST be the third 4 bytes. Internal key for sorting purposes.
    octTm_t     Tm;                             //Local transform
    uint32_t    Mask:24;                        //Bitmask of which cameras are not allowed to draw this object
    uint32_t    DynPal:8;                       //Offset of dynamic palette colors. Zero means use default (bmp's) palette. Each +1 offset means +32 array items (ints).
    int16_t     Zw, Zh;                         //These are object's, not bitmap's: i.e. label keeps here it's text zone dimensions
    uint8_t     Lut;                            //Bmp index for special effects
    int8_t      Param1;                         //
    int8_t      Param2;                         //
    int8_t      Param3;                         //

    uint16_t    Data0;
    uint8_t     Data1, Data2;

    uint32_t    Tags;                           //Custom app-related bitmask of TAG_* literals; usually set when map is loading
    int16_t     Number;                         //Custom number; usually some constant from map
    uint16_t    AgeTicks;                       //Lifetime
    uint16_t    AgeMark;                        //When autokill is true, this marks age when sprite will be deleted; when false - marks any app-related period
    uint16_t    Twistable:1,                    //Will be moved with twist in corresponding direction (by default object ignores any twists)
                Hidden:1,                       //Won't be rendered
                Autokill:1,
                ShowCollider:1,
                FlipH:1,
                FlipV:1,
                Label:2,
                Letter:1,
                Fresh:1,
                Loop:1,
                PingPong:1,
                Line:1;


    //Frame animation
    int16_t     FrameFrom, FrameTo;             //Actual frame animation sequence
    int16_t     Frame;
    uint8_t     FramesCooldown;                 //How many frames to wait after reaching FrameTo
    uint8_t     FAccum;
    int8_t      FrameRate;
    uint8_t     Reverse:1, Paused:1, InDelay:1, InCooldown:1, OnCooldown:1, OnEnd:1;

    uint8_t     Type;                           //Custom and app-related type of the object; usually set when map is loading to TYPE_* constant
    uint8_t     Name;                           //Id that allows to bind app logic to this specific object; usually set to a NAME_* constant when map is loading
    uint8_t     Group;                          //Id that marks subset of the scene; usually set to a GROUP_* constant when map is loading

    int8_t      Layer;                          //Z-order - higher value layers are drawn over lower ones
    uint8_t     Transp:5;                       //Transparency, actual range is [0 .. 31 (OCT_TRANSP_MAX)]

} octSprite_t;
#pragma pack(pop)
