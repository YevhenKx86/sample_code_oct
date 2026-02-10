#pragma once

#include <stddef.h>
#include <stdint.h>
#include "oct_crossplatform.h"

#include <os/os.h>

#include "multi_lcd.h"
//#include "oct_embed.h"
#include "app_bench.h"

static frame_buffer_t* displays[DISPLAY_COUNT];


void hal_gpt_get_free_run_count(hal_gpt_clock_source_t clock_source, uint32_t *count) {
    // TBI
    *count = 0;
}

static void checkDisplay(size_t displayID) {
    if (displays[displayID]) return;

    displays[displayID] = frame_buffer_display_malloc(DISPLAY_WIDTH * DISPLAY_HEIGHT * 2);

    displays[displayID]->fmt = PIXEL_FMT_RGB565;
    displays[displayID]->width = DISPLAY_WIDTH; 
    displays[displayID]->height = DISPLAY_HEIGHT;

    LOGI("%s: %u allocated %p\r\n", __func__, displayID, displays[displayID]);
}

uint16_t* DISPLAY_getFramebufferPtr(size_t displayID) {
    if (displayID >= DISPLAY_COUNT) return NULL;

    checkDisplay(displayID);
    return (uint16_t*)displays[displayID]->frame;
}

static bool displayFlag[DISPLAY_COUNT] = {false};

static avdk_err_t display_frame_free_cb(void *_fb){
    frame_buffer_t* fb = (frame_buffer_t*)_fb;

    for (size_t i = 0; i < DISPLAY_COUNT; i++) {
        if (displays[i] == fb) {
            displayFlag[i] = false;
            //LOGI("%s: %u\r\n", __func__, i);
        }
    }
    return AVDK_ERR_OK;
}

void DISPLAY_WaitDMA() {    

    while (displayFlag[0] && displayFlag[1] && displayFlag[2]) {
        //LOGI("%s\r\n", __func__);
        rtos_delay_milliseconds(1);
    }
}

void DISPLAY_StartDMA(size_t displayID, hal_display_lcd_layer_input_t* layer) {
    if (displayID >= DISPLAY_COUNT) return;

    checkDisplay(displayID);

    multi_lcd_id_t id = (multi_lcd_id_t)displayID;
    frame_buffer_t* frame = displays[displayID];

    //LOGI("%s: %u\r\n", __func__, displayID);
    displayFlag[displayID] = true;

    multi_lcd_display_flush(id, frame, display_frame_free_cb);
}

uint32_t RTOS_getTimeMs(void) {
    return rtos_get_time();
}


static bool loaded = false;

//ATTR_RWDATA_IN_PSRAM_4BYTE_ALIGN static uint8_t app_data[APP_BENCH_SIZE];

ATTR_RWDATA_IN_RAM static uint8_t app_data[APP_BENCH_SIZE];

uint8_t * load_app(void){

    if(loaded){
        return app_data;
    }
    loaded = true;

    memcpy(app_data, app_bench_data, APP_BENCH_SIZE);

    BK_LOGI(NULL,"%s: loaded app data %p\r\n", __func__, app_data);
    /*BK_LOGI(NULL,"%s: OctPalsData %p\r\n", __func__, OctPalsData);
    BK_LOGI(NULL,"%s: OctBack %p\r\n", __func__, OctBack);
    BK_LOGI(NULL,"%s: OctMix %p\r\n", __func__, OctMix);
    BK_LOGI(NULL,"%s: OctCache %p\r\n", __func__, OctCache);*/

    return app_data;
}