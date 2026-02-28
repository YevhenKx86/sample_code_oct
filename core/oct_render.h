#pragma once
#include <math.h>  //sqrtf sinf
#include "oct_vars.h"
#include "oct_tm.h"
#include "oct_scene.h"
#include "oct_shared.h"
#include "oct_display.h"

#include "cmsis_compiler.h"

//
int OCT_acc_visual_x() { return 0; }
int OCT_acc_visual_y() { return 0; }
int OCT_acc_visual_z() { return 0; }


void OCT_background(int32_t rgb565)
    {
        if (rgb565 < 0) { OctClearEnabled = 0; return; }
        OctClearEnabled = 1, OctClearColor = (uint16_t)rgb565;
    }


//Sort sprites according to Order fields
void OCT_RENDER_sort(int* data, int from, int to)
    {
        int mid = (from + to) / 2;
        if (to > from) OCT_RENDER_sort(data, from, mid), OCT_RENDER_sort(data, mid+1, to);  //Subdivide
        if (to <= from || SPRITE_ORDER(data[mid]) < SPRITE_ORDER(data[mid+1])) return;      //Already sorted
        int l = from, r = mid+1, i = 0;
        while (i < to - from + 1)
            if (l <= mid && (r > to || SPRITE_ORDER(data[l]) <= SPRITE_ORDER(data[r]))) OctTempBuf[i++] = data[l++]; else OctTempBuf[i++] = data[r++];

        memcpy(data + from, OctTempBuf, i * sizeof(int));
    }


//Draw text for debug purposes
static void OCT_RENDER_text(uint16_t* framebuf, const char* text, int x, int y, uint16_t color)
    {
        const int k = 1; //Single or doubled pixels
        int cx = x, cy = y;
        for (const uint8_t* c = (const uint8_t*)text;  *c != 0;  c++,  cx += k*8)
        {
            //Newline or autowrap
            if (*c == 0x0A || cx >= 222) { cx = x; cy += k*10; continue; }

            //Skip special chars
            if (*c < 0x20 || *c >= 0x7F) continue;

            //Don't draw letter if it is clipped
            if (cx < 0 || cx >= SIDE-10 || cy < 0 || cy >= SIDE-10) continue;

            /*
            //
            uint8_t idx = (*c)-0x20;

            //Set pixels by mask
            for(int yy = 0;  yy < 8;  yy+=1)
            for(int xx = 0, mf = FONT6x8_ASCII_LSB[idx][yy];  xx < 6;  xx+=1,  mf = mf >> 1)
            {
                if (mf & 1)
                for(int ddy = 0;  ddy < k;  ddy+=1)
                for(int ddx = 0;  ddx < k;  ddx+=1)
                    framebuf[(cy+k*yy+ddy+1) * SIDE + cx+k*xx+ddx+1] = color;
            }

            //Set pixels by mask
            for(int yy = 0;  yy < 10;  yy+=1)
            for(int xx = 0, mf = FONT6x8_BORDER4_8x10_LSB[idx][yy];  xx < 8;  xx+=1,  mf = mf >> 1)
            {
                if (mf & 1)
                for(int ddy = 0;  ddy < k;  ddy+=1)
                for(int ddx = 0;  ddx < k;  ddx+=1)
                    framebuf[(cy+k*yy+ddy) * SIDE + cx+k*xx+ddx] = 0;
            }
            */

            //Set pixels by mask
            for(int yy = 0;  yy < 8;  yy+=1)
            for(int xx = 0, mb = OCT_BITFONT_BORDER[(*c)-0x20][yy], mf = OCT_BITFONT[(*c)-0x20][yy];  xx < 8;  xx+=1,  mb = mb >> 1,  mf = mf >> 1)
            {
                if (mb & 1)
                for(int ddy = 0;  ddy < k;  ddy+=1)
                for(int ddx = 0;  ddx < k;  ddx+=1)
                    framebuf[(cy+k*yy+ddy) * SIDE + cx+k*xx+ddx] = 0;

                if (mf & 1)
                for(int ddy = 0;  ddy < k;  ddy+=1)
                for(int ddx = 0;  ddx < k;  ddx+=1)
                    framebuf[(cy+k*yy+ddy) * SIDE + cx+k*xx+ddx] = color;
            }


        }
    }


const int CS_LEFT = 1, CS_RIGHT = 2, CS_BOTTOM = 4, CS_TOP = 8;

//Cohen�Sutherland outcode for [0..SIDE-1]
static inline int OCT_RENDER_clip_outcode(int x, int y)
{
    int code = 0;
    if (x < 0) code |= CS_LEFT;     else if (x >= SIDE) code |= CS_RIGHT;
    if (y < 0) code |= CS_BOTTOM;   else if (y >= SIDE) code |= CS_TOP;
    return code;
}

