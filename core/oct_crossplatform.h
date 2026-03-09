#pragma once
///#include "aDefines.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define SD_BLOCK_LENGTH   (512)

#define DISPLAY_WIDTH 240
#define DISPLAY_HEIGHT 240
#define DISPLAY_COUNT 3
#define NET_LINES_MAX 3

// #define HAL_GPT_CLOCK_SOURCE_32K 32

#define ATTR_RODATA_IN_TCM
#define ATTR_RWDATA_IN_TCM
#define ATTR_ZIDATA_IN_NONCACHED_RAM_4BYTE_ALIGN
#define ATTR_RWDATA_IN_PSRAM_4BYTE_ALIGN __attribute__ ((__section__(".psram.data"),__aligned__(4)))
#define ATTR_RWDATA_IN_RAM __attribute__((__aligned__(4)))

//#define NET_PKT_SIZE_MIN 32
///#define NET_PKT_SYS_SIZE_MAX  128

// Flash address of FW for update
//#define FLASH_UPDATE_FW_START_ADDRESS 0x400000
//#define FLASH_UPDATE_FW_SIZE 0x400000

// typedef void * TaskHandle_t;

#pragma pack(push, 1)



typedef struct {
  uint16_t* address;
  uint32_t pitch;
  uint32_t width;
  uint32_t height;
} rgb565_buffer_t;

typedef struct ACCEL_Values {
  // TODO: rework structure to replicate BMI160
  int16_t accelerometerX;          // Accelerometer value X axis
  int16_t accelerometerY;          // Accelerometer value Y axis
  int16_t accelerometerZ;          // Accelerometer value Z axis
  int16_t gyroscopeX;              // Gyroscope value X axis
  int16_t gyroscopeY;              // Gyroscope value Y axis
  int16_t gyroscopeZ;              // Gyroscope value Z axis
  int32_t temperature;             // Temperature in milli degrees
} ACCEL_Values_t;

typedef enum {
    FLASH_STATUS_INVALID_ADDRESS = -3,
    FLASH_STATUS_NOT_INIT = -2,
    FLASH_STATUS_ERROR = -1,
    FLASH_STATUS_OK
} FLASH_Status_t;

typedef enum {
    FLASH_SECTOR_4K = 0,    //erase sector 4kB
    FLASH_SECTOR_32K,       //erase block 32kB
    FLASH_SECTOR_64K,       //erase block 64kB
} block_size_type_t;

typedef enum {
    FLASH_SECTOR_4K_SIZE = 0x1000,            //erase sector 4kB
    FLASH_SECTOR_32K_SIZE = 0x8000,           //erase block 32kB
    FLASH_SECTOR_64K_SIZE = 0x10000,          //erase block 64kB

} block_size_t;

typedef enum {
    HAL_RTC_STATUS_ERROR = -2,                              /**< An error occurred. */
    HAL_RTC_STATUS_INVALID_PARAM = -1,                      /**< Invalid parameter is given. */
    HAL_RTC_STATUS_OK = 0                                   /**< The operation completed successfully. */
} hal_rtc_status_t;

typedef struct {
    uint8_t rtc_sec;                                  /**< Seconds after minutes   - [0,59]  */
    uint8_t rtc_min;                                  /**< Minutes after the hour  - [0,59]  */
    uint8_t rtc_hour;                                 /**< Hours after midnight    - [0,23]  */
    uint8_t rtc_day;                                  /**< Day of the month        - [1,31]  */
    uint8_t rtc_mon;                                  /**< Months                  - [1,12]  */
    uint8_t rtc_week;                                 /**< Days in a week          - [0,6]   */
    uint8_t rtc_year;                                 /**< Years                   - [0,127] */
} hal_rtc_time_t;

typedef enum {
    HAL_CHARGER_STATUS_INVALID_PARAMETER = -2,  /**< CHARGER invalid parameter */
    HAL_CHARGER_STATUS_ERROR = -1,              /**< CHARGER function ERROR */
    HAL_CHARGER_STATUS_OK = 0                   /**< CHARGER function OK */
} hal_charger_status_t;

typedef uint8_t cid_t;

#pragma pack(pop)


