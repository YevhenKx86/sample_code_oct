//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//DO NOT TRY TO INCLUDE THIS FILE FROM __ANY__ .C OTHER THAN MAIN.C
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#pragma once
#include "oct_types.h"


TL  uint16_t                    OctClearColor;
TL  int                         OctClearEnabled;
TL  int                         OctBacklightPercent = 30;

TL  uint32_t                    OctSeed = 1;                                        //Current PRNG state
TL  int                         OctState = OCT_STATE_DISABLED;                      //Engine state
TL  uint32_t                    OctStateStamp = 0;                                  //When did last state changed
TL  uint32_t                    OctSyncedTick = 1;
TL  uint32_t                    OctStampLastRun = 0;                                  //When did last state changed

TL  int                         OctDownloadState = 0;

TL  octStat_t                   StatFrametime;
TL  octStat_t                   StatUartDmaRxData[3];

TL TaskHandle_t                                                 OctTaskParseTraffic = 0;

TL  int*                        OctMem = 0;                                     //Pool of extended scene objects, shared with app
TL  int                         OctCap = 0;                                     //How many objects current app decided to use
TL  int                         OctStride = sizeof(octSprite_t) / 4;            //App can extend object, so actual structure size can be greater than sizeof(octSprite_t) and should be updated on app init
TL  octLabel_t                  OctLabels[OCT_LABELS_CAP];                      //Content of label texts

//
TL  uint32_t                    OctCubeId = CUBEID_UNDEFINED;                   //Persistent cube id from 0 to 7
TL  uint32_t                    OctDevMode = 0;                                 //Developer mode flags are helping to debug engine from app's side - without recompiling firmware
TL  char                        OctText[DEBUG_STRINGS][DEBUG_STRING_CAP] = {0};          //Debug strings

TL  int                         OctBufferedAs[3][OCT_IMU_CAP];                  //Recent acc measurements [-128 .. +127]
TL  int                         OctBufferedGs[3][OCT_IMU_CAP];                  //Recent gyro measurements [-128 .. +127]
TL  uint32_t                    OctBufferedMs[OCT_IMU_CAP] = {0};               //Local time in ms when resp. measurement was recorded
TL  int                         OctBufferedLast = 0;                            //Index of the last (the most recent) measurement
TL  uint32_t                    OctBufferedStamp = 0;                           //Last time (in ms) when accumulation was switched to a new sample
TL  int                         OctInputSamplesNum = 1;                         //Now many IMU samples were taken in current delta time span


//Render
TL octTm_t                                                      OctCachedTms[OCT_SPRITES_CAP];                          //Partially transformed sprites, used to speed up culling and sorting
TL int                                                          OctSortBuf[OCT_SPRITES_CAP];                            //Sprites sorted on each frame
TL int                                                          OctTempBuf[OCT_SPRITES_CAP];                            //Temp array for sorting
TL int                                                          OctPalsDataNum;
TL int                                                          OctPalsSwap[OCT_PALS_CAP];                              //Dynamically changes bitmap palette for special effects
TL ATTR_RWDATA_IN_RAM uint32_t                                  OctPalsData[16*256];                                    //Table keeping items of all palettes
TL ATTR_RWDATA_IN_RAM uint32_t                                  OctBack[HALFSIDE*HALFSIDE];                             //Back buffer
TL ATTR_RWDATA_IN_RAM uint32_t                                  OctMix[MIX_ROWS*MIX_COLS];                              //Mixing buffer for effects
TL ATTR_RWDATA_IN_RAM uint32_t                                  OctCache[256];                                          //Last unpacked texels (usually it is color indices, not actual colors)
TL octViewport_t                                                OctViewports[VIEWPORTS_COUNT];                          //Virtual cube consisting of 6 sides with 4 viewports each; i-th viewport means always the same fixed place on virtual cube. Maps scene space to screens.
TL int                                                          OctTopo[24];                                            //Actual topology, same would be set to OctViewports[].Sid but with a delay due to OCT_LAG_COMPENSATION_TICKS and FPS. Not used in app logic; needed only to track topology changes.
TL octPal_t                                                     OctPals[OCT_PALS_CAP];                                  //Palette descriptors
TL int                                                          OctPalsNum = 0;                                         //Number of active palettes

