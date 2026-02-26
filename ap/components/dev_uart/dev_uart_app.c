#include "dev_uart_app.h"
#include <os/mem.h>
#include "dev_info.h"
#if (CONFIG_IMAGE_PLAY_EN)
#include "img_play.h"
#endif


#define TAG "dev_uapp"
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)


typedef struct
{
    int32_t len;
    uint32_t buff_size;
    uint8_t *buff;
} dev_uart_app_data_unit_t;


static const uart_id_t g_dev_uart_app_id[DEV_UART_APP_USE_NUM] = {UART_ID_0, UART_ID_1, UART_ID_2};
static beken_mutex_t g_dev_uart_app_lock = NULL;
static dev_uart_app_data_unit_t g_dev_uart_app_tx[DEV_UART_APP_USE_NUM] = {0};
static dev_uart_app_data_unit_t g_dev_uart_app_rx[DEV_UART_APP_USE_NUM] = {0};


static bk_err_t dev_uart_app_lock(void)
{
    bk_err_t ret = BK_FAIL;
    
    if (g_dev_uart_app_lock != NULL)
    {
        ret = rtos_lock_mutex(&g_dev_uart_app_lock);
        if (ret)
        {
            LOGE("[%s] rtos_lock_mutex fail[%d]!\n", __func__, ret);
            return ret;
        }
    }

    return ret;
}

static bk_err_t dev_uart_app_unlock(void)
{
    bk_err_t ret = BK_FAIL;
    
    if (g_dev_uart_app_lock != NULL)
    {
        ret = rtos_unlock_mutex(&g_dev_uart_app_lock);
        if (ret)
        {
            LOGE("[%s] rtos_unlock_mutex fail[%d]!\n", __func__, ret);
            return ret;
        }
    }

    return ret;
}

bk_err_t dev_uart_app_tx_cmd(dev_uart_drv_id_t drv_id, uint8_t dst_addr, const uint8_t *data, uint32_t data_len)
{
    if (data == NULL)
    {
        LOGE("[%s] pointer to data is null!\n", __func__);
        return BK_FAIL;
    }

    if (data_len == 0)
    {
        LOGE("[%s] data_len is zero!\n", __func__);
        return BK_FAIL;
    }

    bk_err_t ret = BK_OK;
    uint8_t *tx_buff = g_dev_uart_app_tx[drv_id].buff;
    uint32_t tx_buff_size = g_dev_uart_app_tx[drv_id].buff_size;
    int32_t tx_len = 0;

    dev_uart_app_lock();
    dev_uart_prot_set_src_addr((uint8_t)dev_info_get_sn());
    dev_uart_prot_set_dst_addr(dst_addr);
    dev_uart_prot_set_relay_addr(0);

    tx_len = dev_uart_prot_frame_pack(DEV_UART_FRAME_TYPE_CMD, tx_buff, tx_buff_size, data, data_len);
    if (!tx_len)
    {
        LOGE("[%s] dev_uart_prot_frame_pack fail[%d]!\n", __func__, tx_len);
        dev_uart_app_unlock();
        return BK_FAIL;
    }

    ret = dev_uart_drv_write(drv_id, tx_buff, tx_len);
    if (ret)
    {
        LOGE("[%s] dev_uart_drv_write fail[%d]!\n", __func__, ret);
        dev_uart_app_unlock();
        return BK_FAIL;
    }

    g_dev_uart_app_tx[drv_id].len = tx_len;
    dev_uart_app_unlock();

    return ret;
}

bk_err_t dev_uart_app_cmd_sync_player(dev_uart_drv_id_t drv_id, uint8_t dst_addr, uint16_t time)
{
    bk_err_t ret = BK_OK;
    dev_uart_cmd_sync_player_t tx_cmd = {0};

    tx_cmd.cmd = DEV_UART_CMD_TYPE_SYNC_PLAYER;
    tx_cmd.time = time;

    ret = dev_uart_app_tx_cmd(drv_id, dst_addr, (const uint8_t *)(&tx_cmd), sizeof(dev_uart_cmd_sync_player_t));
    if (ret)
    {
        LOGE("[%s] dev_uart_app_tx_cmd fail[%d]!\n", __func__, ret);
        return ret;
    }

    return ret;
}

