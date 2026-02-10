//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//DO NOT TRY TO INCLUDE THIS FILE FROM __ANY__ .C OTHER THAN MAIN.C
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#pragma once
#include "oct_types.h"


uint16_t                            OctClearColor;
int                                 OctClearEnabled;

uint32_t                            OctSeed = 1;                                        //Current PRNG state
int                                 OctState = OCT_STATE_DISABLED;                      //Engine state
uint32_t                            OctStateStamp = 0;                                  //When did last state changed
uint32_t                            OctSyncedTick = 1;

uint32_t                            OctStampLastRun = 0;
int                                 OctStatStamp = 0;
int                                 OctStatLastTick = 0;
octStat_t                           StatFrametime;

int*                                OctMem = 0;                                         //Pool of extended scene objects, shared with app
int                                 OctCap = 0;                                         //How many objects current app decided to use
int                                 OctStride = sizeof(octSprite_t) / 4;                //App can extend object, so actual structure size can be greater than sizeof(octSprite_t) and should be updated on app init
octLabel_t                          OctLabels[OCT_LABELS_CAP];                          //Content of label texts

uint32_t                            OctCubeId = CUBEID_UNDEFINED;                       //Persistent cube id from 0 to 7
uint32_t                            OctDevMode = 0;                                     //Developer mode flags are helping to debug engine from app's side - without recompiling firmware
char                                OctText[DEBUG_STRINGS][DEBUG_STRING_CAP] = {0};     //Debug strings

int                                 OctDisconnectionPlane = 0;
int                                 OctSchedulerRunning;                                    //Mutexes can't be used before FreeRTOS scheduler is active, so this flag is used to track 

//Render
octTm_t                             OctCachedTms[OCT_SPRITES_CAP];                          //Partially transformed sprites, used to speed up culling and sorting
int                                 OctSortBuf[OCT_SPRITES_CAP];                            //Sprites sorted on each frame
int                                 OctTempBuf[OCT_SPRITES_CAP];                            //Temp array for sorting
int                                 OctPalsDataNum;
int                                 OctPalsSwap[OCT_PALS_CAP];                              //Dynamically changes bitmap palette for special effects
ATTR_RWDATA_IN_RAM uint32_t         OctPalsData[16*256];                                    //Table keeping items of all palettes
ATTR_RWDATA_IN_RAM uint32_t         OctBack[HALFSIDE*HALFSIDE];                             //Back buffer
ATTR_RWDATA_IN_RAM uint32_t         OctMix[MIX_ROWS*MIX_COLS];                              //Mixing buffer for effects
ATTR_RWDATA_IN_RAM uint32_t         OctCache[256];                                          //Last unpacked texels (usually it is color indices, not actual colors)
octViewport_t                       OctViewports[VIEWPORTS_COUNT];                          //Virtual cube consisting of 6 sides with 4 viewports each; i-th viewport means always the same fixed place on virtual cube. Maps scene space to screens.
int                                 OctTopo[24];                                            //Actual topology, same would be set to OctViewports[].Sid but with a delay due to OCT_LAG_COMPENSATION_TICKS and FPS. Not used in app logic; needed only to track topology changes.
octPal_t                            OctPals[OCT_PALS_CAP];                                  //Palette descriptors
int                                 OctPalsNum = 0;                                         //Number of active palettes

//Border thickness control in screen pixels, minimal valid value is 2
float   INNER_DELTA = 18, OUTER_DELTA = 12, LIM = SIDE+18+12;

