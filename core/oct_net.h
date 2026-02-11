#pragma once
#include <stddef.h>     //offsetof
#include "oct_vars.h"
#include "oct_tm.h"
#include "oct_render.h" //void OCT_RENDER_graphs_sample(int graphidx, int value, int time_ms, int colorid);
#include "oct_uart.h"

//Forward declarations
void get_id(uint8_t id_buff[8]);                      // Defined in libhal_protected_CM4_GCC
void OCT_BT_process_bt_info(octSignalBtInfo_t* pkt);
bool BT_sendSPPData(uint8_t *data, size_t size);
bool BT_isConnected();

void OCT_NET_process_info(uint32_t line_id, octSignalInfo_t* info);
void OCT_NET_process_meet(uint32_t line_id, octSignalMeet_t* intro);
void OCT_NET_process_ask(bool bt, uint32_t rx_line, octSignalAsk_t* ask);


//Organize stream of packets
void OCT_NET_stream_reinit(octStreamId_t stream_id, octStream_t* stream, octPacket_t* packets, uint32_t cap)
    {
        stream->Id = (uint8_t)stream_id, stream->Packets = packets, stream->Cap = cap, stream->Processed = stream->Unprocessed = stream->AskStamp = 0;
        if (stream->Mutex == 0) stream->Mutex = xSemaphoreCreateMutex();
    }

octStream_t* OCT_NET_blob_stream()      { return &Streams[STREAM_BLOBS]; }
octStream_t* OCT_NET_cmd_stream()       { return &Streams[STREAM_CMDS]; }
octStream_t* OCT_BT_our_stream()        { return OctManagerRole ? &Streams[STREAM_MSGS_CLIENT_BT] : &Streams[STREAM_MSGS_SERVER_BT]; }
octStream_t* OCT_BT_their_stream()      { return OctManagerRole ? &Streams[STREAM_MSGS_SERVER_BT] : &Streams[STREAM_MSGS_CLIENT_BT]; }
octPeerId_t  OCT_NET_our_peer()          { return OctManagerRole ? PEER_SIM : PEER_C0; } //[TODO]: Make useful for all 8 cubes 