typedef enum {
    HAL_CHARGE_CURRENT_0_MA    = 0    , /**< define current as  0       mA */
    HAL_CHARGE_CURRENT_5_MA    = 5    , /**< define current as  5       mA */
    HAL_CHARGE_CURRENT_10_MA   = 10   , /**< define current as  10      mA */
    HAL_CHARGE_CURRENT_15_MA   = 15   , /**< define current as  15      mA */
    HAL_CHARGE_CURRENT_20_MA   = 20   , /**< define current as  20      mA */
    HAL_CHARGE_CURRENT_25_MA   = 25   , /**< define current as  25      mA */
    HAL_CHARGE_CURRENT_30_MA   = 30   , /**< define current as  30      mA */
    HAL_CHARGE_CURRENT_35_MA   = 35   , /**< define current as  35      mA */
    HAL_CHARGE_CURRENT_40_MA   = 40   , /**< define current as  40      mA */
    HAL_CHARGE_CURRENT_45_MA   = 45   , /**< define current as  45      mA */
    HAL_CHARGE_CURRENT_50_MA   = 50   , /**< define current as  50      mA */
    HAL_CHARGE_CURRENT_55_MA   = 55   , /**< define current as  55      mA */
    HAL_CHARGE_CURRENT_60_MA   = 60   , /**< define current as  60      mA */
    HAL_CHARGE_CURRENT_65_MA   = 65   , /**< define current as  65      mA */
    HAL_CHARGE_CURRENT_70_MA   = 70   , /**< define current as  70      mA */
    HAL_CHARGE_CURRENT_75_MA   = 75   , /**< define current as  75      mA */
    HAL_CHARGE_CURRENT_80_MA   = 80   , /**< define current as  80      mA */
    HAL_CHARGE_CURRENT_85_MA   = 85   , /**< define current as  85      mA */
    HAL_CHARGE_CURRENT_90_MA   = 90   , /**< define current as  90      mA */
    HAL_CHARGE_CURRENT_95_MA   = 95   , /**< define current as  95      mA */
    HAL_CHARGE_CURRENT_100_MA  = 100  , /**< define current as  100     mA */
    HAL_CHARGE_CURRENT_105_MA  = 105  , /**< define current as  105     mA */
    HAL_CHARGE_CURRENT_110_MA  = 110  , /**< define current as  110     mA */
    HAL_CHARGE_CURRENT_115_MA  = 115  , /**< define current as  115     mA */
    HAL_CHARGE_CURRENT_120_MA  = 120  , /**< define current as  120     mA */
    HAL_CHARGE_CURRENT_125_MA  = 125  , /**< define current as  125     mA */
    HAL_CHARGE_CURRENT_130_MA  = 130  , /**< define current as  130     mA */
    HAL_CHARGE_CURRENT_135_MA  = 135  , /**< define current as  135     mA */
    HAL_CHARGE_CURRENT_140_MA  = 140  , /**< define current as  140     mA */
    HAL_CHARGE_CURRENT_145_MA  = 145  , /**< define current as  145     mA */
    HAL_CHARGE_CURRENT_150_MA  = 150  , /**< define current as  150     mA */
    HAL_CHARGE_CURRENT_160_MA  = 160  , /**< define current as  160     mA */
    HAL_CHARGE_CURRENT_170_MA  = 170  , /**< define current as  170     mA */
    HAL_CHARGE_CURRENT_180_MA  = 180  , /**< define current as  180     mA */
    HAL_CHARGE_CURRENT_190_MA  = 190  , /**< define current as  190     mA */
    HAL_CHARGE_CURRENT_200_MA  = 200  , /**< define current as  200     mA */
    HAL_CHARGE_CURRENT_210_MA  = 210  , /**< define current as  210     mA */
    HAL_CHARGE_CURRENT_220_MA  = 220  , /**< define current as  220     mA */
    HAL_CHARGE_CURRENT_230_MA  = 230  , /**< define current as  230     mA */
    HAL_CHARGE_CURRENT_240_MA  = 240  , /**< define current as  240     mA */
    HAL_CHARGE_CURRENT_250_MA  = 250  , /**< define current as  250     mA */
    HAL_CHARGE_CURRENT_260_MA  = 260  , /**< define current as  260     mA */
    HAL_CHARGE_CURRENT_270_MA  = 270  , /**< define current as  270     mA */
    HAL_CHARGE_CURRENT_280_MA  = 280  , /**< define current as  280     mA */
    HAL_CHARGE_CURRENT_290_MA  = 290  , /**< define current as  290     mA */
    HAL_CHARGE_CURRENT_300_MA  = 300  , /**< define current as  300     mA */
    HAL_CHARGE_CURRENT_320_MA  = 320  , /**< define current as  320     mA */
    HAL_CHARGE_CURRENT_340_MA  = 340  , /**< define current as  340     mA */
    HAL_CHARGE_CURRENT_360_MA  = 360  , /**< define current as  360     mA */
    HAL_CHARGE_CURRENT_380_MA  = 380  , /**< define current as  380     mA */
    HAL_CHARGE_CURRENT_400_MA  = 400  , /**< define current as  400     mA */
    HAL_CHARGE_CURRENT_420_MA  = 420  , /**< define current as  420     mA */
    HAL_CHARGE_CURRENT_440_MA  = 440  , /**< define current as  440     mA */
    HAL_CHARGE_CURRENT_460_MA  = 460  , /**< define current as  460     mA */
    HAL_CHARGE_CURRENT_480_MA  = 480  , /**< define current as  480     mA */
    HAL_CHARGE_CURRENT_500_MA  = 500  , /**< define current as  500     mA */
    HAL_CHARGE_CURRENT_550_MA  = 550  , /**< define current as  550     mA */
    HAL_CHARGE_CURRENT_600_MA  = 600  , /**< define current as  600     mA */
    HAL_CHARGE_CURRENT_650_MA  = 650  , /**< define current as  650     mA */
    HAL_CHARGE_CURRENT_700_MA  = 700  , /**< define current as  700     mA */
    HAL_CHARGE_CURRENT_750_MA  = 750  , /**< define current as  750     mA */
    HAL_CHARGE_CURRENT_800_MA  = 800  , /**< define current as  800     mA */
    HAL_CHARGE_CURRENT_850_MA  = 850  , /**< define current as  850     mA */
    HAL_CHARGE_CURRENT_900_MA  = 900  , /**< define current as  900     mA */
    HAL_CHARGE_CURRENT_950_MA  = 950  , /**< define current as  950     mA */
    HAL_CHARGE_CURRENT_1000_MA = 1000 , /**< define current as  1000    mA */
    HAL_CHARGE_CURRENT_1100_MA = 1100 , /**< define current as  1100    mA */
    HAL_CHARGE_CURRENT_1200_MA = 1200 , /**< define current as  1200    mA */
    HAL_CHARGE_CURRENT_1300_MA = 1300 , /**< define current as  1300    mA */
    HAL_CHARGE_CURRENT_1400_MA = 1400 , /**< define current as  1400    mA */
    HAL_CHARGE_CURRENT_1500_MA = 1500 , /**< define current as  1500    mA */
    HAL_CHARGE_CURRENT_1600_MA = 1600 , /**< define current as  1600    mA */
    HAL_CHARGE_CURRENT_MAX
} HAL_CHARGE_CURRENT_ENUM;