//Draw line with rectangle clipping then Bresenham rasterization
void OCT_RENDER_line(uint16_t* framebuf, int x0, int y0, int x1, int y1, uint16_t color)
{
    //Clipping against screen rect
    int out0 = OCT_RENDER_clip_outcode(x0, y0),  out1 = OCT_RENDER_clip_outcode(x1, y1);
    for (;;)
    {
        //Trivial cases: accept / reject
        if ((out0 | out1) == 0) break;
        if ((out0 & out1) != 0) return;

        //Pick an endpoint outside the clip window
        int x = 0, y = 0, out = (out0 != 0) ? out0 : out1;

        //Intersect with one of the window boundaries; guard dx/dy = 0 to avoid division by zero
        if      (out & CS_TOP)      { y = SIDE - 1;     if (y1 != y0)   x = x0 + (x1 - x0) * (y - y0) / (y1 - y0);  else  x = x0; }
        else if (out & CS_BOTTOM)   { y = 0;            if (y1 != y0)   x = x0 + (x1 - x0) * (y - y0) / (y1 - y0);  else  x = x0; }
        else if (out & CS_RIGHT)    { x = SIDE - 1;     if (x1 != x0)   y = y0 + (y1 - y0) * (x - x0) / (x1 - x0);  else  y = y0; }
        else        /* CS_LEFT */   { x = 0;            if (x1 != x0)   y = y0 + (y1 - y0) * (x - x0) / (x1 - x0);  else  y = y0; }

        //Replace the outside endpoint and recompute outcode
        if (out == out0)    { x0 = x, y0 = y, out0 = OCT_RENDER_clip_outcode(x0, y0); }
        else                { x1 = x, y1 = y, out1 = OCT_RENDER_clip_outcode(x1, y1); }
    }

    //Bresenham�s integer line rasterization without bounds checks (line was pre-clipped to inside segment)
    int dx = x1 - x0; if (dx < 0) dx = -dx;
    int dy = y1 - y0; if (dy < 0) dy = -dy;
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;

    //Use the common err formulation to handle all octants
    int err = dx + (-dy);
    for (;;)
    {
        framebuf[y0 * SIDE + x0] = color;
        if (x0 == x1 && y0 == y1) break;

        int e2 = err << 1;
        if (e2 >= -dy) { err -= dy;  x0 += sx; }
        if (e2 <= dx)  { err += dx;  y0 += sy; }
    }
}


//Mostly useful in debug or development mode
void OCT_RENDER_clear(uint16_t color)
    {
        for(uint8_t d = 0; d < DISPLAY_COUNT; d++)
        {
            uint16_t* pix = OCT_DISPLAY_framebuffer(d);
            for (int i=0; i<SIDE; i++)
            for (int j=0; j<SIDE; j++)
                *pix = color, pix++;

            //Optionally print debug text
            //if (OctText[0] != 0) OCT_RENDER_text(OCT_DISPLAY_framebuffer(d), OctText, 8, 8, 0xFFFF);

            OCT_DISPLAY_blit(d);
        }
    }


//Draw rect for debug purposes
static void OCT_RENDER_rect(uint16_t* framebuf, float x, float y, int w, int h, int angle, uint16_t color)
    {
        //Classify rotation to know if rect is in a horizontal orientation or a vertical one
        angle = OCT_TM_normalize_angle(angle);
        if ((angle/90) & 1) { int tmp = w; w = h, h = tmp; }

        //4 sides with per-pixel clipping
        for(int i = 0;  i < 2*h;  i+=1) { int xx = (int)x - w;      int yy = (int)y - h + i;    if (yy >= 0 && yy < SIDE && xx >=0 && xx < SIDE) framebuf[yy * SIDE + xx] = color;}
        for(int i = 0;  i < 2*h;  i+=1) { int xx = (int)x + w;      int yy = (int)y - h + i;    if (yy >= 0 && yy < SIDE && xx >=0 && xx < SIDE) framebuf[yy * SIDE + xx] = color;}
        for(int i = 0;  i < 2*w;  i+=1) { int xx = (int)x - w + i;  int yy = (int)y - h;        if (yy >= 0 && yy < SIDE && xx >=0 && xx < SIDE) framebuf[yy * SIDE + xx] = color;}
        for(int i = 0;  i < 2*w;  i+=1) { int xx = (int)x - w + i;  int yy = (int)y + h;        if (yy >= 0 && yy < SIDE && xx >=0 && xx < SIDE) framebuf[yy * SIDE + xx] = color;}
    }


