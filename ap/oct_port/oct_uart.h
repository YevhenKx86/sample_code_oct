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
#include "driver/uart.h"
#include "uart_driver.h"

//Defined inside HAL
//extern TL VDMA_REGISTER_T*         vdma[];
//extern TL VDMA_REGISTER_PORT_T*    vdma_port[];

#define  TMP_DMA_RX_DATA_SIZE  4096
ATTR_RWDATA_IN_PSRAM_4BYTE_ALIGN uint8_t tmpDmaRxData[TMP_DMA_RX_DATA_SIZE];

int hal_uart_deinit(hal_uart_port_t port){
    return bk_uart_deinit((uint32_t)port-1);
}

// TODO double copy data
static inline void uart_copy_bytes_to_oct_buf(uart_id_t id){
    //How much space there is
    uint32_t space = OctUarts[id].RxCacheCap - (OctUarts[id].RxAvailable - OctUarts[id].RxProcessed);

    //How much data can be copied
    uint32_t count = bk_uart_get_port_bytes_available(id); //Inlined vdma_get_available_receive_bytes
    if (count > space) count = space;

    bk_uart_read_port_bytes(id, (void*)tmpDmaRxData, count);

    //Starting after the last busy byte, copy available count
    uint32_t wrapping = OctUarts[id].RxCacheCap - 1;
    for (uint32_t n = OctUarts[id].RxAvailable + 1, copied = 0;  copied < count;  copied++, n++)
        OctUarts[id].RxCache[ n & wrapping ] = tmpDmaRxData[copied];

    //Safely update the counter
    OCT_MEM_BARRIER;  OctUarts[id].RxAvailable += count;
    OCT_stat(OCT_time(), count, &StatUartDmaRxData[id]);
    
}

void rx_done_callback(uart_id_t id, void *param){

    //uart_copy_bytes_to_oct_buf(id);
    //How much space there is
    uint32_t space = OctUarts[id].RxCacheCap - (OctUarts[id].RxAvailable - OctUarts[id].RxProcessed);

    //How much data can be copied
    uint32_t count = bk_uart_get_port_bytes_available(id); //Inlined vdma_get_available_receive_bytes
    if (count > space) count = space;

    bk_uart_read_port_bytes(id, (void*)tmpDmaRxData, count);

    //Starting after the last busy byte, copy available count
    uint32_t wrapping = OctUarts[id].RxCacheCap - 1;
    for (uint32_t n = OctUarts[id].RxAvailable + 1, copied = 0;  copied < count;  copied++, n++)
        OctUarts[id].RxCache[ n & wrapping ] = tmpDmaRxData[copied];

    //Safely update the counter
    OCT_MEM_BARRIER;  OctUarts[id].RxAvailable += count;
    OCT_stat(OCT_time(), count, &StatUartDmaRxData[id]);
}

void OCT_UART_dma_callback(hal_uart_callback_event_t event, void *user_data){
    (void)event; (void)user_data;
}

