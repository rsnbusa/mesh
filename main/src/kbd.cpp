
#define GLOBAL
#include "includes.h"
#include "globals.h"

uint32_t theAddress=0;
uint8_t wmeter=0;

extern void erase_config();
extern void write_to_flash();
extern void start_ota();
extern char * sendData(bool forced);
extern int aes_encrypt(const char* src, size_t son, char *dst,const char *cualKey);

void delay(uint32_t cuanto);

static void lafecha(time_t now, char * donde)
{
    struct tm timeinfo;

    localtime_r(&now, &timeinfo);
    strftime(donde, 300, "%c", &timeinfo);
}


int cmdFram(int argc, char **argv)
{

  time_t now;
  struct tm timeinfo;
  char  strftime_buf[100];

  uint32_t aca;

    int nerrors = arg_parse(argc, argv, (void **)&framArg);
    if (nerrors != 0) {
        arg_print_errors(stderr, framArg.end, argv[0]);
        return 0;
    }

 if (framArg.format->count) {
  int cuanto=0;

    if(strcmp(framArg.format->sval[0],"fast")==0)
      cuanto =1;

    if(cuanto)
      fram.format(0,NULL,1000,true);
    else
      fram.formatSlow(0);
    time(&now);
    fram.writeMany(0,(uint8_t*)&now,LLONG);      //write creation date
    fram.write_guard(0x12345678);
    printf("Fram Formatted speed %s\n",cuanto?"Fast":"Slow");
 }

 if (framArg.address->count) {
    theAddress=framArg.address->ival[0];
    printf("Fram Address set to %d(%x)\n",theAddress,theAddress);
 }

 if (framArg.guard->count) {
    const char *que=framArg.guard->sval[0];

    if(strcmp(que,"write")==0)
    {
        fram.write_guard(0x12345678);
        printf("Fram Guard stored\n");
    }
    if(strcmp(que,"read")==0)
    {
        aca=0;
        fram.read_guard((uint8_t*)&aca);
        printf("Guard read as %x\n",aca);
    }
 }

 if (framArg.write->count) {
    int val=framArg.write->ival[0];
    printf("Write params %d %d\n",theAddress,val);

    fram.write8(theAddress,val);
    printf("Fram wrote at %d(%x) val %d\n",theAddress,theAddress,val);
 }

 if (framArg.read->count) {
    int add=framArg.read->ival[0];
    uint8_t val=0;

    fram.read8(add,&val);
    printf("Fram read from %d(%x) value %d(%x)\n",add,add,val,val);
 }

 if (framArg.time->count) {
    int que=framArg.time->ival[0];

    if(que) //write date time
    {
      time(&now);
      fram.writeMany(0,(uint8_t*)&now,sizeof(now));

        // setenv("TZ", LOCALTIME, 1);
        // tzset();
        localtime_r(&now, &timeinfo);
        strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
        printf("Wrote the current date/time: %s day of Year %d\n", strftime_buf,timeinfo.tm_yday);
    }
    else{
      //read date time
        now=0;
        fram.readMany(0,(uint8_t*)&now,sizeof(now));
        localtime_r(&now, &timeinfo);
        strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
        printf("Read the current date/time  %s day of Year %d\n", strftime_buf,timeinfo.tm_yday);
    }
 }

 if (framArg.fmeter->count) {
    int cual=framArg.fmeter->ival[0];
    if(cual>MAXDEVSS)
    {
      printf("Meter %d out of range\n",cual);
      return 0;
    }
    cual--;

    int err=fram.formatMeter(cual);
    printf("Meter %d formatted %s\n",cual,err?"unsuccessfully":"successfully");
 }

 if (framArg.wmeter->count) {
    wmeter=framArg.wmeter->ival[0];
    printf("Working meter set to %d\n",wmeter);
 }

 if (framArg.midw->count) {
    fram.write_mid(wmeter,(uint8_t*)framArg.midw->sval[0],strlen(framArg.midw->sval[0]));
    printf("Meter[%d] id set to %s\n",wmeter,framArg.midw->sval[0]);
 }

 if (framArg.midr->count) {
    char id[13];
    bzero(id,13);

    fram.read_mid(wmeter,(uint8_t*)id,12);
    printf("Read Meter[%d] id  %s\n",wmeter,id);
 }

 if (framArg.kwhstart->count) {
    int cuanto=framArg.kwhstart->ival[0];
    fram.write_kwhstart(wmeter,cuanto);
    fram.write_lifekwh(wmeter,cuanto);  //start set life as meter is initialized
    printf("Working meter[%d] kwhstart set to %d\n",wmeter,cuanto);
 }

 if (framArg.rstart->count) {
    int cuanto,cuanto1=0;
    fram.read_kwhstart(wmeter,(uint8_t*)&cuanto);
    fram.read_lifekwh(wmeter,(uint8_t*)&cuanto1);
    printf("Working meter[%d] kwhstart %d Lifekwh %d read\n",wmeter,cuanto,cuanto1);
 }


 if (framArg.mtime->count) {
    int cuanto=framArg.mtime->ival[0];
    time_t now;

    if(cuanto)
    {
      time(&now);
      fram.write_lifedate(wmeter,now);
          printf("Working meter[%d] lifedate set %d\n",wmeter,(int)now);

    }
    else{
      fram.read_lifedate(wmeter,(uint8_t*)&now);
      printf("Working meter[%d] lifedate read %d\n",wmeter,(int)now);

    }
 }

 if (framArg.mbeat->count) {
    int cuanto=framArg.mbeat->ival[0];

    if(cuanto>=0)
    {
      fram.write_beat(wmeter,cuanto);
          printf("Working meter[%d] beat set %d\n",wmeter,cuanto);

    }
    else{
      cuanto=0;
      fram.read_beat(wmeter,(uint8_t*)&cuanto);
      printf("Working meter[%d] beat read %d\n",wmeter,cuanto);

    }
 }

if (framArg.dumpm->count) {

    printf("\t Fram Internals\n");
    now=0;
    fram.readMany(0,(uint8_t*)&now,sizeof(now));
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    printf("Fram Init Date  %s ", strftime_buf);
    fram.read_guard((uint8_t*)&aca);
    printf("Guard %x\n",aca);

    for (int a=0;a<MAXDEVSS;a++)
    {
      int beat=0;
      fram.read_beat(a,(uint8_t*)&beat);

      fram.read_lifedate(a,(uint8_t*)&now);
      localtime_r(&now, &timeinfo);
      strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);

      char * buf=(char*)malloc(100);
      if(buf)
      {
        localtime_r((time_t*)&medidor[a].lastupdate, &timeinfo);
        strftime(buf, 100, "%c", &timeinfo);
      }

      char id[13];
      bzero(id,13);
      fram.read_mid(a,(uint8_t*)id,12);
      
      int kwhs=0;
      fram.read_kwhstart(a,(uint8_t*)&kwhs);
      int kwhlife=0;
      fram.read_lifekwh(a,(uint8_t*)&kwhlife);
      printf("Meter [%d]  [id=%s] [BPK %d] [Created %s] [beat %d] [lifebeat %d] [kwhStart %d] [lifekwh %d] [Updated %s] [MaxAmp %d] [MinAmp %d]\n",a
              ,id,medidor[a].bpk,strftime_buf,beat,medidor[a].beatlife,kwhs,kwhlife,buf,medidor[a].maxamp,medidor[a].minamp);
      if (buf)
          free(buf);
    }
}
    if (framArg.initm->count) {
      char str[20];
      int cual=framArg.initm->ival[0];
      if(cual>MAXDEVSS)
      {
        printf("Meter %d Out of Range\n",cual);
        return 0;
      }
      cual--;
      bzero(&medidor[cual],sizeof(meterType));
      
      printf("MID:");
      gets(medidor[cual].mid);
      printf("%s\n",medidor[cual].mid);
      printf("kwh Start:");
      gets(str);
      medidor[cual].kwhstart=medidor[cual].lifekwh=atoi(str);
      printf("%d\n",medidor[cual].kwhstart);
      time((time_t*)&medidor[cual].lifedate);
      medidor[cual].lastupdate=medidor[cual].lifedate;
      printf("BPK:");
      gets(str);
      medidor[cual].bpk=atoi(str);
      printf("%d\n",medidor[cual].bpk);
      fram.write_meter(cual, (uint8_t*)&medidor[cual],sizeof(meterType));
    }
  return 0;
}

