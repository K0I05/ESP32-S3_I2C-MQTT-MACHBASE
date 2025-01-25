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
 * @file pressure_tendency.c
 *
 * Air pressure tendency libary
 * 
 * A air pressure tendency appears after three (3) hours of operation. The tendency 
 * codes and change are based on the 3-hr variance from the previous 3-hr history.
 *
 * Copyright (c) 2024 Eric Gionet (gionet.c.eric@gmail.com)
 *
 * MIT Licensed as described in the file LICENSE
 * 
 * 
 * 
 */
#include <esp_check.h>
#include <esp_log.h>
#include <esp_types.h>

#include <math.h>

#include <pressure_tendency.h>

/*
 * macro definitions
*/
#define ESP_ARG_CHECK(VAL) do { if (!(VAL)) return ESP_ERR_INVALID_ARG; } while (0)
/*
* static constant declerations
*/
static const char *TAG = "pressure_tendency";

const char* pressure_tendency_code_to_string(const pressure_tendency_codes_t code) {
    switch(code) {
        case PRESSURE_TENDENCY_CODE_UNKNOWN:
            return "Unkown";
        case PRESSURE_TENDENCY_CODE_RISING:
            return "Rising";
        case PRESSURE_TENDENCY_CODE_STEADY:
            return "Steady";
        case PRESSURE_TENDENCY_CODE_FALLING:
            return "Falling";
        default:
            return "Unkown";
    }
}

esp_err_t pressure_tendency_init(const uint16_t samples_size, 
                            pressure_tendency_handle_t *pressure_tendency_handle) {
    esp_err_t  ret = ESP_OK;

    /* validate arguments */
    ESP_GOTO_ON_FALSE( samples_size > 2, ESP_ERR_INVALID_ARG, err, TAG, "samples size must be greater than 2, pressure tendency handle initialization failed" );

    /* validate memory availability for pressure tendency handle */
    pressure_tendency_handle_t out_handle = (pressure_tendency_handle_t)calloc(1, sizeof(pressure_tendency_t)); 
    ESP_GOTO_ON_FALSE( out_handle, ESP_ERR_NO_MEM, err, TAG, "no memory for pressure tendency handle, pressure tendency handle initialization failed" );

    /* validate memory availability for sampples array */
    out_handle->samples = (float*)calloc(samples_size, sizeof(float));
    ESP_GOTO_ON_FALSE( out_handle->samples, ESP_ERR_NO_MEM, err_out_handle, TAG, "no memory for pressure tendency handle samples, pressure tendency handle initialization failed" );

    /* copy configuration */
    out_handle->samples_size = samples_size;

    /* set output instance */
    *pressure_tendency_handle = out_handle;

    return ESP_OK;

    err_out_handle:
        free(out_handle);
    err:
        return ret;
}

esp_err_t pressure_tendency_analysis(pressure_tendency_handle_t pressure_tendency_handle, 
                                    const float sample, 
                                    pressure_tendency_codes_t *const code,
                                    float *const change) {
    /* validate arguments */
    ESP_ARG_CHECK(pressure_tendency_handle);

    // have we filled the array?
    if (pressure_tendency_handle->samples_count < pressure_tendency_handle->samples_size) {
        // no! add this observation to the array
        pressure_tendency_handle->samples[pressure_tendency_handle->samples_count] = sample;

        // bump n
        pressure_tendency_handle->samples_count++;
    } else {
        // yes! the array is full so we have to make space
        for (uint16_t i = 1; i < pressure_tendency_handle->samples_size; i++) {
            pressure_tendency_handle->samples[i-1] = pressure_tendency_handle->samples[i];
        }

        // now we can fill in the last slot
        pressure_tendency_handle->samples[pressure_tendency_handle->samples_size-1] = sample;
    }

    // is the array full yet?
    if (pressure_tendency_handle->samples_count < pressure_tendency_handle->samples_size) {
        // no! we are still training
        *code = PRESSURE_TENDENCY_CODE_UNKNOWN;
        *change = NAN;

        return ESP_OK;
    }

    /* subtract pressure from 3-hrs ago from latest pressure */
    float delta = pressure_tendency_handle->samples[pressure_tendency_handle->samples_size-1] - pressure_tendency_handle->samples[0];

    /* evaluate delta aka 3-hr change in pressure */
    /* if the absolute variance is less than 1 hPa, air pressure is steady */
    /* if the delta is negative, and absolute variance is greater than 1 hPa, air pressure is falling */
    /* if the delta is positive, and absolute variance is greater than 1 hPa, air pressure is rising */
    if(fabs(delta) < 1) {
        // steady
        *code = PRESSURE_TENDENCY_CODE_STEADY;
        *change = delta;
    } else {
        if(delta < 0 && fabs(delta) > 1) {
            /* falling */
            *code = PRESSURE_TENDENCY_CODE_FALLING;
            *change = delta;
        } else if(delta > 0 && fabs(delta) > 1) {
            /* rising */
            *code = PRESSURE_TENDENCY_CODE_RISING;
            *change = delta;
        } else {
            /* unknown condition */
            *code = PRESSURE_TENDENCY_CODE_UNKNOWN;
            *change = NAN;
        }
    }

    return ESP_OK;
}

esp_err_t pressure_tendency_reset(pressure_tendency_handle_t pressure_tendency_handle) {
    /* validate arguments */
    ESP_ARG_CHECK(pressure_tendency_handle);

    /* purge samples */
    for(uint16_t i = 0; i < pressure_tendency_handle->samples_size; i++) {
        pressure_tendency_handle->samples[i] = NAN;
    }

    /* reset samples counter */
    pressure_tendency_handle->samples_count = 0;

    return ESP_OK;
}

esp_err_t pressure_tendency_del(pressure_tendency_handle_t pressure_tendency_handle) {
    /* validate arguments */
    ESP_ARG_CHECK(pressure_tendency_handle);
    if(pressure_tendency_handle->samples) 
        free(pressure_tendency_handle->samples);
    free(pressure_tendency_handle);
    return ESP_OK;
}