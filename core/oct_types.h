#pragma once
#include <stdint.h>
#include "oct_shared.h"
#include "oct_consts.h"

#ifdef OCTSIM
#include "../sim/src/sim_hal_windows.h"    //SemaphoreHandle_t
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


//[NOTE]: Fields ordering is designed for possible multi-core use in the future
typedef struct
{
    SemaphoreHandle_t       TxMutex;                            //Several tasks can send data, so protect producer's side of the lockless queue
    uint32_t                TxStatWritten;
    uint32_t                TxStatDropped;
    octStatPerSec_t         TxStatBandwidth;

    uint32_t                RxAvailable;                        //Producer (DMA callback) increases this byte counter after pushing incoming data
    uint8_t*                RxCache;                            //Shared buffer to allow DMA or BT callback finish as fast as possible
    uint32_t                RxCacheCap;                         //
    uint32_t                RxProcessed;                        //Consumer (RX task) increases this byte counter after parsing the data
    uint32_t                RxStatSkipped;
    uint32_t                RxStatBadCrc;

    octStatPerSec_t         RxStatBandwidth;
    octStatPerSec_t         RxStatPackets;
    uint32_t                RxStatLastStamp;
    octStat_t               RxStatUniformity;

} octUart_t; //Channel



//Following structs are tightly packed
#ifdef __GNUC__
    #define ATTR_PACKED __attribute__((packed))
#else 
    //Instead of __attribute__((packed))
    #define ATTR_PACKED
    #pragma pack(push, 1)
#endif


//Accelerometer and gyroscope measurements
typedef struct ATTR_PACKED
{
    uint8_t             GArea[3];
    int8_t              GSign[3];
    int8_t              Reserved[2];
} octImuProcessed_t;

//_________________________________________________________________________________________________________________________________________________________________________________
typedef struct ATTR_PACKED
{
    uint8_t                 SyncAndTTL[4];              //Special sequence of always the same bits, so traffic parser can easily recognize header's start in a stream; last two bits are packet's TTL (how many hops it can do)
    uint32_t                Crc;                        //CRC32 without SyncAndTTL or Crc fields
  //[NOTE]:                 Crc checksum starts from here (previous 8 bytes are skipped) and ends with the entire packet
    uint32_t                Seq;                        //Sequence number inside the stream, must be aligned and occupy 32 bits to enforce atomic R\W instructions
    uint32_t                Stamp;                      //
    uint8_t                 Type;                       //
    uint8_t                 Stream;                     //Where to put this packet
    uint8_t                 Peer;                       //Who emitted this packet
    uint8_t                 TetraSize;                  //This is a nominal field, it is correctly filled (with the size of packet in dwords) but otherwise unused. Exists 'just in case' or simply as a reserved byte that could be repurposed later.
} octPacketHeader_t;


//Opaque packet that should be casted to a specific struct based on Header.Type; sizeof must be a multiple of 4 bytes
typedef struct ATTR_PACKED
{
    octPacketHeader_t       Header;
    uint8_t                 Raw[OCT_PAYLOAD_MAX];
} octPacket_t;


typedef struct ATTR_PACKED
{
    octPacketHeader_t       Header;
    uint64_t                HardwareId;
    uint32_t                CubeId;
    uint32_t                SenderStampMs;
    uint32_t                ReceiverStampMs;            //This field doesn't really need to be sent, it is set on arrival, and exists inside this struct only for convenience
    uint32_t                Reserved;                   //[NOTE]: Rename this field before any use
} octSignalMeet_t;


typedef struct ATTR_PACKED
{
    octPacketHeader_t       Header;
    uint32_t                Way;                            //4 - Tracing route consisting of 2 to 6 UART ids
    uint32_t                LatestUnprocessedCmdSeq;        //4 - The most recent known cmd id (aka OctCmdUnprocessed)
    uint32_t                LatestProcessedCmdSeq;          //4 - The most recent processed cmd id
    uint32_t                TimeMs;
    uint32_t                CubeId;
    uint32_t                ProcessedBlobSeq;               //Used in flow control
    uint32_t                UnprocessedBlobSeq;
    uint32_t                MsgSeq;
    octImuProcessed_t       Mms;
} octSignalInfo_t;


