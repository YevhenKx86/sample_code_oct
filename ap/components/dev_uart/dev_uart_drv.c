#include "dev_uart_drv.h"
#include <os/mem.h>
#include <os/str.h>


#define TAG "dev_udrv"
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)


typedef struct
{
    dev_uart_drv_id_t drv_id;
    uart_id_t uart_id;

    int32_t recv_len;
    uint32_t recv_buff_size;
    uint8_t *recv_buff;

    dev_uart_drv_rx_func_t rx_func;

    uint32_t task_running;
    beken_semaphore_t task_sem;
    beken_thread_t task_handle;
    beken_thread_function_t task_func;
} dev_uart_drv_info_t;


static dev_uart_drv_info_t g_dev_uart_drv_info[DEV_UART_DRV_ID_MAX] = {0};
static const char *g_dev_uart_drv_task_name[] = {"dev_udrv_0", "dev_udrv_1", "dev_udrv_2"};


static bk_err_t dev_uart_drv_open(uart_id_t uart_id, uint32_t baud_rate)
{
    bk_err_t ret = BK_OK;

    uart_config_t config =
    {
        .baud_rate = UART_BAUDRATE_115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_NONE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_FLOWCTRL_DISABLE,
        .src_clk   = UART_SCLK_XTAL_26M,
        .rx_dma_en = 0,
        .tx_dma_en = 0,
    };

    config.baud_rate = baud_rate;
    config.rx_dma_en = true;
    config.tx_dma_en = true;
    ret = bk_uart_init(uart_id, &config);
    if (ret)
    {
        LOGE("%s, %d, bk_uart_init fail[%d]!\n", __func__, __LINE__, ret);
        return ret;
    }
    BK_LOG_ON_ERR(bk_uart_enable_sw_fifo(uart_id));
    BK_LOG_ON_ERR(bk_uart_enable_rx_interrupt(uart_id));

    LOGI("%s, %d, ok!\n", __func__, __LINE__);

    return ret;
}

static bk_err_t dev_uart_drv_close(uart_id_t uart_id)
{
    bk_err_t ret = BK_OK;

    BK_LOG_ON_ERR(bk_uart_disable_rx_interrupt(uart_id));

    ret = bk_uart_deinit(uart_id);
    if (ret)
    {
        LOGE("%s, %d, bk_uart_deinit fail[%d]!\n", __func__, __LINE__, ret);
        return ret;
    }

    LOGI("%s, %d, ok!\n", __func__, __LINE__);

    return ret;
}

static void dev_uart_drv_task_entry(beken_thread_arg_t data)
{
    dev_uart_drv_info_t *drv_info = (dev_uart_drv_info_t *)(data);
    uart_id_t uart_id = drv_info->uart_id;

    drv_info->task_running = true;
    if (drv_info->task_sem)
    {
        rtos_set_semaphore(&drv_info->task_sem);
    }

    while (drv_info->task_running)
    {
        drv_info->recv_len = bk_uart_read_bytes(uart_id, drv_info->recv_buff, drv_info->recv_buff_size, BEKEN_NEVER_TIMEOUT);
        LOGV("%s, %d, bk_uart_read_bytes[%d, %d]!\n", __func__, __LINE__, uart_id, drv_info->recv_len);

        if (drv_info->recv_len > 0)
        {
            if (drv_info->rx_func)
            {
                drv_info->rx_func(uart_id, (const uint8_t *)(drv_info->recv_buff), drv_info->recv_len);
            }
        }
    }

    drv_info->task_handle = NULL;
    rtos_delete_thread(NULL);
}

