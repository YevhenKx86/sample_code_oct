#pragma once
#include <math.h>  //sqrtf sinf
//#include "aDefines"
//#include "aScene"
#include "oct_tm.h"


typedef struct
{
    int         Idx;                            //Unique id
    float       T;                              //Current parameter of tween function
    void*       Context;                        //Pointer to a data meaningful only to application
    void*       Target;                         //Where to put animated result
    int16_t     Progress, Duration;             //How much more time animation will take
    int16_t     Param;                          //Additional parameter modifying some trajectories
    uint8_t     Pause:1, RefFrom:1, RefTo:1, OnEnd:1, OnHalf:1, Twistable:1;
    uint8_t     Field:4, Func:4;
    uint8_t     From[12], To[12];               //Keeps either the value itself or the pointer to a value
} octAnim_t;


#define ANIMS_CAP 100
TL octAnim_t Anims[ANIMS_CAP];

//Forward declarations
void OCT_terminate(const char* txt);


void OCT_ANIM_restart()
    {
        OCT_pool_init((int*)Anims, sizeof(octAnim_t)/4, ANIMS_CAP);
    }


int OCT_ANIM_get_progress(int aidx)
    {
        if (aidx <= 0 || aidx >= ANIMS_CAP) return 0;
        return Anims[aidx].Progress;
    }


int OCT_ANIM_tm_tw(float arc, int delay_ticks, int duration_ticks, void* context, int func, octTm_t *from, octTm_t *to, bool reffrom, bool refto, octTm_t *target)
    {
        int idx = OCT_ANIM_tm(arc, delay_ticks, duration_ticks, context, func, from, to, reffrom, refto, target);
        if (idx == 0) return 0;
        Anims[idx].Twistable = 1;
        return idx;
    }


int OCT_ANIM_tm(float arc, int delay_ticks, int duration_ticks, void* context, int func, octTm_t *from, octTm_t *to, bool reffrom, bool refto, octTm_t *target)
    {
        //Add animation
        int idx = OCT_pool_alloc((int*)Anims, sizeof(octAnim_t)/4);
        //Debug->Trace("Add anim %d\n", )
        if (idx == 0) return 0;

        //Setup
        octAnim_t* a = &Anims[idx];
        a->Param = (int16_t)arc,  a->Progress = -(int16_t)delay_ticks,  a->Duration = (uint16_t)duration_ticks,  a->Context = context,  a->Target = target,  a->Pause = 0,  a->RefFrom = reffrom,  a->RefTo = refto,  a->OnEnd = 0,  a->OnHalf = 0,  a->Field = ANIM_TM,  a->Func = (uint8_t)func,  a->T = 0.0f;
        if (reffrom)    memcpy(a->From, &from, sizeof(void*));  else memcpy(a->From, from, sizeof(octTm_t));
        if (refto)      memcpy(a->To, &to, sizeof(void*));      else memcpy(a->To, to, sizeof(octTm_t));
        return idx;
    }


void OCT_ANIM_del(int aidx)
    {
        if (aidx <= 0 || aidx >= ANIMS_CAP) return;
        if (Anims[aidx].Idx == aidx)
            OCT_pool_dealloc((int*)Anims, sizeof(octAnim_t)/4, aidx);
    }


int OCT_ANIM_on_end(int aidx)
    {
        //Empty animation means no events, otherwise report current event-flag state
        if (aidx <= 0 || aidx >= ANIMS_CAP) return 0;
        octAnim_t* a = &Anims[aidx];
        return (a->Idx == aidx) ? a->OnEnd : 0;
    }


int OCT_ANIM_on_half(int aidx)
    {
        //Empty animation means no events, otherwise report current event-flag state
        if (aidx <= 0 || aidx >= ANIMS_CAP) return 0;
        octAnim_t* a = &Anims[aidx];
        return (a->Idx == aidx) ? a->OnHalf : 0;
    }


//Calculate parametric height of 3 points parabola
inline float OCT_ANIM_ballistic2(float h, float ys, float ye, float t)
    {
        float d = sqrtf((h - ys) / (h - ye));
        float xm1 = d / (d - 1);
        float xm2 = d / (d + 1);
        float xm = 1 - ((xm1 >= 0 && xm1 <= 1) ? xm1 : xm2);
        float k = (h - ye) / (xm*xm);
        float r = (t - xm);
        return h - k * r * r;
    }


// Function to calculate parabola height at T-point
float OCT_ANIM_ballistic(float H, float A, float B, float T)
{
    // Coefficients of the parabolic equation: y = p*x^2 + q*x + r
    float p, q, r;

    // Using the given points to solve for p, q, and r
    // Point 1: (0, A), which gives us the equation: r = A
    r = A;

    // Point 2: (0.5, H), which gives us the equation: 0.25p + 0.5q + r = H
    // Since we know r (A), we can simplify this to: 0.25p + 0.5q = H - A

    // Point 3: (1, B), which gives us the equation: p + q + r = B
    // Since we know r (A), we can simplify this to: p + q = B - A

    // Solving the system of equations to find p and q
    // From equation 3: q = B - A - p
    // Substituting q in equation 2 gives us: 0.25p + 0.5(B - A - p) = H - A
    // Simplifying: 0.25p - 0.5p = H - A - 0.5(B - A)
    // -0.25p = H - A - 0.5B + 0.5A
    // p = -4*(H - 0.5A - 0.5B)

    p = -4.0f * (H - 0.5f * A - 0.5f * B);
    q = B - A - p;

    // Using the parabolic equation to calculate the height at T-point
    float height = p * T * T + q * T + r;
    return height;
}