typedef struct ATTR_PACKED
{
    octPacketHeader_t       Header;
    uint32_t                BtMsgSeq;           //Our stream messages counter
    uint32_t                UnprocessedBlobSeq;    //
    uint32_t                ProcessedBlobSeq;   //Used for flow control in peer that currently is emitting the blob
    uint8_t                 StreamId;           //Same ids as in streams
    uint8_t                 Reserved[3];
} octSignalBtInfo_t;


typedef struct ATTR_PACKED
{
    octPacketHeader_t       Header;
    uint8_t                 Stream;
    uint8_t                 NumSeqs;
    uint8_t                 Reserved1;
    uint8_t                 Reserved2;
    uint32_t                Seqs[OCT_ASK_CAP];
} octSignalAsk_t;


typedef struct ATTR_PACKED
{
    octPacketHeader_t       Header;
    uint32_t                Key;
} octSignalPingPong_t;


typedef struct ATTR_PACKED
{
    octPacketHeader_t       Header;
    uint16_t                Pixels[54];
} octSignalBenchmark_t;


typedef struct ATTR_PACKED {  octPacketHeader_t  Header;     uint16_t Quads[8];                                                         } octCmdLayout_t;   //Sids in 15 bits per cube, 16 bytes total
typedef struct ATTR_PACKED {  octPacketHeader_t  Header;     int32_t  TwistId;      uint32_t DisconnectedMs;                            } octCmdTwist_t;
typedef struct ATTR_PACKED {  octPacketHeader_t  Header;     int32_t  TapId;                                                            } octCmdTap_t;
typedef struct ATTR_PACKED {  octPacketHeader_t  Header;                                                                                } octCmdEmpty_t;
typedef struct ATTR_PACKED {  octPacketHeader_t  Header;     int8_t   A[3];         int8_t  G[3];           int8_t Padding[2];          } octCmdImu_t;      //Measurements for gameplay
typedef struct ATTR_PACKED {  octPacketHeader_t  Header;     int32_t  EventId;      int32_t Params[8];                                  } octCmdEvent_t;
typedef struct ATTR_PACKED {  octPacketHeader_t  Header;     uint32_t DevMode;                                                          } octCmdDev_t;      //Flags
typedef struct ATTR_PACKED {  octPacketHeader_t  Header;     uint8_t  Sec, Min, Hour, Day, Mon, Week, Year, Padding;                    } octCmdTime_t;     //Exactly as in hal_rtc_time_t
typedef struct ATTR_PACKED {  octPacketHeader_t  Header;     int32_t  Ints[8];                                                          } octCmdCtrl_t;
typedef struct ATTR_PACKED {  octPacketHeader_t  Header;     int32_t  Place;        int32_t Size;   uint32_t Hash;  char Name[OCT_ASSET_NAME_MAXLEN];   } octCmdFile_t;

typedef struct ATTR_PACKED {  octPacketHeader_t  Header;     int32_t A[3];          int32_t G[3];                                               } octMsgImu_t;
typedef struct ATTR_PACKED {  octPacketHeader_t  Header;     int32_t State;                                                                     } octMsgRequest_t;
typedef struct ATTR_PACKED {  octPacketHeader_t  Header;     int32_t State;                                                                     } octMsgReport_t;
typedef struct ATTR_PACKED {  octPacketHeader_t  Header;     int32_t Percent;                                                                   } octMsgBacklight_t;
typedef struct ATTR_PACKED {  octPacketHeader_t  Header;     int32_t Flags;                                                                     } octMsgReboot_t;
typedef struct ATTR_PACKED {  octPacketHeader_t  Header;     uint32_t Targets;      int32_t Key;  int8_t Drop, Silent, Reserved1, Reserved2;    } octMsgPing_t;
typedef struct ATTR_PACKED {  octPacketHeader_t  Header;                            int32_t Key;                                                } octMsgPong_t;
typedef struct ATTR_PACKED {  octPacketHeader_t  Header;     int32_t  TwistId;      uint32_t DisconnectedMs;                                    } octMsgTwist_t;
typedef struct ATTR_PACKED {  octPacketHeader_t  Header;     uint32_t OfferSeq;     int32_t State;                                              } octMsgJobDownloadReport_t;
typedef struct ATTR_PACKED {  octPacketHeader_t  Header;     uint32_t Timestamp;    int32_t Id, Event, Value1, Value2;  char Text[OCT_TRACE_TEXT_MAXLEN];   } octMsgTrace_t;

