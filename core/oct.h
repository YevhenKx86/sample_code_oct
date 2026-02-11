#pragma once

#include <assert.h>
#include "oct_vars.h"
#include "oct_tm.h"
#include "oct_anim.h"
#include "oct_render.h"
#include "oct_pack.h"
#include "oct_debug.h"
#include "oct_embed.h"
#include "oct_app.h"
#include "oct_net.h"
#include "oct_net_cmds.h"
#include "oct_net_msgs.h"
#include "oct_net_signals.h"
#include "oct_net_blobs.h"
//----------------------------------------------------------------------------
static bool loaded = false;

ATTR_RWDATA_IN_PSRAM_4BYTE_ALIGN static uint8_t app_data[APP_BENCH_SIZE];

uint8_t * load_app(void){

    if(loaded){
        return app_data;
    }
    loaded = true;

    memcpy(app_data, app_bench_data, APP_BENCH_SIZE);

    //BK_LOGI(NULL,"%s: loaded app data %p\r\n", __func__, app_data);

    return app_data;
}
//----------------------------------------------------------------------------
void OCT_set_embed_pack(const uint8_t* embed)
    {
        //Bind pack
        /*OctPack = embed;
        OctPackHeader = (octPackHeader_t*)OctPack; 
        OctPackAssets = (octAssetDesc_t*)(OctPack + sizeof(octPackHeader_t));*/ 

        OctPack = load_app();
        OctPackHeader = (octPackHeader_t*)OctPack; 
        OctPackAssets = (octAssetDesc_t*)(OctPack + sizeof(octPackHeader_t)); 
    }

//________________________________________________________________________________________________________________________________________________
//This is inlined game code; the game's assets are embedded (in oct_embed.h)

enum BMP { BMP_none = 0, 
BMP_0 = 0, 
BMP_back = 1, 
BMP_cau_00 = 2, 
BMP_cau_01 = 3, 
BMP_cau_02 = 4, 
BMP_cau_03 = 5, 
BMP_cau_04 = 6, 
BMP_cau_05 = 7, 
BMP_cau_06 = 8, 
BMP_cau_07 = 9, 
BMP_cau_08 = 10, 
BMP_cau_09 = 11, 
BMP_cau = 2, 
BMP_cau_end = 11, 
BMP_fish1 = 12, 
BMP_last};

enum MAP { MAP_none = 0, 
MAP_last};

typedef enum BMP BMP;

#define OBJECTS_CAP 400     //Scene capacity - maximum objects count possible
#define GAP         18      //Width of physical border between wowcube's display in pixels

//Add game specific data to scene objects
typedef struct
{
    octSprite_t     Base;
    octTm_t         TmStart, TmEnd;
    int             MovementTick;
} appObject_t;

//[NOTE]: All global variables should be defined with TL macro
TL appObject_t  Objects[OBJECTS_CAP];


void on_init_bench() 
    {
        OCT_restart((int*)Objects, OBJECTS_CAP, sizeof(appObject_t));
        OCT_viewports_layout(SCHEME_CUBE, GAP, GAP);

        for (int q = 0; q < 24; q++)
        {
            int idx = OCT_add(10, true, q/4, 120.0f * XSIGN[q%4], 120.0f * YSIGN[q%4], 0, false, BMP_back, BMP_back, 0);
            Objects[idx].Base.Type = 0;//TYPE_back
        }

        for (int q = 0; q < 24; q++)
        {
            int idx = OCT_add(30, true, q/4, 120.0f * XSIGN[q%4], 120.0f * YSIGN[q%4], 0, false, BMP_fish1, BMP_fish1, 0);
            Objects[idx].Base.Type = 0;//TYPE_fish
        }

        //[NOTE]: Pivot of these sprites is in top-left corner, not center
        for (int q = 0; q < 24; q++)
        {
            int idx = OCT_add(40, false, q/4, 120.0f * XSIGN[q%4] - 120, 120.0f * YSIGN[q%4] + 120, 0, true, BMP_cau, BMP_cau_end, 2);
            Objects[idx].Base.Type = 0;//TYPE_cau
        }    

        OCT_text(-1, "App inited");
    }

//________________________________________________________________________________________________________________________________________________





void OCT_trace(int cubeid, const char* format, ...)
    {
        (void)cubeid, (void)format; 
    }