bk_err_t dev_uart_drv_init(dev_uart_drv_id_t drv_id, uart_id_t uart_id, uint32_t baud_rate, dev_uart_drv_rx_func_t rx_func)
{
    bk_err_t ret = BK_OK;
    bk_err_t ret_val = BK_OK;
    dev_uart_drv_info_t *drv_info = NULL;
    uint8_t *temp_buff = NULL;
    const char *task_name = NULL;

    if (drv_id >= DEV_UART_DRV_ID_MAX)
    {
        LOGE("%s, %d, drv id is out of range[%d]!\n", __func__, __LINE__, drv_id);
        return BK_FAIL;
    }

    if (uart_id >= UART_ID_MAX)
    {
        LOGE("%s, %d, uart id is out of range[%d]!\n", __func__, __LINE__, uart_id);
        return BK_FAIL;
    }

    drv_info = &g_dev_uart_drv_info[drv_id];
    os_memset(drv_info, 0x00, sizeof(dev_uart_drv_info_t));

    ret = dev_uart_drv_open(uart_id, baud_rate);
    if (ret)
    {
        LOGE("%s, %d, dev_uart_drv_open fail[%d]!\n", __func__, __LINE__, ret);
        return ret;
    }

    drv_info->drv_id = drv_id;
    drv_info->uart_id = uart_id;
    temp_buff = (uint8_t *)os_malloc(DEV_UART_DRV_RECV_BUFF_SIZE);
    if (temp_buff == NULL)
    {
        LOGE("%s, %d, os_malloc fail[%d]!\n", __func__, __LINE__);
        ret = BK_FAIL;
        goto __error;
    }
    drv_info->recv_buff_size = DEV_UART_DRV_RECV_BUFF_SIZE;
    drv_info->recv_buff = temp_buff;

    if (rx_func != NULL)
    {
        drv_info->rx_func = rx_func;
    }

    ret = rtos_init_semaphore(&drv_info->task_sem, 1);
    if (ret)
    {
        LOGE("%s, %d, rtos_init_semaphore fail[%d]!\n", __func__, __LINE__, ret);
        goto __error;
    }
    task_name = g_dev_uart_drv_task_name[drv_id];
    drv_info->task_func = dev_uart_drv_task_entry;
    ret = rtos_create_thread(&drv_info->task_handle,
                            BEKEN_DEFAULT_WORKER_PRIORITY,
                            task_name,
                            drv_info->task_func,
                            4*1024,
                            (beken_thread_arg_t)(drv_info));
    if (ret)
    {
        LOGE("%s, %d, rtos_create_thread fail[%d]!\n", __func__, __LINE__, ret);
        goto __error;
    }

    ret = rtos_get_semaphore(&drv_info->task_sem, BEKEN_NEVER_TIMEOUT);
    if (ret)
    {
        LOGE("%s, %d, rtos_get_semaphore fail[%d]!\n", __func__, __LINE__, ret);
        goto __error;
    }

    LOGI("%s, %d, ok!\n", __func__, __LINE__);

    return ret;

__error:
    ret_val = dev_uart_drv_close(uart_id);
    if (ret_val)
    {
        LOGE("%s, %d, dev_uart_drv_close fail[%d]!\n", __func__, __LINE__, ret_val);
    }

    if (drv_info->recv_buff)
    {
        os_free(drv_info->recv_buff);
        drv_info->recv_buff = NULL;
    }

    if (drv_info->task_sem)
    {
        ret_val = rtos_deinit_semaphore(&drv_info->task_sem);
        if (ret_val)
        {
            LOGE("%s, %d, rtos_deinit_semaphore fail[%d]!\n", __func__, __LINE__, ret_val);
        }
        drv_info->task_sem = NULL;
    }

    if (drv_info->task_handle)
    {
        rtos_delete_thread(&drv_info->task_handle);
        drv_info->task_handle = NULL;
    }

    LOGE("%s, %d, fail!\n", __func__, __LINE__);

    return ret;
}

bk_err_t dev_uart_drv_deinit(dev_uart_drv_id_t drv_id)
{
    bk_err_t ret = BK_OK;
    dev_uart_drv_info_t *drv_info = NULL;
    uart_id_t uart_id;

    if (drv_id >= DEV_UART_DRV_ID_MAX)
    {
        LOGE("%s, %d, drv id is out of range[%d]!\n", __func__, __LINE__, drv_id);
        return BK_FAIL;
    }

    drv_info = &g_dev_uart_drv_info[drv_id];
    uart_id = drv_info->uart_id;

    drv_info->task_running = false;

    ret = dev_uart_drv_close(uart_id);
    if (ret)
    {
        LOGE("%s, %d, dev_uart_drv_close fail[%d]!\n", __func__, __LINE__, ret);
    }

    if (drv_info->recv_buff)
    {
        os_free(drv_info->recv_buff);
        drv_info->recv_buff = NULL;
    }

    if (drv_info->task_sem)
    {
        ret = rtos_deinit_semaphore(&drv_info->task_sem);
        if (ret)
        {
            LOGE("%s, %d, rtos_deinit_semaphore fail[%d]!\n", __func__, __LINE__, ret);
        }
    }

    if (drv_info->task_handle)
    {
        rtos_delete_thread(&drv_info->task_handle);
        drv_info->task_handle = NULL;
    }

    LOGI("%s, %d, ok!\n", __func__, __LINE__);

    return ret;
}

bk_err_t dev_uart_drv_write(dev_uart_drv_id_t drv_id, const uint8_t *buff, uint32_t buff_len)
{
    bk_err_t ret = BK_OK;
    dev_uart_drv_info_t *drv_info = NULL;
    uart_id_t uart_id;

    if (drv_id >= DEV_UART_DRV_ID_MAX)
    {
        LOGE("%s, %d, drv id is out of range[%d]!\n", __func__, __LINE__, drv_id);
        return BK_FAIL;
    }

    if ( (buff == NULL) || (buff_len == 0) )
    {
        LOGE("%s, %d, paras is invalid[0x%0X, %d]!\n", __func__, __LINE__, buff, buff_len);
        return BK_FAIL;
    }

    drv_info = &g_dev_uart_drv_info[drv_id];
    uart_id = drv_info->uart_id;

    if (drv_info->task_running == false)
    {
        LOGE("%s, %d, uart not start-up!\n", __func__, __LINE__);
        return BK_FAIL;
    }

    ret = bk_uart_write_bytes(uart_id, buff, buff_len);
    if (ret)
    {
        LOGE("%s, %d, bk_uart_write_bytes fail[%d]!\n", __func__, __LINE__, ret);
    }

    return ret;
}
