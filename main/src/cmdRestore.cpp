#define GLOBAL
#include "globals.h"

int cmdRestore(void *argument)
{
    printf("Restore Factory Settings\n");
    wifi_prov_mgr_reset_provisioning();
    ESP_ERROR_CHECK(esp_wifi_restore());
    nvs_flash_init();
    esp_restart();
    return ESP_OK;
}
