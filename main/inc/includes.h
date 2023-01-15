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
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"
#include "esp_timer.h"
#include "esp_sntp.h"
#include "u8g2.h"
#include "u8g2_esp32_hal.h"
void app_main();
}

#endif
