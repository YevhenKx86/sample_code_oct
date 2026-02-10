#pragma once
//--------------------------------------------------------------------------------------------------------
//Coordinate transformations in a matrix-like manner.
//--------------------------------------------------------------------------------------------------------

#include <string.h>
#include "oct_types.h"
#include "oct_helpers.h"
#include "oct_shared.h"     //octTm_t



//Mask of color channels most signed bits, and a mask excluding those bits
#define MSBS 0x00008410
#define NSBS 0x00007BEF


//From col's side to row's: dx multiplier, dy multiplier, dangle, cos(da), sin(da)
const int OCT_CONV[6*6*5] = {
    0,0,0,1,0,          0,-1,0,1,0,     1,0,90,0,1,     0,1,180,-1,0,       -1,0,-90,0,-1,      0,0,0,1,0,
    0,1,0,1,0,          0,0,0,1,0,      1,0,0,1,0,      0,0,0,1,0,          -1,0,0,1,0,         0,-1,0,1,0,
    0,1,-90,0,-1,       -1,0,0,1,0,     0,0,0,1,0,      1,0,0,1,0,          0,0,0,1,0,          0,-1,90,0,1,
    0,1,-180,-1,0,      0,0,0,1,0,      -1,0,0,1,0,     0,0,0,1,0,          1,0,0,1,0,          0,-1,180,-1,0,
    0,1,90,0,1,         1,0,0,1,0,      0,0,0,1,0,      -1,0,0,1,0,         0,0,0,1,0,          0,-1,-90,0,-1,
    0,0,0,1,0,          0,1,0,1,0,      1,0,-90,0,-1,   0,-1,-180,-1,0,     -1,0,90,0,1,        0,0,0,1,0
};