int cmdController(int argc, char **argv)
{
  char str[20];

      printf("Controller Id:");
      gets(str);
      theConf.controllerid=atoi(str);
      printf("%d\n",theConf.controllerid);


      printf("Provincia:");
      gets(str);
      theConf.provincia=atoi(str);
      printf("%d\n",theConf.provincia);


      printf("Canton");
      gets(str);
      theConf.canton=atoi(str);
      printf("%d\n",theConf.canton);


      printf("Parroquia");
      gets(str);
      theConf.parroquia=atoi(str);
      printf("%d\n",theConf.parroquia);


      printf("Postal");
      gets(str);
      theConf.codpostal=atoi(str);
      printf("%d\n",theConf.codpostal);


      printf("Address");
      gets(theConf.direccion);
      printf("%s\n",theConf.direccion);


      write_to_flash();

   return 0;
}

int cmdMeter(int argc, char **argv)
{
  char update[40],initd[40];
  struct tm timeinfo;

  for (int a=0;a<MAXDEVSS;a++)
  {      
    localtime_r((time_t*)&medidor[a].lastupdate, &timeinfo);
    strftime(update, sizeof(update), "%c", &timeinfo);
    localtime_r((time_t*)&medidor[a].lifedate, &timeinfo);
    strftime(initd, sizeof(initd), "%c", &timeinfo);

    printf("Meter[%d] [id=%s] [BPK %d]  [Beat %d]  [Beatlife %d]  [kwhStart %d]  [KwhLife %d]  [Update %s]  [Init date %s]\n",a,medidor[a].mid,medidor[a].bpk,medidor[a].beat,medidor[a].beatlife,medidor[a].kwhstart,
      medidor[a].lifekwh,update,initd);
    for(int b=0;b<12;b++)
    {
    if(medidor[a].months[b]>0)
      printf("Month[%d]=%d ",b,medidor[a].months[b]);
    }
    printf("\n");
    for(int b=0;b<366;b++)
    {
    if(medidor[a].days[b]>0)
      printf("Day[%d]=%d ",b,medidor[a].days[b]);
    }
    printf("\n");  

  }
  return 0;
}

