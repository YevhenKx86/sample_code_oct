#pragma once
#include "oct_vars.h"
#include "oct_net.h"



void OCT_NET_msg_blob_offer(int32_t blob_type, uint32_t flags, uint32_t hash, uint32_t size, uint32_t offset, uint32_t sender_peer_id, const char* name)
    {
        octMsgBlobOffer_t msg = {0};
        msg.BlobType = blob_type,  msg.Flags = flags,  msg.Hash = hash,  msg.Size = size,  msg.Offset = offset,  msg.SenderPeerId = sender_peer_id,  OCT_strcpy(msg.Name, OCT_ASSET_NAME_MAXLEN, name);
        OCT_NET_emit_msg(&msg.Header, PKT_MSG_BLOB_OFFER);
    }
    

void OCT_NET_msg_blob_annonce(octMsgBlobAnnounce_t* copy, int32_t blob_type, uint32_t flags, uint32_t hash, uint32_t size, uint32_t offset, uint32_t sender_peer_id, uint32_t blob_last_seq, uint32_t offer_seq, const char* name)
    {
        octMsgBlobAnnounce_t msg = {};
        msg.BlobType = blob_type,  msg.Flags = flags,  msg.Hash = hash,  msg.Size = size,  msg.Offset = offset,  msg.SenderPeerId = sender_peer_id,  msg.BlobLastSeq = blob_last_seq,  msg.OfferSeq = offer_seq,  OCT_strcpy(msg.Name, OCT_ASSET_NAME_MAXLEN, name);
        memcpy(copy, &msg, sizeof(msg));
        OCT_NET_emit_msg(&msg.Header, PKT_MSG_BLOB_ANNOUNCE);
    }


void OCT_NET_msg_blob_fetch(int32_t blob_type, uint32_t flags, uint32_t offer_seq, uint32_t sender_peer_id, uint32_t blob_last_seq, uint32_t resulting_state)
    {
        octMsgBlobFetch_t msg = {};
        msg.BlobType = blob_type, msg.Flags = flags, msg.OfferSeq = offer_seq,  msg.SenderPeerId = sender_peer_id,  msg.BlobLastSeq = blob_last_seq,  msg.ResultingState = resulting_state;
        OCT_NET_emit_msg(&msg.Header,PKT_MSG_BLOB_FETCH);
    }


void OCT_NET_msg_job_download_report(uint32_t offer_seq, octJobState_t job_state)
    {
        octMsgJobDownloadReport_t rep = {};
        rep.OfferSeq = offer_seq, rep.State = (int32_t)job_state;
        OCT_NET_emit_msg(&rep.Header, PKT_MSG_JOB_DOWNLOAD_REPORT);
    }

//
void OCT_NET_msg_trace(uint32_t timestamp, int32_t id, int32_t event, int32_t value1, int32_t value2, const char* format, ...)
    {
        octMsgTrace_t tr = {0};
        tr.Timestamp = timestamp,  tr.Id = id,  tr.Event = event,  tr.Value1 = value1,  tr.Value2 = value2;
        
        va_list args;
        va_start(args, format);
        vsnprintf(tr.Text, OCT_TRACE_TEXT_MAXLEN, format, args);
        va_end(args);
        
        OCT_NET_emit_msg(&tr.Header, PKT_MSG_TRACE);
    }


//This message is for tests, so it uses customized sending
void OCT_NET_msg_pong(int8_t ttl, int8_t silent, int32_t key)
    {
        //CubeId must be known
        uint32_t cubeid = OCT_cubeid();
        if (cubeid > CUBES_COUNT) return;

        octMsgPong_t m = {};
        m.Key = key;

        //Allocate new packet and fill it
        octPacket_t* pkt = OCT_NET_packet_add(&m.Header, (octStreamId_t)(STREAM_MSGS_C0 + cubeid), OctSyncedTick, PKT_MSG_PONG, ttl, (octPeerId_t)(PEER_C0+cubeid));
        if (pkt == NULL) OCT_terminate("Can't add msg packet to stream");

        //No need to really send packet in silent mode
        if (silent) return;

        //Send through each UART
        OCT_NET_forward(pkt, OCT_LINES_NUM);
    }


//This message is for tests, so it uses customized sending
void OCT_NET_msg_ping(int8_t ttl, uint32_t targets, int32_t key, int8_t drop, int8_t silent)
    {
        //CubeId must be known
        uint32_t cubeid = OCT_cubeid();
        if (cubeid > CUBES_COUNT) return;

        //Payload
        octMsgPing_t m = {};
        m.Targets = targets, m.Key = key, m.Drop = drop, m.Silent = silent;
        
        //Allocate new packet and fill it
        octPacket_t* pkt = OCT_NET_packet_add(&m.Header, (octStreamId_t)(STREAM_MSGS_C0 + cubeid), OctSyncedTick, PKT_MSG_PING, ttl, (octPeerId_t)(PEER_C0+cubeid));
        if (pkt == NULL) OCT_terminate("Can't add msg packet to stream");

        //No need to really send packet in silent mode
        if (silent) return;

        //Send through each UART
        OCT_NET_forward(pkt, OCT_LINES_NUM);
    }