const float OCT_SIN[360] = {
    0.000f,0.017f,0.035f,0.052f,0.070f,0.087f,0.105f,0.122f,0.139f,0.156f,0.174f,0.191f,0.208f,0.225f,0.242f,0.259f,0.276f,0.292f,0.309f,0.326f,0.342f,0.358f,0.375f,0.391f,0.407f,0.423f,0.438f,0.454f,0.469f,0.485f,
    0.500f,0.515f,0.530f,0.545f,0.559f,0.574f,0.588f,0.602f,0.616f,0.629f,0.643f,0.656f,0.669f,0.682f,0.695f,0.707f,0.719f,0.731f,0.743f,0.755f,0.766f,0.777f,0.788f,0.799f,0.809f,0.819f,0.829f,0.839f,0.848f,0.857f,
    0.866f,0.875f,0.883f,0.891f,0.899f,0.906f,0.914f,0.921f,0.927f,0.934f,0.940f,0.946f,0.951f,0.956f,0.961f,0.966f,0.970f,0.974f,0.978f,0.982f,0.985f,0.988f,0.990f,0.993f,0.995f,0.996f,0.998f,0.999f,0.999f,1.000f,
    1.000f,1.000f,0.999f,0.999f,0.998f,0.996f,0.995f,0.993f,0.990f,0.988f,0.985f,0.982f,0.978f,0.974f,0.970f,0.966f,0.961f,0.956f,0.951f,0.946f,0.940f,0.934f,0.927f,0.921f,0.914f,0.906f,0.899f,0.891f,0.883f,0.875f,
    0.866f,0.857f,0.848f,0.839f,0.829f,0.819f,0.809f,0.799f,0.788f,0.777f,0.766f,0.755f,0.743f,0.731f,0.719f,0.707f,0.695f,0.682f,0.669f,0.656f,0.643f,0.629f,0.616f,0.602f,0.588f,0.574f,0.559f,0.545f,0.530f,0.515f,
    0.500f,0.485f,0.469f,0.454f,0.438f,0.423f,0.407f,0.391f,0.375f,0.358f,0.342f,0.326f,0.309f,0.292f,0.276f,0.259f,0.242f,0.225f,0.208f,0.191f,0.174f,0.156f,0.139f,0.122f,0.105f,0.087f,0.070f,0.052f,0.035f,0.017f,
    -0.000f,-0.017f,-0.035f,-0.052f,-0.070f,-0.087f,-0.105f,-0.122f,-0.139f,-0.156f,-0.174f,-0.191f,-0.208f,-0.225f,-0.242f,-0.259f,-0.276f,-0.292f,-0.309f,-0.326f,-0.342f,-0.358f,-0.375f,-0.391f,-0.407f,-0.423f,-0.438f,-0.454f,-0.469f,-0.485f,
    -0.500f,-0.515f,-0.530f,-0.545f,-0.559f,-0.574f,-0.588f,-0.602f,-0.616f,-0.629f,-0.643f,-0.656f,-0.669f,-0.682f,-0.695f,-0.707f,-0.719f,-0.731f,-0.743f,-0.755f,-0.766f,-0.777f,-0.788f,-0.799f,-0.809f,-0.819f,-0.829f,-0.839f,-0.848f,-0.857f,
    -0.866f,-0.875f,-0.883f,-0.891f,-0.899f,-0.906f,-0.914f,-0.921f,-0.927f,-0.934f,-0.940f,-0.946f,-0.951f,-0.956f,-0.961f,-0.966f,-0.970f,-0.974f,-0.978f,-0.982f,-0.985f,-0.988f,-0.990f,-0.993f,-0.995f,-0.996f,-0.998f,-0.999f,-0.999f,-1.000f,
    -1.000f,-1.000f,-0.999f,-0.999f,-0.998f,-0.996f,-0.995f,-0.993f,-0.990f,-0.988f,-0.985f,-0.982f,-0.978f,-0.974f,-0.970f,-0.966f,-0.961f,-0.956f,-0.951f,-0.946f,-0.940f,-0.934f,-0.927f,-0.921f,-0.914f,-0.906f,-0.899f,-0.891f,-0.883f,-0.875f,
    -0.866f,-0.857f,-0.848f,-0.839f,-0.829f,-0.819f,-0.809f,-0.799f,-0.788f,-0.777f,-0.766f,-0.755f,-0.743f,-0.731f,-0.719f,-0.707f,-0.695f,-0.682f,-0.669f,-0.656f,-0.643f,-0.629f,-0.616f,-0.602f,-0.588f,-0.574f,-0.559f,-0.545f,-0.530f,-0.515f,
    -0.500f,-0.485f,-0.469f,-0.454f,-0.438f,-0.423f,-0.407f,-0.391f,-0.375f,-0.358f,-0.342f,-0.326f,-0.309f,-0.292f,-0.276f,-0.259f,-0.242f,-0.225f,-0.208f,-0.191f,-0.174f,-0.156f,-0.139f,-0.122f,-0.105f,-0.087f,-0.070f,-0.052f,-0.035f,-0.017f
};

const float OCT_N3D[6][3] = {{0, 1, 0},    {0, 0,-1},   {1, 0, 0},    { 0, 0, 1},    {-1, 0, 0},   { 0,-1, 0}};
const float OCT_T3D[6][3] = {{1, 0, 0},    {1, 0, 0},   {0, 0, 1},    {-1, 0, 0},    { 0, 0,-1},   { 1, 0, 0}};
const float OCT_B3D[6][3] = {{0, 0, 1},    {0, 1, 0},   {0, 1, 0},    { 0, 1, 0},    { 0, 1, 0},   { 0, 0,-1}};


//Oppositing quads
const int OCT_OPP_QUAD[24]               = { 23, 22, 21, 20,   13, 12, 15, 14,   17, 16, 19, 18,   5, 4, 7, 6,   9, 8, 11, 10,   3, 2, 1, 0 };

//Sides adjacency by side's local +X +Y -X -Y
const int OCT_SIDE_ADJ[6 * 4] = { 2, 3, 4, 1,   2, 0, 4, 5,     3, 0, 1, 5,     4, 0, 2, 5,     1, 0, 3, 5,     2, 1, 4, 3  };
const int OCT_SIDE_OPP[6 * 1] = { 5,            3,              4,              1,              2,              0           };

