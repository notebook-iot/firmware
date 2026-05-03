#pragma once

/**
 * @file config.h
 * @brief Global hardware and software configuration for the firmware.
 */

/** @brief I2C port used for the slave device (0 or 1). */
#define I2C_SLAVE_PORT 0

/** @brief GPIO number for I2C Slave Serial Data (SDA). */
#define I2C_SLAVE_SDA_IO 3

/** @brief GPIO number for I2C Slave Serial Clock (SCL). */
#define I2C_SLAVE_SCL_IO 4

/** @brief I2C address of this device when acting as a slave. */
#define I2C_SLAVE_ADDR 0x12

/** @brief Size of the RX buffer for incoming I2C data (bytes). */
#define RX_BUFFER_LEN 1024
