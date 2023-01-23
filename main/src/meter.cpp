#include "includes.h"
#include "globals.h"
#include "forwards.h"

#define SSID "Porton"
#define PSW "csttpstt"

xQueueHandle pcnt_evt_queue;   // A queue to handle pulse counter events

/* A sample structure to pass events from the PCNT
 * interrupt handler to the main program.
 */
static int s_retry_num = 0;
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1



void mdelay(uint32_t cuanto)
{
            vTaskDelay(cuanto / portTICK_PERIOD_MS);

}

uint32_t xmillis()
{
	return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

uint32_t xmillisFromISR()
{
	return xTaskGetTickCountFromISR() * portTICK_PERIOD_MS;
}


static void read_flash()
{
	esp_err_t q ;
	size_t largo;

	if(xSemaphoreTake(flashSem, portMAX_DELAY/  portTICK_RATE_MS))
	{
		q = nvs_open("config", NVS_READONLY, &nvshandle);
		if(q!=ESP_OK)
		{
			printf("Error opening NVS Read File %x\n",q);
			xSemaphoreGive(flashSem);
			return;
		}

		largo=sizeof(theConf);
		q=nvs_get_blob(nvshandle,"sysconf",(void*)&theConf,&largo);

		if (q !=ESP_OK)
			printf("Error read %x largo %d aqui %d\n",q,largo,sizeof(theConf));
		nvs_close(nvshandle);
		xSemaphoreGive(flashSem);
	}
}

void write_to_flash() //save our configuration
{
	if(xSemaphoreTake(flashSem, portMAX_DELAY/  portTICK_RATE_MS))
	{
		esp_err_t q ;
		q = nvs_open("config", NVS_READWRITE, &nvshandle);
		if(q!=ESP_OK)
		{
			printf("Error opening NVS File RW %x\n",q);
			xSemaphoreGive(flashSem);
			return;
		}
		size_t req=sizeof(theConf);
		q=nvs_set_blob(nvshandle,"sysconf",&theConf,req);
		if (q ==ESP_OK)
		{
			q = nvs_commit(nvshandle);
			if(q!=ESP_OK)
				printf("Flash commit write failed %d\n",q);
		}
		else
			printf("Fail to write flash %x\n",q);
		nvs_close(nvshandle);
		xSemaphoreGive(flashSem);
	}
}



static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < 5) {
            esp_wifi_connect();
            s_retry_num++;
           printf( "retry to connect to the AP\n");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
       printf("connect to the AP fail\n");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
    //    printf( "got ip:" IPSTR "\n", IP2STR(&event->ip_info.ip));
        // gpio_set_level((gpio_num_t)2,1);
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void sntpget()
{
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    // Is time set? If not, tm_year will be (1970 - 1900).
    if (timeinfo.tm_year < (2016 - 1900)) 
    {
        // printf("Time is not set yet. Connecting to WiFi and getting time over NTP.");
        sntp_setoperatingmode(SNTP_OPMODE_POLL);
        sntp_setservername(0, "pool.ntp.org");
        sntp_init();

        memset(&timeinfo,0,sizeof(timeinfo));
        int retry = 0;
        const int retry_count = 30;
        while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count)
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        
        if(retry>retry_count)
        {
            printf("SNTP failed\n");
            return;
        }
        time(&now);
        setenv("TZ", LOCALTIME, 1);
        tzset();
        localtime_r(&now, &timeinfo);
        char *buf=(char*)malloc(300);
        if(buf)
        {
            bzero(buf,300);
            strftime(buf, 300, "%c", &timeinfo);
            strcpy(fecha,buf);
            // printf("[CMD]The current date/time in %s is: %s day of Year %d\n", LOCALTIME,buf,timeinfo.tm_yday);
            free(buf);
        }
        set_senddata_timer();

    }
}


void wifi_init_sta(void * pArg)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config;
    bzero((void*)&wifi_config,sizeof(wifi_config));
    strcpy((char*)wifi_config.sta.ssid,SSID);
    strcpy((char*)wifi_config.sta.password,PSW);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
       printf( "connected to ap SSID:%s password:%s\n",SSID, PSW);
                 sntpget();
    } else if (bits & WIFI_FAIL_BIT) {
       printf( "Failed to connect to SSID:%s, password:%s\n",
                 SSID, PSW);
    } else {
       printf( "UNEXPECTED EVENT");
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
    vTaskDelete(NULL);

}

