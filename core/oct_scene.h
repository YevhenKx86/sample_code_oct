#pragma once
#include "oct_tm.h"
#include "oct_pack.h"
#include "oct_shared.h"






//Forward declarations
void OCT_terminate(const char* txt);


//Erase all objects from scene
void OCT_clear()
    {
        if (OctCap < 16) OctCap = 16;
        OCT_pool_init(OctMem, OctStride, OctCap);
        OCT_pool_init((int*)OctLabels, sizeof(octLabel_t)/4, OCT_LABELS_CAP);
    }


//
/*
void OCT_clone(octSprite_t* s, bool with_childs)
    {
        //Allocate new item
        //Copy everything
        //Set new coords

    }
    */

//Free sprite's slot
void OCT_del(octSprite_t* s)
    {
        if (s->Idx == 0) { OCT_terminate("zero del");  return; }
        if (s->Idx != OctMem[s->Idx * OctStride]) { OCT_terminate("double del");  return; }

        //Kill child sprites
        for (int i = 1; i < OctCap; i++) if (SPRITE_IDX(i) == i && SPRITE_PARENT(i) == s->Idx) OCT_del(SPRITE_PTR(i));

        //Clear label text
        if (s->Label && s->Data1 > 0) OCT_pool_dealloc((int*)OctLabels, sizeof(octLabel_t)/4, s->Data1),  s->Data1 = 0;

        //Free sprite's slot
        OCT_pool_dealloc(OctMem, OctStride, s->Idx);
    }


//Setup frame animation sequence. Zeroes means keep current value.
void OCT_sequence(octSprite_t* spr, int from, int to, int framerate, octSeqRestart_t restart)
    {
        //Substitute current frame
        if (from > 0) spr->FrameFrom = (int16_t)from;
        if (to > 0)   spr->FrameTo = (int16_t)to;

        //Override speed
        if (framerate >= 0) spr->FrameRate = (int8_t)framerate;

        //Forced restart, forward or backward
        if (restart == OCT_SEQ_RESTART)             spr->Frame = spr->FrameFrom,  spr->FAccum = OCT_BMP_get(spr->Frame)->Rate * spr->FrameRate,  spr->Reverse = 0;
        else if (restart == OCT_SEQ_REVERSE)        spr->Frame = spr->FrameTo,    spr->FAccum = OCT_BMP_get(spr->Frame)->Rate * spr->FrameRate,  spr->Reverse = 1;

        //Single frame animation equals to already ended animation
        //if (spr->FrameFrom == spr->FrameTo) spr->FAccum = 255;

        //Prevent accidental processing by app logic before next tick
        spr->InCooldown = 0,  spr->OnEnd = 0,  spr->OnCooldown = 0;

    }

/*
void OCT_sequence_delayed(octSprite_t* spr, int delay_ticks, int from, int to, int framerate, octSeqRestart_t restart)
    {
        //spr->FAccum
        OCT_sequence(spr, from, to, framerate, OCT_SEQ_REFRESH);
    }
*/

//Update sprite animation state
static void OCT_tick_sequence(octSprite_t* spr)
    {
        //Even still objects are aging
        spr->AgeTicks++;

        //Suicide
        if (spr->Autokill  &&  spr->AgeTicks >= spr->AgeMark) { OCT_del(spr); return; }

        //Reset single-frame signals
        spr->OnEnd = 0,  spr->OnCooldown = 0;

        //Static frame
        if (spr->Paused) return;

        //Animation reached it's end
        if (spr->FAccum == 255) return;

        //Countdown current frametime
        if (spr->FAccum > 0) { spr->FAccum--; return; }

        //Reverse allows to keep FrameTo and FrameFrom always the same
        int16_t endframe = (!spr->Reverse ? spr->FrameTo : spr->FrameFrom);

        //Accum is on zero, so new frame might be needed
        if (spr->Frame != endframe)
        {
            //Switch to a new frame
            if (spr->Frame < endframe) spr->Frame++; else spr->Frame--;

            //Find out frametime
            const octBmp_t* bmp = OCT_BMP_get(spr->Frame);
            spr->FAccum = bmp->Rate * spr->FrameRate;
            return;
        }

        //End of sequence
        if (!spr->InCooldown) spr->OnEnd = 1;

        //Cooldown
        if (spr->FramesCooldown > 0)
        {
            //Start or end cooldown
            if (!spr->InCooldown)   { spr->FAccum = spr->FramesCooldown;  spr->InCooldown = 1;  return; }
            else                    { spr->OnCooldown = 1; spr->InCooldown = 0; }
        }

        //Switch direction in ping-pong mode
        if (spr->PingPong)
        {
            //uint16_t tmp = spr->FrameFrom;    spr->FrameFrom = spr->FrameTo;  spr->FrameTo = tmp;
            spr->Reverse = spr->Reverse ^ 1;

            //[TODO]: Once if not looped
            //if (spr->Reverse == 0  && )
            //return;
        }

        //Start another loop
        if (spr->Loop)
        {
            //Replay, but keep events
            int on_end = spr->OnEnd,  on_cooldown = spr->OnCooldown;
            OCT_sequence(spr, 0, 0, 0, (spr->Reverse ? OCT_SEQ_REVERSE : OCT_SEQ_RESTART));
            spr->OnEnd = on_end,  spr->OnCooldown = on_cooldown;
            return;
        }

        //Stop animation
        spr->FAccum = 255;
    }