//Upload back to frame buffer
static void OCT_RENDER_back_upload(uint16_t* framebuf)
    {
        //Upscale backbuffer, shift red
        uint32_t prev; uint32_t* bb;
        uint32_t* fb4 = (uint32_t*)framebuf;
        for (int row = 0; row < SIDE; row++)
        {
            bb = OctBack + (row>>1) * HALFSIDE;
            prev = *bb;
            for (int col = 0; col < HALFSIDE; col++)
            {
                //Extract red
                uint32_t red = prev & 0xF800;

                //Place it to first pixel
                prev = *bb;
                uint32_t first = ((prev & 0xFFFF07FF) | red);

                //Combine 2 back buffer pixels to single 32-bit write
                //(*fb4) = (prev << 16) | (uint16_t)(first);
                //(*fb4) = (first << 16) | (uint16_t)(first);

                (*fb4) = __REV16(  (first << 16) | (uint16_t)(first));

                fb4++, bb++;
            }
        }
    }


//Per-pixel CPU bitmap drawing
static bool OCT_RENDER_bmp(uint16_t* framebuf, int x, int y, const octBmp_t* bmp, const uint32_t* palette, int angle, int hor, int vert, uint32_t clamped_transp, int gx, int gy, int gz, const octBmp_t* bmp_lut, const uint32_t* palette_lut, int param1, int param2, int param3)
    {
        //Classify rotation
        angle = OCT_TM_normalize_angle(angle);
        int rot = angle/90,  swap_wh = rot&1,  reverse = 1-(rot&2);

        //Combine flips with rotation, to track effective screen space axis orientation
        int sso = 0;
        if (hor < 0)        sso = (vert < 0) ? 2 : 1;
        else if (vert < 0)  sso = 3;
        sso = (rot + sso) % 4;

        //Sides of bmp in screen coordinates, before any clipping
        int last_x, last_y, zoom = (bmp->Flags & OCT_FLAG_FULLSIZE) ? 1 : 2;
        if (swap_wh)    last_y = bmp->W * zoom - 1,  last_x = bmp->H * zoom - 1;
        else            last_x = bmp->W * zoom - 1,  last_y = bmp->H * zoom - 1;
        int sides[4] = {(int)(x), (int)(y), (int)(x) + XSIGN[sso]*last_x, (int)(y) + YSIGN[sso]*last_y};

        //Apply sso to correctly label sides of bitmap
        int label[] = {0, 1, 2, 3};  //L B R T
        if (sso & 2)                label[1] = 3,  label[3] = 1; //B <-> T
        if (sso == 1 || sso == 2 )  label[0] = 2,  label[2] = 0; //L <-> R

        //Cull when out of screen
        if (sides[label[2]] < 0 || sides[label[3]] < 0 || sides[label[0]] > BORDER || sides[label[1]] > BORDER) return false;

        //Clip coordinates but keep clipped amount to adjust coordinates of the first texel
        int crop[] = {0,0,0,0};//,  first_texel[] = {0, 0};
        if (sides[label[0]] < 0)        crop[label[0]] = -sides[label[0]],          sides[label[0]] = 0;
        if (sides[label[1]] < 0)        crop[label[1]] = -sides[label[1]],          sides[label[1]] = 0;
        if (sides[label[2]] > BORDER)   crop[label[2]] =  sides[label[2]] - BORDER, sides[label[2]] = BORDER;
        if (sides[label[3]] > BORDER)   crop[label[3]] =  sides[label[3]] - BORDER, sides[label[3]] = BORDER;

        //Clipped in screen space bmp dimensions
        int cw, ch;
        if (swap_wh)    ch = abs(sides[0] - sides[2])+1,  cw = abs(sides[1] - sides[3])+1;
        else            cw = abs(sides[0] - sides[2])+1,  ch = abs(sides[1] - sides[3])+1;

        //No space between sides, or rasterization disabled
        if (cw <= 0 || ch <= 0 || (OctDevMode & OCT_DEV_NO_RASTER)) return false;

        //Apply cropped pixels count to texture (in texels)
        //first_texel[swap_wh]      =  crop[0] / zoom;
        //first_texel[1 - swap_wh]  =  crop[1] / zoom;
        int first_texel_col = crop[swap_wh] / zoom;
        int first_texel_row = crop[1-swap_wh] / zoom;
        int last_texel_col  = first_texel_col + cw / zoom;
        int last_texel_row  = first_texel_row + ch / zoom;

        //Screen space increments, half resolution for textures in half size
        int nextpix = 0, nextline = 0;
        if (swap_wh)    nextpix = reverse * hor * SIDE,             nextline =  -1      * vert * reverse;
        else            nextpix = reverse * hor * 1,                nextline =   SIDE   * vert * reverse;

        //Setup source data pointers
        ///const octBmpLine_t *linedesc = NULL, *cacheline = NULL;
        ///const octBmpLine_t* trims        = (octBmpLine_t*)((uint8_t*)bmp + sizeof(octBmp_t));
        const uint8_t*      lines       =                 (uint8_t*)bmp + sizeof(octBmp_t);
        const uint8_t*      texels      =                 (uint8_t*)bmp + sizeof(octBmp_t) + sizeof(uint8_t)*((bmp->H+3)/4*4);
        const uint32_t*     transp_xlat = OCT_TRANSP + 32 * clamped_transp;

        //Setup LUT
///     const uint8_t*      luxels = ((bmp_lut == NULL) ? NULL : (uint8_t*)bmp_lut + sizeof(octBmp_t)  );/// + sizeof(octBmpLine_t)*(bmp_lut->H));
///     const int           luxoffset   = angle + param3;

        //Figure out needed blending mode
        const int BLEND_OPAQUE = 0, BLEND_ALPHA = 1, BLEND_TRANSP = 2, BLEND_OPAQUE_TRANSP = 3, BLEND_ADD = 4, BLEND_BG = 5,  BLEND_BUMP = 6,  BLEND_DUDV = 7,  BLEND_REFL = 8;
        int blending = (clamped_transp > 0) ? BLEND_OPAQUE_TRANSP : BLEND_OPAQUE;
        if (bmp->Flags & OCT_FLAG_ALPHA)    blending = (clamped_transp > 0) ? BLEND_TRANSP : BLEND_ALPHA;
        if (bmp->Flags & OCT_FLAG_ADDITIVE) blending = BLEND_ADD;
        if (bmp->Flags & OCT_FLAG_BG)       blending = BLEND_BG;
        if (bmp->Flags & OCT_FLAG_BUMP)     blending = BLEND_BUMP;
        if (bmp->Flags & OCT_FLAG_DUDV)     blending = BLEND_DUDV;
        if (bmp->Flags & OCT_FLAG_REFL)     blending = BLEND_REFL;


        //Fast path
        if (zoom == 2)
        {
            if (swap_wh)    nextpix = reverse * hor * HALFSIDE,         nextline =  -1          * vert * reverse;
            else            nextpix = reverse * hor * 1,                nextline =   HALFSIDE   * vert * reverse;

            //Setup
            int pix_line_start = sides[1]/2 * HALFSIDE + sides[0]/2;
            //int tex_clipped_width = (cw+1)/zoom;
            //int tex_clipped_height = (ch+1)/zoom;

            //Unpacked symbols
            //int cachehead = 0, cachetail = 0;

            //Unpack data
            const int       SYMBOL_BITNESS = bmp->Compression & 0xFF;
            const uint32_t  SYMBOL_MASK         = (1 << SYMBOL_BITNESS) - 1;
            const int       DEFICIT_THRESHOLD   = 32 - (SYMBOL_BITNESS + 7);
            for (int tex_y = 0, tex_line_start_in_bytes = 0;  tex_y < last_texel_row;  tex_line_start_in_bytes += lines[tex_y],  tex_y++)
            {
                //This is skipped row
                if (tex_y < first_texel_row) continue;

                //Convert starting byte offset to 4-byte-pointer and bit-offset within it
                uint32_t* bits      = (uint32_t*)texels + (tex_line_start_in_bytes / 4);
                int cur_bit_index   = (tex_line_start_in_bytes % 4) * 8;
                int pix_current = pix_line_start - first_texel_col * nextpix;

                //Output texels
                int deficit = 32; uint32_t turnover = 0;
                for (int tex_x = 0;;)
                {
                    //Replenish the deficit
                    if (deficit > DEFICIT_THRESHOLD)
                    {
                        //Copy from the current bank
                        turnover |= ((*bits) >> cur_bit_index) << (32 - deficit);

                        int rest = 32 - cur_bit_index;
                        if (rest >= deficit)
                        {
                            cur_bit_index += deficit;
                        }
                        else
                        {
                            //Wasn't enough
                            deficit -= rest;
                            bits++, cur_bit_index = deficit;
                            turnover |= (*bits) << (32 - deficit);
                        }

                        //Don't let bit index stay at 32, because ">> 32" is UB
                        if (cur_bit_index == 32) bits++, cur_bit_index = 0;

                        deficit = 0;
                    }

                    //Literal
                    int texel = turnover & SYMBOL_MASK;                   turnover >>= SYMBOL_BITNESS;

                    //Table encoded repeats
                    int overcode = turnover & COMPR1_LEN_DECODE_MASK;      turnover >>= COMPR1_LEN_CONSUME[overcode];

                    //Process symbols
                    for (int rep = COMPR1_LEN_DECODE[overcode]; rep > 0; rep--)
                    {
                        //Skip clipped or empty texels
                        //pix_current = (sides[1]/2 * HALFSIDE + sides[0]/2) + (tex_x - first_texel_col) * nextpix + (tex_y - first_texel_row) * nextline;

                        if (texel != 0  &&  tex_x >= first_texel_col)
                        {
                            //if (pix_current < 0 || pix_current >= 120 * 120) DebugBreak();

                            //Rasterize
                            switch (blending)
                            {
                                case BLEND_BG:
                                {
                                    OctBack[pix_current] = palette[texel];
                                    break;
                                }

                                case BLEND_OPAQUE:
                                {
                                    OctBack[pix_current] = palette[texel];
                                    break;
                                }

                                case BLEND_OPAQUE_TRANSP:
                                {
                                    uint32_t alpha = OCT_TRANSP_MAX - clamped_transp; //[TODO]: optimize
                                    uint32_t fg = palette[texel] & 0x07e0f81f;
                                    uint32_t bg = OctBack[pix_current];
                                    bg = (bg | bg << 16) & 0x07e0f81f;
                                    bg = (bg + (((fg - bg) * alpha) >> 5)) & 0x07e0f81f;

                                    //Pack GRB
                                    OctBack[pix_current] = (uint16_t)(bg | bg >> 16);
                                    break;
                                }

                                case BLEND_ALPHA:
                                {
                                        uint32_t alpha = palette[texel] >> 27;      //5-bits
                                        uint32_t fg = palette[texel] & 0x07FFFFFF;  //fg = (fg | fg << 16) & 0x07e0f81f;

                                        //Opaque branch
                                        //[TODO]: Need an estimation of how many opaque texels there are, so this condition is actually an optimization
                                        ///if (alpha > 30) { OctBack[pix_current] = (uint16_t)(fg | fg >> 16); continue; }

                                        //Blend: a*fg + (1-a)*bg = bg + (fg-bg)*a
                                        uint32_t bg = OctBack[pix_current];
                                        bg = (bg | bg << 16) & 0x07e0f81f;
                                        bg = (bg + ((fg - bg) * alpha >> 5)) & 0x07e0f81f;

                                        //Pack GRB
                                        OctBack[pix_current] = (uint16_t)(bg | bg >> 16);

                                    break;
                                }

                                case BLEND_TRANSP:
                                {
                                        uint32_t alpha = palette[texel] >> 27;      //5-bits
                                        uint32_t fg = palette[texel] & 0x07FFFFFF;  //fg = (fg | fg << 16) & 0x07e0f81f;
                                        alpha = transp_xlat[alpha];

                                        //Blend: a*fg + (1-a)*bg = bg + (fg-bg)*a
                                        uint32_t bg = OctBack[pix_current];
                                        bg = (bg | bg << 16) & 0x07e0f81f;
                                        bg = (bg + ((fg - bg) * alpha >> 5)) & 0x07e0f81f;

                                        //Pack GRB
                                        OctBack[pix_current] = (uint16_t)(bg | bg >> 16);

                                    break;
                                }

                                case BLEND_ADD:
                                {
                                        uint32_t fg = palette[texel] & 0x07FFFFFF;
                                        if (fg == 0) break;                             //Break is from case, not from loop
                                        uint32_t bg = OctBack[pix_current];

                                        //No sprite MSBs (all channels < 128) means no direct overflow, but can be carried one though
                                        uint32_t msbs = bg & MSBS;                      //Extract and keep bg MSBs
                                        bg = (bg & NSBS) + fg;                          //Cut MSBs to guarantee no overflow
                                        uint32_t over = msbs & bg;                      //Extracted one bit can still cause overflow with sum's accumulated MSBs
                                        over = (over << 1) - (over >> 4);               //Convert overflow to saturation masks 0b1111, zero otherwise

                                        OctBack[pix_current] = (uint16_t)((bg | msbs) | over); //Place extracted MSB back; replace color channels with a saturation mask if any
                                    break;
                                }

                            }
                        }

                        //Next
                        tex_x++,  pix_current += nextpix;

                        //Line processed, break to outmost cycle
                        if (tex_x >= last_texel_col) goto end_of_line;
                    }

                    //Consumed bits
                    deficit += SYMBOL_BITNESS + COMPR1_LEN_CONSUME[overcode];
                }

            end_of_line:
                pix_line_start += nextline;
            }


        }
        else
        {
            if (swap_wh)    nextpix = reverse * hor * SIDE,         nextline =  -1      * vert * reverse;
            else            nextpix = reverse * hor * 1,            nextline =   SIDE   * vert * reverse;

            //Setup
            int pix_line_start = sides[1]/1 * SIDE + sides[0]/1;
            //int tex_clipped_width = (cw+1)/zoom;
            //int tex_clipped_height = (ch+1)/zoom;

            //Unpacked symbols
            //int cachehead = 0, cachetail = 0;

            //Unpack data
            const int       SYMBOL_BITNESS = bmp->Compression & 0xFF;
            const uint32_t  SYMBOL_MASK         = (1 << SYMBOL_BITNESS) - 1;
            const int       DEFICIT_THRESHOLD   = 32 - (SYMBOL_BITNESS + 7);
            for (int tex_y = 0, tex_line_start_in_bytes = 0;  tex_y < last_texel_row;  tex_line_start_in_bytes += lines[tex_y],  tex_y++)
            {
                //This is skipped row
                if (tex_y < first_texel_row) continue;

                //Convert starting byte offset to 4-byte-pointer and bit-offset within it
                uint32_t* bits      = (uint32_t*)texels + (tex_line_start_in_bytes / 4);
                int cur_bit_index   = (tex_line_start_in_bytes % 4) * 8;
                int pix_current = pix_line_start - first_texel_col * nextpix;

                //Output texels
                int deficit = 32; uint32_t turnover = 0;
                for (int tex_x = 0;;)
                {
                    //Replenish the deficit
                    if (deficit > DEFICIT_THRESHOLD)
                    {
                        //Copy from the current bank
                        turnover |= ((*bits) >> cur_bit_index) << (32 - deficit);

                        int rest = 32 - cur_bit_index;
                        if (rest >= deficit)
                        {
                            cur_bit_index += deficit;
                        }
                        else
                        {
                            //Wasn't enough
                            deficit -= rest;
                            bits++, cur_bit_index = deficit;
                            turnover |= (*bits) << (32 - deficit);
                        }

                        //Don't let bit index stay at 32, because ">> 32" is UB
                        if (cur_bit_index == 32) bits++, cur_bit_index = 0;

                        deficit = 0;
                    }

                    //Literal
                    int texel = turnover & SYMBOL_MASK;                   turnover >>= SYMBOL_BITNESS;

                    //Table encoded repeats
                    int overcode = turnover & COMPR1_LEN_DECODE_MASK;      turnover >>= COMPR1_LEN_CONSUME[overcode];

                    //Process symbols
                    for (int rep = COMPR1_LEN_DECODE[overcode]; rep > 0; rep--)
                    {
                        //Skip clipped or empty texels
                        //pix_current = (sides[1]/2 * HALFSIDE + sides[0]/2) + (tex_x - first_texel_col) * nextpix + (tex_y - first_texel_row) * nextline;

                        if (texel != 0  &&  tex_x >= first_texel_col)
                        {
                            //if (pix_current < 0 || pix_current >= 120 * 120) DebugBreak();

                            //Rasterize
                            switch (blending)
                            {
                                case BLEND_BG:
                                {
                                    framebuf[pix_current] = (uint16_t)palette[texel];
                                    break;
                                }

                                case BLEND_OPAQUE:
                                {
                                    framebuf[pix_current] = (uint16_t)palette[texel];
                                    break;
                                }

                                case BLEND_OPAQUE_TRANSP:
                                {
                                    uint32_t alpha = OCT_TRANSP_MAX - clamped_transp; //[TODO]: optimize
                                    uint32_t fg = palette[texel] & 0x07e0f81f;
                                    uint32_t bg = framebuf[pix_current];
                                    bg = (bg | bg << 16) & 0x07e0f81f;
                                    bg = (bg + (((fg - bg) * alpha) >> 5)) & 0x07e0f81f;

                                    //Pack GRB
                                    framebuf[pix_current] = (uint16_t)(bg | bg >> 16);
                                    break;
                                }

                                case BLEND_ALPHA:
                                {
                                        uint32_t alpha = palette[texel] >> 27;      //5-bits
                                        uint32_t fg = palette[texel] & 0x07FFFFFF;  //fg = (fg | fg << 16) & 0x07e0f81f;

                                        //Opaque branch
                                        //[TODO]: Need an estimation of how many opaque texels there are, so this condition is actually an optimization
                                        ///if (alpha > 30) { OctBack[pix_current] = (uint16_t)(fg | fg >> 16); continue; }

                                        //Blend: a*fg + (1-a)*bg = bg + (fg-bg)*a
                                        uint32_t bg = framebuf[pix_current];
                                        bg = (bg | bg << 16) & 0x07e0f81f;
                                        bg = (bg + ((fg - bg) * alpha >> 5)) & 0x07e0f81f;

                                        //Pack GRB
                                        framebuf[pix_current] = (uint16_t)(bg | bg >> 16);

                                    break;
                                }

                                case BLEND_TRANSP:
                                {
                                        uint32_t alpha = palette[texel] >> 27;      //5-bits
                                        uint32_t fg = palette[texel] & 0x07FFFFFF;  //fg = (fg | fg << 16) & 0x07e0f81f;
                                        alpha = transp_xlat[alpha];

                                        //Blend: a*fg + (1-a)*bg = bg + (fg-bg)*a
                                        uint32_t bg = framebuf[pix_current];
                                        bg = (bg | bg << 16) & 0x07e0f81f;
                                        bg = (bg + ((fg - bg) * alpha >> 5)) & 0x07e0f81f;

                                        //Pack GRB
                                        framebuf[pix_current] = (uint16_t)(bg | bg >> 16);

                                    break;
                                }
                            }
                        }

                        //Next
                        tex_x++,  pix_current += nextpix;

                        //Line processed, break to outmost cycle
                        if (tex_x >= last_texel_col) goto end_of_line_hd;
                    }

                    //Consumed bits
                    deficit += SYMBOL_BITNESS + COMPR1_LEN_CONSUME[overcode];
                }

            end_of_line_hd:
                pix_line_start += nextline;
            }

        }

        framebuf = 0;
        gx = gy = gz = 0; param1 = param2 = param3 = 0;
        bmp_lut = 0; palette_lut = 0;
        return true;
    }