typedef enum {
    HAL_BATTERY_VOLT_03_5000_V = 35000,         /**< define voltage as  3500 mV */
    HAL_BATTERY_VOLT_03_6000_V = 36000,         /**< define voltage as  3600 mV */
    HAL_BATTERY_VOLT_03_7000_V = 37000,         /**< define voltage as  3700 mV */
    HAL_BATTERY_VOLT_03_7750_V = 37750,         /**< define voltage as  3775 mV */
    HAL_BATTERY_VOLT_03_8000_V = 38000,         /**< define voltage as  3800 mV */
    HAL_BATTERY_VOLT_03_8500_V = 38500,         /**< define voltage as  3850 mV */
    HAL_BATTERY_VOLT_03_9000_V = 39000,         /**< define voltage as  3900 mV */
    HAL_BATTERY_VOLT_04_0000_V = 40000,         /**< define voltage as  4000 mV */
    HAL_BATTERY_VOLT_04_0500_V = 40500,         /**< define voltage as  4050 mV */
    HAL_BATTERY_VOLT_04_1000_V = 41000,         /**< define voltage as  4100 mV */
    HAL_BATTERY_VOLT_04_1250_V = 41250,         /**< define voltage as  4125 mV */
    HAL_BATTERY_VOLT_04_1375_V = 41375,         /**< define voltage as  4137.5 mV */
    HAL_BATTERY_VOLT_04_1500_V = 41500,         /**< define voltage as  4150 mV */
    HAL_BATTERY_VOLT_04_1625_V = 41625,         /**< define voltage as  4162.5 mV */
    HAL_BATTERY_VOLT_04_1750_V = 41750,         /**< define voltage as  4175 mV */
    HAL_BATTERY_VOLT_04_1875_V = 41875,         /**< define voltage as  4187.5 mV */
    HAL_BATTERY_VOLT_04_2000_V = 42000,         /**< define voltage as  4200 mV */
    HAL_BATTERY_VOLT_04_2125_V = 42125,         /**< define voltage as  4212 mV */
    HAL_BATTERY_VOLT_04_2250_V = 42250,         /**< define voltage as  4225 mV */
    HAL_BATTERY_VOLT_04_2375_V = 42375,         /**< define voltage as  4237.5 mV */
    HAL_BATTERY_VOLT_04_2500_V = 42500,         /**< define voltage as  4250 mV */
    HAL_BATTERY_VOLT_04_2625_V = 42625,         /**< define voltage as  4262.5 mV */
    HAL_BATTERY_VOLT_04_2750_V = 42750,         /**< define voltage as  4275 mV */
    HAL_BATTERY_VOLT_04_2875_V = 42875,         /**< define voltage as  4287.5 mV */
    HAL_BATTERY_VOLT_04_3000_V = 43000,         /**< define voltage as  4300 mV */
    HAL_BATTERY_VOLT_04_3125_V = 43125,         /**< define voltage as  4312.5 mV */
    HAL_BATTERY_VOLT_04_3250_V = 43250,         /**< define voltage as  4325 mV */
    HAL_BATTERY_VOLT_04_3375_V = 43375,         /**< define voltage as  4337.5 mV */
    HAL_BATTERY_VOLT_04_3500_V = 43500,         /**< define voltage as  4350 mV */
    HAL_BATTERY_VOLT_04_3625_V = 43625,         /**< define voltage as  4362.5 mV */
    HAL_BATTERY_VOLT_04_3750_V = 43750,         /**< define voltage as  4375 mV */
    HAL_BATTERY_VOLT_04_3875_V = 43875,         /**< define voltage as  4387.5 mV */
    HAL_BATTERY_VOLT_04_4000_V = 44000,         /**< define voltage as  4400 mV */
    HAL_BATTERY_VOLT_04_4125_V = 44125,         /**< define voltage as  4412.5 mV */
    HAL_BATTERY_VOLT_04_4375_V = 44375,         /**< define voltage as  4437.5 mV */
    HAL_BATTERY_VOLT_04_4250_V = 44250,         /**< define voltage as  4425 mV */
    HAL_BATTERY_VOLT_04_4500_V = 44500,         /**< define voltage as  4450 mV */
    HAL_BATTERY_VOLT_04_4625_V = 44625,         /**< define voltage as  4462.5 mV */
    HAL_BATTERY_VOLT_04_4750_V = 44750,         /**< define voltage as  4475 mV */
    HAL_BATTERY_VOLT_04_4875_V = 44875,         /**< define voltage as  4487.5 mV */
    HAL_BATTERY_VOLT_04_5000_V = 45000,         /**< define voltage as  4500 mV */
    HAL_BATTERY_VOLT_04_5500_V = 45500,         /**< define voltage as  4550 mV */
    HAL_BATTERY_VOLT_04_6000_V = 46000,         /**< define voltage as  4600 mV */
    HAL_BATTERY_VOLT_06_0000_V = 60000,         /**< define voltage as  6000 mV */
    HAL_BATTERY_VOLT_06_5000_V = 65000,         /**< define voltage as  6500 mV */
    HAL_BATTERY_VOLT_07_0000_V = 70000,         /**< define voltage as  7000 mV */
    HAL_BATTERY_VOLT_07_5000_V = 75000,         /**< define voltage as  7500 mV */
    HAL_BATTERY_VOLT_08_5000_V = 85000,         /**< define voltage as  8500 mV */
    HAL_BATTERY_VOLT_09_5000_V = 95000,         /**< define voltage as  9500 mV */
    HAL_BATTERY_VOLT_10_5000_V = 105000,        /**< define voltage as 10500 mV */
    HAL_BATTERY_VOLT_MAX,
    HAL_BATTERY_VOLT_INVALID
} HAL_BATTERY_VOLTAGE_ENUM;