int cmdConfig(int argc, char **argv)
{
  char buf[50],buf2[50],fecha[60],myssid[20];
  time_t lastwrite,now;
  uint8_t *my_mac;

  if(!theConf.meshconf)
  {
    printf("Mesh not configured or started\n");
    return ESP_OK;
  }
  time(&now);
  localtime_r(&now, &timeinfo);
  char *buff=(char*)malloc(300);
  if(buff)
  {
      bzero(buff,300);
      strftime(buff, 300, "%c", &timeinfo);
      strcpy(fecha,buff);
      //  printf("[CMD]The current date/time in %s is: %s day of Year %d\n", LOCALTIME,buff,timeinfo.tm_yday);
      free(buff);
  }

  const esp_partition_t *running = esp_ota_get_running_partition();

  esp_app_desc_t running_app_info;
  if (esp_ota_get_partition_description(running, &running_app_info) != ESP_OK) {
      printf( "Error getting partition versionn");
  }

  bzero(myssid,sizeof(myssid));
  wifi_config_t conf;
  if(esp_wifi_get_config(WIFI_IF_STA, &conf)!=ESP_OK)
  {
    printf("Error readinmg wifi config\n");
    strcpy(myssid,(char*)conf.sta.ssid);
  //  return 0;
  }

  mesh_type_t typ;
  typ=esp_mesh_get_type();
  char *tipo[]={"Idle","ROOT","NODE","LEAF","STA"};
  lafecha(theConf.bornDate,buf);
  fram.read_guard((uint8_t*)&lastwrite);
  lafecha(lastwrite,buf2);

  my_mac=(uint8_t*)malloc(20);
  bzero(my_mac,20);
  if(mesh_started)
      memcpy(my_mac,mesh_netif_get_station_mac(),6);

  printf("\n======= Mesh Configuration Date: %s=======\n",fecha);
  printf("Firmware Version:%s NType:%s MAC:" MACSTR " LogLevel:%d\n", running_app_info.version,tipo[typ],MAC2STR(my_mac),
  theConf.loglevel);
  printf("Mesh config:%s Mesh Id: %02x Meter config:%d SubNode: %d Conf Passw:%d\n",theConf.meshconf?theConf.meshconf>1?"NonRoot":"Provision":"Not Conf",
          theConf.meshid,theConf.meterconf,theConf.subnode,theConf.confpassword);
  printf("Sta %s Psw %s ",conf.sta.ssid,conf.sta.password);
  printf(" Config_Sta %s Config_Psw %s ",theConf.thessid,theConf.thepass);
  esp_wifi_get_config(WIFI_IF_AP, &conf);
  printf("AP %s Pswd %s\n",conf.ap.ssid,conf.ap.password);
  free(my_mac);
  if(esp_mesh_is_root())
  {
    printf("Id : %d  Address: %s Created %s Slot %d  Cycle %d\n",theConf.controllerid,theConf.direccion,buf,theConf.mqttSlots,theConf.pubCycle);
    printf("Provincia: %d Canton: %d Parroquia:%d CodigoPostal:%d\n",theConf.provincia,theConf.canton,theConf.parroquia,theConf.codpostal);
    printf("Command Queue %s\n",cmdQueue);
    printf("Info Queue %s\n",infoQueue);
  }
  printf("BootCount %d LastReset %d Reason %d DownTime %d lastupdate %s\n",theConf.bootcount,theConf.lastResetCode,theConf.lastResetCode,theConf.downtime,buf2);

  esp_mesh_get_routing_table((mesh_addr_t *) &s_route_table,CONFIG_MESH_ROUTE_TABLE_SIZE * 6, &s_route_table_size);
  printf("Mesh Network\n");
  for (int a=0;a<s_route_table_size;a++)
  {
    printf("\tMAC[%d]:" MACSTR " %s\n",a,MAC2STR(s_route_table[a].addr),MAC_ADDR_EQUAL(s_route_table[a].addr, my_mac)?"ME":"");
  }
  return 0;
}

