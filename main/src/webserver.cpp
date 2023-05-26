/*
 * webserver.cpp
 *
 *  Created on: Dec 29, 2019
 *      Author: rsn
 */
#define GLOBAL

#include "includes.h"
#include "defines.h"
// #include "typedef.h"
#include "globals.h"

static esp_err_t conn_base(httpd_req_t *req);
static esp_err_t configure(httpd_req_t *req);

extern const unsigned char params_start[] 		asm("_binary_indexmin_html_start");	//default base html
extern const uint8_t server_cert_pem_start[] 	asm("_binary_serverkey_pem_start");	//for ssl
extern const unsigned char ok_start[] 			asm("_binary_ok_png_start");
extern const unsigned char ok_end[] 			asm("_binary_ok_png_end");
extern const unsigned char nak_start[] 			asm("_binary_nak_png_start");
extern const unsigned char nak_end[] 			asm("_binary_nak_png_end");
extern const unsigned char fav_start[] 			asm("_binary_favicon_ico_start");
extern const unsigned char fav_end[] 			asm("_binary_favicon_ico_end");

extern void delay(uint32_t a);
extern void write_to_flash();
extern uint32_t xmillis();

// int wifi_bytes=wifi_end-wifi_start;
// int check_bytes=check_end-check_start;
// int nocheck_bytes=nocheck_end-nocheck_start;
int ok_bytes=ok_end-ok_start;
int nak_bytes=nak_end-nak_start;
int fav_bytes=fav_end-fav_start;

typedef struct httpd_uri ur;
#define MAXURLS 5
ur urls[MAXURLS];

bool getParam(char *buf,char *cualp,char *donde)
{
	if( httpd_query_key_value(buf, cualp, donde, 30)==ESP_OK)
		return true;
	else
		return false;
}

esp_err_t sendfav(httpd_req_t *req)
{
	httpd_resp_set_hdr(req,"Cache-Control","private, max-age=86400");
	httpd_resp_set_type(req,"image/png");
	httpd_resp_send(req,(char*)fav_start,fav_bytes);
	return ESP_OK;
}

esp_err_t sendok(httpd_req_t *req)
{
	httpd_resp_set_hdr(req,"Cache-Control","private, max-age=86400");
	httpd_resp_set_type(req,"image/png");
	httpd_resp_send(req,(char*)ok_start,ok_bytes);
	return ESP_OK;
}

esp_err_t sendnak(httpd_req_t *req)
{
	httpd_resp_set_hdr(req,"Cache-Control","private, max-age=86400");
	httpd_resp_set_type(req,"image/png");
	httpd_resp_send(req,(char*)nak_start,nak_bytes);

	return ESP_OK;
}

void setHeaders(httpd_req_t *req,char * tipo)
{
	httpd_resp_set_hdr(req,"Cache-Control","public, no-cache");
	httpd_resp_set_hdr(req,"Access-Control-Allow-Credentials","true");
	httpd_resp_set_hdr(req,"Access-Control-Allow-Origin","*");
	httpd_resp_set_type(req,tipo);
}


void init_urls()
{
	urls[0].uri       = "/";				urls[0].handler   = conn_base;
	urls[1].uri       = "/configure";		urls[1].handler   = configure;
	urls[2].uri       = "/ok.png";			urls[2].handler   = sendok;
    urls[3].uri       = "/nak.png";			urls[3].handler   = sendnak;
    urls[4].uri       = "/favicon.ico";			urls[4].handler   = sendfav;
}



static esp_err_t http_event_handle(esp_http_client_event_t *evt)
{
	char *aqui;

    switch(evt->event_id) {
        case HTTP_EVENT_ON_DATA:
			if (!esp_http_client_is_chunked_response(evt->client))
			{
				aqui=(char*)evt->user_data;
				memcpy(aqui,evt->data,evt->data_len);
				aqui[evt->data_len]=0;
			}
            break;
        default:
        	break;
    }
    return ESP_OK;
}

void getSetParameter(char *buf,char * cual, int llimit,int hlimit,int def,int *donde)
{
	char param[60];
	int valor;

	if(getParam(buf,cual,param))
	{
		valor=atoi(param);
		if(valor<llimit)
			valor=llimit;
		if(valor>hlimit)
		{
			valor=hlimit;
		}
	}
	else	// parameter not passed use default
	{
		printf("Could not find %s using default\n",cual);
		valor=def; //default
	}
	*donde=valor;
}

