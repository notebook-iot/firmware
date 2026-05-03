#pragma once

#include <stdint.h>
#include <stddef.h>

typedef void (*i2c_data_callback_t)(const uint8_t *data, size_t len);

/**
 * @brief Initialize and start the I2C slave.
 * 
 * @param callback Callback to be invoked when data is received.
 */
void start_slave(i2c_data_callback_t callback);