#ifdef DISPLAY
void ssdString(int x, int y, char * que,bool centerf)
{
    u8g2_DrawFrame(&u8g2,1,1,126,62);
    if (!centerf)
        u8g2_DrawStr(&u8g2, x,y,que);
    else
        {
            int  w = u8g2_GetStrWidth(&u8g2,que);
            int h = u8g2_GetMaxCharHeight(&u8g2);
            int sw=u8g2_GetDisplayWidth(&u8g2);
            int lstx=(sw-w)/2;
            int lsty=(u8g2_GetDisplayHeight(&u8g2)+h/2)/2;
            u8g2_DrawStr(&u8g2, lstx,lsty,que);
        }
	u8g2_SendBuffer(&u8g2);
}
#endif

esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    mqttMsg_t mqttHandle;
    char *msg;

	// all calls check for eventGroupBits
	// CONNECT starts process  and SUNSCRIBES to cmdQueue in case of incoming messages and which will wait for SUBSCRIBED event
	// SUBSCRIBED will set the MQTT_BIT indicating a connection ready
	// PUBLISH sends message and waits for PUBLISHED bit to be set and then UNSUBSCRIBES and waits for UNSUBSCRIBED which set BIT indicating complete transfer. Now STOP
	// if a Message is available, allocate heap and send the message to a queue. Somebody down the line will have to free this message

    esp_mqtt_client_handle_t client = event->client;
    // your_context_t *context = event->context;
    // cmdType lcmd;

    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:

				printf("Mqtt connected\n");

        	// mqttf=true;
            esp_mqtt_client_subscribe(client,cmdQueue, 0);
            mqttf=true;
          //  printf("Subscribed to [%s]\n",cmdQueue);
            // printf("Subscribed\n");
            //set bit to allow submode to work and not crash
           // xEventGroupSetBits(wifi_event_group, MQTT_BIT);//message sent bit

            break;
        case MQTT_EVENT_DISCONNECTED:

			// if(theConf.traceflag & (1<<MQTTD))
				printf("Mqtt Disco\n");

        	mqttf=false;
            // xEventGroupClearBits(wifi_event_group, MQTT_BIT);
            break;
        case MQTT_EVENT_SUBSCRIBED:
        printf("Sub done\n");
             xEventGroupSetBits(wifi_event_group, MQTT_BIT);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
#ifdef MQTTSUB
            xEventGroupSetBits(wifi_event_group, PUB_BIT|DONE_BIT);//message sent bit
#endif
            mqttf=false;
            break;
        case MQTT_EVENT_PUBLISHED:

        		// printf("Published MsgId %d\n",event->msg_id);
#ifdef MQTTSUB
            esp_mqtt_client_unsubscribe(client, cmdQueue);//bit is set by unsubscribe
#else
            xEventGroupSetBits(wifi_event_group, MQTT_BIT);//message sent bit
#endif
            break;
        case MQTT_EVENT_DATA:
            bzero(&mqttHandle,sizeof(mqttHandle));
            msg=(char*)malloc(100);
            bzero(msg,100);
            memcpy(msg,event->data,event->data_len);
            mqttHandle.message=(uint8_t*)msg;
            mqttHandle.msgLen=event->data_len;

            if(event->data_len)
            {
                printf("TOPIC=%.*s\n",event->topic_len, event->topic);
                printf("DATA=%.*s %d\n", event->data_len, event->data, event->data_len);

                if(xQueueSend( mqttQ,&mqttHandle,0 )!=pdPASS)
                {
                    printf("Cannot add msgd mqtt\n");
                }

                esp_mqtt_client_publish(clientCloud, cmdQueue, "", 0, 0,1);//delete retained
            }
            break;
        case MQTT_EVENT_ERROR:
#ifdef DEBUGX
			if(theConf.traceflag & (1<<MQTTD))
				pprintf("%sMqtt Error\n",MQTTDT);
#endif
        //    xEventGroupClearBits(wifi_event_group, MQTT_BIT);
            break;
        case MQTT_EVENT_BEFORE_CONNECT:
            // xEventGroupClearBits(wifi_event_group, MQTT_BIT|DONE_BIT);
        	break;
        default:
