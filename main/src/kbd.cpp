
#define GLOBAL
#include "includes.h"
#include "globals.h"

uint32_t theAddress=0;
uint8_t wmeter=0;

extern void erase_config();


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
  char buf[50],buf2[50];
  time_t lastwrite;

  lafecha(theConf.bornDate,buf);
  fram.read_guard((uint8_t*)&lastwrite);
  lafecha(lastwrite,buf2);
  printf("======= Controller Configuration  =======\n");
  printf("Id : %d  Address: %s Created %s\n",theConf.controllerid,theConf.direccion,buf);
  printf("Provincia: %d Canton: %d Parroquia:%d CodigoPostal:%d\n",theConf.provincia,theConf.canton,theConf.parroquia,theConf.codpostal);
  printf("BootCount %d LastReset %d Reason %d DownTime %d lastupdate %s\n",theConf.bootcount,theConf.lastResetCode,theConf.lastResetCode,theConf.downtime,buf2);
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
  printf("Setting Mqtt Send Bit");
  xEventGroupSetBits(wifi_event_group, SENDMQTT_BIT);	// clear bit to wait on
  printf("done\n");
  return 0;
}

void kbd()
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
        .command = "send",
        .help = "Send Mqtt Msgsn",
        .hint = NULL,
        .func = &cmdSend,
        .argtable = NULL
    };


    ESP_ERROR_CHECK(esp_console_cmd_register(&fram_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&meter_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&config_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&erase_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&send_cmd));


   ESP_ERROR_CHECK(esp_console_start_repl(repl));
}