//Maps hardware index of uart (3*cubeid+line_id) and screen (3*cubeid+display_id); same table to map one to another and vice versa
const int OCT_SWAP_UART_AND_SID[32] = {1, 0, 2,   4, 3, 5,   7, 6, 8,   10, 9, 11,   13, 12, 14,   16, 15, 17,   19, 18, 20,   22, 21, 23,   31, 31, 31, 31, 31, 31, 31, 31};

//Maps virtual axis to plane space: plane_N_x = virtual_space_vector[mapping[3*N + 0]]
typedef struct
{
    int VAxis[3];
    int VSign[3];
} octVirtualToDisplay;

//const octVirtualToDisplay OCT_VIRTUAL_TO_DISPLAY[] = { {{2,0,1},{1,1,1}}, {{2,0,1},{1,-1,1}}, {{2,0,1},{-1,-1,1}}, {{2,0,1},{-1,1,1}},
const octVirtualToDisplay OCT_VIRTUAL_TO_DISPLAY[] = {  {{0,2,1},{ 1, 1, 1}},   {{2,0,1},{1,-1,1}}, {{2,0,1},{-1,-1,1}}, {{2,0,1},{-1,1,1}},
                                                        {{0,1,2},{ 1, 1,-1}},   {{2,0,1},{1,-1,1}}, {{2,0,1},{-1,-1,1}}, {{2,0,1},{-1,1,1}},
                                                        {{2,1,0},{ 1, 1, 1}},   {{2,0,1},{1,-1,1}}, {{2,0,1},{-1,-1,1}}, {{2,0,1},{-1,1,1}},
                                                        {{0,1,2},{-1, 1, 1}},   {{2,0,1},{1,-1,1}}, {{2,0,1},{-1,-1,1}}, {{2,0,1},{-1,1,1}},
                                                        {{2,1,0},{-1, 1,-1}},   {{2,0,1},{1,-1,1}}, {{2,0,1},{-1,-1,1}}, {{2,0,1},{-1,1,1}},
                                                        {{0,2,1},{ 1, -1,-1}},  {{2,0,1},{1,-1,1}}, {{2,0,1},{-1,-1,1}}, {{2,0,1},{-1,1,1}}
};



//___________________________________________________________________________________________________________________________________________


void OCT_TM_copy(octTm_t* dst, const octTm_t* src)  { memcpy(dst, src, sizeof(octTm_t)); }

//These functions are forcing any angle to be in [0..359] range
int   OCT_TM_normalize_angle(int deg)   { deg = deg % 360;      if (deg < 0) deg += 360;    return deg; }
float OCT_TM_sin(int deg)               { deg = deg % 360;      if (deg < 0) deg += 360;    return OCT_SIN[deg]; }
float OCT_TM_cos(int deg)               { deg = (deg+90) % 360; if (deg < 0) deg += 360;    return OCT_SIN[deg]; }


//Approximation (max error = 0.25 degree)
int OCT_TM_arctan(int y, int x)
{
    //Catch singularities
    if (x == 0) { if (y == 0) return 0;
                  return (y > 0) ? 90 : -90; }

    //Classify quadrant
    const float to_deg = 57.29578f;
    if (x > 0)
    {
        if (y >= 0)     if (x > y)  return      (int)((x * y) * to_deg / (x*x + y*y*0.28125f) + 0.5f);
                        else        return  90 -(int)((x * y) * to_deg / (y*y + x*x*0.28125f) + 0.5f);
        else
                        if (x >-y)  return      (int)((x * y) * to_deg / (x*x + y*y*0.28125f) - 0.5f);
                        else        return -90 -(int)((x * y) * to_deg / (y*y + x*x*0.28125f) - 0.5f);
    }
    else
    {
        if (y >= 0)     if (-x> y)  return  180-    -(int)((x * y) * to_deg / (x*x + y*y*0.28125f) - 0.5f);
                        else        return  180- 90 -(int)((x * y) * to_deg / (y*y + x*x*0.28125f) - 0.5f);
        else
                        if (-x>-y)  return -180+     (int)((x * y) * to_deg / (x*x + y*y*0.28125f) + 0.5f);
                        else        return -180+ 90 -(int)((x * y) * to_deg / (y*y + x*x*0.28125f) + 0.5f);
    }
}