void getSetParameterFloat(char *buf,char * cual, float llimit,float hlimit,float def,float *donde)
{
	char param[60];
	float valor;

	if(getParam(buf,cual,param))
	{
		valor=atof(param);
		if(valor<llimit)
			valor=llimit;
		if(valor>hlimit)
		{
			valor=hlimit;
		}
	}
	else	// parameter not passed use default
	{
		printf("Could not find %s using float default\n",cual);
		valor=def; //default
	}
	*donde=valor;
}

static esp_err_t configure(httpd_req_t *req)
{
	char temp[10],param[40],laclave[10];
	int desde=0,hasta,buf_len;
	char *buf=NULL;
	char *answer;
	bool errores=false;
printf("Configuration\n");
	buf_len = httpd_req_get_url_query_len(req) + 1;
	if (buf_len > 1)
	{
		buf = (char*)malloc(buf_len);
		if(buf)
		{
			if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
			{
				bzero(param,sizeof(param));
				if(getParam(buf,(char*)"passw",param))
				{
					strcpy(laclave,param);
					uint32_t keyread=atoi(laclave);
					printf("Key %d %d\n",keyread,theConf.confpassword);

					if(keyread!=theConf.confpassword)
					{
						printf("Not same key %d %d\n",keyread,theConf.confpassword);
						sendnak(req);
						return ESP_OK;
					}
					errores=false;
					theConf.confpassword=0;			//reset password, ONE USE only
					write_to_flash();
					//mesh and network paramerters
					getSetParameter(buf,"meshid",1,100,1,(int*)&theConf.meshid);
					theConf.meshid+=BASEMESH;		//base is 0x75
					getSetParameter(buf,"nodeid",1,100000,1,(int*)&theConf.controllerid);
					getSetParameter(buf,"subn",1,100,1,(int*)&theConf.subnode);

					// mqtt topic parameters
					getSetParameter(buf,"prov",1,200,1,(int*)&theConf.provincia);
					getSetParameter(buf,"cant",1,500,1,(int*)&theConf.canton);
					getSetParameter(buf,"parro",1,500,1,(int*)&theConf.parroquia);
					getSetParameter(buf,"codp",1,1000000,1,(int*)&theConf.codpostal);

					for (int a=1;a<9;a++)
					{
						sprintf(temp,"m%d",a);
						if(!getParam(buf,temp,(char*)&medidor[a-1].mid))
						{
							printf("Name error meter %s %d\n",temp,a);
							errores=true;
						}
						// printf("%s=%s\n",temp,medidor[a-1].mid);
						sprintf(temp,"b%d",a);
						getSetParameter(buf,temp,800,3000,800,(int*)&medidor[a-1].bpk);
						// printf("%s=%d\n",temp,medidor[a-1].bpk);
						sprintf(temp,"s%d",a);
						getSetParameter(buf,temp,0,3000000,0,(int*)&medidor[a-1].kwhstart);
						// printf("%s=%d\n",temp,medidor[a-1].kwhstart);
					}					
					
					if(errores)
					{
						sendnak(req);
						printf("Configuration aborted\n");
					}
					else
					{
						sendok(req);
						// save data to flash and Fram
						theConf.meterconf=1; // configuration is done
						theConf.cid=0;
						write_to_flash();
						for(int a=0;a<8;a++)
						{
							if(medidorlock[a] )
							// if(medidorlock[a] || strlen(medidor[a].mid)==0)
							{
								// printf("Skipping %d\n",a);
								continue;		//skip this
							}

							medidor[a].lifekwh=medidor[a].kwhstart;
							fram.write_meter(a,(uint8_t*)&medidor[a],sizeof(medidor[0]));
						}
						delay(1000);
						printf("Configuration done\n");
					}
					printf("Restarting\n");
					esp_restart();
				}					
			}
		}
		free(buf);
	}
	return ESP_OK;
}