bk_err_t dev_uart_app_relay_frame(dev_uart_drv_id_t drv_id, const uint8_t *frame, uint32_t frame_len)
{
    if (frame == NULL)
    {
        LOGE("[%s] pointer to frame is null!\n", __func__);
        return BK_FAIL;
    }

    if (frame_len == 0)
    {
        LOGE("[%s] frame_len is zero!\n", __func__);
        return BK_FAIL;
    }

    bk_err_t ret = BK_OK;
    uint8_t *tx_buff = g_dev_uart_app_tx[drv_id].buff;
    uint32_t tx_buff_size = g_dev_uart_app_tx[drv_id].buff_size;
    int32_t tx_len = 0;

    if (frame_len > tx_buff_size)
    {
        LOGE("[%s] frame_len is too large[%d, %d]!\n", __func__, frame_len, tx_buff_size);
        return BK_FAIL;
    }

    dev_uart_app_lock();
    os_memcpy(tx_buff, frame, frame_len);
    tx_len = frame_len;

    dev_uart_prot_set_relay_addr((uint8_t)dev_info_get_sn());

    tx_len = dev_uart_prot_frame_relay_update(tx_buff, tx_len);
    if (frame_len != tx_len)
    {
        LOGE("[%s] dev_uart_prot_frame_relay_update fail[%d, %d]!\n", __func__, frame_len, tx_len);
        dev_uart_app_unlock();
        return BK_FAIL;
    }

    ret = dev_uart_drv_write(drv_id, tx_buff, tx_len);
    if (ret)
    {
        LOGE("[%s] dev_uart_drv_write fail[%d]!\n", __func__, ret);
        dev_uart_app_unlock();
        return BK_FAIL;
    }

    g_dev_uart_app_tx[drv_id].len = tx_len;
    dev_uart_app_unlock();

    return ret;
}

static void dev_uart_app_rx_cmd_process(dev_uart_drv_id_t drv_id, const uint8_t *frame, uint32_t frame_len)
{
    if (frame == NULL)
    {
        LOGE("[%s] pointer to frame is null!\n", __func__);
        return;
    }

    if (frame_len == 0)
    {
        LOGE("[%s] frame_len is zero!\n", __func__);
        return;
    }

    bk_err_t ret = BK_OK;
    dev_uart_frame_t *cmd_frame = (dev_uart_frame_t *)(frame);
    uint32_t cmd_frame_len = frame_len;
    uint8_t cmd_fsn = cmd_frame->fsn;

    dev_uart_frame_t *temp_frame = NULL;
    uint32_t temp_frame_len;
    uint8_t temp_fsn;

    dev_uart_app_lock();
    // repeat packet process.
    for (uint8_t index=DEV_UART_DRV_ID_0; index<DEV_UART_APP_USE_NUM; ++index)
    {
        if (g_dev_uart_app_rx[index].len > 0)
        {
            if (index == drv_id)
            {
                continue;
            }

            temp_frame = (dev_uart_frame_t *)(g_dev_uart_app_rx[index].buff);
            temp_frame_len = g_dev_uart_app_rx[index].len;
            temp_fsn = temp_frame->fsn;

            if ( (cmd_frame_len == temp_frame_len) && (cmd_fsn == temp_fsn) )
            {
                LOGI("[%s] rx repeat: drv_id=%d, frame_len=%d, fsn=%d!\n", __func__, index, cmd_frame_len, cmd_fsn);
                dev_uart_app_unlock();
                return;
            }
        }

        if (g_dev_uart_app_tx[index].len > 0)
        {
            temp_frame = (dev_uart_frame_t *)(g_dev_uart_app_tx[index].buff);
            temp_frame_len = g_dev_uart_app_tx[index].len;
            temp_fsn = temp_frame->fsn;

            if ( (cmd_frame_len == temp_frame_len) && (cmd_fsn == temp_fsn) )
            {
                LOGI("[%s] tx repeat: drv_id=%d, frame_len=%d, fsn=%d!\n", __func__, index, cmd_frame_len, cmd_fsn);
                dev_uart_app_unlock();
                return;
            }
        }
    }

    // cmd process.
    uint8_t cmd = cmd_frame->payload[0];
    switch (cmd)
    {
        case DEV_UART_CMD_TYPE_SYNC_PLAYER:
        {
            // sync player.
            dev_uart_cmd_sync_player_t *cmd_info = (dev_uart_cmd_sync_player_t *)(cmd_frame->payload);
            LOGI("[%s] sync player cmd, cmd=0x%X, time=%d!\n", __func__, cmd_info->cmd, cmd_info->time);
            if (DEV_ROLE_SLAVE == dev_info_get_role())
            {
                #if (CONFIG_IMAGE_PLAY_EN)
                    img_play_send_msg(IMG_PLAY_MSG_SYNC, cmd_info->time);
                #endif
            }
        }
        break;

        default:
        {
            LOGI("[%s] not support this cmd[0x%X]!\n", __func__, cmd);
        }
        break;
    }
    dev_uart_app_unlock();

    // relay cmd.
    for (uint8_t index=DEV_UART_DRV_ID_0; index<DEV_UART_APP_USE_NUM; ++index)
    {
        if (index == drv_id)
        {
            continue;
        }

        ret = dev_uart_app_relay_frame(index, frame, frame_len);
        if (ret)
        {
            LOGI("[%s] dev_uart_app_relay_frame fail[%d]!\n", __func__, ret);
        }
    }
}