int cmdErase(int argc, char **argv)
{
  printf("Erase Controller...");
  erase_config();
  printf("done\n");
  return 0;
}

int cmdSend(int argc, char **argv)
{
      mqttSender_t mensaje;
      mensaje.msg=sendData(true);
  
      if(mensaje.msg)
      {
        mensaje.lenMsg=strlen(mensaje.msg);
        printf("KBD Send Mqtt [%s] size %d\n",mensaje.msg,mensaje.lenMsg);
      }
      if(xQueueSend(mqttSender,&mensaje,0)!=pdPASS)
      {
            printf("Error queueing msg\n");
            if(mensaje.msg)
                free(mensaje.msg);  //due to failure
      }
  printf("Setting Mqtt Send Bit");
  xEventGroupSetBits(wifi_event_group, SENDMQTT_BIT);	// clear bit to wait on
  printf("done\n");
  return 0;
}

int cmdSendMesh(int argc, char **argv)
{
  mesh_data_t data;
  if (!esp_mesh_is_root())
  {
    char *mensaje;
    mensaje=sendData(true);
    if(!mensaje)
    {
      mensaje=(char*)malloc(100);
      bzero(mensaje,100);
      strcpy(mensaje,"No hay data");
    }
      data.proto = MESH_PROTO_BIN;
      data.tos = MESH_TOS_P2P;
      data.data=(uint8_t*)mensaje;
      data.size=strlen(mensaje)+1;
      printf("Sending Root [%s]...",(char*)data.data);
      int err = esp_mesh_send(NULL, &data, 0, NULL, 0);
      // int err = esp_mesh_send(NULL, &data, MESH_DATA_P2P, NULL, 0);
      printf("done %d\n",err);
      free(mensaje);
  }
  else{
      uint8_t *my_mac = mesh_netif_get_station_mac();
      esp_mesh_get_routing_table((mesh_addr_t *) &s_route_table,CONFIG_MESH_ROUTE_TABLE_SIZE * 6, &s_route_table_size);
      printf("Send Mesh Network\n");
      
      cJSON *root=cJSON_CreateObject();
      cJSON_AddStringToObject(root,"CMD","host");
      cJSON_AddStringToObject(root,"SSID","Porton");
      cJSON_AddStringToObject(root,"PSSW","csttpstt");
      char *lmessage=cJSON_PrintUnformatted(root);
      if(lmessage)
      {
        printf("Sending %s\ to %d stations\n",lmessage,s_route_table_size);
        data.data=(uint8_t*)lmessage;
        data.size=strlen(lmessage);

        for (int a=0;a<s_route_table_size;a++)
        {
          // if(!MAC_ADDR_EQUAL(s_route_table[a].addr, my_mac))  //not me
          // {
            int err = esp_mesh_send(&s_route_table[a], &data, MESH_DATA_FROMDS, NULL, 0);
            // int err = esp_mesh_send(&s_route_table[a], &data, MESH_DATA_P2P, NULL, 0);
            printf("\tSending to MAC[%d]:" MACSTR "\n",a,MAC2STR(s_route_table[a].addr));
          // }
        }
        free(lmessage);
      }
      cJSON_Delete(root);     //gone with this structure free it
  }
  return 0;
}

