#include "includes.h"
#include "globals.h"
#include "forwards.h"

#define SSID "Porton"
#define PSW "csttpstt"

// xQueueHandle pcnt_evt_queue;   // A queue to handle pulse counter events

void mqtt_sender(void *pArg);
static void    mqtt_app_start();

void delay(uint32_t cuanto)
{
    vTaskDelay(cuanto / portTICK_PERIOD_MS);
}

uint32_t xmillis()
{
    return esp_timer_get_time()/1000;
}

uint32_t xmillisFromISR()
{
	return xTaskGetTickCountFromISR() * portTICK_PERIOD_MS;
}

void static mesh_recv_cb(mesh_addr_t *from, mesh_data_t *data)
{
        mqttSender_t mqttMsg;

    // if (data->data[0] == CMD_ROUTE_TABLE) {
    //     int size =  data->size - 1;
    //     if (s_route_table_lock == NULL || size%6 != 0) {
    //         ESP_LOGE(MESH_TAG, "Error in receiving raw mesh data: Unexpected size");
    //         return;
    //     }
    //     xSemaphoreTake(s_route_table_lock, portMAX_DELAY);
    //     s_route_table_size = size / 6;
    //     for (int i=0; i < s_route_table_size; ++i) {
    //         ESP_LOGI(MESH_TAG, "Received Routing table [%d] "
    //                 MACSTR, i, MAC2STR(data->data + 6*i + 1));
    //     }
    //     memcpy(&s_route_table, data->data + 1, size);
    //     xSemaphoreGive(s_route_table_lock);
    // } else if (data->data[0] == CMD_KEYPRESSED) {
    //     if (data->size != 7) {
    //         ESP_LOGE(MESH_TAG, "Error in receiving raw mesh data: Unexpected size");
    //         return;
    //     }
    //     ESP_LOGW(MESH_TAG, "Keypressed detected on node: "
    //             MACSTR, MAC2STR(data->data + 1));
    // } else {
    //     ESP_LOGE(MESH_TAG, "Error in receiving raw mesh data: Unknown command");
    // }
    // printf("Msg in [%s] from " MACSTR "\n",(char *)data->data, MAC2STR(from->addr));
    char *mensaje=(char *)malloc(1000);
    if(!mensaje)
    {
        printf("Now RAM for mesh msg\n");
        return;
    }
    strcpy(mensaje,(char *)data->data);
    mqttMsg.msg=mensaje;
    mqttMsg.lenMsg=strlen(mensaje);
    // printf("Sending mqtt mesh msg [%s]\n",mensaje);
     if(xQueueSend(mqttSender,&mqttMsg,0)!=pdPASS)
        {
            printf("Error queueing msg\n");
            if(mqttMsg.msg)
                free(mqttMsg.msg);  //due to failure
        }
        else
            //must set the wifi_event_bit SEND_MQTT_BIT, else it will just collect the message in the queue
            xEventGroupSetBits(wifi_event_group, SENDMQTT_BIT);	// Send everything now !!!!!
    
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
const int WIFI_CONNECTED_EVENT = BIT0;

static void event_handler_prov(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
#ifdef CONFIG_EXAMPLE_RESET_PROV_MGR_ON_FAILURE
    static int retries;
#endif
    if (event_base == WIFI_PROV_EVENT) {
        switch (event_id) {
            case WIFI_PROV_START:
                ESP_LOGI(MESH_TAG, "Provisioning started");
                break;
            case WIFI_PROV_CRED_RECV: {
                wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
                ESP_LOGI(MESH_TAG, "Received Wi-Fi credentials"
                         "\n\tSSID     : %s\n\tPassword : %s",
                         (const char *) wifi_sta_cfg->ssid,
                         (const char *) wifi_sta_cfg->password);
                break;
            }
            case WIFI_PROV_CRED_FAIL: {
                wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
                ESP_LOGE(MESH_TAG, "Provisioning failed!\n\tReason : %s"
                         "\n\tPlease reset to factory and retry provisioning",
                         (*reason == WIFI_PROV_STA_AUTH_ERROR) ?
                         "Wi-Fi station authentication failed" : "Wi-Fi access-point not found");
                break;
            }
            case WIFI_PROV_CRED_SUCCESS:
                ESP_LOGI(MESH_TAG, "Provisioning successful");
                break;
            case WIFI_PROV_END:
                ESP_LOGI(MESH_TAG, "Provisioning Ended");
                /* De-initialize manager once provisioning is finished */
                wifi_prov_mgr_deinit();
                break;
            default:
                break;
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(MESH_TAG, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
        /* Signal main application to continue execution */
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_EVENT);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(MESH_TAG, "Disconnected. Connecting to the AP again...");
        esp_wifi_connect();
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
        // ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
    //    printf( "got ip:" IPSTR "\n", IP2STR(&event->ip_info.ip));
        // gpio_set_level((gpio_num_t)2,1);
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

    }
}

void sntpget(void *pArg)
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
            vTaskDelete(NULL);
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
             printf("[CMD]The current date/time in %s is: %s day of Year %d\n", LOCALTIME,buf,timeinfo.tm_yday);
            free(buf);
        }
        set_senddata_timer();
    }
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

				//  printf("Mqtt connected\n");

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
				// printf("Mqtt Disco\n");

        	mqttf=false;
            // xEventGroupClearBits(wifi_event_group, MQTT_BIT);
            break;
        case MQTT_EVENT_SUBSCRIBED:
        //  printf("Sub done\n");
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
            xEventGroupSetBits(wifi_event_group, PUB_BIT);//message sent bit
