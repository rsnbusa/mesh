#define GLOBAL
#include "globals.h"

int cmdFormatMeter(void *argument)
{
    cJSON *elcmd=(cJSON *)argument;
    if(elcmd)
    {
        cJSON *cualm= 		cJSON_GetObjectItem(elcmd,"id");
        if(cualm)
        {
            int cual=cualm->valueint;
            if(cual<0 || cual>MAXDEVSS)
                return ESP_FAIL;
            printf("Format Meter %d... ",cual);
            fram.formatMeter(cual);
            printf("done\n");
        }
    }
    return ESP_OK;
}
