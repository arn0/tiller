/*
 *
 */

#include "pp_queue.h"

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

void queue_init() {

	/* Create a queue capable of containing 10 uint64_t values. */
	tx_Queue = xQueueCreateStatic( TX_QUEUE_LENGTH, TX_QUEUE_ITEM_SIZE, tx_ucQueueStorageArea, &tx_xStaticQueue );
	rx_Queue = xQueueCreateStatic( RX_QUEUE_LENGTH, RX_QUEUE_ITEM_SIZE, rx_ucQueueStorageArea, &rx_xStaticQueue );

	/* pxQueueBuffer was not NULL so xQueue should not be NULL. */

	if ( ( tx_Queue == 0 ) | ( rx_Queue == 0 ) ) {
		ESP_LOGE(TAG, "Failed to create queue" );
	}
}

bool queue_put_tx( void* packet ) {
	UBaseType_t uxNumberOfItems = uxQueueMessagesWaiting( tx_Queue );
	if( uxNumberOfItems < TX_QUEUE_LENGTH ) {
		xQueueSend( tx_Queue, packet, (TickType_t) 0 );
		return( true );
	} else {
		ESP_LOGE(TAG, "tx queue overload" );
		return( false );
	}
}

bool queue_get_tx( void* packet ) {
	UBaseType_t uxNumberOfItems = uxQueueMessagesWaiting( tx_Queue );
	if( uxNumberOfItems > 0 ) {
		xQueueReceive( tx_Queue, packet, (TickType_t) 0 );
		return( true );
	}
	return( false );
}

bool queue_put_rx( void* packet ) {
	UBaseType_t uxNumberOfItems = uxQueueMessagesWaiting( rx_Queue );
	if( uxNumberOfItems < TX_QUEUE_LENGTH ) {
		xQueueSend( rx_Queue, (void*) packet, (TickType_t) 0 );
		return( true );
	} else {
		ESP_LOGE(TAG, "rx queue overload" );
		return( false );
	}
}

bool queue_get_rx( void* packet ) {
	UBaseType_t uxNumberOfItems = uxQueueMessagesWaiting( rx_Queue );
	if( uxNumberOfItems > 0 ) {
		xQueueReceive( rx_Queue, (void*) packet, (TickType_t) 0 );
		return( true );
	}
	return( false );
}