int cmdOTA(int argc, char **argv)
{
  printf("Starting OTA kbd");
  start_ota();
  return 0;
}

int cmdLogLevel(int argc, char **argv)
{
      int nerrors = arg_parse(argc, argv, (void **)&loglevel);
    if (nerrors != 0) {
        arg_print_errors(stderr, loglevel.end, argv[0]);
        return 0;
    }
  if (loglevel.level->count) 
  {
      int lev=loglevel.level->ival[0];
      printf("Level set to %d\n",lev);
      theConf.loglevel=lev;
      write_to_flash();
      esp_log_level_set("*",(esp_log_level_t)theConf.loglevel);

  }
  return 0;
}

int cmdEnDecrypt(int argc, char **argv)
{
  int dkey,err;
  const char *mode;
  char kkey[17],laclave[33],ikey[10];

      int nerrors = arg_parse(argc, argv, (void **)&endec);
    if (nerrors != 0) {
        arg_print_errors(stderr, endec.end, argv[0]);
        return 0;
    }

      if (endec.key->count) 
      {
        dkey=endec.key->ival[0];
        if(dkey<=0)
          return 0;
        sprintf(kkey,"%016d",dkey);
        // printf("num [%s]\n",kkey);
        sprintf(laclave,"%s%s",kkey,kkey);
        // printf("clave [%s] %d\n",laclave,strlen(laclave));
        char *aca=(char*)malloc (1000);
        err=aes_encrypt(SUPERSECRET,sizeof(SUPERSECRET),aca,laclave);
        printf("%02x%02x%02x%02x\n",aca[0],aca[1],aca[2],aca[3]);
        ESP_LOG_BUFFER_HEX(MESH_TAG,aca,err);
      }
      
  return 0;
}

int cmdResetConf(int argc, char **argv)
{
  uint32_t pop=0;
  mqttSender_t mensaje;
  char *mqttmsg;
  mesh_data_t data;
  wifi_config_t       configsta;
  int err;

    int nerrors = arg_parse(argc, argv, (void **)&resetlevel);
    if (nerrors != 0) {
        arg_print_errors(stderr, resetlevel.end, argv[0]);
        return 0;
    }
  if (resetlevel.cflags->count) 
  {
      int lev=resetlevel.cflags->ival[0];
      printf("Flags set to %d\n",lev);
      switch(lev) {
        case 0:
          theConf.meterconf=0;
          theConf.meshconf=0;
          erase_config();
          break;
        case 1:
          theConf.meterconf=0;
          break;
        case 2:
          theConf.meshconf=0;
          break;
        case 3:
          err=esp_wifi_get_config( WIFI_IF_STA,&configsta);      // get station ssid and password
          if(!err)
          {
            memcpy(&configsta.sta.ssid,(char*)"Porton\0",7);
            memcpy(&configsta.sta.password,(char*)"csttpstt\0",9);
            err=esp_wifi_set_config( WIFI_IF_STA,&configsta);      // save new ssid and password
            if(err)
                printf("Failed to save new ssid %x\n",err);
            else
              {
                theConf.meshconf=1;
                write_to_flash();
                printf("Sta saved\n");
              }
          }
          else 
            printf("Error getting STA config %x\n",err);
            break;
        default:
          printf("Wrong choice of reset\n");
      }
      write_to_flash();

  }
  return 0;
}

