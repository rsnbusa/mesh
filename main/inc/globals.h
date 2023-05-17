#ifndef MAIN_GLOBALS_H
#define MAIN_GLOBALS_H

#ifdef GLOBAL
    #define EXTERN extern
#else
    #define EXTERN
#endif

#include "typedef.h"    
#include "defines.h"
const static int MQTT_BIT 				= BIT0;
const static int WIFI_BIT 				= BIT1;
const static int PUB_BIT 				= BIT2;
const static int DONE_BIT 				= BIT3;
const static int SNTP_BIT 				= BIT4;
const static int SENDMQTT_BIT			= BIT5;
const static int SENDH_BIT 				= BIT6;

EXTERN pcnt_config_t                pcnt_config[8];
EXTERN uint8_t                      ssignal[8],dia,mes,displayMode,suma[MAXDEVSS],s_mesh_tx_payload[CONFIG_MESH_ROUTE_TABLE_SIZE*6+1],MESH_ID[6];
EXTERN int16_t                      theGuard,lastFont,lastalign,oldcual,timeSlotStart,timeSlotEnd,sentMqtt;
EXTERN int                          oldCurBeat[MAXDEVSS],oldCurLife[MAXDEVSS],lastkwh[MAXDEVSS],msgcount,s_retry_num,mesh_layer ;
EXTERN bool                         donef,mqttf,is_running;
EXTERN FramSPI						fram;
EXTERN SemaphoreHandle_t 		    framSem,flashSem,s_route_table_lock;
EXTERN bool                         framFlag;
EXTERN framArgs_t                   framArg;
EXTERN meterType                    medidor[8];
EXTERN time_t                       now,oldnow;
EXTERN struct tm                    timeinfo;
EXTERN esp_mqtt_client_config_t 	mqtt_cfg;
EXTERN esp_mqtt_client_handle_t     clientCloud;
EXTERN QueueHandle_t 				mqttQ,mqttR,mqttSender,pcnt_evt_queue;
EXTERN cmdRecord 					cmds[MAXCMDS];
EXTERN config_flash                 theConf;
EXTERN nvs_handle 					nvshandle;
EXTERN char                         cmdQueue[30],infoQueue[30],fecha[50];
EXTERN float                        amps[MAXDEVSS];
#ifdef DISPLAY  
EXTERN u8g2_t                       u8g2; // a structure which will contain all the data for one display
#endif
EXTERN EventGroupHandle_t 			wifi_event_group,s_wifi_event_group;
EXTERN TimerHandle_t                firstTimer,repeatTimer;
// EXTERN char*                        MESH_TAG;
EXTERN mesh_addr_t                  s_route_table[CONFIG_MESH_ROUTE_TABLE_SIZE];
EXTERN int                          s_route_table_size;
EXTERN loglevel_t                   loglevel;
EXTERN esp_netif_t*                 esp_sta; 
EXTERN mesh_addr_t                  mesh_parent_addr;    
EXTERN esp_ip4_addr_t               s_current_ip;       
#endif