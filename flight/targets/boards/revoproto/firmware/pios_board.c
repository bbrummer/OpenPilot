/**
 *****************************************************************************
 * @file       pios_board.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2011.
 * @author     PhoenixPilot, http://github.com/PhoenixPilot, Copyright (C) 2012
 * @addtogroup OpenPilotSystem OpenPilot System
 * @{
 * @addtogroup OpenPilotCore OpenPilot Core
 * @{
 * @brief Defines board specific static initializers for hardware for the revolution board.
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "inc/openpilot.h"
#include <pios_board_info.h>
#include <uavobjectsinit.h>
#include <hwsettings.h>
#include <manualcontrolsettings.h>
#include <taskinfo.h>

/*
 * Pull in the board-specific static HW definitions.
 * Including .c files is a bit ugly but this allows all of
 * the HW definitions to be const and static to limit their
 * scope.
 *
 * NOTE: THIS IS THE ONLY PLACE THAT SHOULD EVER INCLUDE THIS FILE
 */
#include "../board_hw_defs.c"

/**
 * Sensor configurations
 */

#if defined(PIOS_INCLUDE_ADC)
#include "pios_adc_priv.h"
void PIOS_ADC_DMC_irq_handler(void);
void DMA2_Stream4_IRQHandler(void) __attribute__((alias("PIOS_ADC_DMC_irq_handler")));
struct pios_adc_cfg pios_adc_cfg = {
    .adc_dev = ADC1,
    .dma     = {
        .irq                                       = {
            .flags = (DMA_FLAG_TCIF4 | DMA_FLAG_TEIF4 | DMA_FLAG_HTIF4),
            .init  = {
                .NVIC_IRQChannel    = DMA2_Stream4_IRQn,
                .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_LOW,
                .NVIC_IRQChannelSubPriority        = 0,
                .NVIC_IRQChannelCmd = ENABLE,
            },
        },
        .rx                                        = {
            .channel = DMA2_Stream4,
            .init    = {
                .DMA_Channel                       = DMA_Channel_0,
                .DMA_PeripheralBaseAddr            = (uint32_t)&ADC1->DR
            },
        }
    },
    .half_flag = DMA_IT_HTIF4,
    .full_flag = DMA_IT_TCIF4,
};
void PIOS_ADC_DMC_irq_handler(void)
{
    /* Call into the generic code to handle the IRQ for this specific device */
    PIOS_ADC_DMA_Handler();
}

#endif /* if defined(PIOS_INCLUDE_ADC) */

#if defined(PIOS_INCLUDE_HMC5X83)
#include "pios_hmc5x83.h"

pios_hmc5x83_dev_t onboard_mag = 0;

