#define GLOBAL
#include "includes.h"
#include "globals.h"

void drawString(int x, int y, char* que, int fsize, int align,displayType showit,overType erase)
{

	if(fsize!=lastFont)
	{
		lastFont=fsize;
		switch (fsize)
		{
		case 6:
			u8g2_SetFont(&u8g2,u8g2_font_squeezed_r6_tr );
			break;
		case 8:
			u8g2_SetFont(&u8g2,u8g2_font_5x8_tr );
			break;
		case 10:
			u8g2_SetFont(&u8g2,u8g2_font_6x12_tr);
			break;
		case 12:
			u8g2_SetFont(&u8g2,u8g2_font_7x14_tr);
			break;
		case 16:
			u8g2_SetFont(&u8g2,u8g2_font_10x20_tr);
			break;
		case 24:
			u8g2_SetFont(&u8g2,u8g2_font_logisoso24_tr);
			break;
		default:
			break;
		}
	}

	if(lastalign!=align)
	{
		lastalign=align;
	}

	if(erase==REPLACE)
	{
		int w=u8g2_GetStrWidth(&u8g2,que);
		int xx=0;
		switch (lastalign) {
		case TEXT_ALIGN_LEFT:
			xx=x;
			break;
		case TEXT_ALIGN_CENTER:
			xx=x-w/2;
			break;
		case TEXT_ALIGN_RIGHT:
			xx=x-w;
			break;
		default:
			break;
		}
		u8g2_SetDrawColor(&u8g2,0);
		u8g2_DrawBox(&u8g2,xx,y,w,lastFont);
		u8g2_SetDrawColor(&u8g2,1);
	}

	u8g2_DrawStr(&u8g2,x,y,que);
	if (showit==DISPLAYIT)
		u8g2_SendBuffer(&u8g2);
}

static void displayBeats()
{
	uint8_t posx[MAXDEVSS]={0,40,80,0,40,80,0,40},posy[MAXDEVSS]={10,10,10,30,30,30,50,50};
	char textt[20];
	uint16_t count;

	for(int a=0;a<MAXDEVSS;a++)
	{
		if(medidor[a].beatlife>oldCurBeat[a])
		{
			pcnt_get_counter_value((pcnt_unit_t)a,(short int *) &count);
			sprintf(textt,"%5d-%d",medidor[a].beatlife,count);
			drawString(posx[a], posy[a],textt, 8, TEXT_ALIGN_LEFT,DISPLAYIT, REPLACE);
			oldCurBeat[a]=medidor[a].beatlife;
		}
	}
}

static void displayKwH()
{

	char textt[20];
	uint8_t posx[MAXDEVSS]={0,40,80,0,40,80,0,40},posy[MAXDEVSS]={10,10,10,30,30,30,50,50};
	// uint8_t posx[5]={0,70,35,0,70},posy[5]={1,1,20,38,38};
	uint16_t count;

	for(int a=0;a<MAXDEVSS;a++)
	{
		if(medidor[a].lifekwh>oldCurLife[a])
		{
			pcnt_get_counter_value((pcnt_unit_t)a,(short int *) &count);
			sprintf(textt,"%5d",medidor[a].lifekwh);
			// sprintf(textt,"%5d-%d",medidor[a].lifekwh,count);
			drawString(posx[a], posy[a],textt, 12, TEXT_ALIGN_LEFT,DISPLAYIT, REPLACE);
			oldCurLife[a]=medidor[a].lifekwh;
		}
	}
}

void displayMeter(int cual)
{
	char textd[50];
	if(oldcual!=cual)
	{
		sprintf(textd,"%4dBPK    MID:%s",medidor[cual].bpk,medidor[cual].mid);
		drawString(0, 12, textd, 8, TEXT_ALIGN_LEFT,DISPLAYIT, REPLACE);
		oldcual=cual;
	}

	sprintf(textd,"%d kWh",medidor[cual].lifekwh);
	drawString(35, 37, textd, 16, TEXT_ALIGN_CENTER,DISPLAYIT, REPLACE);

}

void displayManager(void *arg) 
{
	time_t t = 0;
	struct tm timeinfo ;
	char textd[100],textt[100];

	uint8_t displayMode=0;// nothing

  	gpio_config_t 	        io_conf;
    
    bzero(&io_conf,sizeof(io_conf));

    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en =GPIO_PULLUP_ENABLE;
	io_conf.pin_bit_mask =  (1ULL<<0); //input pins
	gpio_config(&io_conf);

	bzero(oldCurBeat,sizeof(oldCurBeat));
	bzero(oldCurLife,sizeof(oldCurLife));
	// u8g2_ClearBuffer(&u8g2);
	// u8g2_SendBuffer(&u8g2);


	while(true)
	{
		vTaskDelay(400/portTICK_PERIOD_MS);
		if(!gpio_get_level((gpio_num_t)0))
		{
			displayMode++;
			if(displayMode>10)
				displayMode=0;
			bzero(oldCurBeat,sizeof(oldCurBeat));
			bzero(oldCurLife,sizeof(oldCurLife));
			u8g2_ClearBuffer(&u8g2);
			u8g2_SendBuffer(&u8g2);
		}
		if(displayMode>0)
		{
			time(&t);
			localtime_r(&t, &timeinfo);
			sprintf(textd,"%02d/%02d/%04d",timeinfo.tm_mday,timeinfo.tm_mon+1,1900+timeinfo.tm_year);
			sprintf(textt,"%02d:%02d:%02d",timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec);
			drawString(0, 63, textd, 10, TEXT_ALIGN_LEFT,NODISPLAY, REPLACE);
			drawString(81, 63, textt, 10, TEXT_ALIGN_LEFT,DISPLAYIT, REPLACE);
			// if(framGuard)
			// 	drawString(60, 51, "FG", 10, TEXT_ALIGN_LEFT,DISPLAYIT, REPLACE);
			// else
			// 	drawString(60, 51, "OK", 10, TEXT_ALIGN_LEFT,DISPLAYIT, REPLACE);
			if(displayMode==1)
				displayBeats();
			else
				if(displayMode==2)
					displayKwH();
				else
					displayMeter(displayMode-3);
			
		}
	}
}

