/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_transport.h"
#include "esp_transport_tcp.h"
#include "esp_transport_socks_proxy.h"

#include "events.h"
#include "pp_packet.h"
#include "pp_queue.h"
#include "crc.h"
#include "../../secret.h"

static const char *TAG = "> tcp_transport_client";

void tcp_transport_client_task(void *pvParameters)
{
	char tx_buffer[4];
	char rx_buffer[128];
	esp_transport_handle_t transport = esp_transport_tcp_init();
	UBaseType_t uxNumberOfItems;

	while (true) {
		if (transport == NULL) {
			ESP_LOGE(TAG, "Error occurred during esp_transport_tcp_init()");
			break;
		}
		int err = esp_transport_connect(transport, SECRET_ADDR, SECRET_PORT, -1);
		if (err != 0) {
//			ESP_LOGE(TAG, "Client unable to connect: errno %d", errno);
		break;
		}
		ESP_LOGI(TAG, "Successfully connected");
		xEventGroupSetBits( s_wifi_event_group, TCP_CONNECTED_BIT );

		while (true) {
			/* Transmit */
			uxNumberOfItems = uxQueueMessagesWaiting( tx_Queue );
			if( uxNumberOfItems > 0 ) {
				xQueueReceive( tx_Queue, (void*) tx_buffer, (TickType_t) 0 );
				int bytes_written = esp_transport_write( transport, tx_buffer, sizeof(tx_buffer), 10 );
				if (bytes_written < 0) {
					ESP_LOGE(TAG, "Error occurred during sending: esp_transport_write() returned %d, errno %d", bytes_written, errno);
					break;
				}
				//ESP_LOGI(TAG, "TX : %hx %hx %hx %hx", tx_buffer[0], tx_buffer[1], tx_buffer[2], tx_buffer[3] );
			}

			/* Receive */
			int len = esp_transport_read(transport, rx_buffer, sizeof(rx_buffer), 10 );
			if ( len < 0 ) {
				// Error occurred during receiving
				ESP_LOGE(TAG, "recv failed: esp_transport_read() returned %d, errno %d", len, errno);
				break;
			} else if ( len > 0 ) {
				//ESP_LOGI(TAG, "received %d bytes", len);
				pp_decode( rx_buffer, len );
			}
			//vTaskDelay( 10 / portTICK_PERIOD_MS );
		}

		ESP_LOGE(TAG, "Shutting down transport and restarting...");
		esp_transport_close(transport);
	}
	esp_transport_destroy(transport);
	xEventGroupSetBits( s_wifi_event_group, TCP_FAILED_BIT );

	pp_set_rx_sync( false );
	vTaskDelete(NULL);
}
