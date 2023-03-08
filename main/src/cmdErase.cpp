#define GLOBAL
#include "globals.h"
extern void erase_config();

int cmdErase(void *argument)
{
    printf("Erase Configuration\n");
    erase_config();
    esp_restart();
    return ESP_OK;
}