void OCT_ANIM_apply(octAnim_t* a)
    {
        static const float B1 = 1 / 2.75f, B2 = 2 / 2.75f, B3 = 1.5f / 2.75f, B4 = 2.5f / 2.75f, B5 = 2.25f / 2.75f, B6 = 2.625f / 2.75f;

        //Animation already has OnEnd set, auto delete [TODO]: Make option for this
        if (a->OnEnd != 0) { OCT_ANIM_del(a->Idx); return; }

        //Local tick
        a->Progress++;
        if (a->Progress < 0 || a->Progress > a->Duration) return;

        //Normalized progress
        float t = (float)(a->Progress) / (float)(a->Duration);

        //Reset single-frame signals
        a->OnEnd = 0,  a->OnHalf= 0;

        //Events
        if (t >= 1.0f) a->OnEnd = 1,  t = 1.0f;
        if (t >= 0.5f && a->OnHalf == 0) a->OnHalf = 1;

        //Convert progress to interpolator factor
        switch (a->Func)
        {
        case FUNC_BOUNCEOUT:
        {
            if      (t < B1) t = 7.5625f * t * t;
            else if (t < B2) t = 7.5625f * (t - B3) * (t - B3) + 0.75f;
            else if (t < B4) t = 7.5625f * (t - B5) * (t - B5) + 0.9375f;
            else             t = 7.5625f * (t - B6) * (t - B6) + 0.984375f;
            break;
        }

        case FUNC_LINEAR:   t = t;                                      break;
        case FUNC_ACC:      t = t*t;                                    break;
        case FUNC_DEC:      t = 1 - (1-t)*(1-t);                        break;
        case FUNC_QUAD:     t = (1 - (2*t-1) * (2*t-1));                break;
        case FUNC_FALL:     t = sqrtf(t*t*t);                           break;
        case FUNC_JELLY:    t = sinf(2 * 2 * OCT_PI * t) * (1 - t);     break;
        case FUNC_SPAWN:    t = sinf(4 * 2 * OCT_PI * t) * (t * t);     break;
        default:
            //Special case
            /*if (tw->Func == FUNC_BALLISTIC)
            {
                float dx = tw->EndRef[0] - tw->StartRef[0],  dy = tw->EndRef[1] - tw->StartRef[1];
                float h = 0.5f * sqrtf(dx*dx + dy*dy);
                if (h < 60) h = 60; //[TODO]: param somehow
                h += (tw->EndRef[1] > tw->StartRef[1]) ? tw->EndRef[1] : tw->StartRef[1];
                h = octBallisticHeight(h, tw->StartRef[1], tw->EndRef[1], t);
                tw->Target[0] = tw->StartRef[0] * (1.0f - t)  +  tw->EndRef[0] * t;
                tw->Target[1] = h;
                return;
            }
            */
            OCT_terminate("Unknown animator function type");
            break;
        }


        //What is to interpolate
        switch (a->Field)
        {
        /*case ANIM_TM_REL: //in local space, relative
        {
            //Get actual values
            octTm_t* target = (octTm_t*)a->Target;
            octTm_t from = *((octTm_t*) a->From);
            octTm_t to   = *((octTm_t*) a->To);

            //Convert to the same side
            OCT_TM_change_side(&to, from.Side);

            //What is change relative to previous frame
            float dt = t - a->T;    //Delta in parametric space per this tick


            a->T = t;
            break;
        }*/

        case ANIM_TM:   //in global space
        {
            //Get actual values
            octTm_t* target = (octTm_t*)a->Target;
            octTm_t from = a->RefFrom ? **((octTm_t**) a->From) : *((octTm_t*) a->From);
            octTm_t to   = a->RefTo   ? **((octTm_t**) a->To)   : *((octTm_t*) a->To);

            //Convert to the same side
            OCT_TM_change_plane(&to, from.Plane);
            target->Plane = from.Plane;

            //Find ballistic y
            float h = 0.5f * (to.Y + from.Y) + a->Param;
            target->Y = OCT_ANIM_ballistic(h, from.Y, to.Y, t);

            //Lerp x
            target->X = from.X * (1.0f - t)  +  to.X * t;
            OCT_TM_fix(target), OCT_TM_wrap(target);
            //OCT_TM_lerp(target, &from, &to, t);
            break;
        }
        }

            //octTm_t* from = a->RefFrom ? *((octTm_t**) a->From) : (octTm_t*) a->From;
            //octTm_t* to       = a->RefTo   ? *((octTm_t**) a->To)   : (octTm_t*) a->To;

        //Apply lerp
        //float s = 1.0f - t;
        //for (int i = 0; i < (int)tw->Field; i++)  tw->Target[i] = tw->StartRef[i] * s  +  tw->EndRef[i] * t;
    }




void OCT_ANIM_update()
    {
        for (int i = 1; i < ANIMS_CAP; i++)
        {
            octAnim_t* a = &Anims[i];
            if (a->Idx != i) continue;

            OCT_ANIM_apply(a);
        }
        //tick = 0;
    }


