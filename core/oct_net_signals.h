#pragma once
#include "oct_net.h"


//Called from timer task. Allows modules to collect each other's HWIDs
void OCT_NET_emit_meet()
    {
        //Useful only right after initialization, otherwise it's too early or too late to build CubeIDs mapping (unless we are isolated - then still try)
        if (!OCT_is_state(OCT_STATE_MEETING) && !OCT_is_state(OCT_STATE_INCOMPLETE)) return;

        //Identify this cube
        octSignalMeet_t meet = {0};
        get_id((uint8_t*)&meet.HardwareId);
        meet.CubeId        = OCT_cubeid();
        meet.SenderStampMs = OCT_time();

        //Send through each UART
        OCT_NET_set_header(STREAM_NONE, OctSyncedTick, &meet.Header, meet.SenderStampMs, PKT_SIGNAL_MEET, OCT_TTL_FOR_BROADCAST, PEER_UNDEFINED);
        OCT_NET_send((octPacket_t*)&meet, OCT_LINES_NUM);
    }


//Discover all cubes of wowcube
void OCT_NET_process_meet(uint32_t line_id, octSignalMeet_t* intro)
    {
        //Relay as is
        OCT_NET_forward((octPacket_t*)intro, line_id);

        //Not collecting hwids anymore
        if (!OCT_is_state(OCT_STATE_MEETING) && !OCT_is_state(OCT_STATE_INCOMPLETE)) return;

        //[TODO]: If process takes too long, switch cube to isolation mode (or try random numbers instead of hwid)

        //Mark delivery time
        intro->ReceiverStampMs = OCT_time();

        //Identify own cube
        uint64_t own_hwid = 0;
        get_id((uint8_t*)&own_hwid);

        //Push own hwid on the first processing
        if (OctHwidsNum == 0) OctHwids[0].HardwareId = own_hwid,  OctHwids[0].CubeId = OCT_cubeid(),  OctHwids[0].ReceiverStampMs = OctHwids[0].SenderStampMs = OCT_time(),  OctHwidsNum++;

        //Update cubeid (each other cube will broadcast us it's index when figures it out)
        for (uint32_t i = 0; i < OctHwidsNum; i++) if (OctHwids[i].HardwareId == intro->HardwareId)
        {
            //Should be a real cube id to assign it
            if (intro->CubeId != CUBEID_UNDEFINED) OctHwids[i].CubeId = intro->CubeId;

            //Already have seen such hwid
            return;
        }

        //[TODO]: Somehow we've got more than CUBES_COUNT different hwids, so maybe reset the wowcube or just the hwids table
        if (OctHwidsNum == CUBES_COUNT) return;

        //Got new hwid, find the one with less value if can
        uint32_t place = 0;
        while (place < OctHwidsNum  &&  OctHwids[place].HardwareId > intro->HardwareId) place++;

        //Shift existing items to make space
        for (uint32_t i = OctHwidsNum; i > place; i--) memcpy(&OctHwids[i], &OctHwids[i-1], sizeof(OctHwids[i]));

        //Insert item keeping array sorted
        memcpy(&OctHwids[place], intro, sizeof(OctHwids[place])),  OctHwidsNum++;

        //[TODO]:
        //Now all hwids are known, so find out own cubeid
        //if (OctHwidsNum == CUBES_COUNT)
        
        //Actualize own cubeid
        for (uint32_t i = 0; i < OctHwidsNum; i++) if (OctHwids[i].HardwareId == own_hwid) { OctHwids[i].CubeId = OctCubeId = i; return; }
    }