#endif
            break;
        case MQTT_EVENT_DATA:
            bzero(&mqttHandle,sizeof(mqttHandle));
            msg=(char*)malloc(1000);
            bzero(msg,1000);
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

    while(true)
    {
        if( xQueueReceive( mqttQ, &mqttHandle, portMAX_DELAY ))	//mqttHandle has a pointer to the original message. MUST be freed at some point
        {
            printf("Message In MQtt %s len %d\n",mqttHandle.message,mqttHandle.msgLen);
            elcmd= cJSON_Parse((char*)mqttHandle.message);		//plain text to cJSON... must eventually cDelete elcmd
			if(elcmd)
			{
				cJSON *order= 		cJSON_GetObjectItem(elcmd,"cmd");
                if(order)
                {
                    int cual=findCommand(order->valuestring);
                    if(cual>=0)
                    {
#ifdef DISPLAY
                        u8g2_ClearBuffer(&u8g2);
                        ssdString(10,38,cmds[cual].abr,true);
#endif
                        (*cmds[cual].code)((void*)elcmd);	// call the cmd and wait for it to end
                    }
                    else    
                        printf("Invalid cmd received %s\n",order->valuestring);
                }
                cJSON_Delete(elcmd);
            }
            else
                printf("Invalid parse\n");

            if(mqttHandle.message)
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
    printf("MQTT NodeId %s\n",who);
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
        printf("Failed queue Cmd\n");
    mqttSender = xQueueCreate( 20, sizeof( mqttSender_t ) );
    if(!mqttSender)
        printf("Failed queue Sender\n");
    else
        printf("Mqttsender created\n"); 

    clientCloud=NULL;

    clientCloud = esp_mqtt_client_init(&mqtt_cfg);

    if(clientCloud)
    {
        printf("Mqtt started\n");
        xTaskCreate(&mqttMgr,"mqtt",10240,NULL, 5, NULL);
        xTaskCreate(&mqtt_sender,"mqttsend",10240,NULL, 5, NULL);
    }
    else
    {
        printf("Not configured Mqtt\n");	//should crash. No conn with Host defeats the purpose but lets save beats at least
        return;
    }
}

void esp_mesh_p2p_rx_main(void *arg)
{
    int recv_count = 0;
    uint32_t aca;
    esp_err_t err;
    mesh_addr_t from;
    int send_count = 0;
    mesh_data_t data;
    int flag = 0;
    data.data = (uint8_t*)malloc(1000);;
    data.size = 1000;
    is_running = true;
        mqttSender_t mqttMsg;


    while (true) {
        data.size = 1000;
        aca=0;
        err = esp_mesh_recv(&from, &data, portMAX_DELAY, &flag, NULL, 0);
        if (err != ESP_OK || !data.size) {
            ESP_LOGE(MESH_TAG, "err:0x%x, size:%d", err, data.size);
            continue;
        }      
        memcpy(&aca,data.data,4);
        if (aca!=0xffffffff)
        {
            // esp_log_buffer_hex("NADA",data.data,data.size);
            ESP_LOGI(MESH_TAG,
                        "%s [L:%d] parent:"MACSTR", receive from "MACSTR", size:%d, heap:%d, flag:%d[err:0x%x, proto:%d, tos:%d]",data.data,
                        mesh_layer, MAC2STR(mesh_parent_addr.addr), MAC2STR(from.addr),
                        data.size, esp_get_minimum_free_heap_size(), flag, err, data.proto,
                        data.tos);
        

         char *mensaje=(char *)malloc(1000);
    if(!mensaje)
    {
        printf("Now RAM for mesh msg\n");
        return;
    }
    strcpy(mensaje,(char *)data.data);
    mqttMsg.msg=mensaje;
    mqttMsg.lenMsg=strlen(mensaje);
    printf("Sending mqtt mesh msg [%s]\n",mensaje);
     if(xQueueSend(mqttSender,&mqttMsg,0)!=pdPASS)
        {
            printf("Error queueing msg\n");
            if(mqttMsg.msg)
                free(mqttMsg.msg);  //due to failure
        }
        else
            //must set the wifi_event_bit SEND_MQTT_BIT, else it will just collect the message in the queue
            xEventGroupSetBits(wifi_event_group, SENDMQTT_BIT);	// Send everything now !!!!!
        }
    }
}



void mesh_event_handler(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data)
{
    mesh_addr_t id = {0,};
    static uint8_t last_layer = 0;
// printf("Mesh event %d \n",event_id);

    switch (event_id) {
    case MESH_EVENT_STARTED: {
        esp_mesh_get_id(&id);
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_MESH_STARTED>ID:"MACSTR"", MAC2STR(id.addr));
        mesh_layer = esp_mesh_get_layer();
    }
    break;
    case MESH_EVENT_STOPPED: {
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_STOPPED>");
        mesh_layer = esp_mesh_get_layer();
    }
    break;
    case MESH_EVENT_CHILD_CONNECTED: {
        mesh_event_child_connected_t *child_connected = (mesh_event_child_connected_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHILD_CONNECTED>aid:%d, "MACSTR"",
                 child_connected->aid,
                 MAC2STR(child_connected->mac));
    }
    break;
    case MESH_EVENT_CHILD_DISCONNECTED: {
        mesh_event_child_disconnected_t *child_disconnected = (mesh_event_child_disconnected_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHILD_DISCONNECTED>aid:%d, "MACSTR"",
                 child_disconnected->aid,
                 MAC2STR(child_disconnected->mac));
    }
    break;
    case MESH_EVENT_ROUTING_TABLE_ADD: {
        mesh_event_routing_table_change_t *routing_table = (mesh_event_routing_table_change_t *)event_data;
        ESP_LOGW(MESH_TAG, "<MESH_EVENT_ROUTING_TABLE_ADD>add %d, new:%d",
                 routing_table->rt_size_change,
                 routing_table->rt_size_new);
    }
    break;
    case MESH_EVENT_ROUTING_TABLE_REMOVE: {
        mesh_event_routing_table_change_t *routing_table = (mesh_event_routing_table_change_t *)event_data;
        ESP_LOGW(MESH_TAG, "<MESH_EVENT_ROUTING_TABLE_REMOVE>remove %d, new:%d",
                 routing_table->rt_size_change,
                 routing_table->rt_size_new);
    }
    break;
    case MESH_EVENT_NO_PARENT_FOUND: {
        mesh_event_no_parent_found_t *no_parent = (mesh_event_no_parent_found_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_NO_PARENT_FOUND>scan times:%d",
                 no_parent->scan_times);
    }
    /* TODO handler for the failure */
    break;
    case MESH_EVENT_PARENT_CONNECTED: {
        mesh_event_connected_t *connected = (mesh_event_connected_t *)event_data;
        esp_mesh_get_id(&id);
        mesh_layer = connected->self_layer;
        memcpy(&mesh_parent_addr.addr, connected->connected.bssid, 6);
        ESP_LOGI(MESH_TAG,
                 "<MESH_EVENT_PARENT_CONNECTED>layer:%d-->%d, parent:"MACSTR"%s, ID:"MACSTR"",
                 last_layer, mesh_layer, MAC2STR(mesh_parent_addr.addr),
                 esp_mesh_is_root() ? "<ROOT>" :
                 (mesh_layer == 2) ? "<layer2>" : "", MAC2STR(id.addr));
        last_layer = mesh_layer;
        mesh_netifs_start(esp_mesh_is_root());
    }
    break;
    case MESH_EVENT_PARENT_DISCONNECTED: {
        mesh_event_disconnected_t *disconnected = (mesh_event_disconnected_t *)event_data;
        ESP_LOGI(MESH_TAG,
                 "<MESH_EVENT_PARENT_DISCONNECTED>reason:%d",
                 disconnected->reason);
        mesh_layer = esp_mesh_get_layer();
        mesh_netifs_stop();
    }
    break;
    case MESH_EVENT_LAYER_CHANGE: {
        mesh_event_layer_change_t *layer_change = (mesh_event_layer_change_t *)event_data;
        mesh_layer = layer_change->new_layer;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_LAYER_CHANGE>layer:%d-->%d%s",
                 last_layer, mesh_layer,
                 esp_mesh_is_root() ? "<ROOT>" :
                 (mesh_layer == 2) ? "<layer2>" : "");
        last_layer = mesh_layer;
    }
    break;
    case MESH_EVENT_ROOT_ADDRESS: {
        mesh_event_root_address_t *root_addr = (mesh_event_root_address_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_ADDRESS>root address:"MACSTR"",
                 MAC2STR(root_addr->addr));
    }
    break;
    case MESH_EVENT_VOTE_STARTED: {
        mesh_event_vote_started_t *vote_started = (mesh_event_vote_started_t *)event_data;
        ESP_LOGI(MESH_TAG,
                 "<MESH_EVENT_VOTE_STARTED>attempts:%d, reason:%d, rc_addr:"MACSTR"",
                 vote_started->attempts,
                 vote_started->reason,
                 MAC2STR(vote_started->rc_addr.addr));
    }
    break;
    case MESH_EVENT_VOTE_STOPPED: {
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_VOTE_STOPPED>");
        break;
    }
    case MESH_EVENT_ROOT_SWITCH_REQ: {
        mesh_event_root_switch_req_t *switch_req = (mesh_event_root_switch_req_t *)event_data;
        ESP_LOGI(MESH_TAG,
                 "<MESH_EVENT_ROOT_SWITCH_REQ>reason:%d, rc_addr:"MACSTR"",
                 switch_req->reason,
                 MAC2STR( switch_req->rc_addr.addr));
    }
    break;
    case MESH_EVENT_ROOT_SWITCH_ACK: {
        /* new root */
        mesh_layer = esp_mesh_get_layer();
        esp_mesh_get_parent_bssid(&mesh_parent_addr);
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_SWITCH_ACK>layer:%d, parent:"MACSTR"", mesh_layer, MAC2STR(mesh_parent_addr.addr));
    }
    break;
    case MESH_EVENT_TODS_STATE: {
        mesh_event_toDS_state_t *toDs_state = (mesh_event_toDS_state_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_TODS_REACHABLE>state:%d", *toDs_state);
    }
    break;
    case MESH_EVENT_ROOT_FIXED: {
        mesh_event_root_fixed_t *root_fixed = (mesh_event_root_fixed_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_FIXED>%s",
                 root_fixed->is_fixed ? "fixed" : "not fixed");
    }
    break;
    case MESH_EVENT_ROOT_ASKED_YIELD: {
        mesh_event_root_conflict_t *root_conflict = (mesh_event_root_conflict_t *)event_data;
        ESP_LOGI(MESH_TAG,
                 "<MESH_EVENT_ROOT_ASKED_YIELD>"MACSTR", rssi:%d, capacity:%d",
                 MAC2STR(root_conflict->addr),
                 root_conflict->rssi,
                 root_conflict->capacity);
    }
    break;
    case MESH_EVENT_CHANNEL_SWITCH: {
        mesh_event_channel_switch_t *channel_switch = (mesh_event_channel_switch_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHANNEL_SWITCH>new channel:%d", channel_switch->channel);
    }
    break;
    case MESH_EVENT_SCAN_DONE: {
        mesh_event_scan_done_t *scan_done = (mesh_event_scan_done_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_SCAN_DONE>number:%d",
                 scan_done->number);
    }
    break;
    case MESH_EVENT_NETWORK_STATE: {
        mesh_event_network_state_t *network_state = (mesh_event_network_state_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_NETWORK_STATE>is_rootless:%d",
                 network_state->is_rootless);
    }
    break;
    case MESH_EVENT_STOP_RECONNECTION: {
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_STOP_RECONNECTION>");
    }
    break;
    case MESH_EVENT_FIND_NETWORK: {
        mesh_event_find_network_t *find_network = (mesh_event_find_network_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_FIND_NETWORK>new channel:%d, router BSSID:"MACSTR"",
                 find_network->channel, MAC2STR(find_network->router_bssid));
    }
    break;
    case MESH_EVENT_ROUTER_SWITCH: {
        mesh_event_router_switch_t *router_switch = (mesh_event_router_switch_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROUTER_SWITCH>new router:%s, channel:%d, "MACSTR"",
                 router_switch->ssid, router_switch->channel, MAC2STR(router_switch->bssid));
    }
    break;
    default:
        ESP_LOGI(MESH_TAG, "unknown id:%d", event_id);
        break;
    }
}


void init_vars()
{
    gpio_config_t 	        io_conf;

    // mesh stuff 
    mesh_layer = -1;
    // mesh id as we cannot use static variable configuration
    memset(MESH_ID,0x77,6);
    // printf("Mesh %x\n",theConf.meshid);
    MESH_ID[5]=theConf.meshid;

//cmd and info queue names
    sprintf(cmdQueue,"meter/%d/%d/%d/%d/%d/cmd",theConf.provincia,theConf.canton,theConf.parroquia,theConf.codpostal,theConf.controllerid);
    sprintf(infoQueue,"meter/%d/%d/%d/%d/%d/info",theConf.provincia,theConf.canton,theConf.parroquia,theConf.codpostal,theConf.controllerid);
    printf("Cmd Q %s\nInfo %s\n",cmdQueue,infoQueue);

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
    strcpy((char*)&cmds[x].comando,         "restore");			        cmds[x].code=cmdRestore;		strcpy((char*)&cmds[x].abr,         "RST");		
	strcpy((char*)&cmds[++x].comando,       "format");			        cmds[x].code=cmdFormat;			strcpy((char*)&cmds[x].abr,         "FRMT");		
    strcpy((char*)&cmds[++x].comando,       "formatmeter");			    cmds[x].code=cmdFormatMeter;	strcpy((char*)&cmds[x].abr,         "FRMTM");    				
	strcpy((char*)&cmds[++x].comando,       "initmeter");			    cmds[x].code=cmdInitMeter;		strcpy((char*)&cmds[x].abr,         "INTIM");			
	strcpy((char*)&cmds[++x].comando,       "metrics");			        cmds[x].code=cmdMetrics;		strcpy((char*)&cmds[x].abr,         "METRC");			
	strcpy((char*)&cmds[++x].comando,       "controller");		        cmds[x].code=cmdController;		strcpy((char*)&cmds[x].abr,         "CNTRL");			
	strcpy((char*)&cmds[++x].comando,       "update");		            cmds[x].code=cmdUpdate;			strcpy((char*)&cmds[x].abr,         "UPDT");		
	strcpy((char*)&cmds[++x].comando,       "erase");		            cmds[x].code=cmdErase;			strcpy((char*)&cmds[x].abr,         "ERSE");		
	strcpy((char*)&cmds[++x].comando,       "ota");		                cmds[x].code=cmdOTA;			strcpy((char*)&cmds[x].abr,         "OTA");		

	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pull_up_en =GPIO_PULLUP_ENABLE;
	io_conf.pull_down_en =GPIO_PULLDOWN_DISABLE;
	io_conf.pin_bit_mask = (1ULL<<0);     //input pins
	gpio_config(&io_conf);
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
        bzero(&lastkwh[0],sizeof(lastkwh));
		//load all devices counters from FRAM
		// fram.write_guard(theGuard);				// theguard is dynamic and will change every boot.
        // printf("Medidor size %d\n",sizeof(medidor[0]));
		if(load)
        {
			for (int a=0;a<8;a++)
            {
				fram.read_meter(a,(uint8_t*)&medidor[a],sizeof(meterType));
                medidor[a].lastclock=xmillis();
                lastkwh[a]=medidor[a].lifekwh;      //track progress to avoid sending same reading every time 
                if(medidor[a].lifekwh>medidor[a].kwhstart)  // see if meter has had activity 
                    medidorlock[a]=true;    //lock it, cannot be changed
                else   
                    medidorlock[a]=false;   // not locked, can be modified by comfiguration manager
                    // printf("Med[%d] %s\n",a,medidorlock[a]?"Y":"N");
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
    srand(time(NULL));
    wifi_prov_mgr_reset_provisioning();
    esp_wifi_restore();
    nvs_flash_erase();
    nvs_flash_init();
    bzero(&theConf,sizeof(theConf));
    theConf.controllerid = 1 + (rand() % 999999);       //will be displayed as the node for configuration
    theConf.centinel=CENTINEL;
    time((time_t*)&theConf.bornDate);
    theConf.mqttSlots=125;
    theConf.pubCycle=6;
    theConf.loglevel=3;
    write_to_flash();
}

void mqtt_sender(void *pArg)        // MQTTT data sender task
{
    mqttSender_t mensaje;
    uint32_t starttest,endtest;
    int err;

    xEventGroupClearBits(wifi_event_group, SENDMQTT_BIT);	// clear bit to wait on

    while(true)
    {
        xEventGroupWaitBits(wifi_event_group,SENDMQTT_BIT,pdFALSE,true,portMAX_DELAY);    //wait forever, this is the starting gun
        xEventGroupClearBits(wifi_event_group, SENDMQTT_BIT);	// clear bit to wait on for next msg

        starttest=xmillis();
	    err=esp_mqtt_client_start(clientCloud);         //start a MQTT connection, wait for it in MQTT_BIT
        if(err)
            printf("Could not start Mqtt %d %x\n",err,err);

        xEventGroupWaitBits(wifi_event_group, MQTT_BIT, false, true,  portMAX_DELAY/ portTICK_RATE_MS); //what happens if timeout???

        // CRITICAL READ. Tema Latencia de RX o TX de WiFi
        // Delay required so that the MQTT Task is able to perform internal functions and receive data
        //delay(100); // NEEDs this time to "think" I guess or work on getting messages etc. 1000 seen as minimum but whne changing AMPDU in Menuconfig 
        // to DISABLED it works WITHOUT delays

        while(true)
        {
            if( xQueueReceive( mqttSender, &mensaje, 10 ))	//mqttHandle has a pointer to the original message. MUST be freed at some point
            {
                if(mensaje.msg)
                {
                    xEventGroupClearBits(wifi_event_group, PUB_BIT);	// clear bit to wait on
                    esp_mqtt_client_publish(clientCloud, infoQueue, (char*)mensaje.msg,strlen((char*)mensaje.msg), 1,0);
                    // get confirmation of msg being published
                    xEventGroupWaitBits(wifi_event_group, PUB_BIT, false, true,  portMAX_DELAY/ portTICK_RATE_MS);
                    endtest=xmillis();
                    printf("Mqtt time %dms\n",endtest-starttest);
                    starttest=xmillis();
                    free(mensaje.msg);
                }               
            }
            else
            {
                err=esp_mqtt_client_stop(clientCloud);
                if(err)
                    printf("Error stoping mqtt %x\n",err);
                break;
            }
        }
    }
}


 void repeatCallback( TimerHandle_t xTimer )
 {
    time_t now;
    struct tm timeinfo;
    mqttSender_t mensaje;

    // printf("Repeat timer\n");
    time(&now);

    localtime_r(&now, &timeinfo);
    mensaje.msg=sendData(false);            //will be freed by sender
    if(mensaje.msg)
        mensaje.lenMsg=strlen(mensaje.msg);
    if(mensaje.msg!=NULL)   //if somtehting to send do it
    {
        if(xQueueSend(mqttSender,&mensaje,0)!=pdPASS)
        {
            printf("Error queueing msg\n");
            if(mensaje.msg)
                free(mensaje.msg);  //due to failure
        }
        else
            //must set the wifi_event_bit SEND_MQTT_BIT, else it will just collect the message in the queue
            xEventGroupSetBits(wifi_event_group, SENDMQTT_BIT);	// Send everything now !!!!!
    }
 }

/**
 * ? que sera
 * ! warning
 * * Important
 * TODO: algo
 * @param xTimer este parametro
 */
 void firstCallback( TimerHandle_t xTimer )
 {
    time_t now;
    struct tm timeinfo;
    mqttSender_t mensaje;

    time(&now);

    localtime_r(&now, &timeinfo);
    mensaje.msg=sendData(true);            //will be freed by sender, Force sending first caller to allow incoming messages
    if(mensaje.msg)
    {
        mensaje.lenMsg=strlen(mensaje.msg);
        printf("FirstTimer %s\n",mensaje.msg);
    }
    if(mensaje.msg!=NULL)   //if somtehting to send do it
    {
        if(xQueueSend(mqttSender,&mensaje,0)!=pdPASS)
        {
            printf("Error queueing msg\n");
            if(mensaje.msg)
                free(mensaje.msg);  //due to failure
        };
    }
    // repeatTimer=xTimerCreate("Timer",pdMS_TO_TICKS(20000),pdTRUE,( void * ) 0, repeatCallback);    // every 20secs for now
    repeatTimer=xTimerCreate("Timer",pdMS_TO_TICKS(3600000),pdTRUE,( void * ) 0, repeatCallback);    // every hour for now
    // repeatTimer=xTimerCreate("Timer",pdMS_TO_TICKS((theConf.mqttSlots*theConf.pubCycle+1)*1000),pdTRUE,( void * ) 0, repeatCallback);
    // repeatTimer=xTimerCreate("Timer",pdMS_TO_TICKS(86400000),pdFALSE,( void * ) 0, repeatCallback);

    if( xTimerStart(repeatTimer, 0 ) != pdPASS )
    {
        printf("Repeat Timer failed\n");
    }

    xEventGroupSetBits(wifi_event_group, SENDMQTT_BIT);	// Send everything !!!!!!

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
    timeinfo.tm_mday++;
    timeinfo.tm_hour=0;
    timeinfo.tm_min=0;
    timeinfo.tm_sec=0; //midnight
    time_t midnight = mktime(&timeinfo);
    time_t son=midnight-now;
    printf("Secs to midnight %d final %d incr %d RECYCLE %dsecs\n",(uint32_t)son,timeSlotStart,incr,theConf.mqttSlots*theConf.pubCycle+1);
    // firstimer will be secs to midnight + timeSlotStart then after that a timer every 86400 secs (1 day)
}

void post_root()
{
    u8g2_ClearBuffer(&u8g2);
    ssdString(10,38,(char*)"Time",true);
    xTaskCreate(&sntpget,"sntp",10240,NULL, 10, NULL); 	        // show booting sequence active
    u8g2_ClearBuffer(&u8g2);
    ssdString(10,38,(char*)"MQTT",true);

    // xTaskCreate(&testMqtt,"mqtt",10240,fecha, 5, NULL);
    firstTimer=xTimerCreate("Timer",pdMS_TO_TICKS(2000),pdFALSE,( void * ) 0, firstCallback);
    if( xTimerStart(firstTimer, 0 ) != pdPASS )
    {
        printf("First Timer failed\n");
    }
    u8g2_ClearBuffer(&u8g2);
    u8g2_SendBuffer(&u8g2);
    
    if (!theConf.nodeConf)
    {
        char tmp[12];
        sprintf(tmp,"%d",theConf.controllerid);
        printf("Node %s\n",tmp);
        u8g2_ClearBuffer(&u8g2);
        ssdString(10,38,tmp,true);
    }
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
        //  xTaskCreate(&start_webserver,"web",10240,NULL, 10, NULL); 	        // show booting sequence active

}

void ip_event_handler(void *arg, esp_event_base_t event_base,
                      int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    ESP_LOGI(MESH_TAG, "<IP_EVENT_STA_GOT_IP>IP:" IPSTR, IP2STR(&event->ip_info.ip));
    s_current_ip.addr = event->ip_info.ip.addr;
#if !CONFIG_MESH_USE_GLOBAL_DNS_IP
    esp_netif_t *netif = event->esp_netif;
    esp_netif_dns_info_t dns;
    ESP_ERROR_CHECK(esp_netif_get_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns));
    mesh_netif_start_root_ap(esp_mesh_is_root(), dns.ip.u_addr.ip4.addr);
#endif
    // esp_mesh_comm_mqtt_task_start();
         if (esp_mesh_is_root()) {
    mqtt_app_start();
    post_root();
 }
}

static void get_device_service_name(char *service_name, size_t max)
{
    uint8_t eth_mac[6];
    const char *ssid_prefix = "PROV_";
    esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
    snprintf(service_name, max, "%s%02X%02X%02X",
             ssid_prefix, eth_mac[3], eth_mac[4], eth_mac[5]);
}

esp_err_t new_provision()
{

    /* Register our event handler for Wi-Fi, IP and Provisioning related events */
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler_prov, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler_prov, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler_prov, NULL));

    /* Initialize Wi-Fi including netif with default config */
    esp_sta=esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* Configuration for the provisioning manager */
    wifi_prov_mgr_config_t config = {
        .scheme = wifi_prov_scheme_ble,
        .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM
    };

    /* Initialize provisioning manager with the
     * configuration parameters set above */
    ESP_ERROR_CHECK(wifi_prov_mgr_init(config));

    bool provisioned = false;
    /* Let's find out if the device is provisioned */
    //    wifi_prov_mgr_reset_provisioning();


    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));

    /* If device is not yet provisioned start provisioning service */
    if (!provisioned) {
        ESP_LOGI(MESH_TAG, "Starting provisioning");

        /* What is the Device Service Name that we want
         * This translates to :
         *     - Wi-Fi SSID when scheme is wifi_prov_scheme_softap
         *     - device name when scheme is wifi_prov_scheme_ble
         */
        char service_name[12];
        get_device_service_name(service_name, sizeof(service_name));

        /* What is the security level that we want (0 or 1):
         *      - WIFI_PROV_SECURITY_0 is simply plain text communication.
         *      - WIFI_PROV_SECURITY_1 is secure communication which consists of secure handshake
         *          using X25519 key exchange and proof of possession (pop) and AES-CTR
         *          for encryption/decryption of messages.
         */
        wifi_prov_security_t security = WIFI_PROV_SECURITY_1;

        /* Do we want a proof-of-possession (ignored if Security 0 is selected):
         *      - this should be a string with length > 0
         *      - NULL if not used
         */
        const char *pop = "csttpstt";

        /* What is the service key (could be NULL)
         * This translates to :
         *     - Wi-Fi password when scheme is wifi_prov_scheme_softap
         *          (Minimum expected length: 8, maximum 64 for WPA2-PSK)
         *     - simply ignored when scheme is wifi_prov_scheme_ble
         */
        const char *service_key = NULL;


        /* This step is only useful when scheme is wifi_prov_scheme_ble. This will
         * set a custom 128 bit UUID which will be included in the BLE advertisement
         * and will correspond to the primary GATT service that provides provisioning
         * endpoints as GATT characteristics. Each GATT characteristic will be
         * formed using the primary service UUID as base, with different auto assigned
         * 12th and 13th bytes (assume counting starts from 0th byte). The client side
         * applications must identify the endpoints by reading the User Characteristic
         * Description descriptor (0x2901) for each characteristic, which contains the
         * endpoint name of the characteristic */
        uint8_t custom_service_uuid[] = {
            /* LSB <---------------------------------------
             * ---------------------------------------> MSB */
            0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b, 0xf4, 0xbf,
            0xea, 0x4a, 0x82, 0x03, 0x04, 0x90, 0x1a, 0x02,
        };

        /* If your build fails with linker errors at this point, then you may have
         * forgotten to enable the BT stack or BTDM BLE settings in the SDK (e.g. see
         * the sdkconfig.defaults in the example project) */
        wifi_prov_scheme_ble_set_service_uuid(custom_service_uuid);


        /* An optional endpoint that applications can create if they expect to
         * get some additional custom data during provisioning workflow.
         * The endpoint name can be anything of your choice.
         * This call must be made before starting the provisioning.
         */
        /* Start provisioning service */
        ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(security, pop, service_name, service_key));

        /* The handler for the optional endpoint created above.
         * This call must be made after starting the provisioning, and only if the endpoint
         * has already been created above.
         */

        /* Uncomment the following to wait for the provisioning to finish and then release
         * the resources of the manager. Since in this case de-initialization is triggered
         * by the default event loop handler, we don't need to call the following */
        // wifi_prov_mgr_wait();
        // wifi_prov_mgr_deinit();
        /* Print QR code for provisioning */

        // printf("Done Prov...waiting\n");
        // delay(1000);
        // esp_restart();

    } 
    else
    {
        // printf("Ya esta\n");
        wifi_prov_mgr_deinit();
        /* Start Wi-Fi station */
        // wifi_init_sta();         // not when using Mesh that does this 
        ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler_prov));
        ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler_prov));
        ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler_prov));
        return ESP_OK;
    }
   
    // printf("Waiting event...\n");
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_EVENT, false, true, portMAX_DELAY);
    // printf("Event done\n");

    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler_prov));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler_prov));
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler_prov));

    return ESP_OK;


}