//16-bit biased LCG, never gives dmax (unless it equals to dmin of course)
int OCT_random(int dmin, int dmax)
    {
        OctSeed = (214013*OctSeed+2531011);
        return dmin + (int)((dmax - dmin) * ((OctSeed>>16)&0x7FFF) / (float)(0x7FFF + 1));
    }


//Runtime error
void OCT_terminate(const char* txt)
    {
        OCT_text(-1, txt);
        (void)txt; 
    }


void OCT_viewports_unbind() { for (int i = 0; i < SCREENS_COUNT; i++) OctTopo[i] = OctViewports[i].Sid = OCT_SID_NONE; }


int OCT_disconnected_axis() { return OctDisconnectionPlane;  }


//Sets the most trivial cameras layout (maps 6 planes to 6 sides)
void OCT_layout_default()
    {
        //[NOTE]: Transform values should be already inverted
        for (int side = 0; side < SIDES_COUNT;   side++)
        for (int sect = 0; sect < SECTORS_COUNT; sect++)
        {
            octViewport_t* vp = &OctViewports[side * SECTORS_COUNT + sect];
            vp->Mode = 0,  vp->Tm.X = vp->Tm.Y = -INNER_DELTA,  vp->Tm.A = -90 * (int16_t)sect,  vp->Tm.Plane = (int8_t)side,  vp->Tm.XFlip = vp->Tm.YFlip = 1;
        }
    }


//Allows to customize scene layout and visualization
void OCT_modify_viewport(int vid, int plane, int angle, float x, float y, int xflip, int yflip, int mode)
    {
        octViewport_t* vp = &OctViewports[vid];
        vp->Tm.Plane = (int8_t)plane,  vp->Tm.A = (int16_t)angle,  vp->Tm.X = x,  vp->Tm.Y = y,  vp->Tm.XFlip = xflip,  vp->Tm.YFlip = yflip;
        if (mode >= 0) vp->Mode = mode;
    }


//Setup cameras
void OCT_viewports_layout(int scheme, int inside_border_width, int outside_border_width)
    {
        if (scheme == SCHEME_CUBE)
            INNER_DELTA = (float)inside_border_width + 0.01f,  OUTER_DELTA = (float)outside_border_width,  LIM = (float)(240+inside_border_width+outside_border_width),  OCT_layout_default();
    }


//Setup engine once per OS session. Call must be made after hal_clock_init()
void OCT_init()
    {
        OCT_set_state(OCT_STATE_DISABLED);

        OCT_viewports_unbind(), OCT_viewports_layout(SCHEME_CUBE, 18, 18);

        OCT_NET_restart();
    }


//Engine reset
int OCT_restart(int* sprites, int spritescap, int spritesize)//, int32_t* anims, int animscap, void* vars, int varssize)
    {
        //Assert
        if ((spritesize % 4) != 0) { OCT_terminate("Object's type size must be multiply of 4"); return 0; }

        //Application can choose to use less than maximum number of sprites
        if (spritescap > OCT_SPRITES_CAP) { OCT_terminate("More than OCT_SPRITES_CAP sprites is not supported"); return 0; }

        OCT_RENDER_clear(COLOR_GRAY);

        //Load palettes
        OctPalsNum = 0;
        int palres = OCT_PACK_getSpriteIdByName("pal");
        if (palres < 0) { OCT_terminate("pal (.png) was not found"); return 0; }

        OCT_RENDER_clear(COLOR_LIGHT_GRAY);

        //Prepare palettes
        uint32_t* palnum = (uint32_t*)OCT_PACK_getSprite(palres);

        OctPalsDataNum = 0;
        octPal_t* pal = (octPal_t*)(palnum+1);
        uint32_t* palcolors = (uint32_t*)(pal + *palnum);
        for (uint32_t i = 0; i < *palnum; ++i)
        {
            //Copy descriptors
            memcpy(&OctPals[OctPalsNum++], &pal[i], sizeof(octPal_t));

            //Copy pixels
            memcpy(&OctPalsData[pal[i].ColorsBufferIndex], &palcolors[pal[i].ColorsBufferIndex], sizeof(int) * (pal[i].K+1));

            //[TODO]: put to file
            OctPalsDataNum += (pal[i].K+1);
        }


        //Clear sprites
        OctMem = sprites,  OctCap = spritescap,  OctStride = spritesize/4,  OCT_clear();

        OCT_ANIM_restart();

        //App inited
        OCT_set_state(OCT_STATE_RUNNING);
        OCT_RENDER_clear(COLOR_VIOLET);
        return 1;
    }


