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
EXTERN int16_t                      theGuard,oldCurBeat[MAXDEVSS],oldCurLife[MAXDEVSS],lastFont,lastalign,oldcual;
EXTERN bool                         donef;
EXTERN FramSPI						fram;
EXTERN SemaphoreHandle_t 		    framSem;
EXTERN bool                         framFlag;
EXTERN framArgs_t                   framArg;
EXTERN meterType                    medidor[8];
EXTERN time_t                       now,oldnow;
EXTERN struct tm                    timeinfo;
#ifdef DISPLAY  
EXTERN u8g2_t                       u8g2; // a structure which will contain all the data for one display
#endif
#endif