void park_state()
{
    uint32_t startm,elapsed;
    //check for flash button and time elapsed
    #ifdef DISPLAY
    u8g2_ClearBuffer(&u8g2);
    ssdString(10,38,(char*)"Mesh",true);
#endif
printf("Mesh\n");
    while(true)
    {
        delay(500);
        if(!gpio_get_level((gpio_num_t)0))
        {
                printf("start btn\n");
                //start counting millis >2000 x < 2500 is Non Root Mesh else >2500 provision
                startm=xmillis();
                while(!gpio_get_level((gpio_num_t)0))    // released
                    delay(100);
            
                //button released
                elapsed=xmillis()-startm;
                printf("Elapsed %d\n",elapsed);
            
            if(elapsed>2000)
            {
                if(elapsed>4000)
                {
                    printf("New Provision\n");
                    #ifdef DISPLAY
                    u8g2_ClearBuffer(&u8g2);
                    ssdString(10,38,(char*)"PROV",true);
                    #endif
                    //provision chosen
                    ESP_ERROR_CHECK(esp_netif_init());
                    ESP_ERROR_CHECK(esp_event_loop_create_default());
                    wifi_event_group = xEventGroupCreate();
                    new_provision();
                    theConf.meshconf=1;
                    write_to_flash();

                    esp_restart();
                }
                else
                {
                    #ifdef DISPLAY
                    u8g2_ClearBuffer(&u8g2);
                    ssdString(10,38,(char*)"NROOT",true);
                    #endif
                    printf("Non Root\n");
                    //non root chosen
                    theConf.meshconf=2;
                    write_to_flash();
                    delay(5000);
                    esp_restart();
                }
            }
        }
    }
}