//Called from separate task (timer)
void OCT_NET_emit_info()
    {
        //[NOTE]: Benchmark sits in incomplete mode
        //Cubes should already be acquainted with each other - so we can use cube ids
        if (OCT_get_state() <= OCT_STATE_MEETING || OCT_get_state() == OCT_STATE_INCOMPLETE) return;

        //Directly update info about own cube
        const uint32_t cubeid = OCT_cubeid();
        if (cubeid >= CUBES_COUNT) return;
        OctInfo[cubeid].Seq++;

        //[TODO] Specify integration time span for disconnected state ?

        //Look back from now in a fixed time span
        const int timespan_ms = 500, now_ms = OCT_time();

        //Estimate timer
        OCT_stat(now_ms, now_ms - OctInfo[cubeid].ReceiverStampMs, &OctInfo[cubeid].StatInfoDeliveryLag);

        //Integrate buffered samples area
        int abs_area[3] = {0,0,0}, signed_area[3] = {0,0,0};
        int old_index = OctBufferedLast, num_samples = 1;
        for (int i = 1; i < OCT_IMU_CAP;  i++, num_samples++)
        {
            //Decrement
            int cur_index = (old_index + OCT_IMU_CAP - 1) % OCT_IMU_CAP;

            //Ignore too old samples
            if (OCT_difference(OctBufferedMs[cur_index], now_ms) > timespan_ms) break;

            //Add one more sample to absolute area
            int dt_ms = (OctBufferedMs[old_index] - OctBufferedMs[cur_index]); //[TODO]: skip first old_index because OctBufferedLast item changes several times over each 20ms (OCT_IMU_DT_MS)
            for (int axis = 0; axis < 3; axis++)
            {
                //Gives exact area when both values have same sign, but rough approximation when signs are opposite
                //abs_area[axis] += (abs(OctBufferedGs[axis][cur_index] + OctBufferedGs[axis][old_index])) * dt_ms; //[NOTE]: div 2 is factored out of this cycle

                //Coarse area approximation
                abs_area[axis] += abs(OctBufferedGs[axis][old_index]) * dt_ms;

                //Signed and weighted. This area is even less accurate when graph is changing sign.
                signed_area[axis] += OctBufferedGs[axis][old_index] * dt_ms * (timespan_ms - (now_ms - OctBufferedMs[old_index]));
            }

            old_index = cur_index;
        }

        //Compress to packet
        octImuProcessed_t mms;
        for (int axis = 0; axis < 3; axis++)
        {
            //Normalize relative to the sampling frequency, scaled x8 to increase sensitivity
            abs_area[axis]      = abs_area[axis]    * 8 / (1 * OCT_IMU_DT_MS * num_samples);
            signed_area[axis]   = signed_area[axis] * 8 / (1 * OCT_IMU_DT_MS * num_samples * timespan_ms);
            mms.GArea[axis]     = UINT8_SAT(abs_area[axis]);
            mms.GSign[axis]     = INT8_SAT(signed_area[axis]);
        }

        //Partially update own local info
        OctInfo[cubeid].LatestUnprocessedCmdSeq = OCT_NET_cmd_stream()->Unprocessed;
        OctInfo[cubeid].LatestProcessedCmdSeq   = OCT_NET_cmd_stream()->Processed;
        OctInfo[cubeid].SenderStampMs           = now_ms;
        OctInfo[cubeid].ReceiverStampMs         = now_ms;
        OctInfo[cubeid].ProcessedBlobSeq        = OCT_NET_blob_stream()->Processed;
        OctInfo[cubeid].MsgSeq                  = Streams[STREAM_MSGS_C0 + cubeid].Processed;

        memcpy(&OctInfo[cubeid].Mms, &mms, sizeof(octImuProcessed_t));

        //Form the packet
        octSignalInfo_t info = {0};
        info.LatestUnprocessedCmdSeq    = OctInfo[cubeid].LatestUnprocessedCmdSeq;
        info.LatestProcessedCmdSeq      = OctInfo[cubeid].LatestProcessedCmdSeq;
        info.TimeMs                     = OctInfo[cubeid].SenderStampMs;
        info.ProcessedBlobSeq           = OctInfo[cubeid].ProcessedBlobSeq;
        info.MsgSeq                     = OctInfo[cubeid].MsgSeq;
        info.CubeId                     = cubeid;
        info.Way                        = 0xFFFFFFFF;
        memcpy(&info.Mms, &mms, sizeof(octImuProcessed_t));


        //Send through each UART
        OCT_NET_set_header(STREAM_NONE, OctSyncedTick, &info.Header, OctInfo[cubeid].Seq, PKT_SIGNAL_INFO, OCT_TTL_FOR_BROADCAST, (octPeerId_t)(PEER_C0 + cubeid));
        OCT_NET_send((octPacket_t*)&info, OCT_LINES_NUM);
    }