//Draw sprites visible through the viewport
static void OCT_RENDER_scene(int vid, uint16_t* framebuf)
    {
        //Viewport is turned off
        if (OctViewports[vid].Mode == 2) return;

        //Collect visible sprites after some conservative culling
        int count = 0,  viewmask = 1 << vid;
        octTm_t combo_tm;
        const octTm_t* viewtm = &OctViewports[vid].Tm;
        for (int j = 1; j < OctCap; j++)
        {
            octSprite_t* s = SPRITE_PTR(j);
            if (s->Idx == j  &&  !s->Hidden  &&  s->Frame != 0  &&  ((s->Mask & viewmask) != 0) /*&&  OCT_SIDE_OPP[Tms[j].Side] != viewtm->Side*/)
            {
                //Get sprite asset
                const octBmp_t* bmp = (octBmp_t*)OCT_PACK_getSprite(s->Frame);
                //const uint32_t* palette   = (const uint32_t*) (OctPalsData + OctPals[bmp->Pidx].ColorsBufferIndex);
                OCT_TM_copy(&combo_tm, &s->Tm);

                //Through sprites hierarchy to side space
                if (s->Parent != 0)
                {
                    const octSprite_t* ps = SPRITE_PTR(s->Parent);
                    OCT_TM_combine(&combo_tm, &ps->Tm, false);
                }

                //Opposite sides don't see each other
                if (OCT_SIDE_OPP[combo_tm.Plane] == viewtm->Plane) continue;

                //Change sprite side to viewport's
                OCT_TM_change_plane(&combo_tm, viewtm->Plane);

                OctSortBuf[count++] = j;
                if (!s->Letter) s->Order = s->Layer * 8192 - (int)(combo_tm.Y /*- bmp->H * 0.5f*/);
                else            s->Order = s->Layer * 8192 + 512 - s->Data1; //[TODO]:

                //Layer of fullsize bitmaps automatically raised
                if (bmp->Flags & OCT_FLAG_FULLSIZE) s->Order += 1000000; //[TODO]: move to creation

                //Convert to screen space
                OCT_TM_combine(&combo_tm, viewtm, true);

                //[TODO]: bounding circle culling

                //Apply pivot to bmp
                octTm_t* tm = &OctCachedTms[j];
                tm->XFlip = tm->YFlip = 1,  tm->Plane = -1,  tm->A = 0,  tm->X = -(bmp->PivotX) * (1-2*s->FlipH),  tm->Y = (bmp->PivotY) * (1-2*s->FlipV);
                OCT_TM_combine(tm, &combo_tm, false);

                //OctCachedNum++;
            }

            //[TODO]: Better merge with prev condition
            //[TODO]: Filter by viewport
            if (s->Idx == j  &&  !s->Hidden  &&  s->Line)
            {

                OCT_TM_copy(&combo_tm, &s->Tm);

                //Opposite sides don't see each other
                if (OCT_SIDE_OPP[combo_tm.Plane] == viewtm->Plane) continue;

                OctSortBuf[count++] = j;
                s->Order = s->Layer * 8192;

                //Change sprite side to viewport's
                OCT_TM_change_plane(&combo_tm, viewtm->Plane);

                //Convert to screen space
                OCT_TM_combine(&combo_tm, viewtm, true);
                OCT_TM_copy(&OctCachedTms[j], &combo_tm);
            }

        }

        //Sort objects
        OCT_RENDER_sort(OctSortBuf, 0, count-1);


        int stat = 0;

        //Rasterize
        bool pass = 0;
        for (int j=0; j < count; j++)
        {
            //Get sprite asset
            const octSprite_t* s = SPRITE_PTR(OctSortBuf[j]);

            if (s->Line)
            {
                continue;
            }

            const octBmp_t* bmp = (octBmp_t*)OCT_PACK_getSprite(s->Frame);
            const uint32_t* palette = (const uint32_t*) (OctPalsData + OctPals[bmp->Pidx].ColorsBufferIndex);

            //[TMP]:
            octBmp_t* bmp_lut = 0; uint32_t* palette_lut = 0;
            if (s->Lut != 0)
            {
                bmp_lut = (octBmp_t*)OCT_PACK_getSprite(s->Lut);
                palette_lut = (uint32_t*) (OctPalsData + OctPals[bmp_lut->Pidx].ColorsBufferIndex);
            }

            //Separate pass for fullsize bitmaps
            if (pass == 0 && s-> Order >= 1000000)  OCT_RENDER_back_upload(framebuf), pass = 1;

            //Replace with gray
            if (OctViewports[vid].Mode == 1)
                palette = &OctPalsData[OctPalsSwap[bmp->Pidx]];

            //Clamp transparency
            int transp = s->Transp;
            if (transp < 0) transp = 0; else if (transp > OCT_TRANSP_MAX) transp = OCT_TRANSP_MAX;

            //Rasterize sprite's bmp with space transformed to display's (diagonally mirrored and swapped x\y axes)
            octTm_t* tm = &OctCachedTms[OctSortBuf[j]];
            bool drawn = OCT_RENDER_bmp(framebuf, (int)roundf(tm->Y), (int)roundf(tm->X), bmp, palette, 90-tm->A, (1-2*s->FlipH), (1-2*s->FlipV), transp, 0, 0, 0, bmp_lut, palette_lut, s->Param1, s->Param2, s->Param3);

            //Mark skipped in a hackish way
            if (drawn) stat++; else tm->Plane = -1;
        }


        if (pass == 0) OCT_RENDER_back_upload(framebuf);



        for (int j=0; j < count; j++)
        {
            //Get sprite asset
            const octSprite_t* s = SPRITE_PTR(OctSortBuf[j]);

            //
            if (s->Line)
            {
                //Transform point from local space to screen's

                octTm_t* tm = &OctCachedTms[OctSortBuf[j]];
                octTm_t ctm;
                ctm.XFlip = ctm.YFlip = 1,  ctm.Plane = -1,  ctm.A = 0,  ctm.X = (float)(s->Zw) * (1-2*s->FlipH),  ctm.Y = (float)(s->Zh) * (1-2*s->FlipV);
                OCT_TM_combine(&ctm, &s->Tm, false);
                OCT_TM_change_plane(&ctm, viewtm->Plane);
                OCT_TM_combine(&ctm, viewtm, true);
                OCT_RENDER_line(framebuf, (int)tm->Y, (int)tm->X, (int)ctm.Y, (int)ctm.X, s->Data0);
                continue;
            }
        }


        //Collider visualization for debug
        if (OctDevMode & OCT_DEV_COLLIDERS)
            for (int j=0; j < count; j++)
            {
                const octSprite_t* s = SPRITE_PTR(OctSortBuf[j]);
                if (s->Line)
                {
                    continue;
                }
                const octBmp_t* bmp = (octBmp_t*)OCT_PACK_getSprite(s->Frame);
                if (OctCachedTms[OctSortBuf[j]].Plane == -1 || bmp->Bw == 0 || bmp->Bh == 0 || s->Letter) continue; //&& s->ShowCollider

                //NodeTM * bounds center
                octTm_t ctm;
                //ctm.XFlip = ctm.YFlip = 1,  ctm.Side = -1,  ctm.A = 0,  ctm.X = (bmp->Bx + bmp->Bw/2) * (1-2*s->FlipH),  ctm.Y = (bmp->By - bmp->Bh/2) * (1-2*s->FlipV);
                ctm.XFlip = ctm.YFlip = 1,  ctm.Plane = -1,  ctm.A = 0,  ctm.X = (bmp->Bx) * (1-2*s->FlipH),  ctm.Y = (bmp->By) * (1-2*s->FlipV);
                OCT_TM_combine(&ctm, &s->Tm, false);
                OCT_TM_change_plane(&ctm, viewtm->Plane);
                OCT_TM_combine(&ctm, viewtm, true);

                //Draw debug rect
                if (bmp->Bh > 1 && bmp->Bw > 1)
                {
                    OCT_RENDER_rect(framebuf, ctm.Y, ctm.X, (int)(bmp->Bw), (int)(bmp->Bh), 90-ctm.A, 0xFFFF);
                }
                else
                for (int n=0; n<360; n++)
                {
                    int cx = (int)(ctm.Y + bmp->Bw * cosf(n*1.0f));
                    int cy = (int)(ctm.X + bmp->Bw * sinf(n*1.0f));
                    if (cx < 0 || cx >= SIDE || cy < 0 || cy >= SIDE) continue;
                    framebuf[cy * SIDE + cx] = 0xFFFF;
                }
            }

        //Debug->Trace("stat %d\n", stat);
    }