static void wifi_event_handler_ap(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(MESH_TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(MESH_TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);// test remote
    }
}

void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler_ap,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config;
    bzero(&wifi_config,sizeof(wifi_config));
    char apssid[32];
    char appsw[8];
    uint8_t mac[6];

    esp_wifi_get_mac(WIFI_IF_AP,mac);
    sprintf(apssid,"mweb%2x%2x%2x%2x\n", mac[0],mac[1],mac[2],mac[3]);
    printf("Web Name %s\n",apssid);
    // strcpy(apssid,"web");
    strcpy(appsw,"csttpstt");

    memcpy(&wifi_config.ap.ssid,apssid,strlen(apssid));
    memcpy(&wifi_config.ap.password,appsw,strlen(appsw));
    // printf("SSID [%s] Pass [%s]\n",(char*)wifi_config.ap.ssid,(char*)wifi_config.ap.password);
    wifi_config.ap.ssid_len = strlen(apssid);
    wifi_config.ap.channel = 4;
    wifi_config.ap.max_connection = 1;
    wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(MESH_TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             apssid, appsw, 4);
             
             //start web server
             start_webserver(NULL);
}

void meter_configure()
{
    printf("Configure Meters\n");
    wifi_init_softap();
    while(true)
    {
        delay(1000);        // Webserver will restart after configuration or timeout
    }
}

