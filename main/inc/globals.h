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

EXTERN esp_aes_context		        ctx ;
EXTERN pcnt_config_t                pcnt_config[8];
EXTERN uint8_t                      ssignal[8],dia,mes,displayMode,suma[MAXDEVSS],s_mesh_tx_payload[CONFIG_MESH_ROUTE_TABLE_SIZE*6+1],MESH_ID[6];
EXTERN int16_t                      theGuard,lastFont,lastalign,oldcual,timeSlotStart,timeSlotEnd,sentMqtt;
EXTERN int                          oldCurBeat[MAXDEVSS],oldCurLife[MAXDEVSS],lastkwh[MAXDEVSS],msgcount,s_retry_num,mesh_layer ;
EXTERN bool                         donef,mqttf,is_running,webLogin;
EXTERN bool                         nakf,logof,okf,favf;
EXTERN FramSPI						fram;
EXTERN SemaphoreHandle_t 		    framSem,flashSem,s_route_table_lock,I2CSem;
EXTERN bool                         framFlag;
EXTERN framArgs_t                   framArg;
EXTERN meterType                    medidor[8];
EXTERN bool                         medidorlock[8];
EXTERN time_t                       now,oldnow;
EXTERN struct tm                    timeinfo;
EXTERN esp_mqtt_client_config_t 	mqtt_cfg;
EXTERN esp_mqtt_client_handle_t     clientCloud;
EXTERN QueueHandle_t 				mqttQ,mqttR,mqttSender,pcnt_evt_queue;
EXTERN cmdRecord 					cmds[MAXCMDS];
EXTERN config_flash                 theConf;
EXTERN nvs_handle 					nvshandle;
EXTERN char                         cmdQueue[30],infoQueue[30],configQueue[30],fecha[50],internal_cmds[MAXINTCMDS][20];
EXTERN float                        amps[MAXDEVSS];
#ifdef DISPLAY  
EXTERN u8g2_t                       u8g2; // a structure which will contain all the data for one display
#endif
EXTERN EventGroupHandle_t 			wifi_event_group,s_wifi_event_group;
EXTERN TimerHandle_t                firstTimer,repeatTimer,webTimer;
// EXTERN char*                        MESH_TAG;
EXTERN mesh_addr_t                  s_route_table[CONFIG_MESH_ROUTE_TABLE_SIZE];
EXTERN int                          s_route_table_size;
EXTERN loglevel_t                   loglevel;
EXTERN resetconf_t                  resetlevel;
EXTERN aes_en_dec_t                 endec;
EXTERN esp_netif_t*                 esp_sta; 
EXTERN mesh_addr_t                  mesh_parent_addr;    
EXTERN esp_ip4_addr_t               s_current_ip;       
EXTERN httpd_handle_t 				wserver;
EXTERN wstate_t						webState;
EXTERN char                         gwStr[20],*tempb,topic[100],iv[16],key[32];
EXTERN mesh_addr_t                  GroupID; 

#endif