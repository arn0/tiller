/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "timer_wakeup.h"
#include "uart_wakeup.h"

static const char *TAG = "light_sleep";

void light_sleep_task(void *args)
{
	while (true) {
		ESP_LOGI(TAG, "Entering light sleep");
		/* To make sure the complete line is printed before entering sleep mode,
		 * need to wait until UART TX FIFO is empty:
		 */
		uart_wait_tx_idle_polling(UART_NUM_0);

		/* Get timestamp before entering sleep */
		int64_t t_before_us = esp_timer_get_time();

		/* Enter sleep mode */
		esp_light_sleep_start();

		/* Get timestamp after waking up from sleep */
		int64_t t_after_us = esp_timer_get_time();

		/* Determine wake up reason */
		const char* wakeup_reason;
		switch (esp_sleep_get_wakeup_cause()) {
			case ESP_SLEEP_WAKEUP_TIMER:
				wakeup_reason = "timer";
				break;
			case ESP_SLEEP_WAKEUP_GPIO:
				wakeup_reason = "pin";
				break;
			case ESP_SLEEP_WAKEUP_UART:
				wakeup_reason = "uart";
				/* Hang-up for a while to switch and execuse the uart task
				 * Otherwise the chip may fall sleep again before running uart task */
				vTaskDelay(1);
				break;
			default:
				wakeup_reason = "other";
				break;
		}
		ESP_LOGI(TAG, "Returned from light sleep, reason: %s, t=%lld ms, slept for %lld ms\n", wakeup_reason, t_after_us / 1000, (t_after_us - t_before_us) / 1000);
	}
	vTaskDelete(NULL);
}

void light_sleep_prepare(void)
{
	/* Enable wakeup from light sleep by timer */
	register_timer_wakeup();

	/* Enable wakeup from light sleep by uart */
	register_uart_wakeup();

//    xTaskCreate(light_sleep_task, "light_sleep_task", 4096, NULL, 6, NULL);
}