//
float OCT_TM_gravity_x(int side) { return 0.0f; }
float OCT_TM_gravity_y(int side) { return 0.0f; }
float OCT_TM_gravity_n(int side) { return 0.0f; }

int OCT_TM_top_side()
{
    int top = 0; float d_min = 1.0f;
    for (int i=0; i < SIDES_COUNT; i++)
    {
        float d = OCT_TM_gravity_n(i);
        if (d_min > d) d_min = d, top = i;
    }
    return top;
}

int OCT_TM_bottom_side()
{
    int bottom = 0; float d_max = -1.0f;
    for (int i=0; i < SIDES_COUNT; i++)
    {
        float d = OCT_TM_gravity_n(i);
        if (d_max < d) d_max = d, bottom = i;
    }
    return bottom;
}

//
int OCT_TM_twist_impulse(int quad, octTwistId_t twid)
    {
        //Check if sprite is turning on ring quads
        const octTwist_t* tw = &OCT_TWISTS[(uint32_t)twid % 12];

        //Lies on ring quads
        if ((tw->RingsMask >> quad) & 1) return tw->Impulse[quad/4];

        //Lies on disk quads
        if (quad/4 == tw->QuadsDisk[0]/4) return tw->Impulse[quad/4];

        //Not affected
        return 360;
    }


//Clean up instabilities after any transformations
void OCT_TM_fix(octTm_t* tm)
    {
        //Prevent ambiguous near-zero coords
        if (tm->X >= 0) { if (tm->X < 0.01f) tm->X = 0.01f; } else { if (tm->X > -0.01f) tm->X = -0.01f; }
        if (tm->Y >= 0) { if (tm->Y < 0.01f) tm->Y = 0.01f; } else { if (tm->Y > -0.01f) tm->Y = -0.01f; }

        //Same for border coords - push inside
        float eps = 0.01f;
        if (tm->X > LIM - eps && tm->X < LIM + eps)     tm->X = LIM - eps;  else
        if (tm->X >-LIM - eps && tm->X < -LIM + eps)    tm->X = -LIM + eps;

        if (tm->Y > LIM - eps && tm->Y < LIM + eps)     tm->Y = LIM - eps;  else
        if (tm->Y >-LIM - eps && tm->Y < -LIM + eps)    tm->Y = -LIM + eps;

        //Normalize the angle
        tm->A = tm->A % 360;
        if (tm->A < 0) tm->A += 360;
    }


//Correction for angle to keep same direction when changing side
int OCT_TM_adapt_angle(int oldside, int newside)
    {
        //Same
        if (oldside == newside) return 0;

        //Opposite side (both deltas are zero)
        int tt = newside*6*5 + oldside*5;
#ifdef OCTSIM
        if (OCT_CONV[tt + 0] == OCT_CONV[tt + 1]) __debugbreak(); //"opposite sides"
#endif

        //Angle delta
        return OCT_CONV[tt + 2];
    }


void OCT_TM_rotate(octTm_t* tm, int angle_delta)
    {
        //Rotate both angle and coordinates
        angle_delta = OCT_TM_normalize_angle(angle_delta);
        float x = tm->X * OCT_TM_cos(angle_delta) - tm->Y * OCT_TM_sin(angle_delta);
        float y = tm->X * OCT_TM_sin(angle_delta) + tm->Y * OCT_TM_cos(angle_delta);
        tm->X = x;
        tm->Y = y;
        tm->A += (int16_t)angle_delta;
    }