//Add sprite to the scene
int OCT_add(int layer, bool twistable, int plane, float x, float y, int a,  bool loop, int bmpfrom, int bmpto, int framelen)
    {
        if (bmpfrom < 0 || bmpto < 0) { OCT_terminate("bmp OOB"); return 0; }

        octSprite_t* o = NULL;
        int oidx = OCT_pool_alloc(OctMem, OctStride);
        if (oidx == 0) return 0;
        o = SPRITE_PTR(oidx);

        //10+ side means coordinates are already in scene space
        bool ignore_borders = false;
        if (plane >= 9)  plane -= 10,  ignore_borders = true;

        //Sprite transform
        OCT_TM_set(&o->Tm, (float)x, (float)y, a, plane);

        //Account for thickness
        if (plane >= 0  &&  !ignore_borders)
        {
            int q = (OCT_TM_quad(&o->Tm)) % 4;
            o->Tm.X += XSIGN[q] * INNER_DELTA,  o->Tm.Y += YSIGN[q] * INNER_DELTA;
        }

        //Setup added sprite
        o->Twistable    = twistable;
        o->Layer        = (int8_t)layer;
        o->Loop         = loop;
        o->Mask         = 0x00FFFFFF;
        o->DynPal       = 0;

        //o->FlipH = o->FlipV = 1;
        OCT_sequence(o, bmpfrom, bmpto, framelen, OCT_SEQ_RESTART);
        return o->Idx;
    }


//Add label object - sprite without bitmap
int OCT_add_label(int layer, bool twistable, int side, float x, float y, int a, int font_idx, int align)
    {
        //Asserts
        if (font_idx < 1 || font_idx > 3) OCT_terminate("Font index is out of [1..3] range");

        //Allocate text storage
        int slot = OCT_pool_alloc((int*)OctLabels, sizeof(octLabel_t)/4);

        //No space for text
        if (slot <= 0) return 0;

        //Allocate object for label
        int oidx = OCT_add(layer, twistable, side, x, y, a, false, 0, 0, 0);
        if (oidx <= 0) return 0;

        //Setup label storage
        SPRITE_PTR(oidx)->Label = (uint16_t)font_idx;
        SPRITE_PTR(oidx)->Data1 = (uint8_t)slot;
        SPRITE_PTR(oidx)->Data2 = (uint8_t)align;
        return oidx;
    }


//Set label letters
int OCT_label_set(octSprite_t* label, const char* text)
    {
        //Not valid label or no need to change text
        if (!label->Label || label->Data1 == 0 || label->Data1 >= OCT_LABELS_CAP || !strncmp(OctLabels[label->Data1].Text, text, OCT_LABEL_LEN)) return 0;

        //Number of sprites needed
        int len = (int)strlen(text);
        if (len > OCT_LABEL_LEN) len = OCT_LABEL_LEN;
        uint16_t p = 0, zoom = 2;
        float cx = 0;
        float cy = 0;

        //Del old letters [TODO]: reuse
        for (int i = 1; i < OctCap; i++) if (SPRITE_IDX(i) == i && SPRITE_PARENT(i) == label->Idx) OCT_del(SPRITE_PTR(i));

        //Get the n-space
        char letter[32];
        sprintf(letter, "font_%d_%05d", label->Label, (int)'n');
        int bmpidx = OCT_PACK_getSpriteIdByName(letter);
        if (bmpidx < 0) { return 0; }
        const octBmp_t* bmp = OCT_BMP_get(bmpidx);
        if (OctPals[bmp->Pidx].Flags & OCT_FLAG_FULLSIZE) zoom = 1;
        float space = (float)zoom * bmp->W;

        //Add new letter
        int line[128], linelen = 0;
        while (p != len)
        {
            //bool finalize_line = false;
            int code = text[p++];
            if (code == '\n')
            {
                cy -= bmp->Bh;
            }
            else
            {
                sprintf(letter, "font_%d_%05d", label->Label, code);
                bmpidx = OCT_PACK_getSpriteIdByName(letter);
                if (bmpidx < 0) { cx += space; continue;}

                bmp = OCT_BMP_get(bmpidx);
                int i = OCT_add(label->Layer, false, -1, cx, cy, 0, false, bmpidx, bmpidx, 0);
                SPRITE_PARENT(i) = label->Idx;
                SPRITE_PTR(i)->Data1 = (uint8_t)p;
                SPRITE_PTR(i)->Letter = 1;
                cx += zoom * (bmp->Bw - bmp->PivotX)     / 1;     //////////////
                line[linelen++] = i;
            }

            //End of line
            if (p == len || code == '\n')
            {
                //Set caret to end of last char, not the beginning of the next one
                cx += zoom * (bmp->W - bmp->Bw);

                //Central alignment
                if (label->Data2 == ALIGN_CENTER)
                    for (int k = 0; k < linelen; k++) SPRITE_PTR(line[k])->Tm.X -= cx/2.0f;     /// - label->Zw/2.0f;
                if (label->Data2 == ALIGN_RIGHT)
                    for (int k = 0; k < linelen; k++) SPRITE_PTR(line[k])->Tm.X -= cx/1.0f;     /// - label->Zw/2.0f;


                cx = 0, linelen = 0;
            }
        }

        //Save string
        OCT_strcpy(OctLabels[label->Data1].Text, OCT_LABEL_LEN, text);
        return len;
    }