#ifdef DEBUGX
        	if(theConf.traceflag & (1<<MQTTD))
        		pprintf("%sEvent %d\n",MQTTDT,event->event_id);
#endif
            break;
    }
    return ESP_OK;
}

static int findCommand(const char * cual)
{
	for (int a=0;a<MAXCMDS;a++)
	{
		if(strcmp(cmds[a].comando,cual)==0)
			return a;
	}
	return ESP_FAIL;
}


void mqttMgr(void *pArg)
{
    mqttMsg_t mqttHandle;
	cJSON 	*elcmd;

	int err=esp_mqtt_client_start(clientCloud);
    if(err)
    {
        printf("Could not start Mqtt %d %x\n",err,err);
    }

    while(true)
    {
        if( xQueueReceive( mqttQ, &mqttHandle, portMAX_DELAY ))	//mqttHandle has a pointer to the original message. MUST be freed at some point
        {
            printf("Message In MQtt %s len %d\n",mqttHandle.message,mqttHandle.msgLen);
            // char *algo=(char*)malloc(1000);
            // memcpy((void*)algo,(void*)mqttHandle.message,mqttHandle.msgLen);
            elcmd= cJSON_Parse((char*)mqttHandle.message);		//plain text to cJSON... must eventually cDelete elcmd
			if(elcmd)
			{
				cJSON *order= 		cJSON_GetObjectItem(elcmd,"cmd");
                if(order)
                {
                    int cual=findCommand(order->valuestring);
                    if(cual>=0)
                        (*cmds[cual].code)((void*)elcmd);	// call the cmd and wait for it to end
                    else    
                        printf("Invalid cmd received %s\n",order->valuestring);
                }
                cJSON_Delete(elcmd);
            }
            else
                printf("Invalid parse\n");
            
            free(mqttHandle.message);
        }
    }
}

/// to get ssl certificate 
//                                                <------------------ Site/Port ------------------------->
// echo "" | openssl s_client -showcerts -connect 8cb3b896a9ff4973ab94b219d8ef1de8.s2.eu.hivemq.cloud:8883 | sed -n "1,/Root/d; /BEGIN/,/END/p" | openssl x509 -outform PEM >hive.pem

static void mqtt_app_start(void)
{
    char who[20],mac[8];
    esp_base_mac_addr_get((uint8_t*)mac);
    bzero(who,sizeof(who));
    sprintf(who,"Meterserver%d",theConf.controllerid);
    printf("Who %s\n",who);
// extern const uint8_t hive_start[]   asm("_binary_hive_pem_start");
// extern const uint8_t hive_end[]   asm("_binary_hive_pem_end");

// int ssllen=hive_end-hive_start;

        wifi_event_group = xEventGroupCreate();


        bzero((void*)&mqtt_cfg,sizeof(mqtt_cfg));
        mqtt_cfg.client_id=				    who;
        mqtt_cfg.username=					"wckwlvot";
        mqtt_cfg.password=					"MxoMTQjeEIHE";
        mqtt_cfg.uri = 					    "mqtt://m13.cloudmqtt.com:18747";
        // mqtt_cfg.username=					    "user1";
        // mqtt_cfg.password=					    "Csttpstt1";
        // mqtt_cfg.uri = 					        "ssl://8cb3b896a9ff4973ab94b219d8ef1de8.s2.eu.hivemq.cloud:8883";
        mqtt_cfg.event_handle = 			    mqtt_event_handler;
	    mqtt_cfg.disable_auto_reconnect=	true;
        // mqtt_cfg.cert_pem=NULL;
        // mqtt_cfg.cert_pem=(char*)hive_start;
        // mqtt_cfg.cert_len=ssllen;

		mqttQ = xQueueCreate( 20, sizeof( mqttMsg_t ) );
		if(!mqttQ)
			printf("Failed queue\n");

		clientCloud=NULL;

	    clientCloud = esp_mqtt_client_init(&mqtt_cfg);

	    if(clientCloud)
        {
             printf("Mqtt started\n");
	    	xTaskCreate(&mqttMgr,"mqtt",10240,NULL, 5, NULL);
        }
	    else
	    {
	    	printf("Not configured Mqtt\n");	//should crash. No conn with Host defeats the purpose but lets save beats at least
	    	return;
	    }
}


