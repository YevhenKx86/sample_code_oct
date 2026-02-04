
#include "mlcd_test.h"
#include "multi_lcd.h"
#include "benchmark_video.h"
#include "blur.h"
#include "time_labels.h"
#include "screen_images.h"
//#include "media_types.h"


//#define MLCD_TEST_RUN
#define MLCD_PALETTE_TEST_RUN

#ifdef MLCD_PALETTE_TEST_RUN
#include "palette.h" 
#endif

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define TAG "C2_0_LCD"

#define MLOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define MLOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define MLOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define MLOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define DISPLAY_COUNT   3

#define DW_SCREEN   240
#define DH_SCREEN   240

#define DW_FRAME 120
#define DH_FRAME 120

#define DH_FRAME_OFFSET (DH_SCREEN-DH_FRAME)/2
#define DW_FRAME_OFFSET (DW_SCREEN-DW_FRAME)/2

#define TEXT_X  15
#define TEXT_Y  15

#define RED_COLOR       0xF800
#define GREEN_COLOR     0x07E0
#define BLUE_COLOR      0x001F
#define WHITE_COLOR      0xFFFF
#define BLACK_COLOR     0x0000
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define TIME_LBL_COUNT  4
#define TIME_LBL_PERIOD 30
TimeStamp_TypeDef lbl1;
TimeStamp_TypeDef lbl2;
TimeStamp_TypeDef lbl3;
TimeStamp_TypeDef lbl4;

TimeStamp_TypeDef * lbl_stack[TIME_LBL_COUNT] = {&lbl1, &lbl2, &lbl3, &lbl4};

//uint16_t frameTmp[DW_SCREEN*DH_SCREEN];
uint16_t img120x120_tmp1[DW_FRAME*DW_FRAME];
uint16_t img120x120_tmp2[DW_FRAME*DW_FRAME];
int animationCnt = 0;

uint8_t * AssertsStorageArr[20];

