#define GLOBAL
#include "globals.h"
extern void start_ota();

int cmdOTA(void *argument)
{
    printf("OTA Remote Command\n");
    start_ota();
    return ESP_OK;
}