void init_vars()
{
//cmd and info queue names
    sprintf(cmdQueue,"meter/%d/%d/%d/%d/%d/cmd",theConf.provincia,theConf.canton,theConf.parroquia,theConf.codpostal,theConf.controllerid);
    sprintf(infoQueue,"meter/%d/%d/%d/%d/%d/info",theConf.provincia,theConf.canton,theConf.parroquia,theConf.codpostal,theConf.controllerid);
    // printf("Cmd Q %s\nInfo %s\n",cmdQueue,infoQueue);

    oldcual=9999;
    ssignal[0]=14;
    ssignal[1]=27;
    ssignal[2]=26;
    ssignal[3]=25;
    ssignal[4]=33;
    ssignal[5]=32;
    ssignal[6]=35;
    ssignal[7]=34;
    oldnow=0;

    #ifdef DISPLAY
	u8g2_esp32_hal_t   u8g2_esp32_hal;//
    bzero(&u8g2_esp32_hal,sizeof(u8g2_esp32_hal));
	u8g2_esp32_hal.sda   =(gpio_num_t)SDA;
	u8g2_esp32_hal.scl  = (gpio_num_t)SCL;
	u8g2_esp32_hal_init(u8g2_esp32_hal);
	u8g2_Setup_ssd1306_i2c_128x64_noname_f(
		&u8g2,
		U8G2_R0,
		//u8x8_byte_sw_i2c,
		u8g2_esp32_i2c_byte_cb,
		u8g2_esp32_gpio_and_delay_cb);  // init u8g2 structure
	u8x8_SetI2CAddress(&u8g2.u8x8,0x78);    //FIXED 
    u8g2_InitDisplay(&u8g2); // send init sequence to the display, display is in sleep mode after this,
	u8g2_SetPowerSave(&u8g2, 0); // wake up display
    u8g2_SetFont(&u8g2, u8g2_font_ncenB14_tr);
    #ifdef DISPLAY
    u8g2_ClearBuffer(&u8g2);
    ssdString(10,38,(char*)"MeterIoT",true);
#endif
    // stx=10;
    // sty=38;
#endif
    int x=0;

// {"cmd":"initmeter","unit":7,"mid":"888888888888","bpk":800,"kwh":8888}
    strcpy((char*)&cmds[x].comando,         "restore");			        cmds[x].code=cmdRestore;				
	strcpy((char*)&cmds[++x].comando,       "format");			        cmds[x].code=cmdFormat;					
    strcpy((char*)&cmds[++x].comando,       "formatmeter");			    cmds[x].code=cmdFormatMeter;					
	strcpy((char*)&cmds[++x].comando,       "initmeter");			    cmds[x].code=cmdInitMeter;					
	strcpy((char*)&cmds[++x].comando,       "metrics");			        cmds[x].code=cmdMetrics;					
	strcpy((char*)&cmds[++x].comando,       "controller");		        cmds[x].code=cmdController;					
	strcpy((char*)&cmds[++x].comando,       "update");		            cmds[x].code=cmdUpdate;					
	strcpy((char*)&cmds[++x].comando,       "erase");		            cmds[x].code=cmdErase;					
}

// {"cmd":"initmeter","unit":0,"mid":"1111111111","kwh":1111,"bpk":800}
// {"cmd":"metrics","unit":0}

static void init_fram( bool load)
{
	// FRAM Setup. Will initialize the Semaphore. If NO FRAM, NO METER so it should really stop but for sakes of testing lets go on...
	theGuard = esp_random();

	spi_flash_init();

	framFlag=fram.begin(FMOSI,FMISO,FCLK,FCS,&framSem); //will create SPI channel and Semaphore
	if(framFlag)
	{
		//load all devices counters from FRAM
		// fram.write_guard(theGuard);				// theguard is dynamic and will change every boot.
        // printf("Medidor size %d\n",sizeof(medidor[0]));
		if(load)
        {
			for (int a=0;a<8;a++)
            {
				fram.read_meter(a,(uint8_t*)&medidor[a],sizeof(meterType));
                medidor[a].lastclock=xmillis();
            }
	    }
        else
            framSem=NULL;
    }
   /*
    printf("FramSize %d\n FRAMDATE %d GUARDM %d SCRATCH %d\
    BEATSTART %d MID %d MAXAMP %d MINAMP %d BPK %d BEATLIFE %d MKWMSTART %d\
    MUPDATE %d LIFEKWH %d LIFEDATE %d\
    MONTHSTART %d DAYSTART %d DATAEND %d\n",fram.intframWords,FRAMDATE,GUARDM,SCRATCH,
             BEATSTART, MID, MAXAMP,MINAMP,BPK,BEATLIFE, MKWHSTART,
             MUPDATE, LIFEKWH, LIFEDATE,
              MONTHSTART, DAYSTART, DATAEND);
       */      
}