//Overwrite previously stored info from packet
void OCT_NET_process_info(uint32_t line_id, octSignalInfo_t* info)
    {
        //Since we get an info message, it means everyone introduced themselves so server advanced it's state and now clients should too
        if (OCT_get_state() == OCT_STATE_MEETING || OCT_get_state() == OCT_STATE_INCOMPLETE) OCT_set_state(OCT_STATE_CONSOLE);

        //Discard received packet if it's older than we already have
        uint32_t source_cid = info->CubeId;
        if (OCT_difference(OctInfo[source_cid].Seq, info->Header.Seq) <= 0  ||  OCT_get_state() == OCT_STATE_DISABLED) return;

        //Update the way with rx-line idx
        info->Way = (info->Way << 5) | (3 * OCT_cubeid() + line_id);

        //Estimate lag
        uint32_t now = OCT_time();
        OCT_stat(now, now - OctInfo[source_cid].ReceiverStampMs, &OctInfo[source_cid].StatInfoDeliveryLag);

        //[TODO]: Maybe wrap trace r\w into mutex to just do not think about race conditions inside info
        //Keep the most recent tracing info
        //[NOTE]: write is not atomic nor syncronized, so engine can read partially updated data - but this doesn't noticeably changes anything
        memcpy(&OctInfo[source_cid].Mms, &info->Mms, sizeof(octImuProcessed_t));
        OctInfo[source_cid].ReceiverStampMs         = now;
        OctInfo[source_cid].SenderStampMs           = info->TimeMs; //Allows engine task to better sync with server tick
        OctInfo[source_cid].Seq                     = info->Header.Seq;
        OctInfo[source_cid].LatestProcessedCmdSeq   = info->LatestProcessedCmdSeq;
        OctInfo[source_cid].LatestUnprocessedCmdSeq = info->LatestUnprocessedCmdSeq;
        OctInfo[source_cid].W                       = info->Way;
        OctInfo[source_cid].MsgSeq                  = info->MsgSeq;
        
        //[TODO]: reset on starting new blob
        OctInfo[source_cid].ProcessedBlobSeq        = info->ProcessedBlobSeq;
        OctInfo[source_cid].UnprocessedBlobSeq      = info->UnprocessedBlobSeq;

        //Advance message stream
        Streams[source_cid].Unprocessed = info->MsgSeq;

        //Relay
        OCT_NET_forward((octPacket_t*)info, line_id);
    }


//Retransmit missing packet
void OCT_NET_process_ask(bool bt, uint32_t rx_line, octSignalAsk_t* ask)
    {
        //Engine disabled
        if (OCT_get_state() == OCT_STATE_DISABLED) return;

        //Incorrect ask
        if (ask->Stream >= STREAMS_TOTAL) return;

        //[TODO]: Someone requested too old packet (stored seq already isn't the same), chances are that cube will not be able to recover actual state; detect such desync on asker

        //Search for asked packets
        octStream_t* stream = &Streams[ask->Stream];
        for (int i = 0; i < ask->NumSeqs; i++)
        {
            uint32_t place = ask->Seqs[i] % stream->Cap;
            octPacket_t* stored = &stream->Packets[place];
            if (stored->Header.Seq == ask->Seqs[i])
            {
                //Resend cached packet straight to who's asking (zero TTL) 
                stored->Header.SyncAndTTL[3] = (stored->Header.SyncAndTTL[3] & SYNC_MASK);
                if (bt)
                    ///OCT_trace(-1, "Ask answered %lu:%lu\n", ask->Stream, stored->Header.Seq),
                    BT_sendSPPData((uint8_t*)(stored), PKT_SIZES[stored->Header.Type]);
                else
                    OCT_UART_send(rx_line, stored);
                stream->StatAsksAnswered++;
            }
            else
            {
                //Have no such packet too (yet). Or maybe it is too old.
                ///OCT_trace(-1, "Ask ignored: asked for %lu, but has %lu\n", ask->Seqs[i], stored->Header.Seq);
                stream->StatAsksIgnored++;
            }
        }
    }

