#pragma once
#include <string.h>
//#include "memory_attribute.h"
//#include "uart_dma.h"
//#include "flow.h"
//#include "packet.h"
//#include "FreeRTOS.h"
//#include "task.h"
//#include "task_utils.h"
#include "oct_vars.h"
#include "oct_helpers.h"
#include "dev_uart_drv.h"

//Defined inside HAL
//extern TL VDMA_REGISTER_T*         vdma[];
//extern TL VDMA_REGISTER_PORT_T*    vdma_port[];

#define OCT_UART_BAUDRATE 3000000

#define  TMP_DMA_RX_DATA_SIZE  4096
ATTR_RWDATA_IN_PSRAM_4BYTE_ALIGN uint8_t tmpDmaRxData[TMP_DMA_RX_DATA_SIZE];

// TODO double copy data
//[NOTE]: Called from task
void rx_done_callback(dev_uart_drv_id_t drv_id, const uint8_t *buff, uint32_t buff_len){

    //How much space there is
    uint32_t space = OctUarts[drv_id].RxCacheCap - (OctUarts[drv_id].RxAvailable - OctUarts[drv_id].RxProcessed);

    //How much data can be copied
    uint32_t count = buff_len;
    if (count > space) count = space;

    //Starting after the last busy byte, copy available count
    uint32_t wrapping = OctUarts[drv_id].RxCacheCap - 1;
    for (uint32_t n = OctUarts[drv_id].RxAvailable + 1, copied = 0;  copied < count;  copied++, n++)
        OctUarts[drv_id].RxCache[ n & wrapping ] = buff[copied];

    //Safely update the counter
    OCT_MEM_BARRIER;  OctUarts[drv_id].RxAvailable += count;
    OCT_stat(OCT_time(), count, &StatUartDmaRxData[drv_id]);

    OCT_text(-1, "[%d] %d %d %d", drv_id, buff_len, space, OctUarts[drv_id].RxAvailable);   
        
    /*if(buff_len > 0){
        char str[128] = {0};
        for(int i = 0; i < buff_len; i++)
            sprintf(str + 2*i, "%02X ", buff[i]);
        OCT_text(-1, "%s", str);
    }*/
    
}

/*void OCT_UART_dma_callback(hal_uart_callback_event_t event, void *user_data){
    (void)event; (void)user_data;
}*/

//[NOTE]: Called from ISR, so just copy without ANY blocking
void OCT_UART_dma_callback2(hal_uart_callback_event_t event, void *user_data){      

        /*const uint32_t rx_channel_offset = OCT_LINE_TO_RX_CHANNEL_OFFSET[line_id];

        if (event == HAL_UART_EVENT_READY_TO_READ)
        {
            ///uint32_t count = vdma[rx_channel_offset]->VDMA_FFCNT;
            ///for (uint32_t copied = 0;  copied < count;  copied++)
            ///    (void)(uint8_t)(vdma_port[rx_channel_offset]->VDMA_PORT);


            //How much space there is
            uint32_t space = OctUarts[line_id].RxCacheCap - (OctUarts[line_id].RxAvailable - OctUarts[line_id].RxProcessed);

            //How much data can be copied
            uint32_t count = vdma[rx_channel_offset]->VDMA_FFCNT; //Inlined vdma_get_available_receive_bytes
            if (count > space) count = space;

            //[TODO]: Collecting bytes into uint (when possible) before copying into an array probably would be slightly faster. Need additional handling of non-32bit-aligned head and tail bytes though.
            
#ifdef OCTSIM
            //Batch version of extracting bytes from "port" (possible in sim only - to lower CPU load)
            vdma_port[rx_channel_offset]->VDMA_PORT.MoveToRingBuffer(OctUarts[line_id].RxCache, OctUarts[line_id].RxAvailable + 1, OctUarts[line_id].RxCacheCap, count);
#else
            //Starting after the last busy byte, copy available count
            uint32_t wrapping = OctUarts[line_id].RxCacheCap - 1;
            for (uint32_t n = OctUarts[line_id].RxAvailable + 1, copied = 0;  copied < count;  copied++, n++)
                OctUarts[line_id].RxCache[ n & wrapping ] = (uint8_t)(vdma_port[rx_channel_offset]->VDMA_PORT);  //Inlined vdma_pop_data

#endif
            //Safely update the counter
            OCT_MEM_BARRIER;  OctUarts[line_id].RxAvailable += count;
            OCT_stat(OCT_time(), count, &StatUartDmaRxData[line_id]);

            //Unpause parsing task
            ///BaseType_t hpw = pdFALSE;
            ///vTaskNotifyGiveFromISR(OctTaskParseTraffic, &hpw);
            ///portYIELD_FROM_ISR(hpw);
        }
        else if (event == HAL_UART_EVENT_TRANSACTION_ERROR)
        {
           /// OCT_text(-1, "HAL_UART_EVENT_TRANSACTION_ERROR");
              //[TODO]: Is it ever registered? Possible errors:
                //Break Interrupt Line held low longer than a character time (break condition)
                //Framing Error   Stop bit not detected where expected
                //Parity Error    Parity bit does not match expected value
                //Overrun Error   Receiver buffer full, new data lost or overwritten
        }
        else if (event == HAL_UART_EVENT_READY_TO_WRITE) {}*/
}