typedef enum {
    HAL_DISPLAY_LCD_LAYER_ROTATE_0 = 0x0,                           /**< 0 degree. */
    HAL_DISPLAY_LCD_LAYER_ROTATE_90 = 0x1,                          /**< 90 degrees. */
    HAL_DISPLAY_LCD_LAYER_ROTATE_180 = 0x2,                         /**< 180 degrees. */
    HAL_DISPLAY_LCD_LAYER_ROTATE_270 = 0x3,                         /**< 270 degrees. */
    HAL_DISPLAY_LCD_LAYER_ROTATE_0_FLIP = 0x4,                      /**< 0 degree rotation and flip. */
    HAL_DISPLAY_LCD_LAYER_ROTATE_90_FLIP = 0x5,                     /**< 90 degrees rotation and flip. */
    HAL_DISPLAY_LCD_LAYER_ROTATE_180_FLIP = 0x6,                    /**< 180 degrees rotation and flip. */
    HAL_DISPLAY_LCD_LAYER_ROTATE_270_FLIP = 0x7,                    /**< 270 degrees rotation and flip. */
    HAL_DISPLAY_LCD_LAYER_ROTATE_NUM                                /**< The number of layer rotation options (invalid layer rotation). */
} hal_display_lcd_layer_rotate_t;

