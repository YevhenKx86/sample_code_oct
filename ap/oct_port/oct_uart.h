#pragma once
#include "common/bk_typedef.h"
#include "driver/uart.h"
#include "driver/hal/hal_uart_types.h"
#include <stdint.h>
#include <string.h>
//#include "memory_attribute.h"
//#include "uart_dma.h"
//#include "flow.h"
//#include "packet.h"
//#include "FreeRTOS.h"
//#include "task.h"
//#include "task_utils.h"
#include "dev_uart_drv.h"
#include "oct_helpers.h"
#include "oct_vars.h"
#include "oct.h"
#include "oct_helpers.h"
#include "oct_port.h"
#include "os/str.h"
#include "os/os.h"
#include "os/mem.h"

//Defined inside HAL
//extern TL VDMA_REGISTER_T*         vdma[];
//extern TL VDMA_REGISTER_PORT_T*    vdma_port[];

#define OCT_UART_BAUDRATE 4333333
#define MAX_DISP_DATA 16
#define TEST_PAYLOAD_LEN 64UL
#define TEST_PAYLOAD_CRC 0xBC103D92

// static uint32_t crc = 0;
// static size_t rx_len = 0;

static uint8_t uart_rx_buf[16384] = { 0 };
static beken_thread_t uart_task_hnd = NULL;

static void uart_rx_task (beken_thread_arg_t arg);

// TODO double copy data
//[NOTE]: Called from task
void rx_done_callback (uart_id_t id, void *param)
{
}

/*void OCT_UART_dma_callback(hal_uart_callback_event_t event, void *user_data){
    (void)event; (void)user_data;
}*/

//[NOTE]: Called from ISR, so just copy without ANY blocking
void OCT_UART_dma_callback2(hal_uart_callback_event_t event, void *user_data){      

}

void OCT_UART_reinit_port (hal_uart_port_t port)
{
    const uart_config_t cfg =
    {
        .baud_rate = OCT_UART_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_NONE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_FLOWCTRL_DISABLE,
        .src_clk   = UART_SCLK_XTAL_26M,
        .rx_dma_en = true,
        .tx_dma_en = true,
    };

    BK_LOG_ON_ERR(bk_uart_init((uart_id_t)port, &cfg));
    // BK_LOG_ON_ERR(bk_uart_register_rx_isr((uart_id_t)port, rx_done_callback, NULL));
    bk_uart_register_dma_rx_isr((uart_id_t)port, rx_done_callback, NULL);
    BK_LOG_ON_ERR(bk_uart_enable_rx_interrupt((uart_id_t)port));
}

//[NOTE]: Called from main but also on each wake up
bool OCT_UART_reinit(){

    // [NOTE]: Tmp solution. Mayby need to use psram_malloc for init oct_vars.h variables.
    OctHwidsNum = 0;
    for(int i = 0; i < DEBUG_STRINGS; i++){
        OctText[i][0] = '\0';
    }
    //---

    //Once per OS lifetime create internal objects and setup buffers
    for (int i = 0; i < NET_LINES_MAX; i++){

        os_memset((void*)&OctUarts[i], 0, sizeof(octUart_t));

        OctUarts[i].TxMutex = xSemaphoreCreateMutex();
        OctUarts[i].RxCache = OctUartsCache[i];
        OctUarts[i].RxCacheCap = OCT_UART_BUF_CAP;
    } 

    //Reset parsing buffer
    for (int i = 0; i < NET_LINES_MAX; i++) OctUarts[i].RxAvailable = OctUarts[i].RxProcessed = 0;

    OCT_UART_reinit_port(HAL_UART_0);
    OCT_UART_reinit_port(HAL_UART_1);
    OCT_UART_reinit_port(HAL_UART_2);

    rtos_create_thread(&uart_task_hnd, BEKEN_DEFAULT_WORKER_PRIORITY, "uart_rx", uart_rx_task, 8192, NULL);

    OctUartInitialized = true;

    return true;
}

static void uart_rx_task (beken_thread_arg_t arg)
{
    while (1)
    {
        int pos = 0;
        int ret = 0;
        do
        {
            ret = bk_uart_read_bytes(0, uart_rx_buf + pos, sizeof(uart_rx_buf), 1);
            if (ret > 0)
            {
                pos += ret;
            }
        } while (ret > 0);

        if (pos > 0)
        {
            OCT_text(-1, "rx:%d pos:%d", ret, pos);
            bk_uart_write_bytes(0, uart_rx_buf, pos);
        }
        rtos_delay_milliseconds(1);
    }
}