static void IRAM_ATTR pcnt_example_intr_handler(void *arg)
{
    int pcnt_unit = (int)arg;
    pcnt_evt_t evt;
    evt.unit = pcnt_unit;
    pcnt_get_event_status((pcnt_unit_t)pcnt_unit, &evt.status);
    xQueueSendFromISR(pcnt_evt_queue, &evt, NULL);
}

static void pcnt_example_init(pcnt_unit_t unit)
{
    /* Prepare configuration for the PCNT unit */
    // printf("Install Unit %d GPIO %d HL %d\n",unit,ssignal[unit],medidor[unit].bpk/100);
    bzero(&pcnt_config,sizeof(pcnt_config[unit]));

    pcnt_config[unit] .pulse_gpio_num = ssignal[unit];
    pcnt_config[unit] .ctrl_gpio_num = -1;
    pcnt_config[unit] .channel = PCNT_CHANNEL_0;
    pcnt_config [unit].unit =unit;
    pcnt_config [unit].pos_mode = PCNT_COUNT_INC;   // Count up on the positive edge
    pcnt_config[unit] .neg_mode = PCNT_COUNT_DIS;   // Keep the counter value on the negative edge
    pcnt_config[unit] .counter_h_lim =1;        // max loss is 10% of BPK of meter
    pcnt_unit_config(&pcnt_config[unit]);

    /* Configure and enable the input filter */
    pcnt_set_filter_value(unit, 1000);
    pcnt_filter_enable(unit);
    pcnt_event_enable(unit, PCNT_EVT_H_LIM);
    pcnt_counter_pause(unit);
    pcnt_counter_clear(unit);

    if(!donef)
    {
        pcnt_isr_service_install(0);
        donef=true;
    }

    pcnt_isr_handler_add(unit, pcnt_example_intr_handler, (void *)unit);
    pcnt_counter_resume(unit);
}

void erase_config()
{
 
    wifi_prov_mgr_reset_provisioning();
    esp_wifi_restore();
    nvs_flash_init();
    bzero(&theConf,sizeof(theConf));
    theConf.centinel=CENTINEL;
    theConf.provincia=0;
    theConf.canton=0;
    theConf.parroquia=0;
    theConf.codpostal=0;
    time((time_t*)&theConf.bornDate);
    theConf.mqttSlots=125;
    theConf.pubCycle=6;
    write_to_flash();
}

void testMqtt(void * pArg)
{

// printf("Start test\n");
    uint32_t starttest=xmillis();
	// int err=esp_mqtt_client_start(clientCloud);
    // if(err)
    // {
    //     printf("Could not start Mqtt %d %x\n",err,err);
    //     // vTaskDelete(NULL);
    // }
    // printf("Waiting connect\n");
    xEventGroupWaitBits(wifi_event_group, MQTT_BIT, false, true,  portMAX_DELAY/ portTICK_RATE_MS);

    xEventGroupClearBits(wifi_event_group, MQTT_BIT);	// clear bit to wait on
    // printf("Sending mqtt %s\n",infoQueue);
    esp_mqtt_client_publish(clientCloud, infoQueue, (char*)pArg,strlen((char*)pArg), 1,0);

    xEventGroupWaitBits(wifi_event_group, MQTT_BIT, false, true,  portMAX_DELAY/ portTICK_RATE_MS);
    uint32_t endtest=xmillis();
    printf("Mqtt test ended duration %d\n",endtest-starttest);
    // err=esp_mqtt_client_stop(clientCloud);

    // vTaskDelete(NULL);
}

 void repeatCallback( TimerHandle_t xTimer )
 {
    printf("Repeat timer\n");
    testMqtt((void*)"Repeat");

 }

 void firstCallback( TimerHandle_t xTimer )
 {
    printf("First TImer called\n");
    repeatTimer=xTimerCreate("Timer",pdMS_TO_TICKS(10000),pdTRUE,( void * ) 0, repeatCallback);
    // repeatTimer=xTimerCreate("Timer",pdMS_TO_TICKS(86400000),pdFALSE,( void * ) 0, repeatCallback);
    // if( xTimerStart(repeatTimer, 0 ) != pdPASS )
    // {
    //     printf("Repeat Timer failed\n");
    // }
    testMqtt((void*)"First");
 }

