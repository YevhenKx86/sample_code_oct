#ifndef __DEV_UART_APP_H_
#define __DEV_UART_APP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <components/system.h>
#include <os/os.h>
#include "dev_uart_drv.h"
#include "dev_uart_prot.h"


#define DEV_UART_APP_USE_NUM  (3)
#define DEV_UART_APP_BAUDRATE (460800) // unit: bits/s


bk_err_t dev_uart_app_cmd_sync_player(dev_uart_drv_id_t drv_id, uint8_t dst_addr, uint16_t time);
bk_err_t dev_uart_app_init(void);


#ifdef __cplusplus
}
#endif

#endif /*__DEV_UART_APP_H_*/
