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
#include "../../secret.h"

/* Test */

/* The queue is to be created to hold a maximum of 32 uint32_t variables. */
#define TX_QUEUE_LENGTH 32
#define TX_QUEUE_ITEM_SIZE sizeof( uint32_t )
#define RX_QUEUE_LENGTH ( TX_QUEUE_LENGTH * 4 )
#define RX_QUEUE_ITEM_SIZE sizeof( uint8_t )

/* The variable used to hold the queue's data structure. */
static StaticQueue_t tx_xStaticQueue;
static StaticQueue_t rx_xStaticQueue;

/* The array to use as the queue's storage area.  This must be at least
(uxQueueLength * uxItemSize) bytes. */
uint8_t tx_ucQueueStorageArea[ TX_QUEUE_LENGTH * TX_QUEUE_ITEM_SIZE ];
uint8_t rx_ucQueueStorageArea[ RX_QUEUE_LENGTH * RX_QUEUE_ITEM_SIZE ];

QueueHandle_t tx_queue;
QueueHandle_t rx_queue;



static const char *TAG = "tcp_transport_client";

void tcp_transport_client_task(void *pvParameters)
{
	
	char tx_buffer[4];
	char rx_buffer[128];
	char host_ip[] = SECRET_ADDR;
	esp_transport_handle_t transport = esp_transport_tcp_init();
	UBaseType_t uxNumberOfItems;

	while (true) {
		if (transport == NULL) {
			ESP_LOGE(TAG, "Error occurred during esp_transport_tcp_init()");
			break;
		}
		int err = esp_transport_connect(transport, SECRET_ADDR, SECRET_PORT, -1);
		if (err != 0) {
			ESP_LOGE(TAG, "Client unable to connect: errno %d", errno);
		break;
		}
		ESP_LOGI(TAG, "Successfully connected");
		xEventGroupSetBits( s_wifi_event_group, TCP_CONNECTED_BIT );

		while (true) {
			/* Transmit */
			uxNumberOfItems = uxQueueMessagesWaiting( tx_queue );
			if( uxNumberOfItems > 0 ) {
				xQueueReceive( tx_queue, (void*) tx_buffer, (TickType_t) 0 );
				int bytes_written = esp_transport_write( transport, tx_buffer, sizeof(tx_buffer), 0 );
				if (bytes_written < 0) {
					ESP_LOGE(TAG, "Error occurred during sending: esp_transport_write() returned %d, errno %d", bytes_written, errno);
					break;
				}
			}
			/* Receive */
			int len = esp_transport_read(transport, rx_buffer, sizeof(rx_buffer) - 1, 0 );
			if ( len > 0 ) {
				uxNumberOfItems = uxQueueMessagesWaiting( rx_queue );
				if( uxNumberOfItems < RX_QUEUE_LENGTH ) {
					int i = 0;
					do {
						xQueueSend( rx_queue, (void*) &rx_buffer[i++], (TickType_t) 0 );
						} while ( i < len && i < RX_QUEUE_LENGTH - uxNumberOfItems );
					} else {
						ESP_LOGE(TAG, "rx_queue overflow" );
					}


			} else if ( len > 0 ){
				// Error occurred during receiving
				ESP_LOGE(TAG, "recv failed: esp_transport_read() returned %d, errno %d", len, errno);
				break;
			}
			
			
			
			
			
			

			// Data received
			rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
			ESP_LOGI(TAG, "Received %d bytes from %s:", len, host_ip);
			ESP_LOGI(TAG, "Received data : %s", rx_buffer);

			vTaskDelay(2000 / portTICK_PERIOD_MS);
		}

		ESP_LOGE(TAG, "Shutting down transport and restarting...");
		esp_transport_close(transport);
	}
	esp_transport_destroy(transport);
	xEventGroupSetBits( s_wifi_event_group, TCP_FAILED_BIT );

	vTaskDelete(NULL);
}


/* Test */

void test_init() {

	/* Create a queue capable of containing 10 uint64_t values. */
	tx_queue = xQueueCreateStatic( TX_QUEUE_LENGTH, TX_QUEUE_ITEM_SIZE, tx_ucQueueStorageArea, &tx_xStaticQueue );
	rx_queue = xQueueCreateStatic( RX_QUEUE_LENGTH, RX_QUEUE_ITEM_SIZE, rx_ucQueueStorageArea, &rx_xStaticQueue );

	/* pxQueueBuffer was not NULL so xQueue should not be NULL. */

	if ( ( tx_queue == 0 ) | ( rx_queue == 0 ) ) {
		ESP_LOGE(TAG, "Failed to create queue" );
	}
}

void test_task_tx() {
	char txBuffer[4] = { 'a','b','c',0 };
	UBaseType_t uxNumberOfItems;
	do {
		uxNumberOfItems = uxQueueMessagesWaiting( tx_queue );
		if( uxNumberOfItems <= TX_QUEUE_LENGTH ) {
			xQueueSend( tx_queue, (void*) txBuffer, (TickType_t) 0 );
		}
		vTaskDelay( 100 / portTICK_PERIOD_MS );
	} while( true );
}

void test_task_rx() {
	char rxBuffer[4];
	UBaseType_t uxNumberOfItems;

	do {
		/* How many items are currently in the queue referenced by the xQueue handle? */
		uxNumberOfItems = uxQueueMessagesWaiting( rx_queue );
		if( uxNumberOfItems > 0 ) {
			xQueueReceive( tx_queue, (void*) rxBuffer, (TickType_t) 0 );
		}
		vTaskDelay( 10 / portTICK_PERIOD_MS );
	} while( true );
}