//Add predefined objects to the scene
void OCT_add_map(int mapbmp)
    {
        //Parse header
        int* raw = (int*)OCT_PACK_getSprite(mapbmp);
        int version = raw[1], placesnum = raw[2];
        const octPlace_t* placess = (octPlace_t*)(raw + 3);
        const octPlace_t* plc = placess;

        //Prevent unknown version
        if (version != 1) { OCT_terminate("Unknown map version"); return; }

        //Map place to scene sprite index
        int xlat[256] = {0};
        int fresh_oidx[OCT_SPRITES_CAP], fresh_count = 0;

        //Spawn objects
        int layer = 0;
        for (int i = 0; i < placesnum; i++, plc++)
        {

            //Apply layer mark
            if (plc->BmpIdx == 0 && plc->Name == 0 && plc->Tags == 0 && plc->Type == 0 && plc->Number > 0) { layer = plc->Number; continue; }

            //Walk to the end of sequence
            int16_t bmpidx = plc->BmpIdx;
            if (bmpidx != 0)
                for (const octBmp_t* bmp = OCT_BMP_get(bmpidx); bmp->Seq == 1;) bmp = OCT_BMP_get(++bmpidx);

            //Create sprite, even without bitmap
            int idx = OCT_add(layer, false, OCT_PLANE_WITH_BORDER + plc->Side, (float)plc->X, (float)plc->Y, plc->Rot * 90, plc->Looped, plc->BmpIdx, bmpidx, 1);
            octSprite_t* spr = SPRITE_PTR(idx);
            xlat[plc->Name] = idx;
            spr->Tags = plc->Tags,  spr->Name = plc->Name,  spr->Type = plc->Type,  spr->Group = plc->Group,  spr->Number = plc->Number,  spr->Hidden = plc->Hidden,  spr->Twistable = plc->Twistable,  spr->PingPong = plc->PingPong,  spr->Label = plc->Label, spr->Paused = plc->Paused;
            spr->Parent = plc->Parent, spr->Fresh = 1, fresh_oidx[fresh_count++] = idx;
            spr->FlipH = plc->FlipH, spr->FlipV = plc->FlipV,  spr->Zw = plc->W,  spr->Zh = plc->H;

            //Additional setup for label node
            if (spr->Label)
            {
                spr->Data1 = (uint8_t)OCT_pool_alloc((int*)OctLabels, sizeof(octLabel_t)/4);
                spr->Data2 = plc->Rate;
                if (spr->Data1 <= 0) { OCT_terminate("No text space for label from map"); return; }
            }
        }

        //Resolve parents
        for (int i = 0; i < fresh_count; i++)
        {
            octSprite_t* spr = SPRITE_PTR(fresh_oidx[i]);
            if (spr->Parent != 0)
            {
                spr->Parent = xlat[spr->Parent];
                spr->Tm.X -= SPRITE_PTR(spr->Parent)->Tm.X;
                spr->Tm.Y -= SPRITE_PTR(spr->Parent)->Tm.Y;
            }
        }
    }


//Transform twistable sprites
void OCT_twist_sprites(octTwistId_t twid)
    {
        if (twid >= 0 && twid < OCT_TWIST_HALF)
        {
            for (int j = 1; j < OctCap; j++)
            {
                octSprite_t* s = SPRITE_PTR(j);
                if (s->Idx == j && s->Twistable)
                    OCT_TM_twist(&s->Tm, twid);
            }

            //Correct animation tms of the twistable objects
            for (int j = 1; j < ANIMS_CAP; j++)
            {
                if (Anims[j].Idx == j && Anims[j].Twistable)
                {
                    if(!Anims[j].RefFrom)   OCT_TM_twist((octTm_t*)Anims[j].From, twid);
                    if(!Anims[j].RefTo)     OCT_TM_twist((octTm_t*)Anims[j].To, twid);
                }
            }
        }
    }