//Reset all globals
void OCT_NET_restart()
    {
        //Sizes of every known packet
        PKT_SIZES[PKT_SIGNAL_MEET]       = sizeof(octSignalMeet_t);        PKT_SIZES[PKT_SIGNAL_INFO]       = sizeof(octSignalInfo_t);          PKT_SIZES[PKT_SIGNAL_ASK]       = sizeof(octSignalAsk_t);           PKT_SIZES[PKT_SIGNAL_BT_INFO]   = sizeof(octSignalBtInfo_t);
        PKT_SIZES[PKT_SIGNAL_PING]       = sizeof(octSignalPingPong_t);    PKT_SIZES[PKT_SIGNAL_PONG]       = sizeof(octSignalPingPong_t);      PKT_SIZES[PKT_SIGNAL_BENCHMARK] = sizeof(octSignalBenchmark_t);
        PKT_SIZES[PKT_CMD_LAYOUT]        = sizeof(octCmdLayout_t);         PKT_SIZES[PKT_CMD_TWIST]         = sizeof(octCmdTwist_t);            PKT_SIZES[PKT_CMD_TAP]          = sizeof(octCmdTap_t);              PKT_SIZES[PKT_CMD_REBOOT]       = sizeof(octCmdEmpty_t);
        PKT_SIZES[PKT_CMD_IMU]           = sizeof(octCmdImu_t);            PKT_SIZES[PKT_CMD_EVENT]         = sizeof(octCmdEvent_t);            PKT_SIZES[PKT_CMD_DEV]          = sizeof(octCmdDev_t);              PKT_SIZES[PKT_CMD_APP]          = sizeof(octCmdFile_t);
        PKT_SIZES[PKT_CMD_TIME]          = sizeof(octCmdTime_t);           PKT_SIZES[PKT_CMD_CTRL_BACKLIGHT] = sizeof(octCmdCtrl_t);            PKT_SIZES[PKT_CMD_CTRL_CURRENT] = sizeof(octCmdCtrl_t);             PKT_SIZES[PKT_CMD_FILE]         = sizeof(octCmdFile_t);
        PKT_SIZES[PKT_CMD_INSTALL]       = sizeof(octCmdFile_t);           PKT_SIZES[PKT_CMD_DATA]          = sizeof(octPacket_t);
        PKT_SIZES[PKT_MSG_IMU]           = sizeof(octMsgImu_t);            PKT_SIZES[PKT_MSG_REQUEST]       = sizeof(octMsgRequest_t);          PKT_SIZES[PKT_MSG_REPORT]        = sizeof(octMsgReport_t);          PKT_SIZES[PKT_MSG_REBOOT]       = sizeof(octMsgReboot_t);
        PKT_SIZES[PKT_MSG_TRACE]         = sizeof(octMsgTrace_t);          PKT_SIZES[PKT_MSG_BACKLIGHT]     = sizeof(octMsgBacklight_t);        PKT_SIZES[PKT_MSG_PING]          = sizeof(octMsgPing_t);            PKT_SIZES[PKT_MSG_PONG]         = sizeof(octMsgPong_t); 
        PKT_SIZES[PKT_MSG_BLOB_OFFER]    = sizeof(octMsgBlobOffer_t);      PKT_SIZES[PKT_MSG_BLOB_FETCH]    = sizeof(octMsgBlobFetch_t);        PKT_SIZES[PKT_MSG_BLOB_ANNOUNCE] = sizeof(octMsgBlobAnnounce_t);    PKT_SIZES[PKT_MSG_TWIST]        = sizeof(octMsgTwist_t);
        PKT_SIZES[PKT_MSG_JOB_DOWNLOAD_REPORT] = sizeof(octMsgJobDownloadReport_t);
        PKT_SIZES[PKT_BLOB]              = sizeof(octBlob_t);              PKT_SIZES[PKT_BLOB_FORCED]       = sizeof(octBlobForced_t);

#ifdef OCTSIM
        //Check that each packet's size is not exceeding possible max and is a multiple of 4
        for (int i = 0; i < 256; i++) if (PKT_SIZES[i] > sizeof(octPacket_t)  ||  (PKT_SIZES[i] % 4) != 0) DIE("Wrong packet struct");
#endif

        memset(OctBufferedMs, 0, OCT_IMU_CAP * sizeof(int)),  memset(OctInfo, 0, CUBES_COUNT * sizeof(octInfo_t));
        OctBufferedLast = 0,  OctBufferedStamp = 0,  OctInputSamplesNum = 1;
        OctDevMode = 0,  OctNetOutdated = 0,  OctNetDisconnections = 0;

        //[TODO]: Redo way tracing implementation so that zeroes would work instead of 1's
        OctInfo[0].W = OctInfo[1].W = OctInfo[2].W = OctInfo[3].W = OctInfo[4].W = OctInfo[5].W = OctInfo[6].W = OctInfo[7].W = 0xFFFFFFFF;

        //[TODO]: Keep what peers are we tracking (meaning trying to fill gaps and do the actual packet processing)

        //Setup packet streams
        for (uint8_t i = 0; i < STREAMS_MSGS; i++) OCT_NET_stream_reinit((octStreamId_t)(STREAM_MSGS_C0+i), &Streams[i], &MsgPackets[i * MSGS_CAP], MSGS_CAP);
        OCT_NET_stream_reinit(STREAM_CMDS,  &Streams[STREAM_CMDS],  CmdPackets,  CMDS_CAP);
        OCT_NET_stream_reinit(STREAM_BLOBS, &Streams[STREAM_BLOBS], BlobPackets, BLOBS_CAP);
    }


//Verify cubeids reported by others against order in our own sorted table
bool OCT_NET_is_introduction_complete()
    {
        //Someone is missing
        if (OctHwidsNum != CUBES_COUNT) return false;

        //Detect invalid mapping
        for (uint32_t i = 0; i < OctHwidsNum; i++) if (OctHwids[i].CubeId != i) return false;

        //All checks passed
        return true;
    }