void app_main(void)
{
    // OTA reboot section 

    const esp_partition_t *running = esp_ota_get_running_partition();

    esp_app_desc_t running_app_info;
    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
        printf( "Running firmware version: %s\n", running_app_info.version);
    }

    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        // printf("get state %d\n",ota_state);
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            printf("Verifing previous OTA\n");
            // run diagnostic function ...
            bool diagnostic_is_ok =true;;
            if (diagnostic_is_ok) {
                printf("Diagnostics completed successfully! Continuing execution ...\n");
                esp_ota_mark_app_valid_cancel_rollback();
            } else {
                printf("Diagnostics failed! Start rollback to the previous version ...\n");
                esp_ota_mark_app_invalid_rollback_and_reboot();
            }
        }
    }
    else{
        printf("Error getting partition\n");
    }

// end OTA section

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

    esp_log_level_set("*",(esp_log_level_t)theConf.loglevel);

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

//check mesh config
    if(!theConf.meshconf)
        park_state();

    printf("Meter Conf %d\n",theConf.meterconf);
    
    if(!theConf.meterconf )
    {
        meter_configure();
    }

   xTaskCreate(&displayManager,"displ",10240,NULL, 10, NULL); 	        // show booting sequence active
    
    ESP_ERROR_CHECK(esp_netif_init());
    wifi_event_group = xEventGroupCreate();

    if(theConf.meshconf==2)
        esp_mesh_fix_root(true);        //force him to seek a root node.

    if(theConf.meshconf<2)
    {
        printf("trying newconfigure\n");
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        new_provision();

        esp_event_loop_delete_default();
        // // ESP_ERROR_CHECK(esp_event_loop_init(NULL,NULL))
        ESP_ERROR_CHECK(esp_wifi_deinit());
        esp_netif_destroy_default_wifi(esp_sta);
    }
    esp_netif_dhcpc_stop(esp_sta);

    ESP_ERROR_CHECK(esp_event_loop_init(NULL,NULL));

    /*  crete network interfaces for mesh (only station instance saved for further manipulation, soft AP instance ignored */
    ESP_ERROR_CHECK(mesh_netifs_init(mesh_recv_cb));
    
    wifi_init_config_t configg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&configg));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_wifi_start());
