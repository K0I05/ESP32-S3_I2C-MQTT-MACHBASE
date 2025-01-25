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
 * @file network_connect.c
 *
 * MQTT connection libary
 *
 * Copyright (c) 2024 Eric Gionet (gionet.c.eric@gmail.com)
 *
 * MIT Licensed as described in the file LICENSE
 */
#include <esp_event.h>
#include <esp_check.h>
#include <esp_log.h>
#include <esp_types.h>
#include <mqtt_client.h>

#include <mqtt_connect.h>


/**
 * @brief MQTT definitions
 */
//#define MQTT_BROKER_ADDRESS_URI                 "mqtt://192.168.2.189:5653" /*!< address uri for MQTT broker -> Windows Environment */
#define MQTT_BROKER_ADDRESS_URI                 "mqtt://192.168.2.156:5653" /*!< address uri for MQTT broker -> Ubuntu (Linux) Environment */
#define MQTT_BROKER_USERNAME                    ""                          /*!< username for MQTT broker */
#define MQTT_BROKER_PASSWORD                    ""                          /*!< password for MQTT broker */
#define MQTT_BROKER_CLIENT_ID                   "CA.NB.AWS.01-1000"         /*!< unique client identifier for MQTT broker */

/**
 * @brief Event group definitions
 */
/* 
 * The mqtt event group allows multiple bits for each event, but we only care about 3 events:
 *
 * 0 - MQTT client connected to broker
 * 1 - MQTT client discconnected from broker
 * 2 - MQTT client connection error
 */
#define MQTT_EVTGRP_CONNECTED_BIT               (BIT0)
#define MQTT_EVTGRP_DISCONNECTED_BIT            (BIT1)
#define MQTT_EVTGRP_ERROR_BIT                   (BIT2)


static const char *TAG = "mqtt_connect";

/* global variables */;
static EventGroupHandle_t       s_mqtt_evtgrp_hdl      = NULL;  /*!< mqtt event group handle */

/* external variables */
volatile bool                   mqtt_connected         = false; /*!< mqtt connection state, true when connected */
esp_mqtt_client_handle_t        mqtt_client_hdl        = NULL;  /*!< mqtt client handle */


