#define GLOBAL
#include "globals.h"

char * sendData(bool forced)
{
    time_t now;
    struct tm timeinfo;
    char buf[50];
    char *lmessage=NULL;
    uint8_t fueron=0;


    cJSON *root=cJSON_CreateObject();
    if(root==NULL)
    {
        printf("cannot create root senddata\n");
        return NULL;
    }
    cJSON *medArray=cJSON_CreateArray();
    if(medArray)
    {
        for (int a=0;a<MAXDEVSS;a++)
        {
            if(strlen(medidor[a].mid)>0)
            {
                if(lastkwh[a]<medidor[a].lifekwh || forced)
                {
                    fueron++;
                    cJSON *datos=cJSON_CreateObject();
                    if(datos)
                    {
                        cJSON_AddStringToObject(datos,"MID",medidor[a].mid);
                        cJSON_AddNumberToObject(datos,"kwh",medidor[a].lifekwh);
                        cJSON_AddItemToArray(medArray, datos);
                        lastkwh[a]=medidor[a].lifekwh;  //update counter
                    }
                }
            }
        }
        // create timestamp for Human inspection
        time(&now);
        localtime_r(&now, &timeinfo);
        bzero(buf,sizeof(buf));
        strftime(buf, 50, "%c", &timeinfo);

        //add members 
        cJSON_AddNumberToObject(root,"ControlId",theConf.controllerid);
        cJSON_AddStringToObject(root,"TS",buf);
        if(medArray)
            cJSON_AddItemToObject(root, "Medidores",medArray);

        // formatted for Human Inspection 
        if (fueron)
            lmessage=cJSON_Print(root);
        else
            lmessage=NULL;      //nothing to send
    }
    cJSON_Delete(root);     //gone with this structure free it
    return lmessage;        //must be freed by caller
}
