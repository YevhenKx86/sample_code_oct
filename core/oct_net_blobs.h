#pragma once
#include "oct_net.h"



void OCT_BT_emit_blob(uint8_t* data, uint32_t data_size, int32_t blob_id, int32_t total, int32_t offset, int32_t useful)
    {
        //[TODO]: raise an error
        //Only server can emit
        if (!OCT_is_leader() || !BT_isConnected() || data_size > BLOB_MAX) return;

        //[TODO]: Limit here and in net emitting if we have no free place in stream; return bool so external code can retry later

        //[TODO]: Useless copying
        octBlob_t blob;
        memcpy(blob.Data, data, data_size);
        blob.BlobId = blob_id, blob.Total = total, blob.Offset = offset, blob.Useful = useful;

        octPacket_t* pkt = OCT_NET_packet_add(&blob.Header, STREAM_BLOBS, OctSyncedTick, PKT_BLOB, OCT_TTL_FOR_BROADCAST, OCT_NET_our_peer());
        if (pkt == NULL) OCT_terminate("Can't add PKT_BLOB packet to stream");

        ///OCT_trace(0, "OCT_BT_emit_blob #%d: %d at %d, %d\n", pkt->Header.Seq, total, offset, data_size);
        BT_sendSPPData((uint8_t*)(pkt), sizeof(blob));
    }


void OCT_BT_emit_blob_forced(int32_t blob_id, int32_t total, int32_t mode, uint32_t hash, const char* name)
    {
        if (!OCT_is_leader() || !BT_isConnected()) return;

        octBlobForced_t forced;
        forced.BlobId = blob_id, forced.Total = total, forced.Mode = mode, forced.Hash = hash, OCT_strcpy(forced.Name, OCT_ASSET_NAME_MAXLEN, name);

        octPacket_t* pkt = OCT_NET_packet_add(&forced.Header, STREAM_BLOBS, OctSyncedTick, PKT_BLOB_FORCED, 3, OCT_NET_our_peer());
        if (pkt == NULL) OCT_terminate("Can't add PKT_BLOB_FORCED packet to stream");
        BT_sendSPPData((uint8_t*)(pkt), sizeof(forced));
    }





