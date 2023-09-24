/*
 *
 */

#include "pp_queue.h"

/* Test */

/* The variable used to hold the queue's data structure. */
StaticQueue_t tx_xStaticQueue;
StaticQueue_t rx_xStaticQueue;

/* The array to use as the queue's storage area.  This must be at least
(uxQueueLength * uxItemSize) bytes. */
uint8_t tx_ucQueueStorageArea[ TX_QUEUE_LENGTH * TX_QUEUE_ITEM_SIZE ];
uint8_t rx_ucQueueStorageArea[ RX_QUEUE_LENGTH * RX_QUEUE_ITEM_SIZE ];

QueueHandle_t tx_Queue;
QueueHandle_t rx_Queue;


static const char *TAG = "> pp_queue";



/* Test */

void test_init() {

	/* Create a queue capable of containing 10 uint64_t values. */
	tx_Queue = xQueueCreateStatic( TX_QUEUE_LENGTH, TX_QUEUE_ITEM_SIZE, tx_ucQueueStorageArea, &tx_xStaticQueue );
	rx_Queue = xQueueCreateStatic( RX_QUEUE_LENGTH, RX_QUEUE_ITEM_SIZE, rx_ucQueueStorageArea, &rx_xStaticQueue );

	/* pxQueueBuffer was not NULL so xQueue should not be NULL. */

	if ( ( tx_Queue == 0 ) | ( rx_Queue == 0 ) ) {
		ESP_LOGE(TAG, "Failed to create queue" );
	}
}

void queue_send_tx( void* packet ) {
	UBaseType_t uxNumberOfItems = uxQueueMessagesWaiting( tx_Queue );
	if( uxNumberOfItems <= TX_QUEUE_LENGTH ) {
		xQueueSend( tx_Queue, packet, (TickType_t) 0 );
	} else {
		ESP_LOGE(TAG, "tx queue overload" );
	}
}

void test_task_tx() {
	char txBuffer[4] = { 'a','b','c',0 };
	UBaseType_t uxNumberOfItems;
	do {
		uxNumberOfItems = uxQueueMessagesWaiting( tx_Queue );
		if( uxNumberOfItems <= TX_QUEUE_LENGTH ) {
			xQueueSend( tx_Queue, (void*) txBuffer, (TickType_t) 0 );
		}
		vTaskDelay( 100 / portTICK_PERIOD_MS );
	} while( true );
}

void test_task_rx() {
	char rxBuffer[4];
	UBaseType_t uxNumberOfItems;

	do {
		/* How many items are currently in the queue referenced by the xQueue handle? */
		uxNumberOfItems = uxQueueMessagesWaiting( rx_Queue );
		if( uxNumberOfItems > 0 ) {
			xQueueReceive( tx_Queue, (void*) rxBuffer, (TickType_t) 0 );
		}
		vTaskDelay( 10 / portTICK_PERIOD_MS );
	} while( true );
}