typedef struct ATTR_PACKED {  octPacketHeader_t  Header;     int32_t  BlobType;     uint32_t Flags;  uint32_t Hash;  uint32_t Size;  uint32_t Offset;  uint32_t SenderPeerId;                                   char Name[OCT_ASSET_NAME_MAXLEN];     } octMsgBlobOffer_t;
typedef struct ATTR_PACKED {  octPacketHeader_t  Header;     int32_t  BlobType;     uint32_t Flags;  uint32_t Hash;  uint32_t Size;  uint32_t Offset;  uint32_t SenderPeerId;  uint32_t BlobLastSeq, OfferSeq;  char Name[OCT_ASSET_NAME_MAXLEN];     } octMsgBlobAnnounce_t;
typedef struct ATTR_PACKED {  octPacketHeader_t  Header;     int32_t  BlobType;     uint32_t Flags;                                                    uint32_t SenderPeerId;  uint32_t BlobLastSeq, OfferSeq;  uint32_t ResultingState;              } octMsgBlobFetch_t;

typedef struct ATTR_PACKED {  octPacketHeader_t  Header;     uint32_t BlobId;       int32_t  Total; int32_t Offset; int32_t Useful;     uint8_t Data[BLOB_MAX];                   } octBlob_t;
typedef struct ATTR_PACKED {  octPacketHeader_t  Header;     uint32_t BlobId;       int32_t  Total; int32_t Mode;   uint32_t Hash;      char Name[OCT_ASSET_NAME_MAXLEN];         } octBlobForced_t;

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
    uint32_t                MsgSeq;
    uint32_t                BtSeq;
    uint32_t                ProcessedBlobSeq;
    uint32_t                UnprocessedBlobSeq;

    octStat_t               StatInfoDeliveryLag;

    octImuProcessed_t       Mms;
    //int8_t                  Unused[2];                  //Pad structure to 4 bytes boundary [TODO]: assert somewhere
} octInfo_t;


typedef struct
{
    uint32_t            Processed;          //Last seq received (and optionally processed)
    octPacket_t*        Packets;            //Storage, may have gaps because of missing packets
    uint32_t            Unprocessed;        //Should be received and processed (if current code instance care about tracking this stream)
    uint32_t            Cap;                //Max count of packets in storage
    uint32_t            AskStamp;           //Prevent spamming by having a cooldown before allowed to repeat ask
    uint32_t            StatAsksSent;       //
    uint32_t            StatAsksAnswered;   //
    uint32_t            StatAsksIgnored;    //
    uint8_t             Id;                 //Logical assignment
    uint8_t             Reserved1;
    uint8_t             Reserved2;
    uint8_t             Reserved3;
    SemaphoreHandle_t   Mutex;
} octStream_t;


//Continous process with complex state
typedef struct
{
    uint8_t             Buffer[OCT_WRITE_ACCUM_CAP];
    uint32_t            Buffered;
    uint32_t            Erased;
    uint32_t            Written;
    octJobState_t       State;
    octJobState_t       Reports[CUBES_COUNT];
    uint32_t            Crc;
    uint32_t            DiskOffset;
    octMsgBlobAnnounce_t Announce;
    octMsgBlobFetch_t   Fetch;
    SemaphoreHandle_t   Mutex;              //Disk writer is a separate task
} octJobDownload_t;



