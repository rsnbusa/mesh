#define GLOBAL
#include "globals.h"

void lafecha(time_t now, char * donde)
{
    struct tm timeinfo;

    localtime_r(&now, &timeinfo);
    strftime(donde, 300, "%c", &timeinfo);
}

int cmdMetrics(void *argument)
{
    cJSON *elcmd=(cJSON *)argument;

    if(elcmd)
    {
        cJSON *unit= 		cJSON_GetObjectItem(elcmd,"unit");

        if(unit )
        {
            int unitt=unit->valueint;
            printf("Metrics Meter  %d ",unitt);
            if(unitt<MAXDEVSS)
            {
                cJSON *root=cJSON_CreateObject();
                if(root==NULL)
                {
                    printf("cannot create root\n");
                    return ESP_FAIL;
                }
                cJSON_AddNumberToObject(root,"Unit",unitt);
                cJSON_AddStringToObject(root,"MID",medidor[unitt].mid);
                cJSON_AddNumberToObject(root,"Beats", medidor[unitt].beat);
                cJSON_AddNumberToObject(root,"BeatsLife", medidor[unitt].beatlife);
                cJSON_AddNumberToObject(root,"KwH Start", medidor[unitt].kwhstart);
                cJSON_AddNumberToObject(root,"KwH Life", medidor[unitt].lifekwh);

                char *buf=(char*)malloc(300);
                if(buf)
                {
                    lafecha((time_t)medidor[unitt].lifedate,buf);
                    cJSON_AddStringToObject(root,"Born",buf);
                    lafecha((time_t)medidor[unitt].lastupdate,buf);
                    cJSON_AddStringToObject(root,"Update",buf);              
                    time(&now);
                    lafecha(now,buf);
                    cJSON_AddStringToObject(root,"Date",buf);
                    free(buf);
                }
                //unformatted
                char *lmessage=cJSON_Print(root);
                cJSON_Delete(root);

                int mqtterr=esp_mqtt_client_publish(clientCloud,infoQueue, (char*)lmessage,strlen(lmessage) ,2,0);
                free(lmessage);
                return ESP_OK;
            }
        }
    }
    else
        return ESP_FAIL;
    
    return ESP_OK;
}