//Fixed-step update
void OCT_tick(uint32_t curtick)
    {
        //Update sprites
        if (OCT_get_state() == OCT_STATE_RUNNING)
        {
            for (int j = 1; j < OctCap; j++)
            {
                octSprite_t* s = SPRITE_PTR(j);
                if (s->Idx == j) OCT_tick_sequence(s);
            }

            //Call app logic
            OCT_on_tick();
        }
    }


//Visualize different states
static void OCT_render(uint8_t display, int vid, uint16_t* framebuf)
    {
        if (OctState == OCT_STATE_MEETING || OctState == OCT_STATE_CONSOLE || OctMem == NULL)
        {
            //Clear screen texture
            uint32_t* framebuf4 = (uint32_t*)framebuf;
            for (int i=0; i < 240; i++) for (int j=0; j < 120; j++) framebuf4[i*120+j] = 0x08080808;
            return;
        }

        //No scene to render
        if (OctMem == NULL) return;

        //Slows render down, so an actual game shouldn't be using this (only to test something)
        if (OctClearEnabled)
        {
            //[TODO]: use 32-bit
            for (int i=0; i < HALFSIDE; i++) for (int j=0; j < HALFSIDE; j++)
                OctBack[i*HALFSIDE+j] = OctClearColor;
        }
        else
        if (OctDevMode & OCT_DEV_CLEAR_SCREEN) {
            memset(OctBack, 0x01, HALFSIDE*HALFSIDE*sizeof(OctBack[0]));
        }

        //Playing
        OCT_RENDER_scene(vid, framebuf);
    }


//Main engine function
void OCT_run()
    {
        //Safeguard allowing run system code without OS
        if (OCT_is_state( OCT_STATE_DISABLED)) return;

        //Stamp new frame
        uint32_t cur_time = OCT_time();
        OCT_stat(cur_time, cur_time - OctStampLastRun, &StatFrametime);
        OctStampLastRun = cur_time;

        //Meeting the neighbors
        if (OCT_get_state() == OCT_STATE_MEETING)
        {
            //Incomplete mode - when time out and module still alone - show console, enable BT, allow dev scenarios
            if (!OCT_is_cooldown(OctStateStamp, cur_time, 1000))
            {
                OCT_set_state(OCT_STATE_INCOMPLETE);
                
                //Single module
                if (OctCubeId == CUBEID_UNDEFINED) OctCubeId = 0;
        
            }
        }


        //State check disallows ticking until cube gets out of meeting, meaning all 7 other modules are found and working (or timed out and we are in incomplete mode)
        if (OCT_is_state(OCT_STATE_INCOMPLETE) || OCT_get_state() >= OCT_STATE_CONSOLE)
        {
            //Extrapolate what time is it on server, knowing last stamps from when we synchronized our clocks
            uint32_t extrapolated_time = cur_time;

            //How many ticks we are falling behind
            int ticks = extrapolated_time/OCT_FRAME_MS - OctSyncedTick;

            //[NOTE]: This is benchmark hack, when we see another module THEN run the game
            if ((OCT_is_state(OCT_STATE_INCOMPLETE) && (OctHwidsNum == 2))  ||  OCT_is_state(OCT_STATE_CONSOLE)) 
            {
                OCT_set_embed_pack(app_bench_data);
                EXTERNAL_on_init = on_init_bench;
                OCT_set_state(OCT_STATE_APP);
                OCT_on_init();
            }


            //Fixed time steps
            while (ticks > 0)
            {
                OCT_tick(OctSyncedTick);

                OCT_ANIM_update();          

                ticks--, OctSyncedTick++;
            }
        }


        {
            //Output scene to cube displays
            for(uint8_t d = 0; d < DISPLAY_COUNT; d++)
            {
                //Find camera bound to this display
                uint16_t* framebuf = OCT_DISPLAY_framebuffer(d);
                uint32_t cur_sid = 3 * OCT_cubeid() + d;

                //Find screen's binding
                uint32_t vid = cur_sid;

                //Draw the scene
                OCT_render(d, vid, framebuf);

                //Visualize isolation state
                //if (OctState == OCT_STATE_INCOMPLETE) OCT_RENDER_rect(framebuf, 120, 120, 120, 120, 0, COLOR_RED);

                //Draw console messages
                OCT_print_debug_text(framebuf);
                
                //Draw FPS and  any other stats
                OCT_print_debug_stats(framebuf, vid);

                //Upload framebuffer
                OCT_DISPLAY_blit(d);
            }
        }
    }