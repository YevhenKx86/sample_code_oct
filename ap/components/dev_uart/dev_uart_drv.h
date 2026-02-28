#ifndef __DEV_UART_DRV_H_
#define __DEV_UART_DRV_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <components/system.h>
#include <os/os.h>
#include <driver/uart.h>


#define DEV_UART_DRV_RECV_BUFF_SIZE (64)

typedef enum
{
    DEV_UART_DRV_ID_0 = 0x00,
    DEV_UART_DRV_ID_1,
    DEV_UART_DRV_ID_2,
    DEV_UART_DRV_ID_MAX,
} dev_uart_drv_id_t;


typedef void (*dev_uart_drv_rx_func_t)(dev_uart_drv_id_t drv_id, const uint8_t *buff, uint32_t buff_len);

bk_err_t dev_uart_drv_init(dev_uart_drv_id_t drv_id, uart_id_t uart_id, uint32_t baud_rate, dev_uart_drv_rx_func_t rx_func);
bk_err_t dev_uart_drv_deinit(dev_uart_drv_id_t drv_id);
bk_err_t dev_uart_drv_write(dev_uart_drv_id_t drv_id, const uint8_t *buff, uint32_t buff_len);


#ifdef __cplusplus
}
#endif

#endif /*__DEV_UART_DRV_H_*/