int sendHtmlInt(char *que, char * params, char *answer)
{
	char	textl[100];

	esp_http_client_config_t lconfig;

	memset(&lconfig,0,sizeof(lconfig));
	sprintf(textl,"%s",que);		//ap of esp32
	// sprintf(textl,"https://www.meteriot.site/%s",que);
	lconfig.url=textl;
	lconfig.user_data=(void*)answer;
	lconfig.cert_pem = (char *)server_cert_pem_start;
	lconfig.skip_cert_common_name_check = true;

	#ifdef DEBUGX
		if(theConf.traceflag & (1<<WEBD))
			pprintf("%sSending HTML %s params %s\n",HOSTDT,textl,params);
	#endif

		lconfig.event_handler = http_event_handle;							//in charge of saving received data to Fram directly

		esp_http_client_handle_t client = esp_http_client_init(&lconfig);
		if(client)
		{
			if(strlen(params))
			{
		    	esp_http_client_set_method(client, HTTP_METHOD_POST);
			    esp_http_client_set_post_field(client, params, strlen(params));
			}
			esp_err_t err = esp_http_client_perform(client);			// do the hard work
			if (err == ESP_OK)
			{
	#ifdef DEBUGX
				if(theConf.traceflag & (1<<HOSTD))
					pprintf("%sStatus = %d, content_length = %d\n",HOSTDT,
						esp_http_client_get_status_code(client),
						esp_http_client_get_content_length(client));
	#endif
				if(esp_http_client_get_status_code(client)!=200)
				{
					// if(theConf.traceflag & (1<<HOSTD))
						printf("Failed to send HTML %x\n",esp_http_client_get_status_code(client));
					esp_http_client_cleanup(client);
					return ESP_FAIL;
				}
			}
			else
			{
#ifdef DEBUGX
				if(theConf.traceflag & (1<<HOSTD))
					pprintf("%sFailed to send HTML %x\n",HOSTDT,err);
				esp_http_client_cleanup(client);
				return ESP_FAIL;
#endif
			}
			// all is well, cleanup and read back to our working array
			esp_http_client_cleanup(client);
			return ESP_OK;
		}
		else
		{
			printf("Failed to create HTTP CLient\n");
			return ESP_FAIL;
		}
		return ESP_OK;

}

int sendHtml(httpd_req_t *req,const char * format, ...)
{
	va_list args;
	char *ltemp;

	ltemp=(char*)malloc(7000);
	if(!ltemp)
		return ESP_FAIL;
	else
		bzero(ltemp,7000);

	va_start (args, format);
	vsprintf (ltemp,format, args);
	va_end (args);

    if(strlen(ltemp)<=7000)
	    httpd_resp_send(req,ltemp, strlen(ltemp));
	FREEANDNULL(ltemp);
	return ESP_OK;
}

void wmonitorCallback( TimerHandle_t xTimer )
{
	webState=wNONE;
	printf("TimeOut Webserver\n");
	// esp_restart();
}

static esp_err_t conn_base(httpd_req_t *req)		//default page
{
	char texto[100];

	setHeaders(req,(char*)"text/html");

	webLogin=false;			//Reset login state
	bzero(tempb,7000);

	if(sendHtml(req,(char*)params_start,
	theConf.meshid-BASEMESH,theConf.controllerid,theConf.subnode,
	theConf.provincia,theConf.canton,theConf.parroquia,theConf.codpostal,
	medidor[0].mid,medidorlock[0]?"disabled":"",medidor[0].bpk,medidorlock[0]?"disabled":"",medidor[0].kwhstart,medidorlock[0]?"disabled":"",medidor[0].lifekwh,
	medidor[01].mid,medidorlock[1]?"disabled":"",medidor[1].bpk,medidorlock[1]?"disabled":"",medidor[1].kwhstart,medidorlock[1]?"disabled":"",medidor[1].lifekwh,
	medidor[02].mid,medidorlock[2]?"disabled":"",medidor[02].bpk,medidorlock[02]?"disabled":"",medidor[02].kwhstart,medidorlock[02]?"disabled":"",medidor[2].lifekwh,
	medidor[03].mid,medidorlock[3]?"disabled":"",medidor[03].bpk,medidorlock[03]?"disabled":"",medidor[03].kwhstart,medidorlock[03]?"disabled":"",medidor[3].lifekwh,
	medidor[04].mid,medidorlock[04]?"disabled":"",medidor[04].bpk,medidorlock[04]?"disabled":"",medidor[04].kwhstart,medidorlock[04]?"disabled":"",medidor[4].lifekwh,
	medidor[05].mid,medidorlock[05]?"disabled":"",medidor[05].bpk,medidorlock[05]?"disabled":"",medidor[05].kwhstart,medidorlock[05]?"disabled":"",medidor[5].lifekwh,
	medidor[06].mid,medidorlock[06]?"disabled":"",medidor[06].bpk,medidorlock[06]?"disabled":"",medidor[06].kwhstart,medidorlock[06]?"disabled":"",medidor[6].lifekwh,
	medidor[07].mid,medidorlock[07]?"disabled":"",medidor[07].bpk,medidorlock[07]?"disabled":"",medidor[07].kwhstart,medidorlock[07]?"disabled":"",medidor[7].lifekwh
	)!=0)
		printf("Failed to send html\n");

	webState=wNONE;
	xTimerStop(webTimer,0); //Stop it in case we are coming back
	xTimerStart(webTimer,0); //Start it
	return ESP_OK;
}

