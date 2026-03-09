#pragma once
#include <stdint.h>
#include <string.h> // memset

#ifndef OCTSIM
  #include "FreeRTOS.h"
  #include "task.h"
  #include "semphr.h"
#endif

#include "oct_vars.h"
#include "oct_net.h"



//Task to parse bytes copied out from DMA RX-buffer, verify packets and route them depending on type
void OCT_UART_task(void* arg){

    do
    {
        //Process each uart's buffer
        for (uint32_t line_id = 0;  line_id < NET_LINES_MAX;  line_id++)
        {
            //OCT_UART_dma_callback2(HAL_UART_EVENT_READY_TO_READ, (void*)(uintptr_t)(line_id+1));
            OCT_per_second(OCT_time(), (octStatPerSec_t*)&OctUarts[line_id].RxStatPackets);
            OCT_per_second(OCT_time(), (octStatPerSec_t*)&OctUarts[line_id].RxStatBandwidth);
            OCT_per_second(OCT_time(), (octStatPerSec_t*)&OctUarts[line_id].TxStatBandwidth);
            OCT_NET_extract_packets(false, line_id, &OctUarts[line_id]);
        }                
    
        OCT_text(7, "Time %d s", rtos_get_time()/1000);    

        rtos_delay_milliseconds(2); 
    }
    while(arg == NULL);

}

