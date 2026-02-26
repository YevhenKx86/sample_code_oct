#pragma once
#include "oct_shared.h"

int SND_play(int id, int volume);
int SND_getAssetId(const char* name);
int SND_cacheSounds(int* ids, int num);

void    OCT_terminate(const char* txt);

int     OCT_restart(int* sprites, int spritescap, int spritesize);
void    OCT_run();
int     OCT_add(int layer, bool twistable, int plane, float x, float y, int a, bool loop, int bmpfrom, int bmpto, int framelen);
void    OCT_add_map(int mapid);
int     OCT_add_label(int layer, bool twistable, int side, float x, float y, int a, int font_idx, int align);
void    OCT_del(octSprite_t* spr);
int     OCT_random(int dmin, int dmax);
void    OCT_sequence(octSprite_t* spr, int from, int to, int framelen, octSeqRestart_t restart);
void    OCT_text(int string_index, const char* format, ...);
void    OCT_modify_viewport(int vid, int plane, int angle, float x, float y, int xflip, int yflip, int mode);
void    OCT_viewports_layout(int scheme, int inside_border_width, int outside_border_width);
void    OCT_twist_sprites(octTwistId_t twid);
void    OCT_trace(int cubeid, const char* format, ...);
int     OCT_label_set(octSprite_t* label, const char* text);
int     OCT_dev_mode(int mode);
int     OCT_acc_visual_x();
int     OCT_acc_visual_y();
int     OCT_acc_visual_z();
int     OCT_get_progress(int type);
void    OCT_background(int32_t rgb565);
int     OCT_disconnected_axis();
void    OCT_align_top();

void    OCT_BMP_info(uint32_t bmp_idx, octBmpInfo_t* info);

int     OCT_ANIM_tm(float arc, int delay_ticks, int duration_ticks, void* context, int func, octTm_t *from, octTm_t *to, bool reffrom, bool refto, octTm_t *target);
int     OCT_ANIM_tm_tw(float arc, int delay_ticks, int duration_ticks, void* context, int func, octTm_t *from, octTm_t *to, bool reffrom, bool refto, octTm_t *target);
int     OCT_ANIM_on_end(int aidx);
int     OCT_ANIM_on_half(int aidx);
void    OCT_ANIM_del(int aidx);
int     OCT_ANIM_get_progress(int aidx);

void    OCT_TM_copy(octTm_t* dst, const octTm_t* src);
void    OCT_TM_move(octTm_t* tm, float dx, float dy);
int     OCT_TM_adapt_angle(int oldside, int newside);
void    OCT_TM_change_plane(octTm_t* tm, int to);
void    OCT_TM_combine(octTm_t* tm, const octTm_t* t, bool checkside);
float   OCT_TM_gravity_x(int plane);
float   OCT_TM_gravity_y(int plane);
float   OCT_TM_gravity_n(int plane);
int     OCT_TM_top_side();
int     OCT_TM_bottom_side();

void    OCT_TM_twist(octTm_t* tm, octTwistId_t twid);
int     OCT_TM_twist_impulse(int quad, octTwistId_t twid);
int     OCT_TM_arctan(int y, int x);
float   OCT_TM_sin(int deg);
float   OCT_TM_cos(int deg);
int     OCT_TM_quad(const octTm_t* tm);
void    OCT_TM_set(octTm_t* tm, float x, float y, int a, int plane);
void    OCT_TM_lerp(octTm_t* tm, octTm_t* a, octTm_t* b, float t);
int     OCT_TM_walk(octTm_t* tm, int forward_direction_angle, float forward_distance, float left_distance, bool wrap);
void    OCT_TM_wrap(octTm_t* tm);
void    OCT_TM_adapt_dir(octTm_t* tm, int oldside, int newside);


inline float OCT_lerp(float from, float to, float t) { return from*t + to*(1.0f-t); }