/**
 * @brief An event handler registered to receive MQTT events.  This subroutine is called by the MQTT event loop.
 *
 * @param handler_args The user data registered to the event.
 * @param event_base Event base for the handler (MQTT events).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static inline void mqtt_event_handler(void *handler_args, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    const esp_mqtt_event_handle_t event = event_data;

    ESP_LOGD(TAG, "MQTT event dispatched from event loop base=%s, event_id=%" PRIi32, event_base, event_id);

    /* handle mqtt events */
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            /* init mqtt event group state bits */
            xEventGroupSetBits(s_mqtt_evtgrp_hdl, MQTT_EVTGRP_CONNECTED_BIT);
            xEventGroupClearBits(s_mqtt_evtgrp_hdl, MQTT_EVTGRP_DISCONNECTED_BIT);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT_EVENT_DISCONNECTED");
            /* init mqtt event group state bits */
            xEventGroupSetBits(s_mqtt_evtgrp_hdl, MQTT_EVTGRP_DISCONNECTED_BIT);
            xEventGroupClearBits(s_mqtt_evtgrp_hdl, MQTT_EVTGRP_CONNECTED_BIT);
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            ESP_LOGI(TAG, "TOPIC=%.*s", event->topic_len, event->topic);
            ESP_LOGI(TAG, "DATA=%.*s", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                ESP_LOGE(TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
                ESP_LOGE(TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
                ESP_LOGE(TAG, "Last captured errno : %d (%s)",  event->error_handle->esp_transport_sock_errno,
                        strerror(event->error_handle->esp_transport_sock_errno));
            } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
                ESP_LOGE(TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
            } else {
                ESP_LOGE(TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
            }
            /* init mqtt event group state bits */
            xEventGroupSetBits(s_mqtt_evtgrp_hdl, MQTT_EVTGRP_ERROR_BIT);
            break;
        default:
            ESP_LOGW(TAG, "Other event id:%d", event->event_id);
            break;
    }
}


esp_err_t mqtt_start(void) {
    esp_err_t     ret = ESP_OK;
    
    /* attempt to instantiate mqtt event group handle */
    s_mqtt_evtgrp_hdl = xEventGroupCreate();
    ESP_RETURN_ON_FALSE( s_mqtt_evtgrp_hdl, ESP_ERR_INVALID_STATE, TAG, "Unable to create MQTT event group handle, MQTT app start failed");

    /* set mqtt client configuration */
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri        = MQTT_BROKER_ADDRESS_URI
        },
        .credentials.client_id  = MQTT_BROKER_CLIENT_ID
    };

    /* attempt to initialize mqtt client handle */
    mqtt_client_hdl = esp_mqtt_client_init(&mqtt_cfg);
    ESP_RETURN_ON_FALSE( mqtt_client_hdl, ESP_ERR_INVALID_STATE, TAG, "Unable to initialize MQTT client, MQTT app start failed");

    /* attempt to register mqtt client event, the last argument may be used 
       to pass data to the event handler, in this example mqtt_event_handler */
    ESP_RETURN_ON_ERROR( esp_mqtt_client_register_event(mqtt_client_hdl, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL), TAG, "Unable to register MQTT client event, MQTT app start failed" );
    
    /* attempt to start mqtt client services */
    ESP_RETURN_ON_ERROR( esp_mqtt_client_start(mqtt_client_hdl), TAG, "Unable to start MQTT client, MQTT app start failed" );

    /* wait for either an mqtt connected, disconnected, or error event bit to be set */
    EventBits_t mqtt_link_bits = xEventGroupWaitBits(s_mqtt_evtgrp_hdl,
        MQTT_EVTGRP_CONNECTED_BIT | MQTT_EVTGRP_DISCONNECTED_BIT | MQTT_EVTGRP_ERROR_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY);
        
    /* xEventGroupWaitBits() returns the bits before the call returned, hence we 
        can test which event actually happened with mqtt link bits. */
    if (mqtt_link_bits & MQTT_EVTGRP_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to MQTT broker");
        mqtt_connected = true;
        ret = ESP_OK;
    } else if (mqtt_link_bits & MQTT_EVTGRP_DISCONNECTED_BIT) {
        ESP_LOGE(TAG, "Disconnected from MQTT broker");
        mqtt_connected = false;
        ret = MQTT_ERROR_TYPE_CONNECTION_REFUSED;
    } else if (mqtt_link_bits & MQTT_EVTGRP_ERROR_BIT) {
        ESP_LOGE(TAG, "MQTT client error");
        mqtt_connected = false;
        ret = MQTT_ERROR_TYPE_NONE;
    } else {
        ESP_LOGE(TAG, "Unexpected MQTT client event");
        mqtt_connected = false;
        ret = ESP_ERR_NOT_SUPPORTED;
    }

    return ret;
}

esp_err_t mqtt_stop(void) {
    /* attempt to disconnect mqtt client */
    ESP_RETURN_ON_ERROR( esp_mqtt_client_disconnect(mqtt_client_hdl), TAG, "Unable to disconnect MQTT client, MQTT app stop failed" );

    /* attempt to stop mqtt client services */
    ESP_RETURN_ON_ERROR( esp_mqtt_client_stop(mqtt_client_hdl), TAG, "Unable to stop MQTT client, MQTT app stop failed" );

    /* attempt to unregister mqtt client event */
    ESP_RETURN_ON_ERROR(esp_mqtt_client_unregister_event(mqtt_client_hdl, ESP_EVENT_ANY_ID, mqtt_event_handler), TAG, "Unable to unregister MQTT client event, MQTT app stop failed" );

    /* clean-up */
    esp_mqtt_client_destroy(mqtt_client_hdl);
    mqtt_client_hdl = NULL;
    free(s_mqtt_evtgrp_hdl);
    s_mqtt_evtgrp_hdl = NULL;

    return ESP_OK;
}