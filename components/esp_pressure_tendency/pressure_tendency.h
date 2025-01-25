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
 * @file pressure_tendency.h
 *
 * Air pressure tendency libary
 * 
 * A air pressure tendency appears after three (3) hours of operation. The tendency 
 * codes and change are based on the 3-hr variance from the previous 3-hr history.
 *
 * Copyright (c) 2024 Eric Gionet (gionet.c.eric@gmail.com)
 *
 * MIT Licensed as described in the file LICENSE
 */
#ifndef __PRESSURE_TENDENCY_H__
#define __PRESSURE_TENDENCY_H__

#include <stdio.h>
#include <esp_check.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Pressure tendency codes enumerator.
 */
typedef enum pressure_tendency_codes_tag { /*!< 3-hr Change */
    PRESSURE_TENDENCY_CODE_UNKNOWN   = 1,  /*!< unknown */
    PRESSURE_TENDENCY_CODE_RISING    = 2,  /*!< rising  */
    PRESSURE_TENDENCY_CODE_STEADY    = 3,  /*!< steady  */
    PRESSURE_TENDENCY_CODE_FALLING   = 4   /*!< falling  */
} pressure_tendency_codes_t;

/**
 * @brief Pressure tendency structure.
 */
struct pressure_tendency_t {
    uint16_t    samples_count; /*!< pressure tendency samples count, state machine variable */
    uint16_t    samples_size;  /*!< pressure tendency samples size, state machine variable */
    float*      samples;       /*!< pressure tendency samples array, state machine variable */
};

/**
 * @brief Pressure tendency type definition.
 */
typedef struct pressure_tendency_t pressure_tendency_t;

/**
 * @brief Pressure tendency handle definition.
 */
typedef struct pressure_tendency_t *pressure_tendency_handle_t;

/**
 * @brief Converts `pressure_tendency_codes_t` enumerator pressure tendency code to string.
 * 
 * @param code Pressure tendency code to convert to a string.
 * @return const char* String representation of the pressure tendency code.
 */
const char* pressure_tendency_code_to_string(const pressure_tendency_codes_t code);

/**
 * @brief Initializes a pressure tendency handle by size of the 3-hr samples 
 * to analyze.  The size of the samples is calculated from the sampling rate.  
 * As an example, if the sampling rate is once every minute, the 
 * size of the samples buffer should be 180 e.g., three (3) hours.
 * 
 * @param samples_size Pressure tendency samples buffer size. 
 * @param pressure_tendency_handle Pressure tendency handle.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t pressure_tendency_init(const uint16_t samples_size, 
                            pressure_tendency_handle_t *pressure_tendency_handle);
/**
 * @brief Analyzes historical samples and pressure tendency appears after three 
 * (3) hours of operation. The pressure tendency are based the 3-hr variance of 
 * the previous 3-hour history.
 * 
 * @param pressure_tendency_handle Pressure tendency handle.
 * @param sample Air pressure sample, in millibars or hecto-pascal, to push onto the samples stack.
 * @param code Pressure tendency code of three (3) hour analysis.  Pressure
 * tendency code `PRESSURE_TENDENCY_UNKNOWN` is reported when there is an 
 * insufficient number of samples to analyze.
 * @param change Pressure tendency variance over the past three (3) hours.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t pressure_tendency_analysis(pressure_tendency_handle_t pressure_tendency_handle, 
                                const float sample, 
                                pressure_tendency_codes_t *const code,
                                float *const change);

/**
 * @brief Purges pressure tendency samples array and resets samples counter.
 * 
 * @param pressure_tendency_handle Pressure tendency handle
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t pressure_tendency_reset(pressure_tendency_handle_t pressure_tendency_handle);

/**
 * @brief Frees pressure tendency handle.
 * 
 * @param pressure_tendency_handle Pressure tendency handle.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t pressure_tendency_del(pressure_tendency_handle_t pressure_tendency_handle);


#ifdef __cplusplus
}
#endif

#endif // __PRESSURE_TENDENCY_H__