void set_senddata_timer()
{
    // get our defined time slot based on the controllerid and expected publish time... in seconds
    // mqttSlots will be defined by default and modified by Host via any cmd message with SLOT primitive in Json msg
    // same for pubCycle CYCLE which is the average time to publish(send) a controllers Data Message containing each meters id and current kwh

    timeSlotStart=theConf.controllerid/theConf.mqttSlots*theConf.pubCycle;
    int incr=theConf.controllerid % theConf.mqttSlots;
    if (incr)
        timeSlotStart++;
   timeSlotStart *=theConf.pubCycle;

    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);           //right now
    // char *buf=(char*)malloc(300);
    // if(buf)
    // {
    //     bzero(buf,300);
    //     strftime(buf, 300, "%c", &timeinfo);
    //     printf("Now %s\n",buf);
    //     free(buf);
    // }
    timeinfo.tm_mday++;
    timeinfo.tm_hour=0;
    timeinfo.tm_min=0;
    timeinfo.tm_sec=0; //midnight
    time_t midnight = mktime(&timeinfo);
    time_t son=midnight-now;
    // buf=(char*)malloc(300);
    // if(buf)
    // {
    //     bzero(buf,300);
    //     strftime(buf, 300, "%c", &timeinfo);
    //     printf("Midnight %s\n",buf);
    //     free(buf);
    // }
    printf("Secs to midnight %d final %d incr %d\n",(uint32_t)son,timeSlotStart,incr);
    // firstimer will be secs to midnight + timeSlotStart then after that a timer every 86400 secs (1 day)

}



void network(void *pArg)
{


   // printf("Starting network\n");
    app_wifi_init();

    wifi_prov_mgr_config_t config = {
    .scheme = wifi_prov_scheme_ble,
    .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM
};

    ESP_ERROR_CHECK(wifi_prov_mgr_init(config));

    bool provisioned = false;
    /* Let's find out if the device is provisioned */
  esp_err_t err=wifi_prov_mgr_is_provisioned(&provisioned);
    // ESP_ERROR_CHECK(wifi_prov_mgr_deinit());

    u8g2_ClearBuffer(&u8g2);
    ssdString(10,38,provisioned?(char*)"WiFI":(char*)"PROV",true);

    err = app_wifi_start(POP_TYPE_NONE,(char*)"PROV_METER",(char *)"csttpstt");
    if (err != ESP_OK) {
        //fail only(i guess) happens when had a SSID and not accesible,s o lets reprovision it
        printf("Could not start Wifi. Reprovisioning!!!\n");
        wifi_prov_mgr_reset_provisioning();
        ESP_ERROR_CHECK(esp_wifi_restore());
        nvs_flash_init();
        esp_restart();
    }

    u8g2_ClearBuffer(&u8g2);
    ssdString(10,38,(char*)"Time",true);
    sntpget();
    u8g2_ClearBuffer(&u8g2);
    ssdString(10,38,(char*)"MQTT",true);

    mqtt_app_start();
    // xTaskCreate(&testMqtt,"mqtt",10240,fecha, 5, NULL);
    firstTimer=xTimerCreate("Timer",pdMS_TO_TICKS(2000),pdFALSE,( void * ) 0, firstCallback);
    if( xTimerStart(firstTimer, 0 ) != pdPASS )
    {
        printf("First Timer failed\n");
    }
    u8g2_ClearBuffer(&u8g2);
    u8g2_SendBuffer(&u8g2);
//check down time and add it to the config structure
    uint32_t lastfecha;
    fram.read_guard((uint8_t*)&lastfecha);
    if(lastfecha>0)
    {
        time_t now;
        time(&now);
        printf("Down time in seconds %d\n",(uint32_t)now-lastfecha);
        theConf.downtime+=(uint32_t)now-lastfecha;
    }
    vTaskDelete(NULL);
}

