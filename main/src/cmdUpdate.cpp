#define GLOBAL
#include "globals.h"

extern void write_to_flash();

int cmdUpdate(void *argument)
{
    cJSON *elcmd=(cJSON *)argument;
    cJSON *unit= 		cJSON_GetObjectItem(elcmd,"unit");
    cJSON *kwh= 		cJSON_GetObjectItem(elcmd,"kwh");
    cJSON *bpk= 		cJSON_GetObjectItem(elcmd,"bpk");
    cJSON *slots= 	    cJSON_GetObjectItem(elcmd,"slots");
    cJSON *cycle= 	    cJSON_GetObjectItem(elcmd,"cycle");

    if(unit )
    {
        printf("Update Meter cmd %d ",unit->valueint);
        if(unit->valueint<MAXDEVSS)
        {
            int unitt=unit->valueint;
            if(kwh)
                printf("kwh %d\n",kwh->valueint);
            if(bpk)
                printf("bpk %d\n",bpk->valueint);
            if(kwh)
                medidor[unitt].kwhstart=medidor[unitt].lifekwh=kwh->valueint;
            time((time_t*)&medidor[unitt].lifedate);
            medidor[unitt].lastupdate=medidor[unitt].lifedate;
            if(bpk)
                medidor[unitt].bpk=bpk->valueint;
            fram.write_meter(unitt, (uint8_t*)&medidor[unitt],sizeof(meterType));
        }
            if(slots)
                theConf.mqttSlots=slots->valueint;
            if(cycle)
                theConf.pubCycle=cycle->valueint;
            if (slots || cycle)
                write_to_flash();
        return ESP_OK;
    }

        return ESP_FAIL;
}