/** @brief The color formats of the input layer. */
typedef enum {
    HAL_DISPLAY_LCD_LAYER_COLOR_8BPP_INDEX = 0,                     /**< 8 bits per pixel index mode. */
    HAL_DISPLAY_LCD_LAYER_COLOR_RGB565,                             /**< RGB565. */
    HAL_DISPLAY_LCD_LAYER_COLOR_UYVY422,                            /**< YUV422. */
    HAL_DISPLAY_LCD_LAYER_COLOR_RGB888,                             /**< RGB888. */
    HAL_DISPLAY_LCD_LAYER_COLOR_ARGB8888,                           /**< ARGB8888. */
    HAL_DISPLAY_LCD_LAYER_COLOR_PARGB8888,                          /**< PARGB8888. */
    HAL_DISPLAY_LCD_LAYER_COLOR_XRGB,                               /**< XRGB. */
    HAL_DISPLAY_LCD_LAYER_COLOR_ARGB6666,                           /**< ARGB6666. */
    HAL_DISPLAY_LCD_LAYER_COLOR_PARGB6666,                          /**< PARGB6666. */
    HAL_DISPLAY_LCD_LAYER_COLOR_4BIT_INDEX,                         /**< 4 bits per pixel index mode. */
    HAL_DISPLAY_LCD_LAYER_COLOR_2BIT_INDEX,                         /**< 2 bits per pixel index mode. */
    HAL_DISPLAY_LCD_LAYER_COLOR_1BIT_INDEX,                         /**< 1 bit per pixel index mode. */
    HAL_DISPLAY_LCD_LAYER_COLOR_NUM                                 /**< The number of the color formats (invalid color format). */
} hal_display_lcd_layer_source_color_format_t;

typedef struct {
    uint8_t                             layer_enable;               /**< The number of the layer to be updated. */
    uint8_t                             alpha;                      /**< Configure alpha blending. */
    bool                                alpha_flag;                 /**< Enable or disable the alpha blending. */
    hal_display_lcd_layer_rotate_t      rotate;                     /**< Layer rotation degree. */
    bool                                source_key_flag;            /**< Enable or disable the source key. */
    bool                                rgb_swap_flag;              /**< Enable or disable the RGB swap. */
    bool                                byte_swap_flag;             /**< Enable or disable the byte swap. */
    bool                                dither_flag;                /**< Enable or disable the dithering function. */
    hal_display_lcd_layer_source_color_format_t     color_format;   /**< Set the color format in this layer. */
    bool                                destination_key_flag;       /**< Enable or disable the destination key. */
    uint32_t                            color_key;                  /**< Set the color key in this layer. */
    uint16_t                            window_x_offset;            /**< The starting x coordinate of the layer buffer to update. */
    uint16_t                            window_y_offset;            /**< The starting y coordinate of the layer buffer to update. */
    uintptr_t                           buffer_address;             /**< The start address of the layer to update. */
    uint16_t                            row_size;                   /**< The row size of output layer to update. */
    uint16_t                            column_size;                /**< The column size of output layer to update. */
    uint16_t                            pitch;                      /**< The pitch of output layer to update. */
} hal_display_lcd_layer_input_t;


/** @brief This enum defines the UART event when an interrupt occurs. */
typedef enum {
    HAL_UART_EVENT_TRANSACTION_ERROR = -1,          /**< Indicates if there is a transaction error when receiving data. */
    HAL_UART_EVENT_READY_TO_READ = 1,               /**< Indicates if there is enough data available in the RX buffer for the user to read from. */
    HAL_UART_EVENT_READY_TO_WRITE = 2               /**< Indicates if there is enough free space available in the TX buffer for the user to write into. */
} hal_uart_callback_event_t;

/** @brief virtual fifo  dma master name */
typedef enum {
    VDMA_START_CHANNEL = 8,              /**< virtual fifo dma start channel */
    VDMA_UART1TX = VDMA_START_CHANNEL,   /**< virtual fifo dma channel 9 */
    VDMA_UART1RX = 9,                    /**<virtual fifo  dma channel 10 */
    VDMA_UART2TX = 10,                    /**<virtual fifo  dma channel 11 */
    VDMA_UART2RX = 11,                    /**< virtual fifo dma channel 12*/
    VDMA_UART3TX = 12,                    /**< virtual fifo dma channel 13 */
    VDMA_UART3RX = 13,                    /**<virtual fifo  dma channel 14 */
    VDMA_UART0TX = 14,                    /**<virtual fifo  dma channel 15 */
    VDMA_UART0RX = 15,                    /**< virtual fifo dma channel 16*/
    VDMA_BTIFTX = 16,                     /**<virtual fifo  dma channel 17 */
    VDMA_BTIFRX = 17,                     /**< virtual fifo dma channel 18*/
    VDMA_END_CHANNEL                      /**< virtual fifo dma  end channel (invalid) */

} vdma_channel_t;


#ifdef __cplusplus
  #define   __I     volatile             /*!< Defines 'read only' permissions                 */
