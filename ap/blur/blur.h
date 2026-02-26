
#include "stdint.h"

typedef struct{
    const uint16_t * pIn;
    uint16_t * pOut;
    uint16_t w;
    uint16_t h;
    uint16_t coreW;
    uint16_t w0;
    uint16_t h0;
    uint16_t w_frame;
    uint16_t h_frame;
}BlurFrameConfig_TypeDef;

void blur_frame_new(BlurFrameConfig_TypeDef * cfg);
void get_rgb_from_color(uint16_t color, uint16_t * r, uint16_t * g, uint16_t * b);
void set_rgb_to_color(uint16_t * color, uint16_t r, uint16_t g, uint16_t b);
