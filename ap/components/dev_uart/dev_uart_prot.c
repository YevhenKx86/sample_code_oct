#include "dev_uart_prot.h"
#include <os/mem.h>


#define TAG "dev_uprot"
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)


static uint8_t g_dev_uart_prot_fsn = 1;
static uint8_t g_dev_uart_prot_src_addr = 0;
static uint8_t g_dev_uart_prot_dst_addr = 0;
static uint8_t g_dev_uart_prot_relay_addr = 0;


static uint8_t dev_uart_prot_get_fsn(void)
{
    uint8_t fsn = g_dev_uart_prot_fsn;

    g_dev_uart_prot_fsn += 2;

    return fsn;
}

void dev_uart_prot_set_src_addr(uint8_t addr)
{
    g_dev_uart_prot_src_addr = addr;
}

void dev_uart_prot_set_dst_addr(uint8_t addr)
{
    g_dev_uart_prot_dst_addr = addr;
}

void dev_uart_prot_set_relay_addr(uint8_t addr)
{
    g_dev_uart_prot_relay_addr = addr;
}

static uint16_t dev_uart_prot_caculate_checksum(const uint8_t *data, uint32_t data_len)
{
    uint16_t checksum = 0;

    if ( (data == NULL) || (data_len == 0) )
    {
        LOGE("[%s] data paras is invalid[0x%0X, %d]!\n", __func__, data, data_len);
        return 0;
    }

    for (int32_t index=0; index<data_len; ++index)
    {
        checksum += data[index];
    }

    return checksum;
}

int32_t dev_uart_prot_frame_pack(dev_uart_frame_type_t type, uint8_t *buff, uint32_t buff_len, const uint8_t *data, uint32_t data_len)
{
    dev_uart_frame_t *pack = NULL;
    int32_t pack_len = 0;
    int32_t offset = 0;
    uint16_t check;
    uint16_t end = DEV_UART_FRAME_END;

    if ( (buff == NULL) || (buff_len == 0) )
    {
        LOGE("[%s] buff paras is invalid[0x%0X, %d]!\n", __func__, buff, buff_len);
        return 0;
    }

    if ( (data == NULL) || (data_len == 0) )
    {
        LOGE("[%s] data paras is invalid[0x%0X, %d]!\n", __func__, data, data_len);
        return 0;
    }

    if ( (data_len + DEV_UART_FRAME_MIN_LEN) > buff_len)
    {
        LOGE("[%s] the sum of %d and %d is larger than %d!\n", __func__, data_len, DEV_UART_FRAME_MIN_LEN, buff_len);
        return 0;
    }

    pack = (dev_uart_frame_t *)buff;

    // start.
    pack->start = DEV_UART_FRAME_START;
    // info.
    pack->type = type;
    pack->fsn = dev_uart_prot_get_fsn();
    pack->src_addr = g_dev_uart_prot_src_addr;
    pack->dst_addr = g_dev_uart_prot_dst_addr;
    pack->relay_addr = g_dev_uart_prot_relay_addr;
    pack->len = data_len;
    pack_len = DEV_UART_FRAME_FRONT_END_LEN;
    // paylaod.
    os_memcpy(pack->payload, data, data_len);
    pack_len += data_len;
    offset += data_len;
    // check.
    check = dev_uart_prot_caculate_checksum(&pack->type, pack_len - sizeof(pack->start));
    os_memcpy(&pack->payload[offset], &check, sizeof(check));
    pack_len += sizeof(check);
    offset += sizeof(check);
    // end.
    os_memcpy(&pack->payload[offset], &end, sizeof(end));
    pack_len += sizeof(end);
    offset += sizeof(end);

    return pack_len;
}

int32_t dev_uart_prot_frame_relay_update(uint8_t *frame, uint32_t frame_len)
{
    dev_uart_frame_t *pack = NULL;
    int32_t pack_len = 0;
    int32_t offset = 0;
    uint16_t check;

    if ( (frame == NULL) || (frame_len == 0) )
    {
        LOGE("[%s] frame paras is invalid[0x%0X, %d]!\n", __func__, frame, frame_len);
        return 0;
    }

    if ( frame_len < DEV_UART_FRAME_MIN_LEN )
    {
        LOGE("[%s] frame len is not match[%d]!\n", __func__, frame_len);
        return 0;
    }

    pack = (dev_uart_frame_t *)frame;
    // relay addr.
    pack->relay_addr = g_dev_uart_prot_relay_addr;
    // check.
    pack_len = DEV_UART_FRAME_FRONT_END_LEN + pack->len;
    offset = pack->len;
    check = dev_uart_prot_caculate_checksum(&pack->type, pack_len - sizeof(pack->start));
    os_memcpy(&pack->payload[offset], &check, sizeof(check));

    return frame_len;
}

int32_t dev_uart_prot_frame_parse(uint8_t *buff, uint32_t buff_len, const uint8_t *data, uint32_t data_len)
{
    dev_uart_frame_t *pack = NULL;
    int32_t pack_len = 0;
    int32_t offset = 0;
    uint16_t check;
    uint16_t caculate_check;
    uint16_t end;

    if ( (buff == NULL) || (buff_len == 0) )
    {
        LOGE("[%s] buff paras is invalid[0x%0X, %d]!\n", __func__, buff, buff_len);
        return 0;
    }

    if ( (data == NULL) || (data_len == 0) )
    {
        LOGE("[%s] data paras is invalid[0x%0X, %d]!\n", __func__, data, data_len);
        return 0;
    }

    if ( data_len < DEV_UART_FRAME_MIN_LEN )
    {
        LOGE("[%s] data_len[%d] is smaller than FRAME_MIN_LEN[%d]!\n", __func__, data_len, DEV_UART_FRAME_MIN_LEN);
        return 0;
    }

    while ( (offset + DEV_UART_FRAME_MIN_LEN) <= data_len )
    {
        pack = (dev_uart_frame_t *)(data + offset);

        LOGV("[%s] start=0x%0X, len=%d!\n", __func__, pack->start, pack->len);
        // start.
        if (pack->start != DEV_UART_FRAME_START)
        {
            offset++;
            continue;
        }

        // len.
        if (pack->len == 0)
        {
            offset++;
            continue;
        }

        // check.
        os_memcpy(&check, &pack->payload[pack->len], sizeof(check));

        pack_len = DEV_UART_FRAME_FRONT_END_LEN + pack->len;
        caculate_check = dev_uart_prot_caculate_checksum(&pack->type, pack_len - sizeof(pack->start));
        LOGV("[%s] check=0x%0X, caculate_check=0x%0X!\n", __func__, check, caculate_check);
        if (check != caculate_check)
        {
            offset++;
            continue;
        }
        pack_len += sizeof(check);

        // end.
        os_memcpy(&end, &pack->payload[pack->len + sizeof(check)], sizeof(end));
        LOGV("[%s] end=0x%0X!\n", __func__, end);
        if (end != DEV_UART_FRAME_END)
        {
            offset++;
            continue;
        }
        pack_len += sizeof(end);

        if (pack_len <= buff_len)
        {
            os_memcpy(buff, pack, pack_len);
            break;
        }
        else
        {
            LOGE("[%s] pack_len[%d] is larger than buff_len[%d]!\n", __func__, pack_len, buff_len);
            return 0;
        }
    }

    LOGV("[%s] offset=%d, pack_len=%d!\n", __func__, offset, pack_len);

    return pack_len;
}