//Logic handler of each message packet type
//Messages are processed in sequential order and without any gaps
void OCT_NET_msgs_process()
    {
        for (uint32_t stream_idx = 0; stream_idx < STREAMS_TOTAL; stream_idx++)
        {
            //Only msg streams
            octStream_t* stream = &Streams[stream_idx];
            if (stream_idx == STREAM_CMDS || stream_idx == STREAM_BLOBS) continue;

            //No need to process own BT stream
            if (stream == OCT_BT_our_stream()) { stream->Processed = stream->Unprocessed; continue; }
            

            //[TODO]: don't even check if don't interested in somebody's messages
            //Process available messages 
            for (uint32_t seq = stream->Processed + 1; seq <= stream->Unprocessed; stream->Processed++, seq++)
            {
                //Stop on absent packet
                int idx             = seq % stream->Cap;
                octPacket_t* pkt    = &stream->Packets[idx];
                if (seq != pkt->Header.Seq)
                    break;
                
                //[TODO]: make insert and stream get 
                //Prevent seeing partially copied data by acquiring lock to make sure entire packet was copied. We already know that packet's seq is valid; so just wait in case some pending memcpy currently is still in progress
                if (xSemaphoreTake(stream->Mutex, portMAX_DELAY) != pdTRUE) break;
                xSemaphoreGive(stream->Mutex);

                //Blobs flow initiation
                if (pkt->Header.Type == PKT_MSG_BLOB_OFFER)
                {
                    //Only leader approves or declines
                    if (OCT_is_leader())
                    {
                        octMsgBlobOffer_t* offer = (octMsgBlobOffer_t*)pkt;
                        OCT_NET_msg_trace(OCT_time(), 0, 0, 0, 0, "PKT_MSG_BLOB_OFFER peer %d, #%d: %s\n", offer->Header.Peer, offer->Header.Seq, offer->Name);

                        //Only if no other download is already in progress
                        if (JobDownload.State != JOB_STATE_OFF) 
                        {
                            //[TODO]: Notify about declined offer
                            JobDownload.State = JOB_STATE_OFF; ///TMP HACK
                            continue;
                        }
                        
                        //That's some other peer
                        //if (offer->SenderPeerId != OCT_cubeid())
                        //{
                            //[TODO]: Any pre-acceptance validations?

                            //Ask others if they are ready
                            OCT_NET_msg_blob_annonce(&JobDownload.Announce, offer->BlobType, offer->Flags, offer->Hash, offer->Size, offer->Offset, offer->SenderPeerId, OCT_NET_blob_stream()->Unprocessed, offer->Header.Seq, offer->Name);
                            JobDownload.State = JOB_STATE_SYNCING_BEFORE;
                        //}
                    }
                    
                    continue;
                }

                //Prepare for blobs
                if (pkt->Header.Type == PKT_MSG_BLOB_ANNOUNCE)
                {
                    octMsgBlobAnnounce_t* announce = (octMsgBlobAnnounce_t*)pkt;
                    OCT_NET_msg_trace(OCT_time(), 0, 0, 0, 0, "PKT_MSG_BLOB_ANNOUNCE %d peer %d, #%d: %s\n", announce->BlobType, announce->Header.Peer, announce->OfferSeq, announce->Name);

                    /*
                    if (xSemaphoreTake(JobDownload.Mutex, portMAX_DELAY) == pdTRUE)
                    {
                        memcpy(&JobDownload.Offer, offer, sizeof(octMsgBlobOffer_t));
                        xSemaphoreGive(JobDownload.Mutex);
                    }*/
                    
                    //[TODO]: On followers finalize first, if it isn't in JOB_STATE_OFF 
                    JobDownload.State = JOB_STATE_OFF;
                    
                    //Report to leader this cube is ready
                    memcpy(&JobDownload.Announce, announce, sizeof(octMsgBlobAnnounce_t));
                    memset(JobDownload.Reports, 0, sizeof(JobDownload.Reports));
                    JobDownload.Crc = JobDownload.Written = JobDownload.Erased = JobDownload.Buffered = 0;
                    if (announce->BlobType == BLOB_TYPE_FIRMWARE || announce->BlobType == BLOB_TYPE_FIRMWARE_DEV) JobDownload.DiskOffset = OCT_FIRMWARE_DOWNLOAD_ADDRESS;
                    if (announce->BlobType == BLOB_TYPE_BOOTLOAD || announce->BlobType == BLOB_TYPE_BOOTLOAD_DEV) JobDownload.DiskOffset = OCT_BOOTLOAD_DOWNLOAD_ADDRESS;

                    if (JobDownload.State == JOB_STATE_OFF) JobDownload.State = JOB_STATE_SYNCING_BEFORE;
                    OCT_NET_msg_job_download_report(announce->OfferSeq, JobDownload.State);
                    continue;
                }

                if (pkt->Header.Type == PKT_MSG_PING)
                {
                    //Pong back if we are included as a ping's target
                    octMsgPing_t* m = (octMsgPing_t*)pkt;
                    if ((m->Targets & (1 << OCT_cubeid())) != 0)
                    {
                        OCT_NET_msg_pong(OCT_TTL_MAX, m->Silent, m->Key);
                    }
                    continue;
                }

                if (pkt->Header.Type == PKT_MSG_REPORT)
                {
                    octMsgReport_t* rep = (octMsgReport_t*)pkt; 
                    OCT_trace(-1, "PKT_MSG_REPORT %d\n", rep->State);
                    continue;
                }

                if (pkt->Header.Type == PKT_MSG_REBOOT)
                {
                    OCT_trace(-1, "PKT_MSG_REBOOT\n");
                    RTOS_sleep(500);
                    RTOS_systemReset();
                    continue;
                }

                if (pkt->Header.Type == PKT_MSG_BACKLIGHT)
                {
                    octMsgBacklight_t* m = (octMsgBacklight_t*)pkt; 
                    if (m->Percent == 0) BACKLIGHT_setOff(); else BACKLIGHT_setOn();
                    //BACKLIGHT_setBacklightLevel((backlightLvl_t)m->Percent); ??
                    continue;
                }


            }
        }
    }