//Border thickness control in screen pixels, minimal valid value is 2
ATTR_RWDATA_IN_PSRAM_4BYTE_ALIGN float   INNER_DELTA = 18, OUTER_DELTA = 12, LIM = SIDE+18+12;

//Network
TL ATTR_RWDATA_IN_PSRAM_4BYTE_ALIGN octSignalMeet_t             OctHwids[CUBES_COUNT] = {0};//OCT_HWID_NONE, OCT_HWID_NONE, OCT_HWID_NONE, OCT_HWID_NONE, OCT_HWID_NONE, OCT_HWID_NONE, OCT_HWID_NONE, OCT_HWID_NONE};
TL ATTR_RWDATA_IN_PSRAM_4BYTE_ALIGN uint32_t                                                     OctHwidsNum = 0;
TL ATTR_RWDATA_IN_PSRAM_4BYTE_ALIGN octInfo_t                   OctInfo[STREAMS_TOTAL] = {0};                             //The most recent info packets received by this module
TL ATTR_RWDATA_IN_PSRAM_4BYTE_ALIGN int                         OctNetOutdated = 0;                                     //Critical error. Set when received a packet with seq from far future; forces module to request server to reload; cleared on server reload command
TL ATTR_RWDATA_IN_PSRAM_4BYTE_ALIGN uint32_t                    OctNetDisconnections = 0;                               //Current maximum number of timed out pings, resets to 0 when everything is connected
TL ATTR_RWDATA_IN_PSRAM_4BYTE_ALIGN uint32_t                    OctNetDisconnectionStartMs = 0;                         //
TL ATTR_RWDATA_IN_PSRAM_4BYTE_ALIGN int                         OctDisconnectionPlane = 0;

TL ATTR_RWDATA_IN_PSRAM_4BYTE_ALIGN octStream_t                 Streams[STREAMS_TOTAL];                                 //Wrappers around arrays with packets
TL ATTR_RWDATA_IN_PSRAM_4BYTE_ALIGN octPacket_t                 CmdPackets[CMDS_CAP];                                   //Circular array storing commands from leader
TL ATTR_RWDATA_IN_PSRAM_4BYTE_ALIGN octPacket_t                 BlobPackets[BLOBS_CAP];                                 //Circular array storing blob parts from anyone
TL ATTR_RWDATA_IN_PSRAM_4BYTE_ALIGN octPacket_t                 MsgPackets[STREAMS_MSGS * MSGS_CAP];                    //Circular arrays storing messages issued by different peers


TL ATTR_RWDATA_IN_PSRAM_4BYTE_ALIGN uint8_t                     OctUartsCache[NET_LINES_MAX][OCT_UART_BUF_CAP];

TL ATTR_RWDATA_IN_PSRAM_4BYTE_ALIGN octJobDownload_t            JobDownload;

//NON-TL
ATTR_RWDATA_IN_PSRAM_4BYTE_ALIGN bool                           OctManagerRole;
ATTR_RWDATA_IN_PSRAM_4BYTE_ALIGN bool                           OctBtImitation;

//Buffers used for UART DMA, must be allocated in non-cached memory
TL ATTR_RWDATA_IN_PSRAM_4BYTE_ALIGN uint8_t                     OctDmaRxBuffers[NET_LINES_MAX][OCT_UART_DMA_CAP];
TL ATTR_RWDATA_IN_PSRAM_4BYTE_ALIGN uint8_t                     OctDmaTxBuffers[NET_LINES_MAX][OCT_UART_DMA_CAP];

TL ATTR_RWDATA_IN_PSRAM_4BYTE_ALIGN volatile octUart_t          OctUarts[NET_LINES_MAX] = {0};

TL ATTR_RWDATA_IN_PSRAM_4BYTE_ALIGN volatile bool               OctUartInitialized = false;

TL ATTR_RWDATA_IN_PSRAM_4BYTE_ALIGN uint8_t                     OctSleepHandleUart = 0;           // Sleep handle to lock sleep while UARTs are active