void app_main(void)
{

	flashSem= xSemaphoreCreateBinary();
	xSemaphoreGive(flashSem);

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
		printf("No free pages erased!!!!\n");
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    read_flash();
    if(theConf.centinel!=CENTINEL)
    {
        printf("Erase Config\n");
        erase_config();
    }

	theConf.lastResetCode=esp_rom_get_reset_reason(0);
    theConf.bootcount++;

    theConf.mqttSlots=100;
    theConf.pubCycle=6;

    write_to_flash();

    init_vars();

    pcnt_evt_queue = xQueueCreate(10, sizeof(pcnt_evt_t));

    init_fram(true);
    // printf("Medidor size %d METRSIZE %d Maxbytes %d\n",sizeof(meterType),DATAEND-BEATSTART,fram.intframWords);

    for(int a=0;a<MAXDEVSS;a++)
        pcnt_example_init((pcnt_unit_t)a);

#ifdef DEBB
    kbd();      //start console
#endif

    xTaskCreate(&displayManager,"displ",10240,NULL, 10, NULL); 	        // show booting sequence active
    xTaskCreate(&network,"displ",10240,NULL, 10, NULL); 	        // show booting sequence active


    uint32_t ahora=0,dif=0;
    time_t now;
    pcnt_evt_t evt;
    portBASE_TYPE res;
    while (true) {      //wait for an event which will be when a High Limit is fired, equivalent to BPK/100
        res = xQueueReceive(pcnt_evt_queue, &evt, portMAX_DELAY / portTICK_PERIOD_MS);
        if (res == pdTRUE) {
            if (evt.status & PCNT_EVT_H_LIM) 
            {
                ahora=xmillis();
                dif=ahora-medidor[evt.unit].lastclock;
                medidor[evt.unit].lastclock=ahora;
                medidor[evt.unit].beat++;
                medidor[evt.unit].beatlife++;
                if(dif>0)
                {
                    amps[evt.unit]=4500.0/dif*AMPSHORA;
                    if(amps[evt.unit]>medidor[evt.unit].maxamp)
                        medidor[evt.unit].maxamp=amps[evt.unit];
                    if(amps[evt.unit]<medidor[evt.unit].minamp)
                        medidor[evt.unit].minamp=amps[evt.unit];
                }
                // printf("Save [%d] van %d dif %d max %.02f min %.02f\n",evt.unit, medidor[evt.unit].beat,dif, medidor[evt.unit].maxamp, medidor[evt.unit].minamp);
                if( medidor[evt.unit].beat>=medidor[evt.unit].bpk)
                {
                    time(&now);
                    localtime_r(&now, &timeinfo);
                    char *buf=(char*)malloc(300);
                    if (buf)
                    {
                        bzero(buf,300);
                        strftime(buf, 300, "%c", &timeinfo);
                        dia=timeinfo.tm_yday;
                        mes=timeinfo.tm_mon;
                        // printf("Write kwh date %s mes %d day %d\n",buf,mes,dia);
                        free(buf);
                    }
                    time(&now);
                    medidor[evt.unit].lifekwh++;
                    medidor[evt.unit].months[mes]++;
                    medidor[evt.unit].days[dia]++;
                    medidor[evt.unit].lastupdate=now;
                    medidor[evt.unit].beat=0;
                    fram.write_meter(evt.unit,(uint8_t*)&medidor[evt.unit],sizeof(meterType));
                    fram.write_guard((uint32_t)now);    //last known date in seconds
                }
                else
                {   
                    suma[evt.unit]++;
                    if( suma[evt.unit]> medidor[evt.unit].bpk/100)
                    {
                        fram.write_beat(evt.unit, medidor[evt.unit].beat);
                        fram.write_beatlife(evt.unit, medidor[evt.unit].beatlife);
                       suma[evt.unit]=0;
                    }
                }
            }
        } 
    }
}