static frame_buffer_t * disp_stack[DISPLAY_COUNT]; 
static beken_semaphore_t g_lcd_flush_finish_sem = NULL;
//-----------------------------------------------------------------------------
void scale_image_120_to_240(const uint16_t * pSrc, uint16_t * pDst){
    for(int y = 0; y < 120; y++){
        for(int x = 0; x < 120; x++){
            uint16_t pix = pSrc[y * 120 + x];
            pDst[(y*2) * 240 + (x*2)] = pix;
            pDst[(y*2) * 240 + (x*2 + 1)] = pix;
            pDst[(y*2 + 1) * 240 + (x*2)] = pix;
            pDst[(y*2 + 1) * 240 + (x*2 + 1)] = pix;
        }
    }
}
//-----------------------------------------------------------------------------
void scale_image_120_to_240_BE(const uint16_t * pSrc, uint16_t * pDst){
    for(int y = 0; y < 120; y++){
        for(int x = 0; x < 120; x++){
            uint16_t pix = pSrc[y * 120 + x];
            pix = (pix >> 8) | ( pix << 8);
            pDst[(y*2) * 240 + (x*2)] = pix;
            pDst[(y*2) * 240 + (x*2 + 1)] = pix;
            pDst[(y*2 + 1) * 240 + (x*2)] = pix;
            pDst[(y*2 + 1) * 240 + (x*2 + 1)] = pix;
        }
    }
}
//-----------------------------------------------------------------------------
frame_buffer_t * _assign_disp_frame(multi_lcd_id_t ID){
    if(ID < DISPLAY_COUNT){
        return disp_stack[ID];
    }
    else{
        MLOGI("%s: FAIL ID %d\r\n", __func__, ID);
        return NULL;
    }
}
//-----------------------------------------------------------------------------
void lcd_qspi_display_fill_pure_color(frame_buffer_t *frame_buffer, uint16_t color)
{
    uint8_t data[2] = {0};

    data[0] = color >> 8;
    data[1] = color;

    for (int i = 0; i < frame_buffer->size; i+=2)
    {
        frame_buffer->frame[i] = data[0];
        frame_buffer->frame[i + 1] = data[1];
    }
}
//-----------------------------------------------------------------------------
void lsd_some_fill_120x120(uint16_t * pframe_buff, int AnimationCounter){
    for (int i=DH_FRAME_OFFSET; i < DH_FRAME_OFFSET + DH_FRAME; i++) {
        for (int j=DW_FRAME_OFFSET; j < DW_FRAME_OFFSET + DW_FRAME; j++) {
            pframe_buff[i*DW_SCREEN+j] = (uint16_t)((i + AnimationCounter) / 4);
        }
    }
}
//-----------------------------------------------------------------------------
void lsd_qspi_fill_image_120x120(multi_lcd_id_t ID, int x, int y){

    frame_buffer_t *df = _assign_disp_frame(ID);

    if(df == NULL) return;

    int bmpx = 0; 
    int bmpy = 0;
    uint8_t data[2]; 

    uint16_t * pDst = (uint16_t*)df->frame;    

    for (int i = y; i < y + 120; i++){
        for (int j = x; j < x + 120; j++){
            data[0] = epd_bitmap_allArray[ID][bmpy * 120 + bmpx] >> 8;
            data[1] = epd_bitmap_allArray[ID][bmpy * 120 + bmpx];
            pDst[i * DH_SCREEN + j] = *(uint16_t*)data;

            bmpx++;
        }   
        bmpy++;   
        bmpx = 0;  
    } 
    
    //MLOGI("%s: display#%d, width %d, height %d, size %d\r\n ", 
    //    __func__, ID, df->width, df->height, df->size);
}
//-----------------------------------------------------------------------------
static avdk_err_t display_frame_free_cb(void *frame){
    //frame_buffer_display_free((frame_buffer_t *)frame);

    if (g_lcd_flush_finish_sem != NULL) {
        rtos_set_semaphore(&g_lcd_flush_finish_sem);
    }

    return AVDK_ERR_OK;
}
//-----------------------------------------------------------------------------
void _init(void){
    uint32_t frame_len = DW_SCREEN * DH_SCREEN * 2;

    for(multi_lcd_id_t i = 0; i < MULTI_LCD_ID_MAX; i++){
        disp_stack[i] = frame_buffer_display_malloc(frame_len);

        disp_stack[i]->fmt = PIXEL_FMT_RGB565;
        disp_stack[i]->width = DW_SCREEN; 
        disp_stack[i]->height = DH_SCREEN;

        lcd_qspi_display_fill_pure_color(disp_stack[i], BLACK_COLOR);

        //scale_image_120_to_240_BE(epd_bitmap_allArray[i], (uint16_t*)disp_stack[i]->frame);

        /*if(i == MULTI_LCD_ID_1){          
            draw_text((uint16_t*)disp_stack[i]->frame, "DISP1", TEXT_X, TEXT_Y, BLACK_COLOR);
            MLOGI("%s: display#%d, %p\r\n", __func__, i, disp_stack[i]);
        }
        else if(i == MULTI_LCD_ID_2){
            draw_text((uint16_t*)disp_stack[i]->frame, "DISP2", TEXT_X, TEXT_Y, BLACK_COLOR);
            MLOGI("%s: display#%d, %p\r\n", __func__, i, disp_stack[i]);
        }
        else{
            draw_text((uint16_t*)disp_stack[i]->frame, "DISP3", TEXT_X, TEXT_Y, BLACK_COLOR);
            MLOGI("%s: display#%d, %p\r\n", __func__, i, disp_stack[i]);
        }

        multi_lcd_display_flush(i, disp_stack[i], display_frame_free_cb);

        if (g_lcd_flush_finish_sem != NULL)  {
            rtos_get_semaphore(&g_lcd_flush_finish_sem, BEKEN_NEVER_TIMEOUT);
        }*/
    }
    
    MLOGI("%s: displays inited.\r\n", __func__);

    // Allocate asserts storage
    /*uint32_t size = 1024 * 1024 * 10;

    AssertsStorageArr[0] = (uint8_t*)psram_malloc(size);
    MLOGI("%s: AssertsStorage[0] %p (%d bytes)\r\n", __func__, AssertsStorageArr[0], size);*/
    
}
//-----------------------------------------------------------------------------
void blur_frame(multi_lcd_id_t ID){

    frame_buffer_t *disp_frame = _assign_disp_frame(ID);

    if(disp_frame != NULL){

        BlurFrameConfig_TypeDef bCfg;
        bCfg.pIn = epd_bitmap_allArray[ID]; //frameTmp;
        bCfg.pOut = (uint16_t*)disp_frame->frame;
        bCfg.coreW = 5;
        bCfg.h = DH_SCREEN;
        bCfg.w = DW_SCREEN;
        bCfg.h_frame = DH_FRAME;
        bCfg.w_frame = DW_FRAME;
        bCfg.h0 = (DH_SCREEN - DH_FRAME)/2;
        bCfg.w0 = (DW_SCREEN - DW_FRAME)/2;

        blur_frame_new(&bCfg);
    } 
}
//-----------------------------------------------------------------------------
void blur_and_scale_frame(multi_lcd_id_t ID){

    frame_buffer_t *disp_frame = _assign_disp_frame(ID);

    if(disp_frame != NULL){

        BlurFrameConfig_TypeDef bCfg;
        bCfg.pIn = epd_bitmap_allArray[ID]; 
        bCfg.pOut = img120x120_tmp1;
        bCfg.coreW = 5;
        bCfg.h = DH_FRAME;
        bCfg.w = DW_FRAME;
        bCfg.h_frame = DH_FRAME;
        bCfg.w_frame = DW_FRAME;
        bCfg.h0 = 0;
        bCfg.w0 = 0;

        blur_frame_new(&bCfg);

        scale_image_120_to_240(img120x120_tmp1, (uint16_t*)disp_frame->frame);
    } 
}
//-----------------------------------------------------------------------------
void draw_frame(multi_lcd_id_t ID){

    frame_buffer_t *disp_frame = _assign_disp_frame(ID);   

    if(disp_frame != NULL){          

        multi_lcd_display_flush(ID, disp_frame, display_frame_free_cb);

        if (g_lcd_flush_finish_sem != NULL){
            rtos_get_semaphore(&g_lcd_flush_finish_sem, BEKEN_NEVER_TIMEOUT);
        }
    }    
}
//-----------------------------------------------------------------------------
void reload_all_pure_color(uint16_t color){
    for(int i = MULTI_LCD_ID_1; i < MULTI_LCD_ID_MAX; i++){
        lcd_qspi_display_fill_pure_color(disp_stack[i], color);
    }
}
//-----------------------------------------------------------------------------
void reload_pure_color(void){
    uint16_t color = RED_COLOR;
    for(int i = MULTI_LCD_ID_1; i < MULTI_LCD_ID_MAX; i++){
        if(i == MULTI_LCD_ID_2){
            color = GREEN_COLOR;
        }
        else if(i == MULTI_LCD_ID_3){
            color = BLUE_COLOR;
        }

        lcd_qspi_display_fill_pure_color(disp_stack[i], color);
    }
}
//-----------------------------------------------------------------------------
void reload_images(void){
    for(int i = 0; i < DISPLAY_COUNT; i++){
        //lsd_qspi_fill_image_120x120(i, DW_FRAME_OFFSET, DH_FRAME_OFFSET);
        scale_image_120_to_240_BE(epd_bitmap_allArray[i], (uint16_t*)(disp_stack[i]->frame));
    }
    //MLOGI("%s: done\r\n", __func__);
}
//-----------------------------------------------------------------------------
#define DISP_TITLE_LENGTH 5
char disp_title[DISP_TITLE_LENGTH + 1] = {'D', 'I', 'S', 'P', 0, 0};
//-----------------------------------------------------------------------------
//-------------------- PALETTE ------------------------------------------------
//-----------------------------------------------------------------------------
#ifdef MLCD_PALETTE_TEST_RUN

