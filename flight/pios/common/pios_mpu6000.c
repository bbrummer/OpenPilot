/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_MPU6000 MPU6000 Functions
 * @brief Deals with the hardware interface to the 3-axis gyro
 * @{
 *
 * @file       pios_mpu000.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @brief      MPU6000 6-axis gyro and accel chip
 * @see        The GNU Public License (GPL) Version 3
 *
 ******************************************************************************
 */
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

#include "pios.h"

#ifdef PIOS_INCLUDE_MPU6000

#include <pios_constants.h>
/* Global Variables */

enum pios_mpu6000_dev_magic {
    PIOS_MPU6000_DEV_MAGIC = 0x9da9b3ed,
};

struct mpu6000_dev {
    uint32_t     spi_id;
    uint32_t     slave_num;
    xQueueHandle queue;
    const struct pios_mpu6000_cfg *cfg;
    enum pios_mpu6000_range gyro_range;
    enum pios_mpu6000_accel_range accel_range;
    enum pios_mpu6000_filter filter;
    enum pios_mpu6000_dev_magic   magic;
};

#ifdef PIOS_MPU6000_ACCEL
#define PIOS_MPU6000_SAMPLES_BYTES    14
#define PIOS_MPU6000_SENSOR_FIRST_REG PIOS_MPU6000_ACCEL_X_OUT_MSB
#else
#define PIOS_MPU6000_SENSOR_FIRST_REG PIOS_MPU6000_TEMP_OUT_MSB
#define PIOS_MPU6000_SAMPLES_BYTES    8
#endif

typedef union {
    uint8_t buffer[1 + PIOS_MPU6000_SAMPLES_BYTES];
    struct {
        uint8_t dummy;
#ifdef PIOS_MPU6000_ACCEL
        uint8_t Accel_X_h;
        uint8_t Accel_X_l;
        uint8_t Accel_Y_h;
        uint8_t Accel_Y_l;
        uint8_t Accel_Z_h;
        uint8_t Accel_Z_l;
#endif
        uint8_t Temperature_h;
        uint8_t Temperature_l;
        uint8_t Gyro_X_h;
        uint8_t Gyro_X_l;
        uint8_t Gyro_Y_h;
        uint8_t Gyro_Y_l;
        uint8_t Gyro_Z_h;
        uint8_t Gyro_Z_l;
    } data;
} mpu6000_data_t;

#define GET_SENSOR_DATA(mpudataptr, sensor) (mpudataptr.data.sensor##_h << 8 | mpudataptr.data.sensor##_l)

// ! Global structure for this device device
static struct mpu6000_dev *dev;
volatile bool mpu6000_configured = false;
static mpu6000_data_t mpu6000_data;

// ! Private functions
static struct mpu6000_dev *PIOS_MPU6000_alloc(const struct pios_mpu6000_cfg *cfg);
static int32_t PIOS_MPU6000_Validate(struct mpu6000_dev *dev);
static void PIOS_MPU6000_Config(struct pios_mpu6000_cfg const *cfg);
static int32_t PIOS_MPU6000_SetReg(uint8_t address, uint8_t buffer);
static int32_t PIOS_MPU6000_GetReg(uint8_t address);
static void PIOS_MPU6000_SetSpeed(const bool fast);
static bool PIOS_MPU6000_HandleData();
static bool PIOS_MPU6000_ReadFifo(bool *woken);
static bool PIOS_MPU6000_ReadSensor(bool *woken);
/**
 * @brief Allocate a new device
 */
static struct mpu6000_dev *PIOS_MPU6000_alloc(const struct pios_mpu6000_cfg *cfg)
{
    struct mpu6000_dev *mpu6000_dev;

    mpu6000_dev = (struct mpu6000_dev *)pios_malloc(sizeof(*mpu6000_dev));
    if (!mpu6000_dev) {
        return NULL;
    }

    mpu6000_dev->magic = PIOS_MPU6000_DEV_MAGIC;

    mpu6000_dev->queue = xQueueCreate(cfg->max_downsample + 1, sizeof(struct pios_mpu6000_data));
    if (mpu6000_dev->queue == NULL) {
        vPortFree(mpu6000_dev);
        return NULL;
    }

    return mpu6000_dev;
}

/**
 * @brief Validate the handle to the spi device
 * @returns 0 for valid device or -1 otherwise
 */
static int32_t PIOS_MPU6000_Validate(struct mpu6000_dev *vdev)
{
    if (vdev == NULL) {
        return -1;
    }
    if (vdev->magic != PIOS_MPU6000_DEV_MAGIC) {
        return -2;
    }
    if (vdev->spi_id == 0) {
        return -3;
    }
    return 0;
}

/**
 * @brief Initialize the MPU6000 3-axis gyro sensor.
 * @return 0 for success, -1 for failure
 */
int32_t PIOS_MPU6000_Init(uint32_t spi_id, uint32_t slave_num, const struct pios_mpu6000_cfg *cfg)
{
    dev = PIOS_MPU6000_alloc(cfg);
    if (dev == NULL) {
        return -1;
    }

    dev->spi_id    = spi_id;
    dev->slave_num = slave_num;
    dev->cfg = cfg;

    /* Configure the MPU6000 Sensor */
    PIOS_MPU6000_Config(cfg);

    /* Set up EXTI line */
    PIOS_EXTI_Init(cfg->exti_cfg);
    return 0;
}

/**
 * @brief Initialize the MPU6000 3-axis gyro sensor
 * \return none
 * \param[in] PIOS_MPU6000_ConfigTypeDef struct to be used to configure sensor.
 *
 */
static void PIOS_MPU6000_Config(struct pios_mpu6000_cfg const *cfg)
{
    PIOS_MPU6000_Test();

    // Reset chip
    while (PIOS_MPU6000_SetReg(PIOS_MPU6000_PWR_MGMT_REG, PIOS_MPU6000_PWRMGMT_IMU_RST) != 0) {
        ;
    }
    PIOS_DELAY_WaitmS(50);

    // Reset chip and fifo
    while (PIOS_MPU6000_SetReg(PIOS_MPU6000_USER_CTRL_REG,
                               PIOS_MPU6000_USERCTL_GYRO_RST |
                               PIOS_MPU6000_USERCTL_SIG_COND |
                               PIOS_MPU6000_USERCTL_FIFO_RST) != 0) {
        ;
    }

    // Wait for reset to finish
    while (PIOS_MPU6000_GetReg(PIOS_MPU6000_USER_CTRL_REG) &
           (PIOS_MPU6000_USERCTL_GYRO_RST |
            PIOS_MPU6000_USERCTL_SIG_COND |
            PIOS_MPU6000_USERCTL_FIFO_RST)) {
        ;
    }
    PIOS_DELAY_WaitmS(10);
    // Power management configuration
    while (PIOS_MPU6000_SetReg(PIOS_MPU6000_PWR_MGMT_REG, cfg->Pwr_mgmt_clk) != 0) {
        ;
    }

    // Interrupt configuration
    while (PIOS_MPU6000_SetReg(PIOS_MPU6000_INT_CFG_REG, cfg->interrupt_cfg) != 0) {
        ;
    }

    // Interrupt configuration
    while (PIOS_MPU6000_SetReg(PIOS_MPU6000_INT_EN_REG, cfg->interrupt_en) != 0) {
        ;
    }

    // FIFO storage
#if defined(PIOS_MPU6000_ACCEL)
    while (PIOS_MPU6000_SetReg(PIOS_MPU6000_FIFO_EN_REG, cfg->Fifo_store | PIOS_MPU6000_ACCEL_OUT) != 0) {
        ;
    }
#else
    while (PIOS_MPU6000_SetReg(PIOS_MPU6000_FIFO_EN_REG, cfg->Fifo_store) != 0) {
        ;
    }
#endif
    PIOS_MPU6000_ConfigureRanges(cfg->gyro_range, cfg->accel_range, cfg->filter);
    // Interrupt configuration
    while (PIOS_MPU6000_SetReg(PIOS_MPU6000_USER_CTRL_REG, cfg->User_ctl) != 0) {
        ;
    }

    // Interrupt configuration
    while (PIOS_MPU6000_SetReg(PIOS_MPU6000_PWR_MGMT_REG, cfg->Pwr_mgmt_clk) != 0) {
        ;
    }

    // Interrupt configuration
    while (PIOS_MPU6000_SetReg(PIOS_MPU6000_INT_CFG_REG, cfg->interrupt_cfg) != 0) {
        ;
    }

    // Interrupt configuration
    while (PIOS_MPU6000_SetReg(PIOS_MPU6000_INT_EN_REG, cfg->interrupt_en) != 0) {
        ;
    }
    if ((PIOS_MPU6000_GetReg(PIOS_MPU6000_INT_EN_REG)) != cfg->interrupt_en) {
        return;
    }

    mpu6000_configured = true;
}
/**
 * @brief Configures Gyro, accel and Filter ranges/setings
 * @return 0 if successful, -1 if device has not been initialized
 */
int32_t PIOS_MPU6000_ConfigureRanges(
    enum pios_mpu6000_range gyroRange,
    enum pios_mpu6000_accel_range accelRange,
    enum pios_mpu6000_filter filterSetting)
{
    if (dev == NULL) {
        return -1;
    }

    // update filter settings
    while (PIOS_MPU6000_SetReg(PIOS_MPU6000_DLPF_CFG_REG, filterSetting) != 0) {
        ;
    }

    // Sample rate divider, chosen upon digital filtering settings
    while (PIOS_MPU6000_SetReg(PIOS_MPU6000_SMPLRT_DIV_REG,
                               filterSetting == PIOS_MPU6000_LOWPASS_256_HZ ?
                               dev->cfg->Smpl_rate_div_no_dlp : dev->cfg->Smpl_rate_div_dlp) != 0) {
        ;
    }

    dev->filter = filterSetting;

    // Gyro range
    while (PIOS_MPU6000_SetReg(PIOS_MPU6000_GYRO_CFG_REG, gyroRange) != 0) {
        ;
    }

    dev->gyro_range = gyroRange;
#if defined(PIOS_MPU6000_ACCEL)
    // Set the accel range
    while (PIOS_MPU6000_SetReg(PIOS_MPU6000_ACCEL_CFG_REG, accelRange) != 0) {
        ;
    }

    dev->accel_range = accelRange;
#endif
    return 0;
}

/**
 * @brief Claim the SPI bus for the accel communications and select this chip
 * @return 0 if successful, -1 for invalid device, -2 if unable to claim bus
 */
static int32_t PIOS_MPU6000_ClaimBus(bool fast_spi)
{
    if (PIOS_MPU6000_Validate(dev) != 0) {
        return -1;
    }
    if (PIOS_SPI_ClaimBus(dev->spi_id) != 0) {
        return -2;
    }
    PIOS_MPU6000_SetSpeed(fast_spi);
    PIOS_SPI_RC_PinSet(dev->spi_id, dev->slave_num, 0);
    return 0;
}


static void PIOS_MPU6000_SetSpeed(const bool fast)
{
    if (fast) {
        PIOS_SPI_SetClockSpeed(dev->spi_id, dev->cfg->fast_prescaler);
    } else {
        PIOS_SPI_SetClockSpeed(dev->spi_id, dev->cfg->std_prescaler);
    }
}

/**
 * @brief Claim the SPI bus for the accel communications and select this chip
 * @return 0 if successful, -1 for invalid device, -2 if unable to claim bus
 * @param woken[in,out] If non-NULL, will be set to true if woken was false and a higher priority
 *                      task has is now eligible to run, else unchanged
 */
static int32_t PIOS_MPU6000_ClaimBusISR(bool *woken, bool fast_spi)
{
    if (PIOS_MPU6000_Validate(dev) != 0) {
        return -1;
    }
    if (PIOS_SPI_ClaimBusISR(dev->spi_id, woken) != 0) {
        return -2;
    }
    PIOS_MPU6000_SetSpeed(fast_spi);
    PIOS_SPI_RC_PinSet(dev->spi_id, dev->slave_num, 0);
    return 0;
}

/**
 * @brief Release the SPI bus for the accel communications and end the transaction
 * @return 0 if successful
 */
static int32_t PIOS_MPU6000_ReleaseBus()
{
    if (PIOS_MPU6000_Validate(dev) != 0) {
        return -1;
    }
    PIOS_SPI_RC_PinSet(dev->spi_id, dev->slave_num, 1);
    return PIOS_SPI_ReleaseBus(dev->spi_id);
}

/**
 * @brief Release the SPI bus for the accel communications and end the transaction
 * @return 0 if successful
 * @param woken[in,out] If non-NULL, will be set to true if woken was false and a higher priority
 *                      task has is now eligible to run, else unchanged
 */
static int32_t PIOS_MPU6000_ReleaseBusISR(bool *woken)
{
    if (PIOS_MPU6000_Validate(dev) != 0) {
        return -1;
    }
    PIOS_SPI_RC_PinSet(dev->spi_id, dev->slave_num, 1);
    return PIOS_SPI_ReleaseBusISR(dev->spi_id, woken);
}

/**
 * @brief Read a register from MPU6000
 * @returns The register value or -1 if failure to get bus
 * @param reg[in] Register address to be read
 */
static int32_t PIOS_MPU6000_GetReg(uint8_t reg)
{
    uint8_t data;

    if (PIOS_MPU6000_ClaimBus(false) != 0) {
        return -1;
    }

    PIOS_SPI_TransferByte(dev->spi_id, (0x80 | reg)); // request byte
    data = PIOS_SPI_TransferByte(dev->spi_id, 0); // receive response

    PIOS_MPU6000_ReleaseBus();
    return data;
}

/**
 * @brief Writes one byte to the MPU6000
 * \param[in] reg Register address
 * \param[in] data Byte to write
 * \return 0 if operation was successful
 * \return -1 if unable to claim SPI bus
 * \return -2 if unable to claim i2c device
 */
static int32_t PIOS_MPU6000_SetReg(uint8_t reg, uint8_t data)
{
    if (PIOS_MPU6000_ClaimBus(false) != 0) {
        return -1;
    }

    if (PIOS_SPI_TransferByte(dev->spi_id, 0x7f & reg) != 0) {
        PIOS_MPU6000_ReleaseBus();
        return -2;
    }

    if (PIOS_SPI_TransferByte(dev->spi_id, data) != 0) {
        PIOS_MPU6000_ReleaseBus();
        return -3;
    }

    PIOS_MPU6000_ReleaseBus();

    return 0;
}

/**
 * @brief Read current X, Z, Y values (in that order)
 * \param[out] int16_t array of size 3 to store X, Z, and Y magnetometer readings
 * \returns 0 if succesful
 */
int32_t PIOS_MPU6000_ReadGyros(struct pios_mpu6000_data *data)
{
    // THIS FUNCTION IS DEPRECATED AND DOES NOT PERFORM A ROTATION
    uint8_t buf[7] = { PIOS_MPU6000_GYRO_X_OUT_MSB | 0x80, 0, 0, 0, 0, 0, 0 };
    uint8_t rec[7];

    if (PIOS_MPU6000_ClaimBus(true) != 0) {
        return -1;
    }

    if (PIOS_SPI_TransferBlock(dev->spi_id, &buf[0], &rec[0], sizeof(buf), NULL) < 0) {
        return -2;
    }

    PIOS_MPU6000_ReleaseBus();

    data->gyro_x = rec[1] << 8 | rec[2];
    data->gyro_y = rec[3] << 8 | rec[4];
    data->gyro_z = rec[5] << 8 | rec[6];

    return 0;
}

/*
 * @brief Read the identification bytes from the MPU6000 sensor
 * \return ID read from MPU6000 or -1 if failure
 */
int32_t PIOS_MPU6000_ReadID()
{
    int32_t mpu6000_id = PIOS_MPU6000_GetReg(PIOS_MPU6000_WHOAMI);

    if (mpu6000_id < 0) {
        return -1;
    }
    return mpu6000_id;
}

/**
 * \brief Reads the queue handle
 * \return Handle to the queue or null if invalid device
 */
xQueueHandle PIOS_MPU6000_GetQueue()
{
    if (PIOS_MPU6000_Validate(dev) != 0) {
        return (xQueueHandle)NULL;
    }

    return dev->queue;
}


float PIOS_MPU6000_GetScale()
{
    switch (dev->gyro_range) {
    case PIOS_MPU6000_SCALE_250_DEG:
        return 1.0f / 131.0f;

    case PIOS_MPU6000_SCALE_500_DEG:
        return 1.0f / 65.5f;

    case PIOS_MPU6000_SCALE_1000_DEG:
        return 1.0f / 32.8f;

    case PIOS_MPU6000_SCALE_2000_DEG:
        return 1.0f / 16.4f;
    }
    return 0;
}

float PIOS_MPU6000_GetAccelScale()
{
    switch (dev->accel_range) {
    case PIOS_MPU6000_ACCEL_2G:
        return PIOS_CONST_MKS_GRAV_ACCEL_F / 16384.0f;

    case PIOS_MPU6000_ACCEL_4G:
        return PIOS_CONST_MKS_GRAV_ACCEL_F / 8192.0f;

    case PIOS_MPU6000_ACCEL_8G:
        return PIOS_CONST_MKS_GRAV_ACCEL_F / 4096.0f;

    case PIOS_MPU6000_ACCEL_16G:
        return PIOS_CONST_MKS_GRAV_ACCEL_F / 2048.0f;
    }
    return 0;
}

/**
 * @brief Run self-test operation.
 * \return 0 if test succeeded
 * \return non-zero value if test succeeded
 */
int32_t PIOS_MPU6000_Test(void)
{
    /* Verify that ID matches (MPU6000 ID is 0x69) */
    int32_t mpu6000_id = PIOS_MPU6000_ReadID();

    if (mpu6000_id < 0) {
        return -1;
    }

    if (mpu6000_id != 0x68) {
        return -2;
    }

    return 0;
}

/**
 * @brief Reads the contents of the MPU6000 Interrupt Status register from an ISR
 * @return The register value or -1 on failure to claim the bus
 */
static int32_t PIOS_MPU6000_GetInterruptStatusRegISR(bool *woken)
{
    /* Interrupt Status register can be read at high SPI clock speed */
    uint8_t data;

    if (PIOS_MPU6000_ClaimBusISR(woken, false) != 0) {
        return -1;
    }
    PIOS_SPI_TransferByte(dev->spi_id, (0x80 | PIOS_MPU6000_INT_STATUS_REG));
    data = PIOS_SPI_TransferByte(dev->spi_id, 0);
    PIOS_MPU6000_ReleaseBusISR(woken);
    return data;
}

/**
 * @brief Resets the MPU6000 FIFO from an ISR
 * @param woken[in,out] If non-NULL, will be set to true if woken was false and a higher priority
 *                      task has is now eligible to run, else unchanged
 * @return  0 if operation was successful
 * @return -1 if unable to claim SPI bus
 * @return -2 if write to the device failed
 */
static int32_t PIOS_MPU6000_ResetFifoISR(bool *woken)
{
    int32_t result = 0;

    if (PIOS_MPU6000_ClaimBusISR(woken, false) != 0) {
        return -1;
    }
    /* Reset FIFO. */
    if (PIOS_SPI_TransferByte(dev->spi_id, 0x7f & PIOS_MPU6000_USER_CTRL_REG) != 0) {
        result = -2;
    } else if (PIOS_SPI_TransferByte(dev->spi_id, (dev->cfg->User_ctl | PIOS_MPU6000_USERCTL_FIFO_RST)) != 0) {
        result = -2;
    }
    PIOS_MPU6000_ReleaseBusISR(woken);
    return result;
}

/**
 * @brief Obtains the number of bytes in the FIFO. Call from ISR only.
 * @return the number of bytes in the FIFO
 * @param woken[in,out] If non-NULL, will be set to true if woken was false and a higher priority
 *                      task has is now eligible to run, else unchanged
 */
static int32_t PIOS_MPU6000_FifoDepthISR(bool *woken)
{
    uint8_t mpu6000_send_buf[3] = { PIOS_MPU6000_FIFO_CNT_MSB | 0x80, 0, 0 };
    uint8_t mpu6000_rec_buf[3];

    if (PIOS_MPU6000_ClaimBusISR(woken, false) != 0) {
        return -1;
    }

    if (PIOS_SPI_TransferBlock(dev->spi_id, &mpu6000_send_buf[0], &mpu6000_rec_buf[0], sizeof(mpu6000_send_buf), NULL) < 0) {
        PIOS_MPU6000_ReleaseBusISR(woken);
        return -1;
    }

    PIOS_MPU6000_ReleaseBusISR(woken);

    return (mpu6000_rec_buf[1] << 8) | mpu6000_rec_buf[2];
}

/**
 * @brief EXTI IRQ Handler.  Read all the data from onboard buffer
 * @return a boolean to the EXTI IRQ Handler wrapper indicating if a
 *         higher priority task is now eligible to run
 */
uint32_t mpu6000_irq = 0;
int32_t mpu6000_count;
uint32_t mpu6000_fifo_backup    = 0;

uint8_t mpu6000_last_read_count = 0;
uint32_t mpu6000_fails = 0;

uint32_t mpu6000_interval_us;
uint32_t mpu6000_time_us;
uint32_t mpu6000_transfer_size;

bool PIOS_MPU6000_IRQHandler(void)
{
    bool woken = false;
    static uint32_t timeval;

    mpu6000_interval_us = PIOS_DELAY_DiffuS(timeval);
    timeval = PIOS_DELAY_GetRaw();

    if (!mpu6000_configured) {
        return false;
    }

    bool read_ok = false;
    if (dev->cfg->User_ctl & PIOS_MPU6000_USERCTL_FIFO_EN) {
        read_ok = PIOS_MPU6000_ReadFifo(&woken);
    } else {
        read_ok = PIOS_MPU6000_ReadSensor(&woken);
    }
    if (read_ok) {
        bool woken2 = PIOS_MPU6000_HandleData();
        woken |= woken2;
    }

    mpu6000_irq++;

    mpu6000_time_us = PIOS_DELAY_DiffuS(timeval);

    return woken;
}

static bool PIOS_MPU6000_HandleData()
{
    // Rotate the sensor to OP convention.  The datasheet defines X as towards the right
    // and Y as forward.  OP convention transposes this.  Also the Z is defined negatively
    // to our convention

    static struct pios_mpu6000_data data;

    // Currently we only support rotations on top so switch X/Y accordingly
    switch (dev->cfg->orientation) {
    case PIOS_MPU6000_TOP_0DEG:
#ifdef PIOS_MPU6000_ACCEL
        data.accel_y = GET_SENSOR_DATA(mpu6000_data, Accel_X); // chip X
        data.accel_x = GET_SENSOR_DATA(mpu6000_data, Accel_Y); // chip Y
#endif
        data.gyro_y  = GET_SENSOR_DATA(mpu6000_data, Gyro_X); // chip X
        data.gyro_x  = GET_SENSOR_DATA(mpu6000_data, Gyro_Y); // chip Y
        break;
    case PIOS_MPU6000_TOP_90DEG:
        // -1 to bring it back to -32768 +32767 range
#ifdef PIOS_MPU6000_ACCEL
        data.accel_y = -1 - (GET_SENSOR_DATA(mpu6000_data, Accel_Y)); // chip Y
        data.accel_x = GET_SENSOR_DATA(mpu6000_data, Accel_X); // chip X
#endif
        data.gyro_y  = -1 - (GET_SENSOR_DATA(mpu6000_data, Gyro_Y)); // chip Y
        data.gyro_x  = GET_SENSOR_DATA(mpu6000_data, Gyro_X); // chip X
        break;
    case PIOS_MPU6000_TOP_180DEG:
#ifdef PIOS_MPU6000_ACCEL
        data.accel_y = -1 - (GET_SENSOR_DATA(mpu6000_data, Accel_X)); // chip X
        data.accel_x = -1 - (GET_SENSOR_DATA(mpu6000_data, Accel_Y)); // chip Y
#endif
        data.gyro_y  = -1 - (GET_SENSOR_DATA(mpu6000_data, Gyro_X)); // chip X
        data.gyro_x  = -1 - (GET_SENSOR_DATA(mpu6000_data, Gyro_Y)); // chip Y
        break;
    case PIOS_MPU6000_TOP_270DEG:
#ifdef PIOS_MPU6000_ACCEL
        data.accel_y = GET_SENSOR_DATA(mpu6000_data, Accel_Y); // chip Y
        data.accel_x = -1 - (GET_SENSOR_DATA(mpu6000_data, Accel_X)); // chip X
#endif
        data.gyro_y  = GET_SENSOR_DATA(mpu6000_data, Gyro_Y); // chip Y
        data.gyro_x  = -1 - (GET_SENSOR_DATA(mpu6000_data, Gyro_X)); // chip X
        break;
    }
#ifdef PIOS_MPU6000_ACCEL
    data.accel_z     = -1 - (GET_SENSOR_DATA(mpu6000_data, Accel_Z));
#endif
    data.gyro_z      = -1 - (GET_SENSOR_DATA(mpu6000_data, Gyro_Z));
    data.temperature = GET_SENSOR_DATA(mpu6000_data, Temperature);

    BaseType_t higherPriorityTaskWoken;
    xQueueSendToBackFromISR(dev->queue, (void *)&data, &higherPriorityTaskWoken);
    return higherPriorityTaskWoken == pdTRUE;
}

static bool PIOS_MPU6000_ReadSensor(bool *woken)
{
    const uint8_t mpu6000_send_buf[1 + PIOS_MPU6000_SAMPLES_BYTES] = { PIOS_MPU6000_SENSOR_FIRST_REG | 0x80 };

    if (PIOS_MPU6000_ClaimBusISR(woken, true) != 0) {
        return false;
    }
    if (PIOS_SPI_TransferBlock(dev->spi_id, &mpu6000_send_buf[0], &mpu6000_data.buffer[0], sizeof(mpu6000_data_t), NULL) < 0) {
        PIOS_MPU6000_ReleaseBusISR(woken);
        mpu6000_fails++;
        return false;
    }
    PIOS_MPU6000_ReleaseBusISR(woken);
    return true;
}

static bool PIOS_MPU6000_ReadFifo(bool *woken)
{
    /* Temporary fix for OP-1049. Expected to be superceded for next major release
     * by code changes for OP-1039.
     * Read interrupt status register to check for FIFO overflow. Must be the
     * first read after interrupt, in case the device is configured so that
     * any read clears in the status register (PIOS_MPU6000_INT_CLR_ANYRD set in
     * interrupt config register) */
    int32_t result;

    if ((result = PIOS_MPU6000_GetInterruptStatusRegISR(woken)) < 0) {
        return false;
    }
    if (result & PIOS_MPU6000_INT_STATUS_FIFO_OVERFLOW) {
        /* The FIFO has overflowed, so reset it,
         * to enable sample sync to be recovered.
         * If the reset fails, we are in trouble, but
         * we keep trying on subsequent interrupts. */
        PIOS_MPU6000_ResetFifoISR(woken);
        /* Return and wait for the next new sample. */
        return false;
    }

    /* Usual case - FIFO has not overflowed. */
    mpu6000_count = PIOS_MPU6000_FifoDepthISR(woken);
    if (mpu6000_count < PIOS_MPU6000_SAMPLES_BYTES) {
        return false;
    }

    if (PIOS_MPU6000_ClaimBusISR(woken, true) != 0) {
        return false;
    }

    const uint8_t mpu6000_send_buf[1 + PIOS_MPU6000_SAMPLES_BYTES] = { PIOS_MPU6000_FIFO_REG | 0x80 };

    if (PIOS_SPI_TransferBlock(dev->spi_id, &mpu6000_send_buf[0], &mpu6000_data.buffer[0], sizeof(mpu6000_data_t), NULL) < 0) {
        PIOS_MPU6000_ReleaseBusISR(woken);
        mpu6000_fails++;
        return false;
    }

    PIOS_MPU6000_ReleaseBusISR(woken);

    // In the case where extras samples backed up grabbed an extra
    if (mpu6000_count >= PIOS_MPU6000_SAMPLES_BYTES * 2) {
        mpu6000_fifo_backup++;
        if (PIOS_MPU6000_ClaimBusISR(woken, true) != 0) {
            return false;
        }

        if (PIOS_SPI_TransferBlock(dev->spi_id, &mpu6000_send_buf[0], &mpu6000_data.buffer[0], sizeof(mpu6000_data_t), NULL) < 0) {
            PIOS_MPU6000_ReleaseBusISR(woken);
            mpu6000_fails++;
            return false;
        }

        PIOS_MPU6000_ReleaseBusISR(woken);
    }
    return true;
}
#endif /* PIOS_INCLUDE_MPU6000 */

/**
 * @}
 * @}
 */