void OCT_TM_adapt_dir(octTm_t* tm, int oldside, int newside)
    {
        //Same sides
        if (oldside == newside) return;

        //Opposite side (both deltas are zero)
        int tt = newside*6*5 + oldside*5;
#ifdef OCTSIM
        if (OCT_CONV[tt + 0] == OCT_CONV[tt + 1]) __debugbreak(); //"opposite sides"
#endif

        //Rotate both angle and coordinates
        float x = (tm->X * OCT_CONV[tt + 3] - tm->Y * OCT_CONV[tt + 4]);
        float y = (tm->X * OCT_CONV[tt + 4] + tm->Y * OCT_CONV[tt + 3]);
        tm->X = x;
        tm->Y = y;
        tm->A += (int16_t)OCT_CONV[tt + 2];
    }


void OCT_TM_change_plane(octTm_t* tm, int to)
    {
        //Same plane, do nothing
        if (to == tm->Plane) return;

        //Index in table of transformations
        int tt = to*6*5 + tm->Plane*5;

        //Opposite side (both deltas are zero)
        if (OCT_CONV[tt + 0] == OCT_CONV[tt + 1])
        {
            /*
            //Select one of 4 possible ways
            octTm_t opttms[4];
            float distmin = 1e9f; int amin = 0;
            for (int a = 0; a < 4; a++)
            {
                //Change side twice
                OCT_TM_copy(&opttms[a], tm);
                OCT_TM_change_plane(&opttms[a], OCT_SIDE_ADJ[4*to + a]);
                OCT_TM_change_plane(&opttms[a], to);

                //Find the shortest path
                float ax = opttms[a].X - tm->X, ay = opttms[a].Y - tm->Y, dd = ax*ax+ay*ay;
                if (dd < distmin) distmin = dd, amin = a;
            }

            //Use result from an adjacent side that leads to geodesic distance
            OCT_TM_copy(tm, &opttms[amin]);*/
            OCT_terminate("OPPOSITE SIDES ARE CURRENTLY NOT SUPPORTED");
            return;
        }

        //Transform from the table
        float x = (tm->X * OCT_CONV[tt + 3] - tm->Y * OCT_CONV[tt + 4]);
        float y = (tm->X * OCT_CONV[tt + 4] + tm->Y * OCT_CONV[tt + 3]);
        tm->X = x + 2 * LIM * OCT_CONV[tt + 0];
        tm->Y = y + 2 * LIM * OCT_CONV[tt + 1];
        tm->A += (int16_t)OCT_CONV[tt + 2];
        tm->Plane = (int8_t)to;
        OCT_TM_fix(tm);
    }


void OCT_TM_wrap(octTm_t* tm)
    {
        for (;;OCT_TM_fix(tm))
        if (tm->X >= LIM)   OCT_TM_change_plane(tm, OCT_SIDE_ADJ[tm->Plane * 4 + 0]); else
        if (tm->Y >= LIM)   OCT_TM_change_plane(tm, OCT_SIDE_ADJ[tm->Plane * 4 + 1]); else
        if (tm->X < -LIM)   OCT_TM_change_plane(tm, OCT_SIDE_ADJ[tm->Plane * 4 + 2]); else
        if (tm->Y < -LIM)   OCT_TM_change_plane(tm, OCT_SIDE_ADJ[tm->Plane * 4 + 3]); else return;
    }


void  OCT_TM_set(octTm_t* tm, float x, float y, int a, int plane)   { tm->X = x, tm->Y = y, tm->A = (int16_t)a, tm->Plane = (int8_t)plane, OCT_TM_fix(tm); }
void  OCT_TM_point(octTm_t* tm, float x, float y)                   { tm->X = x, tm->Y = y, tm->A = 0, tm->Plane = -1; }

float OCT_TM_world_x(octTm_t* tm)                                   {   if (tm->Plane == 2 || tm->Plane == 4) return SIDE; else return tm->X; }
float OCT_TM_world_y(octTm_t* tm)                                   {   if (tm->Plane == 0 || tm->Plane == 5) return SIDE; else return tm->Y; }
float OCT_TM_world_z(octTm_t* tm) {
        if (tm->Plane == 0 || tm->Plane == 5) return tm->Y; else
        if (tm->Plane == 1 || tm->Plane == 3) return SIDE; else
        if (tm->Plane == 2 || tm->Plane == 4) return tm->X;
#ifdef OCTSIM
        __debugbreak(); //"No Z distance for unknown side"
#endif
        return 10000.0f;
    }