//When trying to send packet several times but DMA buffer is still full, old code tries to reset port
void OCT_UART_reset_port(uint32_t line_id){

    //hal_uart_port_t port = (hal_uart_port_t)(line_id + 1);
    dev_uart_drv_deinit(line_id);
    OCT_UART_reinit_port(line_id);
    //...Reset some 'tx overflow' counter
    OCT_terminate("OCT_UART_reset_port NIY");
}

//Called before sleep
void OCT_UART_deinit(void){

    OctUartInitialized = false;

    if (uart_task_hnd)
    {
        rtos_delete_thread(&uart_task_hnd);
        uart_task_hnd = NULL;
    }

    dev_uart_drv_deinit(DEV_UART_DRV_ID_0);
    dev_uart_drv_deinit(DEV_UART_DRV_ID_1);
    dev_uart_drv_deinit(DEV_UART_DRV_ID_2);
}


//Put data to DMA buffer of single port. If there is not enough space then packet would be fully discarded
void OCT_UART_send(uint32_t line_id, const octPacket_t* pkt){

    //OCT_text(-1, "tx %d", line_id);
    //Safety check
    if (!OctUartInitialized  ||  line_id >= NET_LINES_MAX) { return;}

        //Size of packet
        uint32_t data_size = PKT_SIZES[pkt->Header.Type];
        OctUarts[line_id].TxStatBandwidth.Counter += data_size;

        //Lock mutex, this function can be called from different tasks
        //if (xSemaphoreTake(OctUarts[line_id].TxMutex, portMAX_DELAY) == pdTRUE){

            bk_err_t err = 
            dev_uart_drv_write((dev_uart_drv_id_t)line_id, (const uint8_t*)pkt, data_size);
            
            OctUarts[line_id].TxStatWritten += data_size;

            OCT_text(-1, "tx %d, l=%d, err=%d, %ds", line_id, data_size, err, rtos_get_time()/1000);
            /*if(data_size > 0){
                char str[128] = {0};
                for(int i = 0; i < 9; i++)
                    sprintf(str + 3*i, "%02X ", ((uint8_t*)pkt)[i]);
                OCT_text(-1, "%s", str);
            }*/
    //}


            //Release mutex
            //xSemaphoreGive(OctUarts[line_id].TxMutex);
        //}

        //Lock mutex, this function can be called from different tasks
        /*const uint32_t tx_channel_offset = OCT_LINE_TO_TX_CHANNEL_OFFSET[line_id];
        if (xSemaphoreTake(OctUarts[line_id].TxMutex, portMAX_DELAY) == pdTRUE)
        {
            //Check space in DMA buffer, inlined hal_uart_get_available_send_space()
            uint32_t space = vdma[tx_channel_offset]->VDMA_FFSIZE - vdma[tx_channel_offset]->VDMA_FFCNT;

            //Make sure there is no overflow, because no sense in sending partial packet
            const uint8_t*  data        = (const uint8_t*)pkt;
            if (space >= data_size)
            {
                ///vdma[tx_channel_offset]->VDMA_CON &= ~VDMA_CON_ITEN_MASK;  //Disable DMA interrupts [TODO]: Check if there will be any unneeded interrupts without this disabling
                
#ifdef OCTSIM
                //Batch version of pushing bytes to "port" (possible in sim only; gives lower CPU load)
                vdma_port[tx_channel_offset]->VDMA_PORT.PushBatchFromArray(data, data_size);
#else
                //Write to tx buffer
                for (uint32_t i = 0; i < data_size; i++)
                    vdma_port[tx_channel_offset]->VDMA_PORT = data[i];
#endif
                
                ///vdma[tx_channel_offset]->VDMA_CON |= VDMA_CON_ITEN_MASK;   //Enable DMA interrupts
                OctUarts[line_id].TxStatWritten += data_size;              //Just for stats
            }
            else
            {
                OctUarts[line_id].TxStatDropped++;
            } //[TODO]: Add this drop into net statistics. Maybe also reset port if it stays full for too long (as it were in old code)

            //Release mutex
            xSemaphoreGive(OctUarts[line_id].TxMutex);
        }*/
    }