bool pios_board_internal_mag_handler()
{
    return PIOS_HMC5x83_IRQHandler(onboard_mag);
}
static const struct pios_exti_cfg pios_exti_hmc5x83_cfg __exti_config = {
    .vector = pios_board_internal_mag_handler,
    .line   = EXTI_Line5,
    .pin    = {
        .gpio = GPIOB,
        .init = {
            .GPIO_Pin   = GPIO_Pin_5,
            .GPIO_Speed = GPIO_Speed_100MHz,
            .GPIO_Mode  = GPIO_Mode_IN,
            .GPIO_OType = GPIO_OType_OD,
            .GPIO_PuPd  = GPIO_PuPd_NOPULL,
        },
    },
    .irq                                       = {
        .init                                  = {
            .NVIC_IRQChannel    = EXTI9_5_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_LOW,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
    .exti                                      = {
        .init                                  = {
            .EXTI_Line    = EXTI_Line5, // matches above GPIO pin
            .EXTI_Mode    = EXTI_Mode_Interrupt,
            .EXTI_Trigger = EXTI_Trigger_Rising,
            .EXTI_LineCmd = ENABLE,
        },
    },
};

static const struct pios_hmc5x83_cfg pios_hmc5x83_cfg = {
    .exti_cfg    = &pios_exti_hmc5x83_cfg,
    .M_ODR       = PIOS_HMC5x83_ODR_75,
    .Meas_Conf   = PIOS_HMC5x83_MEASCONF_NORMAL,
    .Gain        = PIOS_HMC5x83_GAIN_1_9,
    .Mode        = PIOS_HMC5x83_MODE_CONTINUOUS,
    .Driver      = &PIOS_HMC5x83_I2C_DRIVER,
    .Orientation = PIOS_HMC5X83_ORIENTATION_EAST_NORTH_UP,
};
#endif /* PIOS_INCLUDE_HMC5X83 */

/**
 * Configuration for the MS5611 chip
 */
#if defined(PIOS_INCLUDE_MS5611)
#include "pios_ms5611.h"
static const struct pios_ms5611_cfg pios_ms5611_cfg = {
    .oversampling = MS5611_OSR_4096,
};
#endif /* PIOS_INCLUDE_MS5611 */

/**
 * Configuration for the BMA180 chip
 */
#if defined(PIOS_INCLUDE_BMA180)
#include "pios_bma180.h"
static const struct pios_exti_cfg pios_exti_bma180_cfg __exti_config = {
    .vector = PIOS_BMA180_IRQHandler,
    .line   = EXTI_Line4,
    .pin    = {
        .gpio = GPIOC,
        .init = {
            .GPIO_Pin   = GPIO_Pin_4,
            .GPIO_Speed = GPIO_Speed_100MHz,
            .GPIO_Mode  = GPIO_Mode_IN,
            .GPIO_OType = GPIO_OType_OD,
            .GPIO_PuPd  = GPIO_PuPd_NOPULL,
        },
    },
    .irq                                       = {
        .init                                  = {
            .NVIC_IRQChannel    = EXTI4_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_LOW,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
    .exti                                      = {
        .init                                  = {
            .EXTI_Line    = EXTI_Line4, // matches above GPIO pin
            .EXTI_Mode    = EXTI_Mode_Interrupt,
            .EXTI_Trigger = EXTI_Trigger_Rising,
            .EXTI_LineCmd = ENABLE,
        },
    },
};
static const struct pios_bma180_cfg pios_bma180_cfg = {
    .exti_cfg  = &pios_exti_bma180_cfg,
    .bandwidth = BMA_BW_600HZ,
    .range     = BMA_RANGE_8G,
};
#endif /* PIOS_INCLUDE_BMA180 */

/**
 * Configuration for the MPU6000 chip
 */
#if defined(PIOS_INCLUDE_MPU6000)
#include "pios_mpu6000.h"
#include "pios_mpu6000_config.h"
static const struct pios_exti_cfg pios_exti_mpu6000_cfg __exti_config = {
    .vector = PIOS_MPU6000_IRQHandler,
    .line   = EXTI_Line8,
    .pin    = {
        .gpio = GPIOD,
        .init = {
            .GPIO_Pin   = GPIO_Pin_8,
            .GPIO_Speed = GPIO_Speed_100MHz,
            .GPIO_Mode  = GPIO_Mode_IN,
            .GPIO_OType = GPIO_OType_OD,
            .GPIO_PuPd  = GPIO_PuPd_NOPULL,
        },
    },
    .irq                                       = {
        .init                                  = {
            .NVIC_IRQChannel    = EXTI9_5_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGH,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
    .exti                                      = {
        .init                                  = {
            .EXTI_Line    = EXTI_Line8, // matches above GPIO pin
            .EXTI_Mode    = EXTI_Mode_Interrupt,
            .EXTI_Trigger = EXTI_Trigger_Rising,
            .EXTI_LineCmd = ENABLE,
        },
    },
};

static const struct pios_mpu6000_cfg pios_mpu6000_cfg = {
    .exti_cfg   = &pios_exti_mpu6000_cfg,
    .Fifo_store = PIOS_MPU6000_FIFO_TEMP_OUT | PIOS_MPU6000_FIFO_GYRO_X_OUT | PIOS_MPU6000_FIFO_GYRO_Y_OUT | PIOS_MPU6000_FIFO_GYRO_Z_OUT,
    // Clock at 8 khz
    .Smpl_rate_div_no_dlp = 0,
    // with dlp on output rate is 1000Hz
    .Smpl_rate_div_dlp    = 0,
    .interrupt_cfg  = PIOS_MPU6000_INT_CLR_ANYRD,
    .interrupt_en   = PIOS_MPU6000_INTEN_DATA_RDY,
    .User_ctl             = PIOS_MPU6000_USERCTL_DIS_I2C,
    .Pwr_mgmt_clk   = PIOS_MPU6000_PWRMGMT_PLL_X_CLK,
    .accel_range    = PIOS_MPU6000_ACCEL_8G,
    .gyro_range     = PIOS_MPU6000_SCALE_2000_DEG,
    .filter               = PIOS_MPU6000_LOWPASS_256_HZ,
    .orientation    = PIOS_MPU6000_TOP_0DEG,
    .fast_prescaler = PIOS_SPI_PRESCALER_4,
    .std_prescaler  = PIOS_SPI_PRESCALER_64,
    .max_downsample = 16,
};
#endif /* PIOS_INCLUDE_MPU6000 */

/**
 * Configuration for L3GD20 chip
 */
#if defined(PIOS_INCLUDE_L3GD20)
#include "pios_l3gd20.h"
static const struct pios_exti_cfg pios_exti_l3gd20_cfg __exti_config = {
    .vector = PIOS_L3GD20_IRQHandler,
    .line   = EXTI_Line8,
    .pin    = {
        .gpio = GPIOD,
        .init = {
            .GPIO_Pin   = GPIO_Pin_8,
            .GPIO_Speed = GPIO_Speed_100MHz,
            .GPIO_Mode  = GPIO_Mode_IN,
            .GPIO_OType = GPIO_OType_OD,
            .GPIO_PuPd  = GPIO_PuPd_NOPULL,
        },
    },
    .irq                                       = {
        .init                                  = {
            .NVIC_IRQChannel    = EXTI9_5_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGH,
            .NVIC_IRQChannelSubPriority        = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
    .exti                                      = {
        .init                                  = {
            .EXTI_Line    = EXTI_Line8, // matches above GPIO pin
            .EXTI_Mode    = EXTI_Mode_Interrupt,
            .EXTI_Trigger = EXTI_Trigger_Rising,
            .EXTI_LineCmd = ENABLE,
        },
    },
};

static const struct pios_l3gd20_cfg pios_l3gd20_cfg = {
    .exti_cfg = &pios_exti_l3gd20_cfg,
    .range    = PIOS_L3GD20_SCALE_500_DEG,
};
#endif /* PIOS_INCLUDE_L3GD20 */

/* One slot per selectable receiver group.
 *  eg. PWM, PPM, GCS, SPEKTRUM1, SPEKTRUM2, SBUS
 * NOTE: No slot in this map for NONE.
 */
uint32_t pios_rcvr_group_map[MANUALCONTROLSETTINGS_CHANNELGROUPS_NONE];

#define PIOS_COM_TELEM_RF_RX_BUF_LEN  512
#define PIOS_COM_TELEM_RF_TX_BUF_LEN  512

#define PIOS_COM_GPS_RX_BUF_LEN       32

#define PIOS_COM_TELEM_USB_RX_BUF_LEN 65
#define PIOS_COM_TELEM_USB_TX_BUF_LEN 65

#define PIOS_COM_BRIDGE_RX_BUF_LEN    65
#define PIOS_COM_BRIDGE_TX_BUF_LEN    12

#define PIOS_COM_AUX_RX_BUF_LEN       512
#define PIOS_COM_AUX_TX_BUF_LEN       512

#define PIOS_COM_HKOSD_RX_BUF_LEN     22
#define PIOS_COM_HKOSD_TX_BUF_LEN     22


uint32_t pios_com_aux_id       = 0;
uint32_t pios_com_gps_id       = 0;
uint32_t pios_com_telem_usb_id = 0;
uint32_t pios_com_telem_rf_id  = 0;
uint32_t pios_com_bridge_id    = 0;
uint32_t pios_com_overo_id     = 0;
uint32_t pios_com_hkosd_id     = 0;

uintptr_t pios_uavo_settings_fs_id;
uintptr_t pios_user_fs_id;

uint32_t pios_com_vcp_id = 0;

/*
 * Setup a com port based on the passed cfg, driver and buffer sizes. tx size of -1 make the port rx only
 */
static void PIOS_Board_configure_com(const struct pios_usart_cfg *usart_port_cfg, size_t rx_buf_len, size_t tx_buf_len,
                                     const struct pios_com_driver *com_driver, uint32_t *pios_com_id)
{
    uint32_t pios_usart_id;

    if (PIOS_USART_Init(&pios_usart_id, usart_port_cfg)) {
        PIOS_Assert(0);
    }

    uint8_t *rx_buffer = (uint8_t *)pios_malloc(rx_buf_len);
    PIOS_Assert(rx_buffer);
    if (tx_buf_len != (size_t)-1) { // this is the case for rx/tx ports
        uint8_t *tx_buffer = (uint8_t *)pios_malloc(tx_buf_len);
        PIOS_Assert(tx_buffer);

        if (PIOS_COM_Init(pios_com_id, com_driver, pios_usart_id,
                          rx_buffer, rx_buf_len,
                          tx_buffer, tx_buf_len)) {
            PIOS_Assert(0);
        }
    } else { // rx only port
        if (PIOS_COM_Init(pios_com_id, com_driver, pios_usart_id,
                          rx_buffer, rx_buf_len,
                          NULL, 0)) {
            PIOS_Assert(0);
        }
    }
}

static void PIOS_Board_configure_dsm(const struct pios_usart_cfg *pios_usart_dsm_cfg, const struct pios_dsm_cfg *pios_dsm_cfg,
                                     const struct pios_com_driver *usart_com_driver,
                                     ManualControlSettingsChannelGroupsOptions channelgroup, uint8_t *bind)
{
    uint32_t pios_usart_dsm_id;

    if (PIOS_USART_Init(&pios_usart_dsm_id, pios_usart_dsm_cfg)) {
        PIOS_Assert(0);
    }

    uint32_t pios_dsm_id;
    if (PIOS_DSM_Init(&pios_dsm_id, pios_dsm_cfg, usart_com_driver,
                      pios_usart_dsm_id, *bind)) {
        PIOS_Assert(0);
    }

    uint32_t pios_dsm_rcvr_id;
    if (PIOS_RCVR_Init(&pios_dsm_rcvr_id, &pios_dsm_rcvr_driver, pios_dsm_id)) {
        PIOS_Assert(0);
    }
    pios_rcvr_group_map[channelgroup] = pios_dsm_rcvr_id;
}

/**
 * PIOS_Board_Init()
 * initializes all the core subsystems on this specific hardware
 * called from System/openpilot.c
 */

#include <pios_board_info.h>

void PIOS_Board_Init(void)
{
    const struct pios_board_info *bdinfo = &pios_board_info_blob;

    /* Delay system */
    PIOS_DELAY_Init();

    PIOS_LED_Init(&pios_led_cfg);

    /* Connect flash to the appropriate interface and configure it */
    uintptr_t flash_id;

    // initialize the internal settings storage flash
    if (PIOS_Flash_Internal_Init(&flash_id, &flash_internal_cfg)) {
        PIOS_DEBUG_Assert(0);
    }

    if (PIOS_FLASHFS_Logfs_Init(&pios_uavo_settings_fs_id, &flashfs_internal_cfg, &pios_internal_flash_driver, flash_id)) {
        PIOS_DEBUG_Assert(0);
    }

    /* Set up the SPI interface to the accelerometer*/
    if (PIOS_SPI_Init(&pios_spi_accel_id, &pios_spi_accel_cfg)) {
        PIOS_DEBUG_Assert(0);
    }

    /* Set up the SPI interface to the gyro */
    if (PIOS_SPI_Init(&pios_spi_gyro_id, &pios_spi_gyro_cfg)) {
        PIOS_DEBUG_Assert(0);
    }
#if !defined(PIOS_FLASH_ON_ACCEL)
    /* Set up the SPI interface to the flash */
    if (PIOS_SPI_Init(&pios_spi_flash_id, &pios_spi_flash_cfg)) {
        PIOS_DEBUG_Assert(0);
    }

    /* Connect flash to the appropriate interface and configure it */
    if (PIOS_Flash_Jedec_Init(&flash_id, pios_spi_flash_id, 0)) {
        PIOS_DEBUG_Assert(0);
    }
#else
    /* Connect flash to the appropriate interface and configure it */
    if (PIOS_Flash_Jedec_Init(&flash_id, pios_spi_accel_id, 1)) {
        PIOS_DEBUG_Assert(0);
    }
#endif
    if (PIOS_FLASHFS_Logfs_Init(&pios_user_fs_id, &flashfs_external_cfg, &pios_jedec_flash_driver, flash_id)) {
        PIOS_DEBUG_Assert(0);
    }

#if defined(PIOS_INCLUDE_RTC)
    PIOS_RTC_Init(&pios_rtc_main_cfg);
#endif

    /* IAP System Setup */
    PIOS_IAP_Init();
    // check for safe mode commands from gcs
    if (PIOS_IAP_ReadBootCmd(0) == PIOS_IAP_CLEAR_FLASH_CMD_0 &&
        PIOS_IAP_ReadBootCmd(1) == PIOS_IAP_CLEAR_FLASH_CMD_1 &&
        PIOS_IAP_ReadBootCmd(2) == PIOS_IAP_CLEAR_FLASH_CMD_2) {
        PIOS_FLASHFS_Format(pios_uavo_settings_fs_id);
        PIOS_IAP_WriteBootCmd(0, 0);
        PIOS_IAP_WriteBootCmd(1, 0);
        PIOS_IAP_WriteBootCmd(2, 0);
    }

    /* Initialize the task monitor */
    if (PIOS_TASK_MONITOR_Initialize(TASKINFO_RUNNING_NUMELEM)) {
        PIOS_Assert(0);
    }

    /* Initialize the delayed callback library */
    PIOS_CALLBACKSCHEDULER_Initialize();

    /* Initialize UAVObject libraries */
    EventDispatcherInitialize();
    UAVObjInitialize();

    HwSettingsInitialize();

    /* Initialize the alarms library */
    AlarmsInitialize();

    /* Set up pulse timers */
    PIOS_TIM_InitClock(&tim_1_cfg);
    PIOS_TIM_InitClock(&tim_2_cfg);
    PIOS_TIM_InitClock(&tim_3_cfg);
    PIOS_TIM_InitClock(&tim_4_cfg);
    PIOS_TIM_InitClock(&tim_5_cfg);
    PIOS_TIM_InitClock(&tim_9_cfg);
    PIOS_TIM_InitClock(&tim_10_cfg);
    PIOS_TIM_InitClock(&tim_11_cfg);

    uint16_t boot_count = PIOS_IAP_ReadBootCount();
    if (boot_count < 3) {
        PIOS_IAP_WriteBootCount(++boot_count);
        AlarmsClear(SYSTEMALARMS_ALARM_BOOTFAULT);
    } else {
        /* Too many failed boot attempts, force hwsettings to defaults */
        HwSettingsSetDefaults(HwSettingsHandle(), 0);
        AlarmsSet(SYSTEMALARMS_ALARM_BOOTFAULT, SYSTEMALARMS_ALARM_CRITICAL);
    }


    // PIOS_IAP_Init();

#if defined(PIOS_INCLUDE_USB)
    /* Initialize board specific USB data */
    PIOS_USB_BOARD_DATA_Init();

    /* Flags to determine if various USB interfaces are advertised */
    bool usb_hid_present = false;
    bool usb_cdc_present = false;

#if defined(PIOS_INCLUDE_USB_CDC)
    if (PIOS_USB_DESC_HID_CDC_Init()) {
        PIOS_Assert(0);
    }
    usb_hid_present = true;
    usb_cdc_present = true;
#else
    if (PIOS_USB_DESC_HID_ONLY_Init()) {
        PIOS_Assert(0);
    }
    usb_hid_present = true;
#endif

    uint32_t pios_usb_id;
    PIOS_USB_Init(&pios_usb_id, &pios_usb_main_cfg);

#if defined(PIOS_INCLUDE_USB_CDC)

    uint8_t hwsettings_usb_vcpport;
    /* Configure the USB VCP port */
    HwSettingsUSB_VCPPortGet(&hwsettings_usb_vcpport);

    if (!usb_cdc_present) {
        /* Force VCP port function to disabled if we haven't advertised VCP in our USB descriptor */
        hwsettings_usb_vcpport = HWSETTINGS_USB_VCPPORT_DISABLED;
    }

    uint32_t pios_usb_cdc_id;
    if (PIOS_USB_CDC_Init(&pios_usb_cdc_id, &pios_usb_cdc_cfg, pios_usb_id)) {
        PIOS_Assert(0);
    }

    uint32_t pios_usb_hid_id;
    if (PIOS_USB_HID_Init(&pios_usb_hid_id, &pios_usb_hid_cfg, pios_usb_id)) {
        PIOS_Assert(0);
    }

    switch (hwsettings_usb_vcpport) {
    case HWSETTINGS_USB_VCPPORT_DISABLED:
        break;
    case HWSETTINGS_USB_VCPPORT_USBTELEMETRY:
#if defined(PIOS_INCLUDE_COM)
        {
            uint8_t *rx_buffer = (uint8_t *)pios_malloc(PIOS_COM_TELEM_USB_RX_BUF_LEN);
            uint8_t *tx_buffer = (uint8_t *)pios_malloc(PIOS_COM_TELEM_USB_TX_BUF_LEN);
            PIOS_Assert(rx_buffer);
            PIOS_Assert(tx_buffer);
            if (PIOS_COM_Init(&pios_com_telem_usb_id, &pios_usb_cdc_com_driver, pios_usb_cdc_id,
                              rx_buffer, PIOS_COM_TELEM_USB_RX_BUF_LEN,
                              tx_buffer, PIOS_COM_TELEM_USB_TX_BUF_LEN)) {
                PIOS_Assert(0);
            }
        }
#endif /* PIOS_INCLUDE_COM */
        break;
    case HWSETTINGS_USB_VCPPORT_COMBRIDGE:
#if defined(PIOS_INCLUDE_COM)
        {
            uint8_t *rx_buffer = (uint8_t *)pios_malloc(PIOS_COM_BRIDGE_RX_BUF_LEN);
            uint8_t *tx_buffer = (uint8_t *)pios_malloc(PIOS_COM_BRIDGE_TX_BUF_LEN);
            PIOS_Assert(rx_buffer);
            PIOS_Assert(tx_buffer);
            if (PIOS_COM_Init(&pios_com_vcp_id, &pios_usb_cdc_com_driver, pios_usb_cdc_id,
                              rx_buffer, PIOS_COM_BRIDGE_RX_BUF_LEN,
                              tx_buffer, PIOS_COM_BRIDGE_TX_BUF_LEN)) {
                PIOS_Assert(0);
            }
        }
#endif /* PIOS_INCLUDE_COM */
        break;
    case HWSETTINGS_USB_VCPPORT_DEBUGCONSOLE:
#if defined(PIOS_INCLUDE_COM)
#if defined(PIOS_INCLUDE_DEBUG_CONSOLE)
        {
            uint8_t *tx_buffer = (uint8_t *)pios_malloc(PIOS_COM_DEBUGCONSOLE_TX_BUF_LEN);
            PIOS_Assert(tx_buffer);
            if (PIOS_COM_Init(&pios_com_debug_id, &pios_usb_cdc_com_driver, pios_usb_cdc_id,
                              NULL, 0,
                              tx_buffer, PIOS_COM_DEBUGCONSOLE_TX_BUF_LEN)) {
                PIOS_Assert(0);
            }
        }
#endif /* PIOS_INCLUDE_DEBUG_CONSOLE */
#endif /* PIOS_INCLUDE_COM */

        break;
    }
#endif /* PIOS_INCLUDE_USB_CDC */

#if defined(PIOS_INCLUDE_USB_HID)
    /* Configure the usb HID port */
    uint8_t hwsettings_usb_hidport;
    HwSettingsUSB_HIDPortGet(&hwsettings_usb_hidport);

    if (!usb_hid_present) {
        /* Force HID port function to disabled if we haven't advertised HID in our USB descriptor */
        hwsettings_usb_hidport = HWSETTINGS_USB_HIDPORT_DISABLED;
    }

    switch (hwsettings_usb_hidport) {
    case HWSETTINGS_USB_HIDPORT_DISABLED:
        break;
    case HWSETTINGS_USB_HIDPORT_USBTELEMETRY:
#if defined(PIOS_INCLUDE_COM)
        {
            uint8_t *rx_buffer = (uint8_t *)pios_malloc(PIOS_COM_TELEM_USB_RX_BUF_LEN);
            uint8_t *tx_buffer = (uint8_t *)pios_malloc(PIOS_COM_TELEM_USB_TX_BUF_LEN);
            PIOS_Assert(rx_buffer);
            PIOS_Assert(tx_buffer);
            if (PIOS_COM_Init(&pios_com_telem_usb_id, &pios_usb_hid_com_driver, pios_usb_hid_id,
                              rx_buffer, PIOS_COM_TELEM_USB_RX_BUF_LEN,
                              tx_buffer, PIOS_COM_TELEM_USB_TX_BUF_LEN)) {
                PIOS_Assert(0);
            }
        }
#endif /* PIOS_INCLUDE_COM */
        break;
    }

#endif /* PIOS_INCLUDE_USB_HID */

    if (usb_hid_present || usb_cdc_present) {
        PIOS_USBHOOK_Activate();
    }
#endif /* PIOS_INCLUDE_USB */

    /* Configure IO ports */
    uint8_t hwsettings_DSMxBind;
    HwSettingsDSMxBindGet(&hwsettings_DSMxBind);

    /* Configure Telemetry port */
    uint8_t hwsettings_rv_telemetryport;
    HwSettingsRV_TelemetryPortGet(&hwsettings_rv_telemetryport);

    switch (hwsettings_rv_telemetryport) {
    case HWSETTINGS_RV_TELEMETRYPORT_DISABLED:
        break;
    case HWSETTINGS_RV_TELEMETRYPORT_TELEMETRY:
        PIOS_Board_configure_com(&pios_usart_telem_cfg, PIOS_COM_TELEM_RF_RX_BUF_LEN, PIOS_COM_TELEM_RF_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_telem_rf_id);
        break;
    case HWSETTINGS_RV_TELEMETRYPORT_COMAUX:
        PIOS_Board_configure_com(&pios_usart_telem_cfg, PIOS_COM_AUX_RX_BUF_LEN, PIOS_COM_AUX_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_aux_id);
        break;
    case HWSETTINGS_RV_TELEMETRYPORT_COMBRIDGE:
        PIOS_Board_configure_com(&pios_usart_telem_cfg, PIOS_COM_BRIDGE_RX_BUF_LEN, PIOS_COM_BRIDGE_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_bridge_id);
        break;
    } /*        hwsettings_rv_telemetryport */

    /* Configure GPS port */
    uint8_t hwsettings_rv_gpsport;
    HwSettingsRV_GPSPortGet(&hwsettings_rv_gpsport);
    switch (hwsettings_rv_gpsport) {
    case HWSETTINGS_RV_GPSPORT_DISABLED:
        break;

    case HWSETTINGS_RV_GPSPORT_TELEMETRY:
        PIOS_Board_configure_com(&pios_usart_gps_cfg, PIOS_COM_TELEM_RF_RX_BUF_LEN, PIOS_COM_TELEM_RF_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_telem_rf_id);
        break;

    case HWSETTINGS_RV_GPSPORT_GPS:
        PIOS_Board_configure_com(&pios_usart_gps_cfg, PIOS_COM_GPS_RX_BUF_LEN, -1, &pios_usart_com_driver, &pios_com_gps_id);
        break;

    case HWSETTINGS_RV_GPSPORT_COMAUX:
        PIOS_Board_configure_com(&pios_usart_gps_cfg, PIOS_COM_AUX_RX_BUF_LEN, PIOS_COM_AUX_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_aux_id);
        break;

    case HWSETTINGS_RV_GPSPORT_COMBRIDGE:
        PIOS_Board_configure_com(&pios_usart_gps_cfg, PIOS_COM_BRIDGE_RX_BUF_LEN, PIOS_COM_BRIDGE_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_bridge_id);
        break;
    } /* hwsettings_rv_gpsport */

    /* Configure AUXPort */
    uint8_t hwsettings_rv_auxport;
    HwSettingsRV_AuxPortGet(&hwsettings_rv_auxport);

    switch (hwsettings_rv_auxport) {
    case HWSETTINGS_RV_AUXPORT_DISABLED:
        break;

    case HWSETTINGS_RV_AUXPORT_TELEMETRY:
        PIOS_Board_configure_com(&pios_usart_aux_cfg, PIOS_COM_TELEM_RF_RX_BUF_LEN, PIOS_COM_TELEM_RF_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_telem_rf_id);
        break;

    case HWSETTINGS_RV_AUXPORT_DSM:
        // TODO: Define the various Channelgroup for Revo dsm inputs and handle here
        PIOS_Board_configure_dsm(&pios_usart_dsm_aux_cfg, &pios_dsm_aux_cfg,
                                 &pios_usart_com_driver, MANUALCONTROLSETTINGS_CHANNELGROUPS_DSMMAINPORT, &hwsettings_DSMxBind);
        break;
    case HWSETTINGS_RV_AUXPORT_COMAUX:
        PIOS_Board_configure_com(&pios_usart_aux_cfg, PIOS_COM_AUX_RX_BUF_LEN, PIOS_COM_AUX_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_aux_id);
        break;
    case HWSETTINGS_RV_AUXPORT_COMBRIDGE:
        PIOS_Board_configure_com(&pios_usart_aux_cfg, PIOS_COM_BRIDGE_RX_BUF_LEN, PIOS_COM_BRIDGE_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_bridge_id);
        break;
    case HWSETTINGS_RV_AUXPORT_OSDHK:
        PIOS_Board_configure_com(&pios_usart_hkosd_aux_cfg, PIOS_COM_HKOSD_RX_BUF_LEN, PIOS_COM_HKOSD_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_hkosd_id);
        break;
    } /* hwsettings_rv_auxport */
      /* Configure AUXSbusPort */
      // TODO: ensure that the serial invertion pin is setted correctly
    uint8_t hwsettings_rv_auxsbusport;
    HwSettingsRV_AuxSBusPortGet(&hwsettings_rv_auxsbusport);

    switch (hwsettings_rv_auxsbusport) {
    case HWSETTINGS_RV_AUXSBUSPORT_DISABLED:
        break;
    case HWSETTINGS_RV_AUXSBUSPORT_SBUS:
#ifdef PIOS_INCLUDE_SBUS
        {
            uint32_t pios_usart_sbus_id;
            if (PIOS_USART_Init(&pios_usart_sbus_id, &pios_usart_sbus_auxsbus_cfg)) {
                PIOS_Assert(0);
            }

            uint32_t pios_sbus_id;
            if (PIOS_SBus_Init(&pios_sbus_id, &pios_sbus_cfg, &pios_usart_com_driver, pios_usart_sbus_id)) {
                PIOS_Assert(0);
            }

            uint32_t pios_sbus_rcvr_id;
            if (PIOS_RCVR_Init(&pios_sbus_rcvr_id, &pios_sbus_rcvr_driver, pios_sbus_id)) {
                PIOS_Assert(0);
            }
            pios_rcvr_group_map[MANUALCONTROLSETTINGS_CHANNELGROUPS_SBUS] = pios_sbus_rcvr_id;
        }
#endif /* PIOS_INCLUDE_SBUS */
        break;

    case HWSETTINGS_RV_AUXSBUSPORT_DSM:
        // TODO: Define the various Channelgroup for Revo dsm inputs and handle here
        PIOS_Board_configure_dsm(&pios_usart_dsm_auxsbus_cfg, &pios_dsm_auxsbus_cfg,
                                 &pios_usart_com_driver, MANUALCONTROLSETTINGS_CHANNELGROUPS_DSMMAINPORT, &hwsettings_DSMxBind);
        break;
    case HWSETTINGS_RV_AUXSBUSPORT_COMAUX:
        PIOS_Board_configure_com(&pios_usart_auxsbus_cfg, PIOS_COM_AUX_RX_BUF_LEN, PIOS_COM_AUX_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_aux_id);
        break;
    case HWSETTINGS_RV_AUXSBUSPORT_COMBRIDGE:
        PIOS_Board_configure_com(&pios_usart_auxsbus_cfg, PIOS_COM_BRIDGE_RX_BUF_LEN, PIOS_COM_BRIDGE_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_bridge_id);
        break;
    case HWSETTINGS_RV_AUXSBUSPORT_OSDHK:
        PIOS_Board_configure_com(&pios_usart_hkosd_auxsbus_cfg, PIOS_COM_HKOSD_RX_BUF_LEN, PIOS_COM_HKOSD_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_hkosd_id);
        break;
    } /* hwsettings_rv_auxport */

    /* Configure FlexiPort */

    uint8_t hwsettings_rv_flexiport;
    HwSettingsRV_FlexiPortGet(&hwsettings_rv_flexiport);

    switch (hwsettings_rv_flexiport) {
    case HWSETTINGS_RV_FLEXIPORT_DISABLED:
        break;
    case HWSETTINGS_RV_FLEXIPORT_I2C:
#if defined(PIOS_INCLUDE_I2C)
        {
            if (PIOS_I2C_Init(&pios_i2c_flexiport_adapter_id, &pios_i2c_flexiport_adapter_cfg)) {
                PIOS_Assert(0);
            }
        }
#endif /* PIOS_INCLUDE_I2C */
        break;

    case HWSETTINGS_RV_FLEXIPORT_DSM:
        // TODO: Define the various Channelgroup for Revo dsm inputs and handle here
        PIOS_Board_configure_dsm(&pios_usart_dsm_flexi_cfg, &pios_dsm_flexi_cfg,
                                 &pios_usart_com_driver, MANUALCONTROLSETTINGS_CHANNELGROUPS_DSMMAINPORT, &hwsettings_DSMxBind);
        break;
    case HWSETTINGS_RV_FLEXIPORT_COMAUX:
        PIOS_Board_configure_com(&pios_usart_flexi_cfg, PIOS_COM_AUX_RX_BUF_LEN, PIOS_COM_AUX_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_aux_id);
        break;
    case HWSETTINGS_RV_FLEXIPORT_COMBRIDGE:
        PIOS_Board_configure_com(&pios_usart_flexi_cfg, PIOS_COM_BRIDGE_RX_BUF_LEN, PIOS_COM_BRIDGE_TX_BUF_LEN, &pios_usart_com_driver, &pios_com_bridge_id);
        break;
    } /* hwsettings_rv_flexiport */


    /* Configure the receiver port*/
    uint8_t hwsettings_rcvrport;
    HwSettingsRV_RcvrPortGet(&hwsettings_rcvrport);
    //
    switch (hwsettings_rcvrport) {
    case HWSETTINGS_RV_RCVRPORT_DISABLED:
        break;
    case HWSETTINGS_RV_RCVRPORT_PWM:
#if defined(PIOS_INCLUDE_PWM)
        {
            /* Set up the receiver port.  Later this should be optional */
            uint32_t pios_pwm_id;
            PIOS_PWM_Init(&pios_pwm_id, &pios_pwm_cfg);

            uint32_t pios_pwm_rcvr_id;
            if (PIOS_RCVR_Init(&pios_pwm_rcvr_id, &pios_pwm_rcvr_driver, pios_pwm_id)) {
                PIOS_Assert(0);
            }
            pios_rcvr_group_map[MANUALCONTROLSETTINGS_CHANNELGROUPS_PWM] = pios_pwm_rcvr_id;
        }
#endif /* PIOS_INCLUDE_PWM */
        break;
    case HWSETTINGS_RV_RCVRPORT_PPM:
    case HWSETTINGS_RV_RCVRPORT_PPMOUTPUTS:
#if defined(PIOS_INCLUDE_PPM)
        {
            uint32_t pios_ppm_id;
            PIOS_PPM_Init(&pios_ppm_id, &pios_ppm_cfg);

            uint32_t pios_ppm_rcvr_id;
            if (PIOS_RCVR_Init(&pios_ppm_rcvr_id, &pios_ppm_rcvr_driver, pios_ppm_id)) {
                PIOS_Assert(0);
            }
            pios_rcvr_group_map[MANUALCONTROLSETTINGS_CHANNELGROUPS_PPM] = pios_ppm_rcvr_id;
        }
#endif /* PIOS_INCLUDE_PPM */
    case HWSETTINGS_RV_RCVRPORT_OUTPUTS:

        break;
    }

#if defined(PIOS_OVERO_SPI)
    /* Set up the SPI based PIOS_COM interface to the overo */
    {
        HwSettingsData hwSettings;
        HwSettingsGet(&hwSettings);
        if (hwSettings.OptionalModules.Overo == HWSETTINGS_OPTIONALMODULES_ENABLED) {
            if (PIOS_OVERO_Init(&pios_overo_id, &pios_overo_cfg)) {
                PIOS_DEBUG_Assert(0);
            }
            const uint32_t PACKET_SIZE = 1024;
            uint8_t *rx_buffer = (uint8_t *)pios_malloc(PACKET_SIZE);
            uint8_t *tx_buffer = (uint8_t *)pios_malloc(PACKET_SIZE);
            PIOS_Assert(rx_buffer);
            PIOS_Assert(tx_buffer);
            if (PIOS_COM_Init(&pios_com_overo_id, &pios_overo_com_driver, pios_overo_id,
                              rx_buffer, PACKET_SIZE,
                              tx_buffer, PACKET_SIZE)) {
                PIOS_Assert(0);
            }
        }
    }

#endif /* if defined(PIOS_OVERO_SPI) */

#if defined(PIOS_INCLUDE_HCSR04)
    {
        PIOS_TIM_InitClock(&tim_8_cfg);
        uint32_t pios_hcsr04_id;
        PIOS_HCSR04_Init(&pios_hcsr04_id, &pios_hcsr04_cfg);
    }
#endif


#if defined(PIOS_INCLUDE_GCSRCVR)
    GCSReceiverInitialize();
    uint32_t pios_gcsrcvr_id;
    PIOS_GCSRCVR_Init(&pios_gcsrcvr_id);
    uint32_t pios_gcsrcvr_rcvr_id;
    if (PIOS_RCVR_Init(&pios_gcsrcvr_rcvr_id, &pios_gcsrcvr_rcvr_driver, pios_gcsrcvr_id)) {
        PIOS_Assert(0);
    }
    pios_rcvr_group_map[MANUALCONTROLSETTINGS_CHANNELGROUPS_GCS] = pios_gcsrcvr_rcvr_id;
#endif /* PIOS_INCLUDE_GCSRCVR */

#ifndef PIOS_ENABLE_DEBUG_PINS
    switch (hwsettings_rcvrport) {
    case HWSETTINGS_RV_RCVRPORT_DISABLED:
    case HWSETTINGS_RV_RCVRPORT_PWM:
    case HWSETTINGS_RV_RCVRPORT_PPM:
        /* Set up the servo outputs */
        PIOS_Servo_Init(&pios_servo_cfg);
        break;
    case HWSETTINGS_RV_RCVRPORT_PPMOUTPUTS:
    case HWSETTINGS_RV_RCVRPORT_OUTPUTS:
        // PIOS_Servo_Init(&pios_servo_rcvr_cfg);
        // TODO: Prepare the configurations on board_hw_defs and handle here:
        PIOS_Servo_Init(&pios_servo_cfg);
        break;
    }
#else
    PIOS_DEBUG_Init(pios_tim_servoport_all_pins, NELEMENTS(pios_tim_servoport_all_pins));
#endif

    if (PIOS_I2C_Init(&pios_i2c_mag_adapter_id, &pios_i2c_mag_adapter_cfg)) {
        PIOS_DEBUG_Assert(0);
    }

    if (PIOS_I2C_Init(&pios_i2c_pressure_adapter_id, &pios_i2c_pressure_adapter_cfg)) {
        PIOS_DEBUG_Assert(0);
    }

    PIOS_DELAY_WaitmS(50);

#if defined(PIOS_INCLUDE_ADC)
    PIOS_ADC_Init(&pios_adc_cfg);
#endif

#if defined(PIOS_INCLUDE_HMC5X83)
    onboard_mag = PIOS_HMC5x83_Init(&pios_hmc5x83_cfg, pios_i2c_mag_adapter_id, 0);
    PIOS_HMC5x83_Register(onboard_mag);
#endif

#if defined(PIOS_INCLUDE_MS5611)
    PIOS_MS5611_Init(&pios_ms5611_cfg, pios_i2c_pressure_adapter_id);
    PIOS_MS5611_Register();
#endif

    switch (bdinfo->board_rev) {
    case 0x01:
#if defined(PIOS_INCLUDE_L3GD20)
        PIOS_Assert(0); // L3gd20 has not been ported to Sensor framework!!!
        PIOS_L3GD20_Init(pios_spi_gyro_id, 0, &pios_l3gd20_cfg);
        PIOS_Assert(PIOS_L3GD20_Test() == 0);
#endif
#if defined(PIOS_INCLUDE_BMA180)
        PIOS_Assert(0); // BMA180 has not been ported to Sensor framework!!!
        PIOS_BMA180_Init(pios_spi_accel_id, 0, &pios_bma180_cfg);
        PIOS_Assert(PIOS_BMA180_Test() == 0);
#endif
        break;
    case 0x02:
#if defined(PIOS_INCLUDE_MPU6000)
        PIOS_MPU6000_Init(pios_spi_gyro_id, 0, &pios_mpu6000_cfg);
        PIOS_MPU6000_CONFIG_Configure();
        PIOS_MPU6000_Register();
#endif
        break;
    default:
        PIOS_DEBUG_Assert(0);
    }
#ifdef PIOS_INCLUDE_ADC
    {
        uint8_t adc_config[HWSETTINGS_ADCROUTING_NUMELEM];
        HwSettingsADCRoutingArrayGet(adc_config);
        for (uint32_t i = 0; i < HWSETTINGS_ADCROUTING_NUMELEM; i++) {
            if (adc_config[i] != HWSETTINGS_ADCROUTING_DISABLED) {
                PIOS_ADC_PinSetup(i);
            }
        }
    }
#endif // PIOS_INCLUDE_ADC
}

/**
 * @}
 * @}
 */