void urldecode2(char *dst, const char *src)
{
        char a, b;
        while (*src) {
                if ((*src == '%') &&
                    ((a = src[1]) && (b = src[2])) &&
                    (isxdigit(a) && isxdigit(b))) {
                        if (a >= 'a')
                                a -= 'a'-'A';
                        if (a >= 'A')
                                a -= ('A' - 10);
                        else
                                a -= '0';
                        if (b >= 'a')
                                b -= 'a'-'A';
                        if (b >= 'A')
                                b -= ('A' - 10);
                        else
                                b -= '0';
                        *dst++ = 16*a+b;
                        src+=3;
                } else if (*src == '+') {
                        *dst++ = ' ';
                        src++;
                } else {
                        *dst++ = *src++;
                }
        }
        *dst++ = '\0';
}


void start_webserver(void *pArg)
{
	printf("Stating webserver\n");
	tempb=(char*)malloc(7000);
	if(!tempb)
	{
		printf("No ram\n");
		vTaskDelete(NULL);
	}
	#ifndef SECURE
	httpd_handle_t server = NULL;
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.max_uri_handlers=MAXURLS;
	config.max_open_sockets=1;		//just one connection
	config.stack_size=14000;
	int ret=httpd_start(&wserver, &config);	//global wserver to stop it if Firmware called
#else
	httpd_ssl_config_t config = HTTPD_SSL_CONFIG_DEFAULT();
	config.httpd.max_uri_handlers=MAXURLS;
	config.httpd.max_open_sockets=4;		//just one connection
	config.httpd.stack_size=14000;

	extern const unsigned char cacert_pem_start[] asm("_binary_cacert_pem_start");
	extern const unsigned char cacert_pem_end[]   asm("_binary_cacert_pem_end");
	config.cacert_pem = cacert_pem_start;
	config.cacert_len = cacert_pem_end - cacert_pem_start;
	extern const unsigned char pprvtkey_pem_start[] asm("_binary_wprvtkey_pem_start");
	extern const unsigned char pprvtkey_pem_end[]   asm("_binary_wprvtkey_pem_end");
	config.prvtkey_pem = pprvtkey_pem_start;
	config.prvtkey_len = pprvtkey_pem_end - pprvtkey_pem_start;

   int ret=httpd_ssl_start(&wserver, &config);	//global wserver to stop it if Firmware called
   #endif
//    printf("Webstart code %x\n",ret);
   if(ret== ESP_OK)
    {
	   #ifdef DEBUGX
	      if(theConf.traceflag & (1<<WEBD))
		  #ifdef SECURE
	      	pprintf("%sSSL server started on port:%d\n",WEBDT, config.httpd.server_port);
			  #else
	       	 pprintf("%sServer started on port:%d\n",WEBDT, config.server_port);
				#endif
	   #endif
	   init_urls();

		webState=wNONE;
		webLogin=true;
        webTimer=xTimerCreate("Monitor",60000 /portTICK_PERIOD_MS,pdTRUE,NULL,&wmonitorCallback);	//10 minutes if no activity back to wNONE

    	bzero(gwStr,sizeof(gwStr));
		
        // Set URI handlers
    	for(int a=0;a<MAXURLS;a++)
    	{
    		// printf("url %s\n",urls[a].uri);
    		urls[a].method=HTTP_GET;
    		urls[a].user_ctx=NULL;
    		ret=httpd_register_uri_handler(wserver, &urls[a]);
			if(ret)
				printf("Error setting URL %x for url %d\n",ret,a);
    	}

#ifdef DEBUGX
    if(theConf.traceflag & (1<<WEBD))
    	pprintf("%sSSL WebServer Started\n",WEBDT);
#endif
    }
    else
    	printf("Could not start webserver %x %s\n",ret,esp_err_to_name(ret));
	printf("Web die\n");
    // vTaskDelete(NULL);
}


