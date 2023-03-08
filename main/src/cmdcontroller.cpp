#define GLOBAL
#include "globals.h"
extern void write_to_flash();

int cmdController(void *argument)
{

    printf("Cmd Controller\n");
    cJSON *elcmd=(cJSON *)argument;

    if(elcmd)
    {
        cJSON *cid= 		cJSON_GetObjectItem(elcmd,"id");
        cJSON *address= 	cJSON_GetObjectItem(elcmd,"add");
        cJSON *prov= 		cJSON_GetObjectItem(elcmd,"prov");
        cJSON *canton= 		cJSON_GetObjectItem(elcmd,"canton");
        cJSON *parro= 		cJSON_GetObjectItem(elcmd,"parro");
        cJSON *codpostal= 	cJSON_GetObjectItem(elcmd,"codp");
        cJSON *slots= 	    cJSON_GetObjectItem(elcmd,"slots");
        cJSON *cycle= 	    cJSON_GetObjectItem(elcmd,"cycle");

        if(cid && address && prov && canton && parro && codpostal )
        {
            time((time_t*)&theConf.bornDate);
            theConf.controllerid=cid->valueint;
            strcpy(theConf.direccion,address->valuestring);
            theConf.provincia=prov->valueint;
            theConf.parroquia=parro->valueint;
            theConf.canton=canton->valueint;
            theConf.codpostal=codpostal->valueint;
            theConf.nodeConf=true;
            bzero(infoQueue,sizeof(infoQueue));
            bzero(cmdQueue,sizeof(cmdQueue));
            if(slots)
                theConf.mqttSlots=slots->valueint;
            if(cycle)
                theConf.pubCycle=cycle->valueint;
            sprintf(cmdQueue,"meter/%d/%d/%d/%d/%d/cmd",theConf.provincia,theConf.canton,theConf.parroquia,theConf.codpostal,theConf.controllerid);
            sprintf(infoQueue,"meter/%d/%d/%d/%d/%d/info",theConf.provincia,theConf.canton,theConf.parroquia,theConf.codpostal,theConf.controllerid);
            write_to_flash();
            return ESP_OK;
        }
    }
    else
        return ESP_FAIL;
    
    return ESP_OK;
}