bool OCT_NET_store(uint32_t stream_id, octPacket_t* pkt)
    {
        //Ignore while engine is off
        if (OCT_is_state(OCT_STATE_DISABLED)) return false;

        //Bounds control
        if (stream_id < 0 || stream_id >= STREAMS_TOTAL) return NULL;
        octStream_t* stream = &Streams[stream_id];


        //[TODO]: check if allocated > unprocessed, meaning two peers are emitting to same stream - which is should not be possible

        //Find index in circular buffer where packet must reside
        int place = pkt->Header.Seq % stream->Cap;

        //Prevent duplication (already have stored packet with same seq)
        if (stream->Packets[place].Header.Seq == pkt->Header.Seq)
        {
            //Special case when copy of same packet travelled longer way but comes before this one. In such case we must still forward this packet by returning true, and raise our stored TTL
            if ((stream->Packets[place].Header.SyncAndTTL[3] & TTL_MASK) < (pkt->Header.SyncAndTTL[3] & TTL_MASK))
            {
                stream->Packets[place].Header.SyncAndTTL[3] = pkt->Header.SyncAndTTL[3];
                return true;
            }

            //Already seen earlier so no additional processing is needed
            return false;
        }

        //Cache overflow or sequence gone too far, do full resync
        //[TODO]: difference
        //if (pkt->Header.Seq >= stream->Unprocessed + CAP) { OctNetOutdated = 1; return false; } //[TODO]: Signal to server or to try recover independently?

        //Push to stream atomically, otherwise another task could process it earlier than it would be fully copied
        if (xSemaphoreTake(stream->Mutex, portMAX_DELAY) != pdTRUE) return false;
        memcpy(&stream->Packets[place], pkt, PKT_SIZES[pkt->Header.Type]);
        xSemaphoreGive(stream->Mutex);

        //Allow processing to advance
        //[TODO]: difference
        if (pkt->Header.Seq > stream->Unprocessed) { stream->Unprocessed = pkt->Header.Seq; }

        return true;
    }



void OCT_NET_set_crc(octPacketHeader_t* hdr)
    {
        //Hashing skips the first two header's fields (magic bytes and checksum itself) of 2x4 bytes in total
        hdr->Crc = OCT_crc(((uint8_t*)hdr) + 8,  PKT_SIZES[hdr->Type] - 8);
    }


//Setup header before emitting packet
void OCT_NET_set_header(octStreamId_t stream_id, uint32_t stamp, octPacketHeader_t* hdr, uint32_t seq, octPacketType_t type, uint8_t ttl, octPeerId_t peer_id)
    {
#ifdef OCTSIM
        if (PKT_SIZES[type] == 0) DIE("Packet size is unknown for it's type");
#endif

        hdr->SyncAndTTL[0] = SYNC_MARK[0], hdr->SyncAndTTL[1] = SYNC_MARK[1], hdr->SyncAndTTL[2] = SYNC_MARK[2], hdr->SyncAndTTL[3] = SYNC_MARK[3] | (ttl & TTL_MASK);
        hdr->Seq        = seq;
        hdr->Stamp      = stamp;
        hdr->Type       = (uint8_t)type;
        hdr->Stream     = (uint8_t)stream_id;
        hdr->Peer       = (uint8_t)peer_id;
        hdr->TetraSize  = (uint8_t)(PKT_SIZES[type] / 4);
        OCT_NET_set_crc(hdr);
    }


