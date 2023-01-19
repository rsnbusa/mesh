#define GLOBAL
#include "globals.h"

int cmdFormat(void *argument)
{
    cJSON *elcmd=(cJSON *)argument;
    cJSON *order= 		cJSON_GetObjectItem(elcmd,"cmd");
    if(order)
    {
        printf("Format Fram...");
        fram.format(0,NULL,1000,true);
        printf("done\n");
    }
    return ESP_OK;
}
