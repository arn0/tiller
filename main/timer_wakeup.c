/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include "esp_log.h"
#include "esp_check.h"
#include "esp_sleep.h"

#include "timer_wakeup.h"

#define DEFAULT_SLEEP 60
#define SEC_TO_USEC ( 1000 * 1000 )


static const char *TAG = "timer_wakeup";

esp_err_t register_timer_wakeup(void)
{
    ESP_RETURN_ON_ERROR(esp_sleep_enable_timer_wakeup( DEFAULT_SLEEP * SEC_TO_USEC ), TAG, "Configure timer as wakeup source failed");
    ESP_LOGI(TAG, "timer wakeup source is ready");
    return ESP_OK;
}
