#define GLOBAL
#include "globals.h"
extern void write_to_flash();

int cmdInitMeter(void *argument)
{
    cJSON *elcmd=(cJSON *)argument;
    cJSON *mid= 		cJSON_GetObjectItem(elcmd,"mid");
    cJSON *unit= 		cJSON_GetObjectItem(elcmd,"unit");
    cJSON *kwh= 		cJSON_GetObjectItem(elcmd,"kwh");
    cJSON *bpk= 		cJSON_GetObjectItem(elcmd,"bpk");
    cJSON *slots= 	    cJSON_GetObjectItem(elcmd,"slots");
    cJSON *cycle= 	    cJSON_GetObjectItem(elcmd,"cycle");

    if(mid && unit && kwh && bpk)
    {
        printf("Init Meter Unit %d ",unit->valueint);
        if(unit->valueint<MAXDEVSS)
        {
            int unitt=unit->valueint;
            printf(" MID %s",mid->valuestring);
            printf(" kwh %d",kwh->valueint);
            printf(" bpk %d",bpk->valueint);
            
            if(slots)
                printf(" Slot %d",slots->valueint);
            if(cycle)
                printf(" cycle %d\n",cycle->valueint);

            bzero(medidor[unitt].mid,sizeof(medidor[unitt].mid));
            strcpy(medidor[unitt].mid,mid->valuestring);
            medidor[unitt].kwhstart=medidor[unitt].lifekwh=kwh->valueint;
            time((time_t*)&medidor[unitt].lifedate);
            medidor[unitt].lastupdate=medidor[unitt].lifedate;
            medidor[unitt].bpk=bpk->valueint;
            medidor[unitt].maxamp=0;
            medidor[unitt].minamp=9999;
            if(slots)
                theConf.mqttSlots=slots->valueint;
            if(cycle)
                theConf.pubCycle=cycle->valueint;
            if (slots || cycle)
                write_to_flash();
            fram.write_meter(unitt, (uint8_t*)&medidor[unitt],sizeof(meterType));
        }
        return ESP_OK;
    }
    else
        return ESP_FAIL;
}