// mesh stuff
// router name and password from wifi configuration
wifi_config_t config;
esp_wifi_get_config( WIFI_IF_STA,&config);
// printf("SSID %s password %s\n",config.ap.ssid,config.ap.password);

char missid[30],mipassw[10];

if(strlen((char*)config.ap.ssid)==0)
{
 strcpy(missid,"Patos");
 strcpy(mipassw,"csttpstt");

}
else
{
    strcpy(missid,(char*)config.ap.ssid);
    strcpy(mipassw,(char*)config.ap.password);
}

        /*  mesh initialization */
    ESP_ERROR_CHECK(esp_mesh_init());
    ESP_ERROR_CHECK(esp_event_handler_register(MESH_EVENT, ESP_EVENT_ANY_ID, &mesh_event_handler, NULL));
    ESP_ERROR_CHECK(esp_mesh_set_max_layer(CONFIG_MESH_MAX_LAYER));
    ESP_ERROR_CHECK(esp_mesh_set_vote_percentage(1));

    ESP_ERROR_CHECK(esp_mesh_set_ap_assoc_expire(10));
    mesh_cfg_t cfg = MESH_INIT_CONFIG_DEFAULT();
    /* mesh ID */
    memcpy((uint8_t *) &cfg.mesh_id, MESH_ID, 6);
    /* router */
    cfg.channel = CONFIG_MESH_CHANNEL;
    
    cfg.router.ssid_len =strlen( missid);
    memcpy((uint8_t *) &cfg.router.ssid, missid, cfg.router.ssid_len);
    memcpy((uint8_t *) &cfg.router.password, mipassw,
           strlen(mipassw));
    /* mesh softAP */
    printf("cfg ssid %s password %s\n",cfg.router.ssid,cfg.router.password);
    ESP_ERROR_CHECK(esp_mesh_set_ap_authmode((wifi_auth_mode_t)CONFIG_MESH_AP_AUTHMODE));
    cfg.mesh_ap.max_connection = CONFIG_MESH_AP_CONNECTIONS;
    cfg.mesh_ap.nonmesh_max_connection = CONFIG_MESH_NON_MESH_AP_CONNECTIONS;
    memcpy((uint8_t *) &cfg.mesh_ap.password, "csttpstt",
            8);
    // memcpy((uint8_t *) &cfg.mesh_ap.password, config.ap.password,
    //        strlen((char*)config.ap.password));
    ESP_ERROR_CHECK(esp_mesh_set_config(&cfg));
    /* mesh start */
    ESP_ERROR_CHECK(esp_mesh_start());
    ESP_LOGI(MESH_TAG, "mesh starts successfully, heap:%d, %s\n",  esp_get_free_heap_size(),
             esp_mesh_is_root_fixed() ? "root fixed" : "root not fixed");

     xTaskCreate(&esp_mesh_p2p_rx_main,"mesh",10240,NULL, 10, NULL); 	        // show booting sequence active


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

// nueva version
