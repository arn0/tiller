#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "wifi_station.h"
#include "../../secret.h"

static const char *TAG = "main";


void app_main(void)
{
		esp_log_level_set("*", ESP_LOG_WARN);
		esp_log_level_set("main", ESP_LOG_WARN);
		esp_log_level_set("wifi station", ESP_LOG_INFO);

		//Initialize NVS
		esp_err_t ret = nvs_flash_init();
		if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
			ESP_ERROR_CHECK(nvs_flash_erase());
			ret = nvs_flash_init();
		}
		ESP_ERROR_CHECK(ret);

		ESP_LOGI(TAG, "Start wifi_init_station()");
		wifi_init_station();

// When WiFi gets disconnected, wifi_event_handler will initiate reconnect attempts
// maxes out at ESP_MAXIMUM_RETRY.
// need to make this indefinite, with sleep time.




}