//Add packet to stream before it can be emitted
octPacket_t* OCT_NET_packet_add(const octPacketHeader_t* hdr, octStreamId_t stream_id, uint32_t stamp, octPacketType_t type, uint8_t ttl, octPeerId_t peer_id)
    {
        //Bounds control
        if (stream_id < 0 || stream_id >= STREAMS_TOTAL) return NULL;
        octStream_t* stream = &Streams[stream_id];

        //Prevent race conditions if somehow two tasks would emit using same stream
        if (xSemaphoreTake(stream->Mutex, portMAX_DELAY) != pdTRUE) return NULL;

        //Must have space (number of packets to be processed is less than stream capacity)
        uint32_t seq_allocated = (stream->Unprocessed + 1);
        if ((seq_allocated % stream->Cap) == (stream->Processed % stream->Cap)) {
            xSemaphoreGive(stream->Mutex); return NULL; }

        //Add to stream
        octPacket_t* pkt = &stream->Packets[seq_allocated % stream->Cap];
        memcpy(pkt, hdr, PKT_SIZES[type]);
        
        //Finalize packet
        OCT_NET_set_header(stream_id, stamp, &pkt->Header, seq_allocated, type, ttl, peer_id);
        
        //Allow logic to see this packet
        stream->Unprocessed++;
        xSemaphoreGive(stream->Mutex);
        return pkt;
    }


//Send packet to UART lines - no TTL modification
void OCT_NET_send(octPacket_t* pkt, uint32_t exclude_line)
    {
        //Special logic for info packets
        if (pkt->Header.Type == PKT_SIGNAL_INFO && pkt->Header.Peer < CUBES_COUNT)
        {
            //Only info message appends tx-uart to trace it's way
            octSignalInfo_t* info = (octSignalInfo_t*)pkt;
            int prev_way = info->Way << 5;
            for(uint32_t i = 0; i < OCT_LINES_NUM; i++) if (i != exclude_line)
            {
                //Update way for each uart
                info->Way = prev_way | (3 * OCT_cubeid() + i);
                OCT_NET_set_crc(&pkt->Header);
                OCT_UART_send(i, pkt);
            }
            return;
        }

        //Send packet
        for(uint32_t i = 0; i < OCT_LINES_NUM; i++) if (i != exclude_line)
            OCT_UART_send(i, pkt);
    }


//Forward received packet to neighbors - decrements TTL before sending
void OCT_NET_forward(octPacket_t* pkt, uint32_t exclude_line)
    {
        if ((pkt->Header.SyncAndTTL[3] & TTL_MASK) > 0) pkt->Header.SyncAndTTL[3]--; else return;
        OCT_NET_send(pkt, exclude_line);
    }


void OCT_NET_ask_missing_packets()
    {
        //Check all streams 
        uint32_t curtime = OCT_time();
        for (uint32_t stream_idx = 0; stream_idx < STREAMS_TOTAL; stream_idx++)
        {
            //[TODO]: Mark asking policy on init
            //No need to process own streams
            octStream_t* stream = &Streams[stream_idx];
            if (stream == OCT_BT_our_stream()) continue;
         
            //Detect gaps in stream
            uint8_t         missing_num = 0;
            uint32_t        missing_seqs[OCT_ASK_CAP];
            for (uint32_t seq = stream->Processed + 1;  seq <= stream->Unprocessed;  seq++)  //[TODO]: wrapping\delta: unprocessed can be less than seq if wraps, use difference
            {
                //Recently already asked something, so have to wait for cooldown expiration
                if (OCT_is_cooldown(stream->AskStamp, curtime, OCT_ASK_COOLDOWN_MS)) break;

                //Wrap around sequence number to get it's place index
                int place = seq % stream->Cap;

                //When stored packet has different seq it means no actual packet was received to overwrite it
                if (stream->Packets[place].Header.Seq != seq)
                {
                    //Collect missing packet's seq number
                    missing_seqs[missing_num++] = seq;
                    if (missing_num == OCT_ASK_CAP) break;
                }
            }

            //Send asks
            uint8_t taken = 0;
            if (taken < missing_num)
            {
                //Fill packet with info about what we are missing in this stream
                octSignalAsk_t ask = { .Stream = stream->Id };
                for (; taken < missing_num;  taken++)
                    ask.Seqs[ask.NumSeqs++] = missing_seqs[taken];
                
                //Finalize packet
                OCT_NET_set_header(STREAM_NONE, curtime, &ask.Header, stream->Processed, PKT_SIGNAL_ASK, OCT_TTL_FOR_NEIGHBOR, OCT_NET_our_peer());

                //[TODO]: See in info who already has that packet, don't ask others

                //Should try bluetooth for some streams
                OCT_NET_send((octPacket_t*)&ask, OCT_LINES_NUM);
                
                //Update stream state
                stream->AskStamp = curtime,  stream->StatAsksSent += ask.NumSeqs;
            }
        }
    }