static void dev_uart_app_rx_func(dev_uart_drv_id_t drv_id, const uint8_t *data, uint32_t data_len)
{
    if (data == NULL)
    {
        LOGE("[%s] pointer to data is null!\n", __func__);
        return;
    }

    if (data_len == 0)
    {
        LOGE("[%s] data_len is zero!\n", __func__);
        return;
    }

    LOGD("[%s] origin rx: drv_id=%d, data_len=%d!\n", __func__, drv_id, data_len);

    dev_uart_app_lock();
    g_dev_uart_app_rx[drv_id].len = dev_uart_prot_frame_parse(g_dev_uart_app_rx[drv_id].buff, g_dev_uart_app_rx[drv_id].buff_size, data, data_len);
    dev_uart_app_unlock();
    if (g_dev_uart_app_rx[drv_id].len > 0)
    {
        dev_uart_frame_t *frame = (dev_uart_frame_t *)(g_dev_uart_app_rx[drv_id].buff);

        if (frame->type == DEV_UART_FRAME_TYPE_CMD)
        {
            dev_uart_app_rx_cmd_process(drv_id, (const uint8_t *)(g_dev_uart_app_rx[drv_id].buff), g_dev_uart_app_rx[drv_id].len);
        }
        else
        {
            LOGE("[%s] current not support this frame type[0x%0X]!\n", __func__, frame->type);
            return;
        }
    }
}

bk_err_t dev_uart_app_init(void)
{
    bk_err_t ret = BK_OK;
    bk_err_t ret_val = BK_OK;
    dev_uart_drv_id_t drv_id = DEV_UART_DRV_ID_0;
    uart_id_t uart_id;

    if (g_dev_uart_app_lock != NULL)
    {
        LOGW("[%s] already init!\n", __func__);
        return ret;
    }

    ret = rtos_init_mutex(&g_dev_uart_app_lock);
    if (ret)
    {
        LOGE("%s, %d, rtos_init_mutex fail[%d]!\n", __func__, __LINE__, ret);
        return ret;
    }

    for (uint8_t index=0; index<DEV_UART_APP_USE_NUM; ++index)
    {
        g_dev_uart_app_tx[index].len = 0;
        g_dev_uart_app_tx[index].buff = (uint8_t *)os_malloc(DEV_UART_DRV_RECV_BUFF_SIZE);
        if (g_dev_uart_app_tx[index].buff == NULL)
        {
            ret = BK_FAIL;
            goto __exit;
        }
        g_dev_uart_app_tx[index].buff_size = DEV_UART_DRV_RECV_BUFF_SIZE;

        g_dev_uart_app_rx[index].len = 0;
        g_dev_uart_app_rx[index].buff = (uint8_t *)os_malloc(DEV_UART_DRV_RECV_BUFF_SIZE);
        if (g_dev_uart_app_rx[index].buff == NULL)
        {
            ret = BK_FAIL;
            goto __exit;
        }
        g_dev_uart_app_rx[index].buff_size = DEV_UART_DRV_RECV_BUFF_SIZE;
    }

    for (uint8_t index=0; index<(sizeof(g_dev_uart_app_id)/sizeof(uart_id_t)); ++index, ++drv_id)
    {
        uart_id = g_dev_uart_app_id[index];
        ret = dev_uart_drv_init(drv_id, uart_id, DEV_UART_APP_BAUDRATE, dev_uart_app_rx_func);
        if (ret)
        {
            LOGE("%s, %d, dev_uart_drv_init fail[drv_id=%d, uart_id=%d, ret=%d]!\n", __func__, __LINE__, drv_id, uart_id, ret);
            goto __exit;
        }
        else
        {
            LOGI("%s, %d, dev_uart_drv_init ok[drv_id=%d, uart_id=%d]!\n", __func__, __LINE__, drv_id, uart_id);
        }
    }

    LOGI("%s, %d, ok!\n", __func__, __LINE__);

    return ret;

__exit:
    if (g_dev_uart_app_lock != NULL)
    {
        ret_val = rtos_deinit_mutex(&g_dev_uart_app_lock);
        if (ret_val)
        {
            LOGE("%s, %d, rtos_deinit_mutex fail[%d]!\n", __func__, __LINE__, ret_val);
        }
        g_dev_uart_app_lock = NULL;
    }

    for (uint8_t index=0; index<DEV_UART_APP_USE_NUM; ++index)
    {
        if (g_dev_uart_app_tx[index].buff != NULL)
        {
            os_free(g_dev_uart_app_tx[index].buff);
            g_dev_uart_app_tx[index].buff = NULL;
            g_dev_uart_app_tx[index].buff_size = 0;
        }

        if (g_dev_uart_app_rx[index].buff != NULL)
        {
            os_free(g_dev_uart_app_rx[index].buff);
            g_dev_uart_app_rx[index].buff = NULL;
            g_dev_uart_app_rx[index].buff_size = 0;
        }
    }

    LOGI("%s, %d, fail!\n", __func__, __LINE__);

    return ret;
}
