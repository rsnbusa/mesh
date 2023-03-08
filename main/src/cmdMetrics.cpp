#define GLOBAL
#include "globals.h"

extern void write_to_flash();


void lafecha(time_t now, char * donde)
{
    struct tm timeinfo;

    localtime_r(&now, &timeinfo);
    strftime(donde, 300, "%c", &timeinfo);
}

int cmdMetrics(void *argument)
{
    mqttSender_t mensaje;

    cJSON *elcmd=(cJSON *)argument;

    if(elcmd)
    {
        cJSON *unit= 		cJSON_GetObjectItem(elcmd,"unit");
        cJSON *slots= 	    cJSON_GetObjectItem(elcmd,"slots");
        cJSON *cycle= 	    cJSON_GetObjectItem(elcmd,"cycle");

        if(unit )
        {
            int unitt=unit->valueint;
            printf("Metrics Meter  %d\n",unitt);
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
                cJSON_AddNumberToObject(root,"Max Amps", medidor[unitt].maxamp);
                cJSON_AddNumberToObject(root,"Min Amps", medidor[unitt].minamp);
                cJSON_AddNumberToObject(root,"Inst Amps", amps[unitt]);

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
                //formatted for Humans
                char *lmessage=cJSON_Print(root);
                cJSON_Delete(root);
                mensaje.msg=lmessage;            //will be freed by sender
                mensaje.lenMsg=strlen(lmessage);
                xQueueSend(mqttSender,&mensaje,0);      // WE DO NOT NOT NOT start the Mqtt server. It its time based
                // int mqtterr=esp_mqtt_client_publish(clientCloud,infoQueue, (char*)lmessage,strlen(lmessage) ,2,0);
                // free(lmessage);

                if(slots)
                    theConf.mqttSlots=slots->valueint;
                if(cycle)
                    theConf.pubCycle=cycle->valueint;
                if (slots || cycle)
                    write_to_flash();

                return ESP_OK;
            }
        }
    }
    else
        return ESP_FAIL;
    
    return ESP_OK;
}
