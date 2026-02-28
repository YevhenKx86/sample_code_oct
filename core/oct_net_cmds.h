#pragma once
#include "oct_net.h"
#include "oct_app.h"

void OCT_compensate_twist(octTwistId_t twid);



//Start an app
void OCT_NET_cmd_app()
    {
        octCmdFile_t c = {};
        OCT_NET_emit_cmd(&c.Header, PKT_CMD_APP, false);
    }

void OCT_NET_cmd_layout(const int* topo)
    {
        //Fill and send
        octCmdLayout_t pkt = {0};
        for(int i = 0; i < CUBES_COUNT; i++) pkt.Quads[i] = (uint16_t) ((topo[3*i+2] & OCT_FIVE_BITS) << 10) | ((topo[3*i+1] & OCT_FIVE_BITS) << 5) | (topo[3*i+0] & OCT_FIVE_BITS);  //[TODO]: simplify
        OCT_NET_emit_cmd(&pkt.Header, PKT_CMD_LAYOUT, false);
    }


void OCT_NET_cmd_twist(uint32_t twistid, uint32_t disconnected_ms)
    {
        octCmdTwist_t c = {};  c.TwistId = twistid, c.DisconnectedMs = disconnected_ms;
        OCT_NET_emit_cmd(&c.Header, PKT_CMD_TWIST, false);
    }

void OCT_NET_cmd_tap(uint32_t tapid)
    {
        octCmdTap_t c = {};  c.TapId = tapid;
        OCT_NET_emit_cmd(&c.Header, PKT_CMD_TAP, false);
    }

void OCT_NET_cmds_process(uint32_t curtick)
    {
        //Run available commands
        for (uint32_t seq = OCT_NET_cmd_stream()->Processed; ; OCT_NET_cmd_stream()->Processed++)
        {
            //[TODO]: wrapping\delta
            //Process next packet
            seq++;
            if (seq > OCT_NET_cmd_stream()->Unprocessed) break;

            //Wrap around sequence number
            int index = seq % OCT_NET_cmd_stream()->Cap;

            //Ensure it is actual packet
            octPacket_t* p = &OCT_NET_cmd_stream()->Packets[index];
            if (p->Header.Seq != seq) break;

            //Time has not come yet (accounts lag compensation)
            if (curtick < p->Header.Stamp + OCT_LAG_COMPENSATION_TICKS) break;

            //
            if (p->Header.Type == PKT_CMD_LAYOUT)
            {
                //Set viewports binding to screens
                octCmdLayout_t* cmd = (octCmdLayout_t*)p;
                for(int m = 0; m < CUBES_COUNT; m++) OctViewports[3*m+0].Sid = cmd->Quads[m] & OCT_FIVE_BITS,  OctViewports[3*m+1].Sid = (cmd->Quads[m] >> 5) & OCT_FIVE_BITS,  OctViewports[3*m+2].Sid = (cmd->Quads[m] >> 10) & OCT_FIVE_BITS;

                //Refresh internal topology
                for (int q = 0; q < SCREENS_COUNT; q++) OctTopo[q] = OctViewports[q].Sid;

                ///OCT_set_state(OCT_STATE_PLAYING);
                continue;
            }

            //
            if (p->Header.Type == PKT_CMD_TWIST)
            {
                octCmdTwist_t* cmd = (octCmdTwist_t*)p;
                if (cmd->TwistId >= 100)
                    OCT_twist_sprites(cmd->TwistId-100),  OCT_on_twisted(cmd->TwistId-100, cmd->DisconnectedMs); //octTwistId_t
                else
                    OCT_compensate_twist(cmd->TwistId),  OCT_twist_sprites(cmd->TwistId),  OCT_on_twisted(cmd->TwistId, cmd->DisconnectedMs); //octTwistId_t
                continue;
            }

            //
            if (p->Header.Type == PKT_CMD_TAP)
            {
                octCmdTap_t* cmd = (octCmdTap_t*)p;
                OCT_on_tap(cmd->TapId);
                continue;
            }

            //
            if (p->Header.Type == PKT_CMD_APP)
            {
                OCT_set_state(OCT_STATE_APP);
                OCT_on_init();
                continue;
            }

            //Unknown cmd
            OCT_terminate("NIY");
        }

    }