#else
  #define   __I     volatile const       /*!< Defines 'read only' permissions                 */
#endif
#define     __O     volatile             /*!< Defines 'write only' permissions                */
#define     __IO    volatile             /*!< Defines 'read / write' permissions              */





/** @brief UART port index
 * There are total of four UART ports. Only UART0 and UART1 support hardware flow control.
 * | UART port | Hardware Flow Control |
 * |-----------|-----------------------|
 * |  UART0    |           V           |
 * |  UART1    |           V           |
 * |  UART2    |           X           |
 * |  UART3    |           X           |
 */
typedef enum {
    HAL_UART_0 = 0,                            /**< UART port 0. */
    HAL_UART_1 = 1,                            /**< UART port 1. */
    HAL_UART_2 = 2,                            /**< UART port 2. */
    HAL_UART_3 = 3,                            /**< UART port 3. */
    HAL_UART_MAX                               /**< The total number of UART ports (invalid UART port number). */
} hal_uart_port_t;

/** @brief This struct defines configuration parameters and TX/RX buffers for the VFIFO DMA associated with a specific UART channel. */
typedef struct {
    uint8_t *send_vfifo_buffer;                /**< This field represents the transmitting user allocated VFIFO buffer. It will only be used by the UART driver to send data, must be non-cacheable and aligned to 4bytes. */
    uint32_t send_vfifo_buffer_size;           /**< This field represents the size of the transmitting VFIFO buffer. */
    uint32_t send_vfifo_threshold_size;        /**< This field represents the threshold of the transmitting VFIFO buffer. VFIFO DMA will trigger an interrupt when the available bytes in VFIFO buffer are lower than the threshold. */
    uint8_t *receive_vfifo_buffer;             /**< This field represents the receiving user allocated VFIFO buffer. It will only be used by the UART driver for receiving data, and must be non-cacheable and align to 4bytes. */
    uint32_t receive_vfifo_buffer_size;        /**< This field represents size of the receiving VFIFO buffer. */
    uint32_t receive_vfifo_threshold_size;     /**< This field represents the threshold of the receiving VFIFO buffer. VFIFO DMA will trigger receive interrupt when available bytes in VFIFO buffer are more than the threshold. */
    uint32_t receive_vfifo_alert_size;         /**< This field represents the threshold size of free space left in the VFIFO buffer that activates the UART's flow control system. */
} hal_uart_dma_config_t;

/** @brief This enum defines baud rate of the UART frame. */
typedef enum {
    HAL_UART_BAUDRATE_110 = 0,           /**< Defines UART baudrate as 110 bps. */
    HAL_UART_BAUDRATE_300 = 1,           /**< Defines UART baudrate as 300 bps. */
    HAL_UART_BAUDRATE_1200 = 2,          /**< Defines UART baudrate as 1200 bps. */
    HAL_UART_BAUDRATE_2400 = 3,          /**< Defines UART baudrate as 2400 bps. */
    HAL_UART_BAUDRATE_4800 = 4,          /**< Defines UART baudrate as 4800 bps. */
    HAL_UART_BAUDRATE_9600 = 5,          /**< Defines UART baudrate as 9600 bps. */
    HAL_UART_BAUDRATE_19200 = 6,         /**< Defines UART baudrate as 19200 bps. */
    HAL_UART_BAUDRATE_38400 = 7,         /**< Defines UART baudrate as 38400 bps. */
    HAL_UART_BAUDRATE_57600 = 8,         /**< Defines UART baudrate as 57600 bps. */
    HAL_UART_BAUDRATE_115200 = 9,        /**< Defines UART baudrate as 115200 bps. */
    HAL_UART_BAUDRATE_230400 = 10,       /**< Defines UART baudrate as 230400 bps. */
    HAL_UART_BAUDRATE_460800 = 11,       /**< Defines UART baudrate as 460800 bps. */
    HAL_UART_BAUDRATE_921600 = 12,       /**< Defines UART baudrate as 921600 bps. */
    HAL_UART_BAUDRATE_3000000 = 13,      /**< Defines UART baudrate as 3000000 bps. */
    HAL_UART_BAUDRATE_MAX                /**< Defines maximum enum value of UART baudrate. */
} hal_uart_baudrate_t;


/** @brief This enum defines word length of the UART frame. */
typedef enum {
    HAL_UART_WORD_LENGTH_5 = 0,           /**< Defines UART word length as 5 bits per frame. */
    HAL_UART_WORD_LENGTH_6 = 1,           /**< Defines UART word length as 6 bits per frame. */
    HAL_UART_WORD_LENGTH_7 = 2,           /**< Defines UART word length as 7 bits per frame. */
    HAL_UART_WORD_LENGTH_8 = 3            /**< Defines UART word length as 8 bits per frame. */
} hal_uart_word_length_t;