//[NOTE]: Called from ISR, so just copy without ANY blocking
void OCT_UART_dma_callback2(hal_uart_callback_event_t event, void *user_data){

        //User data is hal_uart_port_t, we are not using UART0, so actual index in arrays would be one less than passed port id
        /*const uint32_t line_id = ((uint32_t)(uintptr_t)user_data) - 1;
        if (line_id >= NET_LINES_MAX) return;

        if(event == HAL_UART_EVENT_READY_TO_READ){
            //if(linesRx_Actions[line_id] > 0){
                //linesRx_Actions[line_id] = 0;

                uint32_t space = OctUarts[line_id].RxCacheCap - (OctUarts[line_id].RxAvailable - OctUarts[line_id].RxProcessed);

                if(space > 0){  // ring buffer wrapping
                    int bytesCount = bk_uart_read_port_bytes(line_id, (void*)(&OctUarts[line_id].RxCache[OctUarts[line_id].RxAvailable]), space);
                    if(bytesCount > 0){
                        //Safely update the counter
                        OCT_MEM_BARRIER;  
                        OctUarts[line_id].RxAvailable += bytesCount;
                        OCT_stat(OCT_time(), bytesCount, &StatUartDmaRxData[line_id]);
                    }
                }
            //}
        }*/

        

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

void OCT_UART_reinit_port(hal_uart_port_t port)
    {
        uint32_t line_id = ((uint32_t)port) - 1;

        bk_uart_driver_init();
        bk_uart_deinit(line_id);

        OctUarts[line_id].RxAvailable = 0;
        OctUarts[line_id].RxProcessed = 0;
        OctUarts[line_id].RxCache = OctDmaRxBuffers[line_id];
        OctUarts[line_id].RxCacheCap = OCT_UART_DMA_CAP;

        uart_config_t config = {0};

        os_memset(&config, 0, sizeof(uart_config_t));
        config.baud_rate = 115200;
        config.data_bits = UART_DATA_8_BITS;
        config.parity = UART_PARITY_NONE;
        config.stop_bits = UART_STOP_BITS_1;
        config.flow_ctrl = UART_FLOWCTRL_DISABLE;
        config.src_clk = UART_SCLK_XTAL_26M;
        if (bk_uart_init(line_id, &config) != BK_OK) {
            DMP("UART initialization failed: %d", status);
        }

        bk_uart_register_rx_isr(line_id, rx_done_callback, (void*)(uintptr_t)line_id);
        bk_uart_enable_rx_interrupt(line_id);

        

        //cfg DMA
        // DMA initialized by CONFIG_UART_RX_DMA and CONFIG_UART_TX_DMA macros

        //---------------------------------------------------------------------------

        //Accepted ports are only 1, 2, 3. Adjust mapping to line id if this ever changes.
        //uint32_t line_id = ((uint32_t)port) - 1;

        //Configure port
        /*hal_uart_config_t uart_config = {HAL_UART_BAUDRATE_3000000, HAL_UART_WORD_LENGTH_8, HAL_UART_STOP_BIT_1, HAL_UART_PARITY_NONE};
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
        if (status != HAL_UART_STATUS_OK) { DMP("UART DMA callback initialization failed: %d", status); return; }*/
    }


//[NOTE]: Called from main but also on each wake up
bool OCT_UART_reinit()
    {
        OCT_UART_reinit_port(HAL_UART_1);
        OCT_UART_reinit_port(HAL_UART_2);
        OCT_UART_reinit_port(HAL_UART_3);

        OctUartInitialized = true;


        //Once per OS lifetime create internal objects and setup buffers
        /*if (OctSleepHandleUart == 0)
        {
            OctSleepHandleUart = hal_sleep_manager_set_sleep_handle(OCT_SLEEP_HANDLE_NAME_UART);
            for (int i = 0; i < NET_LINES_MAX; i++) OctUarts[i].TxMutex = xSemaphoreCreateMutex(),  OctUarts[i].RxCache = OctUartsCache[i],  OctUarts[i].RxCacheCap = OCT_UART_BUF_CAP;
        }

        //Lock sleep as soon as UARTs are active
        hal_sleep_manager_lock_sleep(OctSleepHandleUart);

        //Reset parsing buffer
        for (int i = 0; i < NET_LINES_MAX; i++) OctUarts[i].RxAvailable = OctUarts[i].RxProcessed = 0;

        // Configure pins for UART 1
        hal_gpio_init(HAL_GPIO_4),  hal_pinmux_set_function(HAL_GPIO_4, HAL_GPIO_4_U1RXD);
        hal_gpio_init(HAL_GPIO_5),  hal_pinmux_set_function(HAL_GPIO_5, HAL_GPIO_5_U1TXD);

        // Configure pins for UART 2
        hal_gpio_init(HAL_GPIO_6),  hal_pinmux_set_function(HAL_GPIO_6, HAL_GPIO_6_U2RXD);
        hal_gpio_init(HAL_GPIO_7),  hal_pinmux_set_function(HAL_GPIO_7, HAL_GPIO_7_U2TXD);

        // Configure pins for UART 3
#ifdef HW_VERSION_3_2_5
        hal_gpio_init(HAL_GPIO_2),  hal_pinmux_set_function(HAL_GPIO_2, HAL_GPIO_2_U3RXD);
        hal_gpio_init(HAL_GPIO_3),  hal_pinmux_set_function(HAL_GPIO_3, HAL_GPIO_3_U3TXD);
#else
        hal_gpio_init(HAL_GPIO_18), hal_pinmux_set_function(HAL_GPIO_18, HAL_GPIO_18_U3RXD);
        hal_gpio_init(HAL_GPIO_22), hal_pinmux_set_function(HAL_GPIO_22, HAL_GPIO_22_U3TXD);
#endif

        //[TODO]: Use screen to signal about errors
        //Only ports with values 1, 2, 3 are supported
        OCT_UART_reinit_port(HAL_UART_1),  OCT_UART_reinit_port(HAL_UART_2),  OCT_UART_reinit_port(HAL_UART_3);*/

        //OctUartInitialized = true;
        return true;
    }


//When trying to send packet several times but DMA buffer is still full, old code tries to reset port
void OCT_UART_reset_port(uint32_t line_id)
    {

        hal_uart_port_t port = (hal_uart_port_t)(line_id + 1);
        hal_uart_deinit(port);
        OCT_UART_reinit_port(port);
        //...Reset some 'tx overflow' counter
        OCT_terminate("OCT_UART_reset_port NIY");

        /*hal_uart_port_t port = (hal_uart_port_t)(line_id+1);
        hal_uart_deinit(port);
        OCT_UART_reinit_port(port);
        //...Reset some 'tx overflow' counter
        OCT_terminate("OCT_UART_reset_port NIY");*/
    }


//Called before sleep
void OCT_UART_deinit(void)
    {

        OctUartInitialized = false;

        hal_uart_deinit(HAL_UART_1);
        hal_uart_deinit(HAL_UART_2);
        hal_uart_deinit(HAL_UART_3);

        //No new packets can be send
        /*OctUartInitialized = false;

        //[TODO]: Wait on tx mutex to give module some time to flush?
        hal_uart_deinit(HAL_UART_1), hal_uart_deinit(HAL_UART_2), hal_uart_deinit(HAL_UART_3);

        // Reconfigure pins as I/O
        // UART1 RX\TX
        hal_gpio_init(HAL_GPIO_4),  hal_pinmux_set_function(HAL_GPIO_4, HAL_GPIO_4_GPIO4),      hal_gpio_set_direction(HAL_GPIO_4, HAL_GPIO_DIRECTION_INPUT),  hal_gpio_pull_up(HAL_GPIO_4);
        hal_gpio_init(HAL_GPIO_5),  hal_pinmux_set_function(HAL_GPIO_5, HAL_GPIO_5_GPIO5),      hal_gpio_set_direction(HAL_GPIO_5, HAL_GPIO_DIRECTION_OUTPUT), hal_gpio_set_output(HAL_GPIO_5, HAL_GPIO_DATA_HIGH);

        // UART2 RX\TX
        hal_gpio_init(HAL_GPIO_6),  hal_pinmux_set_function(HAL_GPIO_6, HAL_GPIO_6_GPIO6),      hal_gpio_set_direction(HAL_GPIO_6, HAL_GPIO_DIRECTION_INPUT),  hal_gpio_pull_up(HAL_GPIO_6);
        hal_gpio_init(HAL_GPIO_7),  hal_pinmux_set_function(HAL_GPIO_7, HAL_GPIO_7_GPIO7),      hal_gpio_set_direction(HAL_GPIO_7, HAL_GPIO_DIRECTION_OUTPUT), hal_gpio_set_output(HAL_GPIO_7, HAL_GPIO_DATA_HIGH);

#ifdef HW_VERSION_3_2_5
        // UART3 RX\TX
        hal_gpio_init(HAL_GPIO_2),  hal_pinmux_set_function(HAL_GPIO_2, HAL_GPIO_2_GPIO2),      hal_gpio_set_direction(HAL_GPIO_2, HAL_GPIO_DIRECTION_INPUT),  hal_gpio_pull_up(HAL_GPIO_2);
        hal_gpio_init(HAL_GPIO_3),  hal_pinmux_set_function(HAL_GPIO_3, HAL_GPIO_3_GPIO3),      hal_gpio_set_direction(HAL_GPIO_3, HAL_GPIO_DIRECTION_OUTPUT), hal_gpio_set_output(HAL_GPIO_3, HAL_GPIO_DATA_HIGH);
#else
        // UART3 RX\TX
        hal_gpio_init(HAL_GPIO_18), hal_pinmux_set_function(HAL_GPIO_18, HAL_GPIO_18_GPIO18),   hal_gpio_set_direction(HAL_GPIO_18, HAL_GPIO_DIRECTION_INPUT),  hal_gpio_pull_up(HAL_GPIO_18);
        hal_gpio_init(HAL_GPIO_22), hal_pinmux_set_function(HAL_GPIO_22, HAL_GPIO_22_GPIO22),   hal_gpio_set_direction(HAL_GPIO_22, HAL_GPIO_DIRECTION_OUTPUT), hal_gpio_set_output(HAL_GPIO_22, HAL_GPIO_DATA_HIGH);
#endif

        //UARTs are ready to sleep
        hal_sleep_manager_unlock_sleep(OctSleepHandleUart);*/
    }


//Put data to DMA buffer of single port. If there is not enough space then packet would be fully discarded
void OCT_UART_send(uint32_t line_id, const octPacket_t* pkt)
    {
        //Safety check
        if (!OctUartInitialized  ||  line_id >= NET_LINES_MAX) return;

        //Size of packet
        uint32_t data_size = PKT_SIZES[pkt->Header.Type];
        OctUarts[line_id].TxStatBandwidth.Counter += data_size;

        bk_uart_write_bytes(line_id, pkt, data_size); 

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