#define IMAGE_W DW_FRAME
#define IMAGE_H DH_FRAME

beken_mutex_t _lock;

PALETTE _palette;
uint8_t image_indx0[IMAGE_W * IMAGE_H];
uint8_t image_indx1[IMAGE_W * IMAGE_H];
uint8_t image_indx2[IMAGE_W * IMAGE_H];
uint8_t * image_indx_arr[3] = {image_indx0, image_indx1, image_indx2};
TimeStamp_TypeDef palTmLbl;

//-----------------------------------------------------------------------------
void pal_load_image(const uint16_t * image, PALETTE * palette, uint8_t * image_indexed){
    for(int y = 0; y < IMAGE_H; y++){
        for (int x = 0; x < IMAGE_W; x++){
            image_indexed[y * IMAGE_H + x] = palette_get_color_idx(image[y * IMAGE_H + x], palette);
        }  
        //MLOGI("Row %d\r\n", y);      
    }
}
//-----------------------------------------------------------------------------
void pal_blur_and_scale_frame(multi_lcd_id_t ID){

    frame_buffer_t *disp_frame = _assign_disp_frame(ID);

    if(disp_frame != NULL){
        // Restore color from palette
        for(int y = 0; y < IMAGE_H; y++){
            for (int x = 0; x < IMAGE_W; x++){
                img120x120_tmp1[y * IMAGE_H + x] = _palette[image_indx_arr[ID][y*DW_FRAME + x]];
            }              
        }
        // Blur
        BlurFrameConfig_TypeDef bCfg;
        bCfg.pIn = img120x120_tmp1; 
        bCfg.pOut = img120x120_tmp2;
        bCfg.coreW = 5;
        bCfg.h = DH_FRAME;
        bCfg.w = DW_FRAME;
        bCfg.h_frame = DH_FRAME;
        bCfg.w_frame = DW_FRAME;
        bCfg.h0 = 0;
        bCfg.w0 = 0;

        blur_frame_new(&bCfg);
        // Prepare frame fo display
        scale_image_120_to_240(img120x120_tmp2, (uint16_t*)disp_frame->frame);
    } 
}
//-----------------------------------------------------------------------------
void interpolate_120_to_240(uint16_t * pSrc_120x120, uint16_t * pDst_240x240){
    // filling by original px
    for (int y = 0; y < 120; y++){
        for(int x = 0; x < 120; x++){
            pDst_240x240[y*2*240 + x*2] = pSrc_120x120[y*120 + x];
        }
    }
    // calc subpixels
    uint16_t A, B, rA, gA, bA, rB, gB, bB, tmpColor, tmpR, tmpG, tmpB;

    // 16 FPS
    // LEFT and RIGHT
    for (int y = 0; y < 239; y+=2){
        for(int x = 1; x < 239; x+=2){
            A = pDst_240x240[ y * 240 + x - 1];
            B = pDst_240x240[ y * 240 + x + 1];

            get_rgb_from_color(A, &rA, &gA, &bA);
            get_rgb_from_color(B, &rB, &gB, &bB);

            tmpR = (rA + rB) / 2;
            tmpG = (gA + gB) / 2;
            tmpB = (bA + bB) / 2;

            set_rgb_to_color(&tmpColor, tmpR, tmpG, tmpB);
            pDst_240x240[ y * 240 + x ] = tmpColor;
        }
    }
    // TOP and BOTTOM
    for (int y = 1; y < 239; y+=2){
        for(int x = 0; x < 240; x++){
            A = pDst_240x240[ (y - 1) * 240 + x];
            B = pDst_240x240[ (y + 1) * 240 + x];

            get_rgb_from_color(A, &rA, &gA, &bA);
            get_rgb_from_color(B, &rB, &gB, &bB);

            tmpR = (rA + rB) / 2;
            tmpG = (gA + gB) / 2;
            tmpB = (bA + bB) / 2;

            set_rgb_to_color(&tmpColor, tmpR, tmpG, tmpB);
            pDst_240x240[ y * 240 + x ] = tmpColor;
        }
    }

    // 15 FPS
    /*uint16_t C, D,  rC, gC, bC, rD, gD, bD;
    for (int y = 1; y < 239; y+=2){
        for(int x = 1; x < 239; x+=2){

            A = pDst_240x240[ (y - 1) * 240 + x - 1];
            B = pDst_240x240[ (y - 1) * 240 + x + 1];
            C = pDst_240x240[ (y + 1) * 240 + x - 1];
            D = pDst_240x240[ (y + 1) * 240 + x + 1];

            get_rgb_from_color(A, &rA, &gA, &bA);
            get_rgb_from_color(B, &rB, &gB, &bB);
            get_rgb_from_color(C, &rC, &gC, &bC);
            get_rgb_from_color(D, &rD, &gD, &bD);

            tmpR = (rA + rB) / 2;
            tmpG = (gA + gB) / 2;
            tmpB = (bA + bB) / 2;
            set_rgb_to_color(&tmpColor, tmpR, tmpG, tmpB);

            pDst_240x240[ (y + 1) * 240 + x] = tmpColor; // TOP

            tmpR = (rA + rC) / 2;
            tmpG = (gA + gC) / 2;
            tmpB = (bA + bC) / 2;
            set_rgb_to_color(&tmpColor, tmpR, tmpG, tmpB);

            pDst_240x240[ y * 240 + x - 1 ] = tmpColor; // LEFT

            tmpR = (rB + rD) / 2;
            tmpG = (gB + gD) / 2;
            tmpB = (bB + bD) / 2;
            set_rgb_to_color(&tmpColor, tmpR, tmpG, tmpB);

            pDst_240x240[ y * 240 + x + 1 ] = tmpColor; // RIGHT

            tmpR = (rC + rD) / 2;
            tmpG = (gC + gD) / 2;
            tmpB = (bC + bD) / 2;
            set_rgb_to_color(&tmpColor, tmpR, tmpG, tmpB);

            pDst_240x240[ (y + 1) * 240 + x ] = tmpColor; // BOTTOM

            tmpR = (rA + rB + rC + rD) / 4; 
            tmpG = (gA + gB + gC + gD) / 4;
            tmpB = (bA + bB + bC + bD) / 4;
            set_rgb_to_color(&tmpColor, tmpR, tmpG, tmpB);
            pDst_240x240[ y * 240 + x ] = tmpColor; // CENTER

        }
    }*/
    
}
//-----------------------------------------------------------------------------
void pal_load_image_to_frame_buf_interpolate(multi_lcd_id_t ID, uint8_t * image_indx, PALETTE * palette){    
    
    frame_buffer_t *df = _assign_disp_frame(ID);
    uint16_t * pDst = (uint16_t*)df->frame; 
    uint16_t color;
    
    if(df != NULL){
        for(int y = 0; y < DW_FRAME; y++){
            for(int x = 0; x < DH_FRAME; x++){
                img120x120_tmp1[y*120 + x] = (uint16_t)(*palette)[image_indx[y*DW_FRAME + x]];   
            }
        }

        interpolate_120_to_240(img120x120_tmp1, pDst);

        for(int y = 0; y < 240; y++){
            for(int x = 0; x < 240; x++){
                color = pDst[y * 240 + x];
                color = (color >> 8) | ( color << 8);
                pDst[y * 240 + x] = color;

            }
        }
    }
}
//-----------------------------------------------------------------------------
void pal_load_image_to_frame_buf_simple(multi_lcd_id_t ID, uint8_t * image_indx, PALETTE * palette){    
    
    frame_buffer_t *df = _assign_disp_frame(ID);
    uint16_t * pDst = (uint16_t*)df->frame; 
    uint16_t color;
    
    if(df != NULL){
        for(int y = 0; y < DW_FRAME; y++){
            for(int x = 0; x < DH_FRAME; x++){
                color = (uint16_t)(*palette)[image_indx[y*DW_FRAME + x]];
                color = (color >> 8) | ( color << 8);
                pDst[y*2 * 240 + x*2] = color;
            }
        }
    }
}
//-----------------------------------------------------------------------------
void pal_load_image_to_frame_buf(multi_lcd_id_t ID, uint8_t * image_indx, PALETTE * palette){    
    
    frame_buffer_t *df = _assign_disp_frame(ID);
    uint16_t * pDst = (uint16_t*)df->frame; 
    uint16_t color;
    
    if(df != NULL){
        for(int y = 0; y < DW_FRAME; y++){
            for(int x = 0; x < DH_FRAME; x++){
                color = (uint16_t)(*palette)[image_indx[y*DW_FRAME + x]];
                color = (color >> 8) | ( color << 8);
                //pDst[(y + DH_FRAME_OFFSET)*DH_SCREEN + x + DW_FRAME_OFFSET] = color;
                pDst[y*2 * 240 + x*2] = color;
                pDst[y*2 * 240 + x*2 + 1] = color;
                pDst[(y*2 + 1) * 240 + x*2] = color;
                pDst[(y*2 + 1) * 240 + x*2 + 1] = color;
            }
        }
    }
}
//-----------------------------------------------------------------------------
void pal_init(void){

    _init();

    palette_generate(&_palette);    

    mTimeStamp_Start(&palTmLbl);

    pal_load_image(epd_bitmap_allArray[0], &_palette, image_indx_arr[0]);
    pal_load_image(epd_bitmap_allArray[1], &_palette, image_indx_arr[1]);
    pal_load_image(epd_bitmap_allArray[2], &_palette, image_indx_arr[2]);

    mTimeStamp_Stop(&palTmLbl);
    mTimeStamp_StatisticsAndReset(&palTmLbl);

    MLOGI("%s: load image done (%d ms)!\r\n", __func__, palTmLbl.timeAcc);     
}
//-----------------------------------------------------------------------------
void pal_test_simple(void){
                 
    for(int i = MULTI_LCD_ID_1; i < MULTI_LCD_ID_MAX; i++){

        pal_load_image_to_frame_buf_simple(i, image_indx_arr[i], &_palette); 

        draw_frame(i);
    }            
}
//-----------------------------------------------------------------------------
void pal_test_copy_px(void){
                 
    for(int i = MULTI_LCD_ID_1; i < MULTI_LCD_ID_MAX; i++){

        pal_load_image_to_frame_buf(i, image_indx_arr[i], &_palette); 

        draw_frame(i);
    }            
}
//-----------------------------------------------------------------------------
void pal_test_interpolate(void){
                 
    for(int i = MULTI_LCD_ID_1; i < MULTI_LCD_ID_MAX; i++){

        pal_load_image_to_frame_buf_interpolate(i, image_indx_arr[i], &_palette);

        draw_frame(i);
    }            
}
//-----------------------------------------------------------------------------
void pal_test_with_blur(void){    
                 
    for(int i = MULTI_LCD_ID_1; i < MULTI_LCD_ID_MAX; i++){
            
        pal_blur_and_scale_frame(i);

        draw_frame(i);
    }  
}
//-----------------------------------------------------------------------------
#endif
//-----------------------------------------------------------------------------
//--------------------- PALETTE END -------------------------------------------
//-----------------------------------------------------------------------------
void mlcd_test(void){

    int ret = rtos_init_semaphore_ex(&g_lcd_flush_finish_sem, 1, 1);
    if (ret)
    {
        MLOGE("%s, %d, rtos_init_semaphore_ex fail[%d]!\n", __func__, __LINE__, ret);
        return;
    }

#ifdef MLCD_TEST_RUN

    multi_lcd_init();

    multi_lcd_backlight_open(MULTI_LCD_BACKLIGHT_CTR_PIN);

    _init();  
    
    while(1){

        mTimeStamp_Start(&lbl4);

        for(int i = 0; i < DISPLAY_COUNT; i++){

            mTimeStamp_Start(lbl_stack[i]);
            blur_and_scale_frame(i);
            mTimeStamp_Stop(lbl_stack[i]);

            draw_frame(i);
        }

        mTimeStamp_Stop(&lbl4);

        /*MLOGI("Frame %d done. Total time %d ms\r\n", animationCnt++, lbl4.timeAcc);*/

        if(lbl4.count % TIME_LBL_PERIOD == 0){  
            
            mTimeStamp_StatisticsAndReset(&lbl4);

            MLOGI("FPS: %d\r\n", lbl4.timesPerSecond);

            for (int  i = 0; i < DISPLAY_COUNT; i++) {
                mTimeStamp_StatisticsAndReset(lbl_stack[i]);
                MLOGI("Disp#%d: %d blurs and scales\r\n", i, lbl_stack[i]->timesPerSecond);
            }            
            
            /*for (size_t i = 0; i < DISPLAY_COUNT; i++)
            {
                mTimeStamp_StatisticsAndReset(lbl_stack[i]);
                disp_title[DISP_TITLE_LENGTH - 1] = '0' + i; 
                draw_text((uint16_t*)disp_stack[i]->frame, disp_title, TEXT_X, TEXT_Y, BLACK_COLOR);
                draw_text((uint16_t*)disp_stack[i]->frame, lbl_stack[i]->msg, TEXT_X, TEXT_Y + 10, BLACK_COLOR);
            }            

            mTimeStamp_StatisticsAndReset(&lbl4);            

            draw_text((uint16_t*)disp_stack[0]->frame, lbl4.msg, TEXT_X, TEXT_Y+20, BLACK_COLOR);
            draw_text((uint16_t*)disp_stack[1]->frame, lbl4.msg, TEXT_X, TEXT_Y+20, BLACK_COLOR);
            draw_text((uint16_t*)disp_stack[2]->frame, lbl4.msg, TEXT_X, TEXT_Y+20, BLACK_COLOR);*/

            TimeStamp_TypeDef tmLbl;

            mTimeStamp_Start(&tmLbl);

            for(int i = 0; i < TIME_LBL_PERIOD; i++){                
                reload_images();                
            }

            mTimeStamp_Stop(&tmLbl);

            mTimeStamp_StatisticsAndReset(&tmLbl);     

            MLOGI("Reload images: %d times per second.\r\n", tmLbl.timesPerSecond * TIME_LBL_PERIOD * DISPLAY_COUNT);
            
            //reload_images();
            
            /*draw_text((uint16_t*)disp_stack[0]->frame, tmLbl.msg, TEXT_X + 100, TEXT_Y+20, BLACK_COLOR);

            multi_lcd_display_flush(MULTI_LCD_ID_1, disp_stack[MULTI_LCD_ID_1], display_frame_free_cb);
            multi_lcd_display_flush(MULTI_LCD_ID_2, disp_stack[MULTI_LCD_ID_2], display_frame_free_cb);
            multi_lcd_display_flush(MULTI_LCD_ID_3, disp_stack[MULTI_LCD_ID_3], display_frame_free_cb);
        
            rtos_delay_milliseconds(3000);*/
        }       
        
    }

#endif

#ifdef MLCD_PALETTE_TEST_RUN

    multi_lcd_init();

    multi_lcd_backlight_open(MULTI_LCD_BACKLIGHT_CTR_PIN);

    pal_init();    

    while(1){      
        
        reload_all_pure_color(BLACK_COLOR);

        for (int i = 0; i < TIME_LBL_PERIOD; i++){
            mTimeStamp_Start(&palTmLbl);
            pal_test_simple();
            mTimeStamp_Stop(&palTmLbl); 
        }
          
        mTimeStamp_StatisticsAndReset(&palTmLbl);
        MLOGI("%s: Restore image from 120x120x1 buf with prepared palette and scale to 240x2040x2 (simple)\r\n", __func__);
        MLOGI("%s: FPS (per 3 displays) %d \r\n", __func__, palTmLbl.timesPerSecond);

        reload_all_pure_color(BLACK_COLOR);
        
        for (int i = 0; i < TIME_LBL_PERIOD; i++){
            mTimeStamp_Start(&palTmLbl);
            pal_test_copy_px();
            mTimeStamp_Stop(&palTmLbl); 
        }
          
        mTimeStamp_StatisticsAndReset(&palTmLbl);
        MLOGI("%s: Restore image from 120x120x1 buf with prepared palette and scale to 240x2040x2 (copy pxs)\r\n", __func__);
        MLOGI("%s: FPS (per 3 displays) %d \r\n", __func__, palTmLbl.timesPerSecond);

        reload_all_pure_color(BLACK_COLOR);
        
        for (int i = 0; i < TIME_LBL_PERIOD; i++){
            mTimeStamp_Start(&palTmLbl);
            pal_test_interpolate();
            mTimeStamp_Stop(&palTmLbl); 
        }
          
        mTimeStamp_StatisticsAndReset(&palTmLbl);
        MLOGI("%s: Restore image from 120x120x1 buf with prepared palette and scale to 240x2040x2 (interpolate)\r\n", __func__);
        MLOGI("%s: FPS (per 3 displays) %d \r\n", __func__, palTmLbl.timesPerSecond);
        
        /*for (int i = 0; i < TIME_LBL_PERIOD; i++){
            
            mTimeStamp_Start(&palTmLbl);
            pal_test_with_blur();
            mTimeStamp_Stop(&palTmLbl);                       
                
        }

        mTimeStamp_StatisticsAndReset(&palTmLbl);
        MLOGI("%s: Restore image from 120x120x1 buf with prepared palette, blur and scale to 240x2040x2\r\n", __func__);
        MLOGI("%s: FPS (per 3 displays) %d \r\n", __func__, palTmLbl.timesPerSecond);*/
    }
    
#endif
}
//-----------------------------------------------------------------------------