void kbd(void *pArg)
{
  esp_console_repl_t       *repl=NULL;
  esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
  repl_config.prompt=(char*)"Meter>";
  esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &repl));

  framArg.format =                arg_str0(NULL, "format", "slow | fast", "Format Fram");
  framArg.guard =                 arg_str0(NULL, "guard", "Write or Read", "Set/Get Guard");
  framArg.address =               arg_int0(NULL, "add", "address", "Address to use in write");
  framArg.write =                 arg_int0(NULL, "write", "value", "Write to Fram a  Val");
  framArg.read =                  arg_int0(NULL, "read", "address", "Read from Fram Add ");
  framArg.time =                  arg_int0(NULL, "time", "unix time", "Write current date to fram ");
  framArg.fmeter =                arg_int0(NULL, "fmeter", "meter", "Format a meter ");
  framArg.wmeter =                arg_int0(NULL, "workm", "meter", "Set working meter number");
  framArg.midw =                  arg_str0(NULL, "wmid","id",  "Set id of working meter ");
  framArg.midr =                  arg_int0(NULL, "rmid","dummy",  "Read id of working meter ");
  framArg.kwhstart =              arg_int0(NULL, "kwh", "value", "Set kwh start of working meter ");
  framArg.rstart =                arg_int0(NULL, "rkwh", "dummy", "Read kwh start of working meter ");
  framArg.mtime =                 arg_int0(NULL, "metert", "1 write 0 read", "Write/read working meter date ");
  framArg.mbeat =                 arg_int0(NULL, "beat", "value", "Write beat of working meter ");
  framArg.dumpm =                 arg_int0(NULL, "dump", "meter#", "Dump meter data ");
  framArg.initm =                 arg_int0(NULL, "initm", "meter#", "Init a meter ");
  framArg.end =                   arg_end(16);


  loglevel.level=                 arg_int0(NULL, "l", "0-5(None-Error-Warn-Info-Debug-Verbose)", "Log Level");
  loglevel.end=                   arg_end(1);

  resetlevel.cflags=               arg_int0(NULL, "f", "0-2(0=All 1=Configuration 2=Mesh)", "Reset Flags");
  resetlevel.end=                  arg_end(1);

  endec.key=                       arg_int0(NULL, "k", "AES key numeric", "Aes Key");
  endec.end=                       arg_end(1);

    const esp_console_cmd_t fram_cmd = {
        .command = "fram",
        .help = "Manage Fram",
        .hint = NULL,
        .func = &cmdFram,
        .argtable = &framArg
    };

    const esp_console_cmd_t meter_cmd = {
        .command = "meter",
        .help = "Show Meter",
        .hint = NULL,
        .func = &cmdMeter,
        .argtable = NULL
    };

    const esp_console_cmd_t config_cmd = {
        .command = "config",
        .help = "Show Configuration",
        .hint = NULL,
        .func = &cmdConfig,
        .argtable = NULL
    };

    const esp_console_cmd_t erase_cmd = {
        .command = "erase",
        .help = "Erase Configuration",
        .hint = NULL,
        .func = &cmdErase,
        .argtable = NULL
    };

    const esp_console_cmd_t send_cmd = {
        .command = "sendMqtt",
        .help = "Send Mqtt Msgsn",
        .hint = NULL,
        .func = &cmdSend,
        .argtable = NULL
    };

    const esp_console_cmd_t ota_cmd = {
        .command = "ota",
        .help = "Start Ota Update",
        .hint = NULL,
        .func = &cmdOTA,
        .argtable = NULL
    };

    const esp_console_cmd_t controler_cmd = {
        .command = "control",
        .help = "Configure Controller",
        .hint = NULL,
        .func = &cmdController,
        .argtable = NULL
    };

    const esp_console_cmd_t sendmesh_cmd = {
        .command = "sendMesh",
        .help = "Send a Msg to Root",
        .hint = NULL,
        .func = &cmdSendMesh,
        .argtable = NULL
    };

    const esp_console_cmd_t loglevel_cmd = {
        .command = "loglevel",
        .help = "Set log level",
        .hint = NULL,
        .func = &cmdLogLevel,
        .argtable = &loglevel
    };

    const esp_console_cmd_t resetconf_cmd = {
        .command = "resetconf",
        .help = "Reset Conf flags",
        .hint = NULL,
        .func = &cmdResetConf,
        .argtable = &resetlevel
    };

    const esp_console_cmd_t aes_cmd = {
        .command = "aes",
        .help = "Encrypt Decrypt",
        .hint = NULL,
        .func = &cmdEnDecrypt,
        .argtable = &endec
    };


    ESP_ERROR_CHECK(esp_console_cmd_register(&fram_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&meter_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&config_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&erase_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&send_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&controler_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&ota_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&sendmesh_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&loglevel_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&resetconf_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&aes_cmd));


   ESP_ERROR_CHECK(esp_console_start_repl(repl));
   while(true)
      delay(10000);
}