/** @brief This enum defines stop bit of the UART frame. */
typedef enum {
    HAL_UART_STOP_BIT_1 = 0,              /**< Defines UART stop bit as 1 bit per frame. */
    HAL_UART_STOP_BIT_2 = 1,              /**< Defines UART stop bit as 2 bits per frame. */
} hal_uart_stop_bit_t;


/** @brief This enum defines parity of the UART frame. */
typedef enum {
    HAL_UART_PARITY_NONE = 0,            /**< Defines UART parity as none. */
    HAL_UART_PARITY_ODD = 1,             /**< Defines UART parity as odd. */
    HAL_UART_PARITY_EVEN = 2             /**< Defines UART parity as even. */
} hal_uart_parity_t;

/** @brief This enum defines return status of the UART HAL public API. User should check return value after calling these APIs. */
typedef enum {
    HAL_UART_STATUS_ERROR_PARAMETER = -4,      /**< Invalid user input parameter. */
    HAL_UART_STATUS_ERROR_BUSY = -3,           /**< UART port is currently in use. */
    HAL_UART_STATUS_ERROR_UNINITIALIZED = -2,  /**< UART port has not been initialized. */
    HAL_UART_STATUS_ERROR = -1,                /**< UART driver detected a common error. */
    HAL_UART_STATUS_OK = 0                     /**< UART function executed successfully. */
} hal_uart_status_t;

/** @brief This struct defines UART configure parameters. */
typedef struct {
    hal_uart_baudrate_t baudrate;              /**< This field represents the baudrate of the UART frame. */
    hal_uart_word_length_t word_length;        /**< This field represents the word length of the UART frame. */
    hal_uart_stop_bit_t stop_bit;              /**< This field represents the stop bit of the UART frame. */
    hal_uart_parity_t parity;                  /**< This field represents the parity of the UART frame. */
} hal_uart_config_t;





/** @brief This enum defines the return type of GPIO API. */
typedef enum {
    HAL_GPIO_STATUS_ERROR             = -3,     /**< The GPIO function failed to execute.*/
    HAL_GPIO_STATUS_ERROR_PIN         = -2,     /**< Invalid input pin number. */
    HAL_GPIO_STATUS_INVALID_PARAMETER = -1,     /**< Invalid input parameter. */
    HAL_GPIO_STATUS_OK                = 0       /**< The GPIO function executed successfully. */
} hal_gpio_status_t;

/** @brief This enum defines the return type of pinmux API. */
typedef enum {
    HAL_PINMUX_STATUS_ERROR             = -3,   /**< The pinmux function failed to execute.*/
    HAL_PINMUX_STATUS_ERROR_PORT        = -2,   /**< Invalid input pin port. */
    HAL_PINMUX_STATUS_INVALID_FUNCTION  = -1,   /**< Invalid input function. */
    HAL_PINMUX_STATUS_OK                = 0     /**< The pinmux function executed successfully. */
} hal_pinmux_status_t;

/** @brief This enum defines the GPIO direction. */
typedef enum {
    HAL_GPIO_DIRECTION_INPUT  = 0,              /**<  GPIO input direction. */
    HAL_GPIO_DIRECTION_OUTPUT = 1               /**<  GPIO output direction. */
} hal_gpio_direction_t;


/** @brief This enum defines the data type of GPIO. */
typedef enum {
    HAL_GPIO_DATA_LOW  = 0,                     /**<  GPIO data low. */
    HAL_GPIO_DATA_HIGH = 1                      /**<  GPIO data high. */
} hal_gpio_data_t;

