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
EXTERN uint8_t                      ssignal[8],dia,mes;
EXTERN int16_t                      theGuard;
EXTERN bool                         donef;
EXTERN FramSPI						fram;
EXTERN SemaphoreHandle_t 		    framSem;
EXTERN bool                         framFlag;
EXTERN framArgs_t                   framArg;
EXTERN meterType                    medidor[8];
EXTERN time_t                       now,oldnow;
EXTERN struct tm                    timeinfo;

#endif