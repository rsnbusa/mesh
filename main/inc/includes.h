#ifndef includes_h
#define includes_h
#include "defines.h"
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <string>

extern "C"{
#include "cJSON.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/pcnt.h"
#include "driver/spi_master.h"
#include "framDef.h"
#include "framSPI.h"
#include "freertos/event_groups.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"
#include "esp_timer.h"
#include "esp_sntp.h"
#include "u8g2.h"
#include "u8g2_esp32_hal.h"
#include "mqtt_client.h"
#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_ble.h>
#include "esp_system.h"
#include "spi_mod.h"
#include <esp_log.h>

#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"

#include "esp_mesh.h"
#include "esp_wifi_netif.h"
#include "lwip/lwip_napt.h"
#include "mesh_netif.h"

#include "esp_http_client.h"
#include "esp_http_server.h"
#include "esp_https_server.h"

#include "aes_alt.h"			//hw acceleration

void app_main();
}

#endif
