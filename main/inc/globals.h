#ifndef MAIN_GLOBALS_H
#define MAIN_GLOBALS_H

#ifdef GLOBAL
    #define EXTERN extern
#else
    #define EXTERN
#endif

#include "typedef.h"    
#include "defines.h"

EXTERN pcnt_config_t                pcnt_config[8];
EXTERN uint8_t                      ssignal[8],dia,mes,displayMode;
EXTERN int16_t                      theGuard,lastFont,lastalign,oldcual;
EXTERN int                          oldCurBeat[MAXDEVSS],oldCurLife[MAXDEVSS];
EXTERN bool                         donef;
EXTERN FramSPI						fram;
EXTERN SemaphoreHandle_t 		    framSem,flashSem;
EXTERN bool                         framFlag;
EXTERN framArgs_t                   framArg;
EXTERN meterType                    medidor[8];
EXTERN time_t                       now,oldnow;
EXTERN struct tm                    timeinfo;
EXTERN esp_mqtt_client_config_t 	mqtt_cfg;
EXTERN esp_mqtt_client_handle_t     clientCloud;
EXTERN QueueHandle_t 				mqttQ,mqttR;
EXTERN cmdRecord 					cmds[MAXCMDS];
EXTERN config_flash                 theConf;
EXTERN nvs_handle 					nvshandle;
EXTERN char                         cmdQueue[30],infoQueue[30];
#ifdef DISPLAY  
EXTERN u8g2_t                       u8g2; // a structure which will contain all the data for one display
#endif
#endif