typedef enum {
    HAL_GPIO_0 = 0,                            /**< GPIO pin0. */
    HAL_GPIO_1 = 1,                            /**< GPIO pin1. */
    HAL_GPIO_2 = 2,                            /**< GPIO pin2. */
    HAL_GPIO_3 = 3,                            /**< GPIO pin3. */
    HAL_GPIO_4 = 4,                            /**< GPIO pin4. */
    HAL_GPIO_5 = 5,                            /**< GPIO pin5. */
    HAL_GPIO_6 = 6,                            /**< GPIO pin6. */
    HAL_GPIO_7 = 7,                            /**< GPIO pin7. */
    HAL_GPIO_8 = 8,                            /**< GPIO pin8. */
    HAL_GPIO_9 = 9,                            /**< GPIO pin9. */
    HAL_GPIO_10 = 10,                          /**< GPIO pin10. */
    HAL_GPIO_11 = 11,                          /**< GPIO pin11. */
    HAL_GPIO_12 = 12,                          /**< GPIO pin12. */
    HAL_GPIO_13 = 13,                          /**< GPIO pin13. */
    HAL_GPIO_14 = 14,                          /**< GPIO pin14. */
    HAL_GPIO_15 = 15,                          /**< GPIO pin15. */
    HAL_GPIO_16 = 16,                          /**< GPIO pin16. */
    HAL_GPIO_17 = 17,                          /**< GPIO pin17. */
    HAL_GPIO_18 = 18,                          /**< GPIO pin18. */
    HAL_GPIO_19 = 19,                          /**< GPIO pin19. */
    HAL_GPIO_20 = 20,                          /**< GPIO pin20. */
    HAL_GPIO_21 = 21,                          /**< GPIO pin21. */
    HAL_GPIO_22 = 22,                          /**< GPIO pin22. */
    HAL_GPIO_23 = 23,                          /**< GPIO pin23. */
    HAL_GPIO_24 = 24,                          /**< GPIO pin24. */
    HAL_GPIO_25 = 25,                          /**< GPIO pin25. */
    HAL_GPIO_26 = 26,                          /**< GPIO pin26. */
    HAL_GPIO_27 = 27,                          /**< GPIO pin27. */
    HAL_GPIO_28 = 28,                          /**< GPIO pin28. */
    HAL_GPIO_29 = 29,                          /**< GPIO pin29. */
    HAL_GPIO_30 = 30,                          /**< GPIO pin30. */
    HAL_GPIO_31 = 31,                          /**< GPIO pin31. */
    HAL_GPIO_32 = 32,                          /**< GPIO pin32. */
    HAL_GPIO_33 = 33,                          /**< GPIO pin33. */
    HAL_GPIO_34 = 34,                          /**< GPIO pin34. */
    HAL_GPIO_35 = 35,                          /**< GPIO pin35. */
    HAL_GPIO_36 = 36,                          /**< GPIO pin36. */
    HAL_GPIO_37 = 37,                          /**< GPIO pin37. */
    HAL_GPIO_38 = 38,                          /**< GPIO pin38. */
    HAL_GPIO_39 = 39,                          /**< GPIO pin39. */
    HAL_GPIO_40 = 40,                          /**< GPIO pin40. */
    HAL_GPIO_41 = 41,                          /**< GPIO pin41. */
    HAL_GPIO_42 = 42,                          /**< GPIO pin42. */
    HAL_GPIO_43 = 43,                          /**< GPIO pin43. */
    HAL_GPIO_44 = 44,                          /**< GPIO pin44. */
    HAL_GPIO_45 = 45,                          /**< GPIO pin45. */
    HAL_GPIO_46 = 46,                          /**< GPIO pin46. */
    HAL_GPIO_47 = 47,                          /**< GPIO pin47. */
    HAL_GPIO_48 = 48,                          /**< GPIO pin48. */
    HAL_GPIO_MAX                               /**< The total number of GPIO pins (invalid GPIO pin number). */
} hal_gpio_pin_t;

#define HAL_GPIO_4_U1RXD   6
#define HAL_GPIO_5_U1TXD   6
#define HAL_GPIO_6_U2RXD   4
#define HAL_GPIO_7_U2TXD   4
#define HAL_GPIO_18_U3RXD   3
#define HAL_GPIO_22_U3TXD   3

#define HAL_GPIO_4_GPIO4   0
#define HAL_GPIO_5_GPIO5   0
#define HAL_GPIO_6_GPIO6   0
#define HAL_GPIO_7_GPIO7   0
#define HAL_GPIO_18_GPIO18   0
#define HAL_GPIO_22_GPIO22   0

#define VDMA_CON_ITEN_OFFSET                (15)
#define VDMA_CON_ITEN_MASK                  (0x1<< VDMA_CON_ITEN_OFFSET)



/** @brief This enum defines the Sleep Manager API return status*/
typedef enum {
    HAL_SLEEP_MANAGER_ERROR                           = -1,    /**< An undefined error occurred. */
    HAL_SLEEP_MANAGER_OK                              = 0,     /**< The operation completed successfully. */
} hal_sleep_manager_status_t;

/** @brief GPT clock source */
typedef enum {
    HAL_GPT_CLOCK_SOURCE_32K = 0,            /**< Set the GPT clock source to 32kHz, 1 tick = 1/32768 seconds. */
    HAL_GPT_CLOCK_SOURCE_1M  = 1             /**< Set the GPT clock source to 1MHz, 1 tick = 1 microsecond.*/
} hal_gpt_clock_source_t;

void hal_gpt_get_free_run_count(hal_gpt_clock_source_t clock_source, uint32_t *count);

void DISPLAY_WaitDMA();
void DISPLAY_StartDMA(size_t displayID, hal_display_lcd_layer_input_t* layer);
uint16_t* DISPLAY_getFramebufferPtr(size_t displayID);

uint32_t RTOS_getTimeMs(void);