//Server emits cmd packet to all clients
bool OCT_NET_emit_cmd(const octPacketHeader_t* hdr, octPacketType_t type, bool pending)
    {
        //Only server cube and with inited CID is able to emit cmds
        if (!OCT_is_leader() || type < PKT_CMD_FIRST) return false;

        //Fill new packet
        octPacket_t* cmd = OCT_NET_packet_add(hdr, STREAM_CMDS, OctSyncedTick, type, OCT_TTL_FOR_BROADCAST, PEER_C0);
        if (cmd == NULL) OCT_terminate("Can't add cmd packet to stream");
        
        //Send through each UART
        OCT_NET_send(cmd, OCT_LINES_NUM);

        //On server just short-circuit packet directly to storage
        ///OCT_NET_store_cmd(pkt);

        //Track clients result of this cmd
        (void)pending;
        return true;
    }


//Uses stream by cubeid
void OCT_NET_emit_msg(const octPacketHeader_t* hdr, octPacketType_t type)
    {
        //[TODO]: assert type is a msg
        //CubeId must be known
        uint32_t cubeid = OCT_cubeid();
        if (cubeid > CUBES_COUNT) return;

        //When dock tries to reach leader through single UART, it needs max ttl
        int8_t ttl = (cubeid == PEER_DOCK) ? OCT_TTL_MAX : OCT_TTL_FOR_BROADCAST;

        //Allocate new packet and fill it
        octPacket_t* pkt = OCT_NET_packet_add(hdr, (octStreamId_t)(STREAM_MSGS_C0 + cubeid), OctSyncedTick, type, ttl, (octPeerId_t)(PEER_C0+cubeid));
        if (pkt == NULL) OCT_terminate("Can't add msg packet to stream");
        
        //Send through each UART
        OCT_NET_send(pkt, OCT_LINES_NUM);
    }


//Called from uart task when it gets any valid packet
void OCT_NET_route_packet(bool bt, uint32_t line_id, octPacket_t* pkt)
    {
        //Always process meet
        if (pkt->Header.Type == PKT_SIGNAL_MEET) { OCT_NET_process_meet(line_id, (octSignalMeet_t*)pkt);  return; }

        //To process anything other than meet, cubeid must be already known
        if (OCT_cubeid() > CUBES_COUNT) return;

        //TODO: check peer id is in range

        (void)bt;
        switch (pkt->Header.Type)
        {
            //Signals are processed immediately on arrival, so handlers must return really fast
            case PKT_SIGNAL_INFO:       { OCT_NET_process_info (line_id, (octSignalInfo_t*)pkt);        return; }
            case PKT_SIGNAL_ASK:        { OCT_NET_process_ask  (bt, line_id, (octSignalAsk_t*)pkt);     return; }
            case PKT_SIGNAL_BENCHMARK:  { return; }

            //Need special logic 
            case PKT_MSG_PING:          
            {
                //Sender wants this ping to be unreliable, so roll the dice
                octMsgPing_t* m = (octMsgPing_t*)pkt;
                if (m->Drop != 0  &&  (m->Targets & (1 << OCT_cubeid()))  &&  OCT_random(0, 100) < m->Drop) return;

                //Ping survived
                if (OCT_NET_store(pkt->Header.Stream, pkt)) 
                    OCT_NET_forward(pkt, line_id);
                
                return;
            }

            case PKT_CMD_LAYOUT:
            case PKT_CMD_TWIST:
            case PKT_CMD_TAP:
            case PKT_CMD_APP:
            case PKT_MSG_BLOB_OFFER: 
            case PKT_MSG_BLOB_ANNOUNCE: 
            case PKT_MSG_BLOB_FETCH: 
            case PKT_MSG_JOB_DOWNLOAD_REPORT: 
            case PKT_MSG_TRACE: 
            case PKT_MSG_PONG: 
            case PKT_MSG_REQUEST:
            case PKT_MSG_REPORT:
            case PKT_MSG_REBOOT:
            case PKT_MSG_BACKLIGHT:
            case PKT_BLOB:
            {
                //Filter out packet when it is a dupe; put fresh one into the stream it belongs to
                if (OCT_NET_store(pkt->Header.Stream, pkt))
                {
                    //Relay to neighbours 
                    OCT_NET_forward(pkt, line_id);
                }
                return;
            }
        }

        //[DEV]: For active development time only; remove later
        //Unknown packet
        OCT_terminate("OCT_NET_route_packet unknown packet");
    }


