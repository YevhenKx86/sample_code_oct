#pragma once
#include <stdint.h>
#include <string.h> // memset

#ifndef OCTSIM
  #include "FreeRTOS.h"
  #include "task.h"
  #include "semphr.h"
#endif

#include "oct_vars.h"
#include "oct_uart.h"


//Task to parse bytes copied out from DMA RX-buffer, verify packets and route them depending on type
void OCT_UART_task(void* arg)
    {
        //Task will run each 2 ms (but less often in simulator)
        const TickType_t TASK_PARSE_TRAFFIC_SLEEP_TIME_MS = pdMS_TO_TICKS(2);
        TickType_t last_wake = xTaskGetTickCount();

        do
        {
            //Process each uart's buffer
            for (uint32_t line_id = 0;  line_id < NET_LINES_MAX;  line_id++)
            {
                OCT_UART_dma_callback2(HAL_UART_EVENT_READY_TO_READ, (void*)(uintptr_t)(line_id+1));

                OCT_per_second(OCT_time(), (octStatPerSec_t*)&OctUarts[line_id].RxStatPackets);
                OCT_per_second(OCT_time(), (octStatPerSec_t*)&OctUarts[line_id].RxStatBandwidth);
                OCT_per_second(OCT_time(), (octStatPerSec_t*)&OctUarts[line_id].TxStatBandwidth);
                OCT_NET_extract_packets(false, line_id, &OctUarts[line_id]);
            }
                    

            vTaskDelayUntil(&last_wake, TASK_PARSE_TRAFFIC_SLEEP_TIME_MS);            
        }
        while (arg == NULL);
    }

