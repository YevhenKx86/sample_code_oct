#ifndef __DEV_UART_PROT_H_
#define __DEV_UART_PROT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <components/system.h>
#include <os/os.h>


#define DEV_UART_FRAME_START (0xA5B4)
#define DEV_UART_FRAME_END   (0xC3D2)

#define DEV_UART_FRAME_BROADCAST_ADDR (0xFF)

#define DEV_UART_FRAME_MIN_LEN       (sizeof(dev_uart_frame_t))
#define DEV_UART_FRAME_FRONT_END_LEN ((uint32_t)(((dev_uart_frame_t *)(0))->payload))


typedef enum
{
    DEV_UART_FRAME_TYPE_CMD  = 0x0C,
    DEV_UART_FRAME_TYPE_DATA = 0x0D,
} dev_uart_frame_type_t;

typedef enum
{
    DEV_UART_CMD_TYPE_SYNC_PLAYER = 0x10,
} dev_uart_cmd_type_t;

typedef struct
{
    uint8_t cmd;
    uint16_t time; // uinit: ms, palyer after the time.
} __attribute__((packed)) dev_uart_cmd_sync_player_t;

typedef struct
{
    uint16_t start;                               // start of frame.
    uint8_t type;                                 // frame type.
    uint8_t fsn;                                  // frame serial number.
    uint8_t src_addr;                             // source address.
    uint8_t dst_addr;                             // destination address.
    uint8_t relay_addr;                           // relay address.
    uint8_t len;                                  // frame length.
    uint8_t payload[1];                           // frame payload.
    uint16_t check;                               // frame checksum, which including from type to payload fields.
    uint16_t end;                                 // end of frame.
} __attribute__((packed)) dev_uart_frame_t;


void dev_uart_prot_set_src_addr(uint8_t addr);
void dev_uart_prot_set_dst_addr(uint8_t addr);
void dev_uart_prot_set_relay_addr(uint8_t addr);
int32_t dev_uart_prot_frame_pack(dev_uart_frame_type_t type, uint8_t *buff, uint32_t buff_len, const uint8_t *data, uint32_t data_len);
int32_t dev_uart_prot_frame_relay_update(uint8_t *frame, uint32_t frame_len);
int32_t dev_uart_prot_frame_parse(uint8_t *buff, uint32_t buff_len, const uint8_t *data, uint32_t data_len);


#ifdef __cplusplus
}
#endif

#endif /*__DEV_UART_PROT_H_*/
