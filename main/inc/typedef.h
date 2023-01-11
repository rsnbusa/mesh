#ifndef TYPES_H_
#include "includes.h"

typedef struct fram{
    struct arg_str *format;         //format WHOLE fram
    struct arg_str *guard;          // write FRAM guard centinel
    struct arg_int *write;          // write 1 byte to and Address
    struct arg_int *read;           // read from address 1 byte
    struct arg_int *address;        // set write address
    struct arg_int *time;           // set FRAM creation date
    struct arg_int *fmeter;         // format a meter
    struct arg_int *wmeter;         // set working meter number
    struct arg_str *midw;            // read working meter id
    struct arg_int *midr;            // write working meter id
    struct arg_int *kwhstart;       // write working meter kwh start
    struct arg_int *rstart;         // read working meter kwh start
    struct arg_int *mtime;          // write working meter datetime
    struct arg_int *mbeat;          // write working meter beatstart aka beat
    struct arg_int *dumpm;          // dump meter internal values
    struct arg_int *initm;          // init meter internal values
    struct arg_end *end;
} framArgs_t;

typedef struct {
    int unit;  // the PCNT unit that originated an interrupt
    uint32_t status; // information on the event type that caused the interrupt
} pcnt_evt_t;


typedef struct meterType{
    uint16_t beat;
    char mid[12];
    uint16_t bpk;
    uint32_t beatlife,kwhstart,lastupdate,lifekwh,lifedate;
    uint16_t months[12],days[366];
    
} meterType;

#endif
