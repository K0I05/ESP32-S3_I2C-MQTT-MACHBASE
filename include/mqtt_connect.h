/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2024 Eric Gionet (gionet.c.eric@gmail.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file modbus_connect.h
 *
 * MQTT connection libary
 *
 * Copyright (c) 2024 Eric Gionet (gionet.c.eric@gmail.com)
 *
 * MIT Licensed as described in the file LICENSE
 */
#ifndef __MQTT_CONNECT_H__
#define __MQTT_CONNECT_H__

#include <stdint.h>
#include <mqtt_client.h>
#include <freertos/queue.h>

#ifdef __cplusplus
extern "C" {
#endif

extern volatile bool            mqtt_connected;
extern esp_mqtt_client_handle_t mqtt_client_hdl;

/**
 * @brief Starts the MQTT services.  This function should only be called once connected 
 * to an IP network.  This is a blocking function that waits for event bits to be initialized 
 * based on MQTT event results or it returns an error when the timeout period has elapsed. 
 * 
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t mqtt_start(void);

/**
 * @brief Stops the MQTT services.  Do not use this function within the MQTT event handler.
 * 
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t mqtt_stop(void);



#ifdef __cplusplus
}
#endif

#endif // __MQTT_CONNECT_H__