//Extract complete packets from stream
void OCT_NET_extract_packets(bool bt, uint32_t line_id, volatile octUart_t* traffic)
    {
        //[TODO]: make array global and in tcm
        uint8_t linear[sizeof(octPacket_t)];
        octPacket_t* packet = (octPacket_t*)linear;

        //Need at least the header
        uint32_t processed = 0, count = (traffic->RxAvailable - traffic->RxProcessed);
        if (count < sizeof(octPacketHeader_t)) return;

        //Data is wrapped around ring buffer's end so only per-byte access is guaranteed to be valid
        const volatile uint8_t* bytes = traffic->RxCache;

        //Starting after the last processed byte, scan available bytestream for complete and valid packets
        const uint32_t WRAPPING = traffic->RxCacheCap - 1;
        for (uint32_t n = traffic->RxProcessed + 1;  count >= sizeof(octPacketHeader_t); )
        {
            //At this point there is enough bytes to test if it's really the header's start (excluding last two bits because TTL is carried there)
            if (bytes[(n+0) & WRAPPING] != SYNC_MARK[0] || bytes[(n+1) & WRAPPING] != SYNC_MARK[1] || bytes[(n+2) & WRAPPING] != SYNC_MARK[2] || (bytes[(n+3) & WRAPPING] & SYNC_MASK) != SYNC_MARK[3]) 
            { 
                //Wasn't signature bytes, retry parsing again from next byte 
                n++, count--, processed++; traffic->RxStatSkipped++, traffic->RxStatBandwidth.Counter++;
                continue; 
            }

            //Looks like a packet, get it's size by type
            uint32_t packet_size = PKT_SIZES[ bytes[ (n+offsetof(octPacketHeader_t, Type)) & WRAPPING ] ];
            if (packet_size == 0)
            {
                //Unknown type, treat like corrupted packet
                n++; count--; processed++; traffic->RxStatSkipped++, traffic->RxStatBandwidth.Counter++;
                continue;
            }

            //Need enough bytes to extract full packet, otherwise we're done with this uart for now
            if (count < packet_size) break;

            //[NOTE]: When calculating checksum the first 8 bytes must be skipped
            uint32_t crc32 = 0;
            for (uint32_t i = 8; i < packet_size; i++)
                crc32 = OCT_crc_byte(crc32, bytes[(n+i) & WRAPPING]);

            //Verify checksum
            const uint8_t* crc = (const uint8_t*)&crc32;
            if (crc[0] != bytes[(n+4) & WRAPPING] || crc[1] != bytes[(n+5) & WRAPPING] || crc[2] != bytes[(n+6) & WRAPPING] || crc[3] != bytes[(n+7) & WRAPPING])
            {
                //Checksum mismatched, retry parsing again from next byte 
                n++, count--, processed++; traffic->RxStatSkipped++, traffic->RxStatBadCrc++, traffic->RxStatBandwidth.Counter++;
                continue;
            }

            //Extract from ring buffer to a linear storage
            for (uint32_t i = 0; i < packet_size; i++)
                linear[i] = bytes[(n+i) & WRAPPING];

            //Routing
            OCT_NET_route_packet(bt, line_id, packet);

            //Next packet
            n += packet_size,  count -= packet_size,  processed += packet_size,  traffic->RxStatPackets.Counter++;
        }

        //Safely update the counter
        OCT_MEM_BARRIER;  traffic->RxProcessed += processed;  traffic->RxStatBandwidth.Counter += processed;
    }

