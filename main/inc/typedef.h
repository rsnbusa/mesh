#ifndef TYPES_H_
#include "includes.h"

typedef enum webStates{wNONE,wLOGIN,wMENU,wSETUP,wCHALL} wstate_t;

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

typedef struct logl{
    struct arg_int *level;        
    struct arg_end *end;
} loglevel_t;

typedef struct aes_en_ec{
    struct arg_int *key;        
    struct arg_end *end;
} aes_en_dec_t;

typedef struct resetconf{
    struct arg_int *cflags;        
    struct arg_end *end;
} resetconf_t;

typedef struct {
    int unit;  // the PCNT unit that originated an interrupt
    uint32_t status; // information on the event type that caused the interrupt
} pcnt_evt_t;


typedef struct meterType{
    uint16_t beat;
    char mid[12];
    uint16_t maxamp,minamp,bpk;
    uint32_t beatlife,kwhstart,lastupdate,lifekwh,lifedate,lastclock;
    uint16_t months[12],days[366];
} meterType;

typedef enum displayType {NODISPLAY,DISPLAYIT} displayType;
typedef enum overType {NOREP,REPLACE} overType;

enum OLEDDISPLAY_TEXT_ALIGNMENT {
  TEXT_ALIGN_LEFT = 0,
  TEXT_ALIGN_RIGHT = 1,
  TEXT_ALIGN_CENTER = 2,
  TEXT_ALIGN_CENTER_BOTH = 3
};

typedef struct mqttMsgInt{
	uint8_t 	*message;	// memory of message. MUST be freed by the Submode Routine and allocated by caller
	uint16_t	msgLen;
	char		*queueName;	// queue name to send
	uint32_t	maxTime;	//max ms to wait
}mqttMsg_t;


typedef int (*functrsn)(void *);

typedef struct cmdRecord{
    char 		comando[20];
    char        abr[6];
    functrsn 	code;
    uint32_t	count;
}cmdRecord;

typedef struct mqttRecord{
    char        * msg,*queue;
    uint16_t    lenMsg;
}mqttSender_t;

typedef struct config {
    time_t 		bornDate;
    uint32_t 	bootcount,lastResetCode,centinel;
    uint8_t		provincia,canton,parroquia;
    uint32_t	codpostal,controllerid;
    char		direccion[45];
    uint16_t    maxamp;
    uint32_t    downtime;
    uint16_t    mqttSlots,pubCycle;
    uint16_t    nodeConf;
    uint16_t    loglevel;
    uint8_t     meshconf,meterconf,bootflag,meshid,subnode;
    uint32_t    meshconfdate,meterconfdate,confpassword,cid;
    char        mqttServer[100];
    char        mqttUser[50];
    char        mqttPass[50];
    char        thessid[40], thepass[20];
} config_flash;


typedef struct medidores_mac{
    mesh_addr_t     big_table[MAXNODES];
    bool            received[MAXNODES];
} medidores_mac_t;

#endif

/*
    ESP_LOG_NONE,       
    ESP_LOG_ERROR,      
    ESP_LOG_WARN,       
    ESP_LOG_INFO,       
    ESP_LOG_DEBUG,      
    ESP_LOG_VERBOSE 
    */