void OCT_TM_lerp(octTm_t* tm, octTm_t* a, octTm_t* b, float t)
    {
        //Clamp
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
        float s = 1.0f - t;

        //Convert to b's space then lerp
        memcpy(tm, a, sizeof(octTm_t));
        OCT_TM_change_plane(tm, b->Plane);
        tm->X = s * tm->X  +  t * b->X;
        tm->Y = s * tm->Y  +  t * b->Y;
        OCT_TM_fix(tm), OCT_TM_wrap(tm);
    }


void OCT_TM_move(octTm_t* tm, float dx, float dy)
    {
        tm->X += dx, tm->Y += dy, OCT_TM_fix(tm);
        OCT_TM_wrap(tm);
    }


int OCT_TM_walk(octTm_t* tm, int forward_direction_angle, float forward_distance, float left_distance, bool wrap)
    {
        float vx = OCT_TM_cos(forward_direction_angle), vy = OCT_TM_sin(forward_direction_angle);
        int oldside = tm->Plane;

        //(vy, -vx) is CW perpendicular aka right
        //tm->X += fwd * vx + right * vy,  tm->Y += fwd * vy - right * vx,  OCT_TM_fix(tm);

        //(-vy, vx) is CCW perpendicular aka left
        tm->X += forward_distance * vx - left_distance * vy,  tm->Y += forward_distance * vy + left_distance * vx,  OCT_TM_fix(tm);
        if (wrap) OCT_TM_wrap(tm);
        return oldside;
    }


int OCT_TM_quad(const octTm_t* tm)
    {
        int q = tm->Plane * 4;
        if (tm->Y >= 0) q += (tm->X >= 0) ? 0 : 1; else q += (tm->X >= 0) ? 3 : 2;
        return q;
    }


void OCT_TM_combine(octTm_t* tm, const octTm_t* t, bool checkside)
    {
#ifdef OCTSIM
        //Assert same side, unless we have an undefined one
        if (checkside  &&  t->Plane != tm->Plane) __debugbreak(); //"convert side first";
#endif

        float c = OCT_TM_cos(t->A), s = OCT_TM_sin(t->A);
        float x = tm->X * c - tm->Y * s;
        float y = tm->X * s + tm->Y * c;
        tm->X = x + t->X;
        tm->Y = y + t->Y;
        tm->A += t->A;
        tm->Plane = t->Plane;
        OCT_TM_fix(tm);
    }


//Applies given twist. Use only to process full twists (that is twid from 0 to 11).
void OCT_TM_twist(octTm_t* tm, octTwistId_t twid)
    {
        //Check if sprite is turning on ring quads
        const octTwist_t* tw = &OCT_TWISTS[(uint32_t)twid % 12];
        int q, qt, sq = OCT_TM_quad(tm);
        if ((tw->RingsMask >> sq) & 1)
            for (int j = 0; j < 4; j++)
            {
                //Find next quad in ring to know where this tm should move
                q = tw->QuadsRing1[j], qt = tw->QuadsRing1[(j+1)%4];
                if (sq != q) {
                    q = tw->QuadsRing2[j], qt = tw->QuadsRing2[(j+1)%4];
                    if (sq != q) continue;
                }

                //Shift sprite then convert it's side
                int tt = q/4*6*5 + qt/4*5; //col to row, but inverted to transform into qt's space
                OCT_TM_move(tm,  2*LIM*OCT_CONV[tt + 0], 2*LIM*(float)OCT_CONV[tt + 1]);
                return;
            }

        //Disk quads
        if (tm->Plane == (tw->QuadsDisk[0] / 4))
        {
            //Rotate sprite
            octTm_t rot;  rot.A = (twid&1) ? -90 : 90 /*OCT_ANGLES[twid]*/;  rot.Plane = tm->Plane;  rot.X = rot.Y = 0;
            OCT_TM_combine(tm, &rot, false);
            return;
        }
    }
