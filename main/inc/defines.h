//FRAM pins SPI
#define FMOSI							23
#define FMISO							19
#define FCLK							18
#define FCS								5

#define SDA                             22
#define SCL                             21

#define DEBB
#define LOCALTIME                       "GMT+5"
#define DISPLAY

#define MAXCMDS                         10

#define CENTINEL                        0x12345678

#define AMPSHORA                        (8.33)
#define BIASHOUR                        (14)

#define OTAURL                          "http://161.35.115.228/metermgr.bin"
#define OTADEL                          5000
#define MESH_TAG                        "Mesh"
#define WIFI_CONNECTED_BIT              BIT0
#define WIFI_FAIL_BIT                   BIT1

#define FREEANDNULL(x)		            if(x) {free(x);x=NULL;}
#define BASEMESH                        117
#define ROOT                            true
#define NRT                             false
#define MINB                            2000
#define MAXB                            4000
#define MAXINTCMDS                      5