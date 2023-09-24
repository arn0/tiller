/*
 *
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
extern StaticQueue_t tx_xStaticQueue;
extern StaticQueue_t rx_xStaticQueue;

extern QueueHandle_t tx_Queue;
extern QueueHandle_t rx_Queue;




/* Test */

void test_init();
void test_task_tx();
void test_task_rx();

void queue_send_tx( void* packet );

