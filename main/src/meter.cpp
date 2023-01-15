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
            // printf("[CMD]The current date/time in %s is: %s day of Year %d\n", LOCALTIME,buf,timeinfo.tm_yday);
            free(buf);
        }
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
    //    printf( "connected to ap SSID:%s password:%s\n",SSID, PSW);
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

void init_vars()
{

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
}

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
		if(load)
        {
			for (int a=0;a<8;a++)
				fram.read_meter(a,(uint8_t*)&medidor[a],sizeof(meterType));
	    }
        else
            framSem=NULL;
    }
    /*
    printf("FramSize %d\n FRAMDATE %d GUARDM %d SCRATCH %d\
    BEATSTART %d MID %d BEATLIFE %d MKWMSTART %d\
    MUPDATE %d LIFEKWH %d LIFEDATE %d\
    MONTHSTART %d DAYSTART %d DATAEND %d\n",fram.intframWords,FRAMDATE,GUARDM,SCRATCH,
             BEATSTART, MID, BEATLIFE, MKWHSTART,
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
    pcnt_config[unit] .counter_h_lim =medidor[unit].bpk/100;
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

void app_main(void)
{


    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
		printf("No free pages erased!!!!\n");
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    init_vars();
    pcnt_evt_queue = xQueueCreate(10, sizeof(pcnt_evt_t));
    init_fram(true);
    // printf("Medidor size %d METRSIZE %d Maxbytes %d\n",sizeof(meterType),DATAEND-BEATSTART,fram.intframWords);

    for(int a=0;a<MAXDEVSS;a++)
        pcnt_example_init((pcnt_unit_t)a);

#ifdef DEBB
    kbd();      //start console
#endif
    xTaskCreate(&wifi_init_sta,"wifi",10240,NULL, 10, NULL); 	        // show booting sequence active
    xTaskCreate(&displayManager,"displ",10240,NULL, 10, NULL); 	        // show booting sequence active

    pcnt_evt_t evt;
    portBASE_TYPE res;
    while (true) {
        res = xQueueReceive(pcnt_evt_queue, &evt, portMAX_DELAY / portTICK_PERIOD_MS);
        if (res == pdTRUE) {
            if (evt.status & PCNT_EVT_H_LIM) 
            {
                medidor[evt.unit].beat+=medidor[evt.unit].bpk/100;
                medidor[evt.unit].beatlife+=medidor[evt.unit].bpk/100;

                printf("Save [%d] van %d\n",evt.unit, medidor[evt.unit].beat);
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
                        printf("Write kwh date %s mes %d day %d\n",buf,mes,dia);
                        free(buf);
                    }
                    medidor[evt.unit].lifekwh++;
                    medidor[evt.unit].months[mes]++;
                    medidor[evt.unit].days[dia]++;
                    medidor[evt.unit].lastupdate=now;
                    medidor[evt.unit].beat=0;
                    fram.write_meter(evt.unit,(uint8_t*)&medidor[evt.unit],sizeof(meterType));
                }
                else
                {
                    fram.write_beat(evt.unit, medidor[evt.unit].beat);
                    fram.write_beatlife(evt.unit, medidor[evt.unit].beatlife);
                }
            }
        } 
    }
}