void OCT_UART_reinit_port(hal_uart_port_t port){

        uint32_t line_id = ((uint32_t)port);

        //bk_printf_deinit();
        dev_uart_drv_init((dev_uart_drv_id_t)line_id, (uart_id_t)line_id, OCT_UART_BAUDRATE, rx_done_callback);            
        
        //Accepted ports are only 1, 2, 3. Adjust mapping to line id if this ever changes.
        /*uint32_t line_id = ((uint32_t)port) - 1;

        //Configure port
        hal_uart_config_t uart_config = {HAL_UART_BAUDRATE_3000000, HAL_UART_WORD_LENGTH_8, HAL_UART_STOP_BIT_1, HAL_UART_PARITY_NONE};
        hal_uart_status_t status = hal_uart_init(port, &uart_config);
        if (status != HAL_UART_STATUS_OK) { DMP("UART initialization failed: %d", status); }

        hal_uart_disable_flowcontrol(port);

        //Configure DMA mode
        hal_uart_dma_config_t dma_config = {0};
        dma_config.receive_vfifo_alert_size       = OCT_RECV_ALERT_SIZE;
        dma_config.receive_vfifo_buffer           = OctDmaRxBuffers[line_id];
        dma_config.receive_vfifo_buffer_size      = OCT_UART_DMA_CAP;
        dma_config.receive_vfifo_threshold_size   = OCT_RECV_THRESHOLD_SIZE;
        dma_config.send_vfifo_buffer              = OctDmaTxBuffers[line_id];
        dma_config.send_vfifo_buffer_size         = OCT_UART_DMA_CAP;
        dma_config.send_vfifo_threshold_size      = OCT_SEND_THRESHOLD_SIZE;
        status = hal_uart_set_dma(port, &dma_config);
        if (status != HAL_UART_STATUS_OK) { DMP("UART DMA initialization failed: %d", status); return; }

        status = hal_uart_register_callback(port, OCT_UART_dma_callback, (void*)port);
        if (status != HAL_UART_STATUS_OK) { DMP("UART DMA callback initialization failed: %d", status); return; } */ 
}

//[NOTE]: Called from main but also on each wake up
bool OCT_UART_reinit(){

    //Once per OS lifetime create internal objects and setup buffers
    for (int i = 0; i < NET_LINES_MAX; i++){

        os_memset((void*)&OctUarts[i], 0, sizeof(octUart_t));

        OctUarts[i].TxMutex = xSemaphoreCreateMutex();
        OctUarts[i].RxCache = OctUartsCache[i];
        OctUarts[i].RxCacheCap = OCT_UART_BUF_CAP;
    } 

    //Reset parsing buffer
    for (int i = 0; i < NET_LINES_MAX; i++) OctUarts[i].RxAvailable = OctUarts[i].RxProcessed = 0;

    OCT_UART_reinit_port(UART_ID_0);
    OCT_UART_reinit_port(UART_ID_1);
    OCT_UART_reinit_port(UART_ID_2);

    OctUartInitialized = true;

    return true;
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

    dev_uart_drv_deinit(DEV_UART_DRV_ID_0);
    dev_uart_drv_deinit(DEV_UART_DRV_ID_1);
    dev_uart_drv_deinit(DEV_UART_DRV_ID_2);
}


//Put data to DMA buffer of single port. If there is not enough space then packet would be fully discarded
void OCT_UART_send(uint32_t line_id, const octPacket_t* pkt){


    OCT_text(-1, "tx %d", line_id);
    //Safety check
    if (!OctUartInitialized  ||  line_id >= NET_LINES_MAX) return;

    //Size of packet
    uint32_t data_size = PKT_SIZES[pkt->Header.Type];
    OctUarts[line_id].TxStatBandwidth.Counter += data_size;

    //Lock mutex, this function can be called from different tasks
    //if (xSemaphoreTake(OctUarts[line_id].TxMutex, portMAX_DELAY) == pdTRUE){

    //bk_err_t err = 
    dev_uart_drv_write((dev_uart_drv_id_t)line_id, (const uint8_t*)pkt, data_size);
        OctUarts[line_id].TxStatWritten += data_size;

        //OCT_text(-1, "   tx %d, l=%d, err=%d, %ds", line_id, data_size, err, rtos_get_time()/1000);


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



