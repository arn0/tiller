#define  LOG_LOCAL_LEVEL ESP_LOG_INFO

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif_sntp.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "events.h"
#include "wifi_station.h"
#include "tcp_transport_client.h"
#include "light_sleep.h"
#include "control.h"
#include "../../secret.h"

static const char *TAG = ">main";


void app_main(void)
{
	esp_log_level_set("*", ESP_LOG_INFO);
	esp_log_level_set("main", ESP_LOG_INFO);
	esp_log_level_set("wifi_station", ESP_LOG_INFO);
	esp_log_level_set("tcp_transport_client", ESP_LOG_DEBUG);
	esp_log_level_set("light_sleep", ESP_LOG_VERBOSE);
	esp_log_level_set("timer_wakeup", ESP_LOG_VERBOSE);
	esp_log_level_set("uart_wakeup", ESP_LOG_VERBOSE);

	//Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	setenv( "TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 100 );
	tzset();

	ESP_LOGI(TAG, "Start wifi_init_station()");
	wifi_init_station();

	esp_sntp_config_t sntp_config = ESP_NETIF_SNTP_DEFAULT_CONFIG( SECRET_ADDR );

	test_init();

	EventBits_t bits;
	light_sleep_prepare();

	while( true ) {
		/* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
		 * number of re-tries (WIFI_DISCONNECTED_BIT). The bits are set by event_handler() (see above) */

		bits = xEventGroupWaitBits( s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_DISCONNECTED_BIT | SLEEP_WAKEUP_BIT | TCP_CONNECTED_BIT | TCP_FAILED_BIT, pdTRUE, pdFALSE, portMAX_DELAY );

		if ( bits & WIFI_CONNECTED_BIT ) {
			ESP_LOGI( TAG, "connected to ap SSID:%s", SECRET_SSID );
     		
			/* Use sntp server to set system time */
			esp_netif_sntp_init( &sntp_config );
     		if ( esp_netif_sntp_sync_wait( pdMS_TO_TICKS( 10000 ) ) != ESP_OK ) {
         	ESP_LOGE( TAG, "Failed to update system time within 10s timeout" );
 			}
 			xTaskCreate( tcp_transport_client_task, "tcp_transport_client", 4096, NULL, 5, NULL );
		} else if ( bits & WIFI_DISCONNECTED_BIT ) {
			ESP_LOGI(TAG, "Failed to connect to SSID:%s", SECRET_SSID);
			vTaskDelay( 200 / portTICK_PERIOD_MS );
			ESP_ERROR_CHECK( esp_wifi_stop() );
			xTaskCreate(light_sleep_task, "light_sleep_task", 4096, s_wifi_event_group, 6, NULL);
		} else if ( bits & SLEEP_WAKEUP_BIT ) {
				ESP_LOGI(TAG, "SLEEP_WAKEUP_BIT received");
				ESP_ERROR_CHECK( esp_wifi_start ());
		} else if( bits & TCP_CONNECTED_BIT ){
			/* next step */
 			xTaskCreate( control_loop, "control_loop", 4096, NULL, 5, NULL );
 			xTaskCreate( test_task_rx, "test_task_rx", 4096, NULL, 5, NULL );
		} else if( bits & TCP_FAILED_BIT ){
			/* wait and start tcp task again */
			vTaskDelay( 5000 / portTICK_PERIOD_MS );
 			xTaskCreate( tcp_transport_client_task, "tcp_transport_client", 4096, NULL, 5, NULL );
		} else {
			ESP_LOGE(TAG, "UNEXPECTED EVENT");
		}
	}
}