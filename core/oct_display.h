#pragma once
#include "oct_consts.h"
#include "oct_vars.h"
#include "oct_helpers.h"


//rgb565_buffer_t is needed?

uint16_t* OCT_DISPLAY_framebuffer(uint8_t display_index)
    {
        if (display_index >= DISPLAY_COUNT) display_index = DISPLAY_COUNT - 1;
        /*rgb565_buffer_t buffer = { 0 };
        buffer.address  = (uint16_t*)
        buffer.height   = SIDE;
        buffer.width    = SIDE;
        buffer.pitch    = SIDE;*/
        return (uint16_t*)DISPLAY_getFramebufferPtr(display_index);
    }


void OCT_DISPLAY_blit(uint8_t display_index)
    {
        //rgb565_buffer_t buffer = OCT_DISPLAY_framebuffer(display_index);

        hal_display_lcd_layer_input_t layer_params;
        memset(&layer_params, 0, sizeof(layer_params));
        layer_params.layer_enable   = 0; //HAL_DISPLAY_LCD_LAYER0;
        layer_params.rotate         = HAL_DISPLAY_LCD_LAYER_ROTATE_270;
        layer_params.color_format   = HAL_DISPLAY_LCD_LAYER_COLOR_RGB565;
        layer_params.buffer_address = (uintptr_t)OCT_DISPLAY_framebuffer(display_index);
        layer_params.row_size       = SIDE;
        layer_params.column_size    = SIDE;
        layer_params.pitch          = SIDE * 2;

        DISPLAY_WaitDMA();
        DISPLAY_StartDMA(display_index, &layer_params);
    }

