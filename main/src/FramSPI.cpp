#include "includes.h"

#define METERSIZE	(DATAEND-BEATSTART)

FramSPI::FramSPI(void)
{
	_framInitialised = false;
	spi =NULL;
	intframWords=0;
	addressBytes=0;
	prodId=0;
	manufID=0;
	setw=true;
	maxSpeed=SPI_MASTER_FREQ_8M;
}

bool FramSPI::begin(int MOSI, int MISO, int CLK, int CS,SemaphoreHandle_t *framSem)
{
	int ret;
	spi_bus_config_t 				buscfg;
	spi_device_interface_config_t 	devcfg;

	memset(&buscfg,0,sizeof(buscfg));
	memset(&devcfg,0,sizeof(devcfg));

	*framSem= xSemaphoreCreateBinary();
		if(*framSem)
			xSemaphoreGive(*framSem);  //SUPER important else its born locked
		else
			printf("Cant allocate Fram Sem\n");

	buscfg.mosi_io_num=MOSI;
	buscfg .miso_io_num=MISO;
	buscfg.sclk_io_num=CLK;
	buscfg.quadwp_io_num=-1;
	buscfg .quadhd_io_num=-1;
	buscfg.max_transfer_sz=10000;// useless in Half Duplex, max is set by ESPIDF in SOC_SPI_MAXIMUM_BUFFER_SIZE 64 bytes

	//Initialize the SPI bus
	ret=spi_bus_initialize(VSPI_HOST, &buscfg, 0);
	assert(ret == ESP_OK);

	devcfg .clock_speed_hz=					SPI_MASTER_FREQ_20M;              	//Clock out for test in Saleae clone limited speed
	devcfg.mode=							0;                             		//SPI mode 0
	devcfg.spics_io_num=					CS;               					//CS pin
	devcfg.queue_size=						7;                         			//We want to be able to queue 7 transactions at a time
	devcfg.flags=							SPI_DEVICE_HALFDUPLEX;

	ret=spi_bus_add_device(VSPI_HOST, &devcfg, &spi);
	if (ret==ESP_OK)
	{
		getDeviceID(&manufID, &prodId);
		//Set write enable after chip is identified
		switch(prodId)
		{
		case 0x409:
			addressBytes=2;
			intframWords=16384;//128k
			setw=true;
			maxSpeed=SPI_MASTER_FREQ_20M;
			break;
		case 0x509:
			addressBytes=2;
			intframWords=32768;//256k
			maxSpeed=SPI_MASTER_FREQ_20M;
			setw=true;
			break;
		case 0x2603:
			addressBytes=2;
			intframWords=65536;//512k
			maxSpeed=SPI_MASTER_FREQ_26M;
			setw=true;
			break;
		case 0x2703:
			addressBytes=3;
			intframWords=131072;//1mb
			maxSpeed=SPI_MASTER_FREQ_26M;
			setw=true;
			break;
		case 0x4803:
			addressBytes=3;
			intframWords=262144;//2mb
			maxSpeed=SPI_MASTER_FREQ_26M;
			setw=false;
			break;
		default:
			addressBytes=2;
			intframWords=0;
			return false;
		}
		printf("ManufacturerId %04x ProductId %04x Need %d have %d\n",manufID,prodId,TOTALFRAM,intframWords);

		spi_bus_remove_device(spi); //remove device to reset speed to max

		devcfg.clock_speed_hz=					maxSpeed;
		devcfg.mode=							0;                             		//SPI mode 0
		devcfg.spics_io_num=					CS;               					//CS pin
		devcfg.queue_size=						7;                         			//We want to be able to queue 7 transactions at a time
		devcfg.flags=							SPI_DEVICE_HALFDUPLEX;

		ret=spi_bus_add_device(VSPI_HOST, &devcfg, &spi);

		if(TOTALFRAM>intframWords)
		{
			printf("Not enough space for Meter Definition %d vs %d required\n",intframWords,BEATSTART);
			return false;
		}

		_framInitialised = true;

		devcfg.address_bits=addressBytes*8;
		devcfg.command_bits=8;

		ret=spi_bus_add_device(VSPI_HOST, &devcfg, &spi);

		sendCmd(MBRSPI_WREN);// at least once then if setw every write

		return true;
	}
	else
	{
		printf("could not init spi fram %x\n",ret);
		return false;
	}
}


int  FramSPI::sendCmd (uint8_t cmd)
{
	esp_err_t ret;
	spi_transaction_ext_t t;

	memset(&t, 0, sizeof(t));       //Zero out the transaction no need to set to 0 or null unused params

	t.base.flags= 		(SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR);
	t.base.cmd=			cmd;
	t.command_bits = 	8;
	t.address_bits=		0;
	t.base.length = 	0;
	ret=spi_device_polling_transmit(spi, (spi_transaction_t*)&t);  //Transmit!
	return ret;

}

uint8_t  FramSPI::readStatus ()
{
	spi_transaction_ext_t t;

	memset(&t, 0, sizeof(t));       //Zero out the transaction no need to set to 0 or null unused params

	t.base.flags= 		( SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR| SPI_TRANS_USE_RXDATA );
	t.base.cmd=			MBRSPI_RDSR;
	t.base.rxlength=	8;
	t.command_bits = 	8;
	t.address_bits =	0;
	spi_device_polling_transmit(spi, (spi_transaction_t*)&t);  //Transmit!
	return t.base.rx_data[0];
}

int  FramSPI::writeStatus ( uint8_t streg)
{
	esp_err_t ret;
	spi_transaction_ext_t t;

	memset(&t, 0, sizeof(t));       //Zero out the transaction no need to set to 0 or null unused params

	t.base.flags= 		( SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR| SPI_TRANS_USE_TXDATA);
	t.base.cmd=			MBRSPI_WRSR;
	t.command_bits = 	8;
	t.address_bits=		0;
	t.base.tx_data[0]=	streg;
	t.base.length=		8;
	ret=spi_device_polling_transmit(spi, (spi_transaction_t*)&t);  //Transmit!
	return ret;
}

int FramSPI::writeMany (uint32_t framAddr, uint8_t *valores,uint32_t son)
{
	spi_transaction_ext_t t;
	esp_err_t ret=0;
	int fueron;

	memset(&t,0,sizeof(t));	//Zero out the transaction no need to set to 0 or null unused params

	while(son>0)
	{
		if (setw)
			sendCmd(MBRSPI_WREN);

		fueron=				son>TXL?TXL:son;
		t.base.flags= 		( SPI_TRANS_VARIABLE_ADDR | SPI_TRANS_VARIABLE_CMD );
		t.base.addr = 		framAddr;
		t.base.length = 	fueron*8;
		t.base.tx_buffer = 	valores;
		t.base.cmd=			MBRSPI_WRITE;
		t.command_bits = 	8;
		t.address_bits = 	addressBytes*8;
		ret=spi_device_polling_transmit(spi, (spi_transaction_t*)&t);  //Transmit and wait!

		son					-=fueron; 	// reduce bytes processed
		framAddr			+=fueron;  	// advance Address by fueron bytes processed
		valores				+=fueron;  	// advance Buffer to write by processed bytes
	}
	return ret;
}

int FramSPI::readMany (uint32_t framAddr, uint8_t *valores, uint32_t son)
{
	esp_err_t ret=0;
	spi_transaction_ext_t t;
	int cuantos,fueron;

	memset(&t, 0, sizeof(t));	//Zero out the transaction no need to set to 0 or null unused params

	cuantos=son;
	while(cuantos>0)
	{
		fueron=				cuantos>TXL?TXL:cuantos;

		t.base.flags= 		( SPI_TRANS_VARIABLE_ADDR | SPI_TRANS_VARIABLE_CMD );
		t.base.addr = 		framAddr;
		t.base.cmd=			MBRSPI_READ;
		t.command_bits = 	8;
		t.address_bits = 	addressBytes*8;
		t.base.rx_buffer=	valores;
		t.base.rxlength=	fueron*8;
		ret=spi_device_polling_transmit(spi, (spi_transaction_t*)&t);	//Transmit and wait

		cuantos				-=fueron;
		framAddr			+=fueron;
		valores				+=fueron;
	}
	return ret;
}

int FramSPI::write8 (uint32_t framAddr, uint8_t value)
{
	esp_err_t ret;
	spi_transaction_ext_t t;

	if(setw)
		sendCmd(MBRSPI_WREN);

	memset(&t,0,sizeof(t));	//Zero out the transaction no need to set to 0 or null unused params
	t.base.tx_data[0]=	value;
	t.base.flags= 		( SPI_TRANS_VARIABLE_ADDR | SPI_TRANS_VARIABLE_CMD| SPI_TRANS_USE_TXDATA );
	t.base.addr = 		framAddr;
	t.base.length = 	8;
	t.base.cmd=			MBRSPI_WRITE;
	t.command_bits = 	8;
	t.address_bits = 	addressBytes*8;
	ret=spi_device_polling_transmit(spi, (spi_transaction_t*)&t);  //Transmit and wait!
	return ret;
}

int FramSPI::read8 (uint32_t framAddr,uint8_t *donde)
{
	spi_transaction_ext_t t;
	int ret;

	memset(&t, 0, sizeof(t));       //Zero out the transaction no need to set to 0 or null unused params

	t.base.flags= 		( SPI_TRANS_VARIABLE_ADDR | SPI_TRANS_VARIABLE_CMD| SPI_TRANS_USE_RXDATA );
	t.base.addr = 		framAddr;                                         //set address
	t.base.cmd=			MBRSPI_READ;
	t.command_bits = 	8;
	t.address_bits = 	addressBytes*8;
	t.base.rxlength=	8;
	ret=spi_device_polling_transmit(spi, (spi_transaction_t*)&t);  //Transmit!
	*donde=t.base.rx_data[0];
	return ret;
}

void FramSPI::getDeviceID(uint16_t *manufacturerID, uint16_t *productID)
{
	spi_transaction_ext_t t;

	memset(&t, 0, sizeof(t));       //Zero out the transaction no need to set to 0 or null unused params

	t.base.flags= 		( SPI_TRANS_VARIABLE_CMD| SPI_TRANS_USE_RXDATA );
	t.base.cmd=			MBRSPI_RDID;
	t.command_bits = 	8;
	t.base.rxlength=	32;
	spi_device_polling_transmit(spi, (spi_transaction_t*)&t);  //Transmit and wait!

	// Shift values to separate manuf and prod IDs
	// See p.10 of http://www.fujitsu.com/downloads/MICRO/fsa/pdf/products/memory/fram/MB85RC256V-DS501-00017-3v0-E.pdf
	*manufacturerID=(t.base.rx_data[0]<<8)+t.base.rx_data[1];
	*productID=(t.base.rx_data[2]<<8)+t.base.rx_data[3];
}



int FramSPI::format(uint8_t valor, uint8_t *lbuffer,uint32_t len,bool all)
{
	uint32_t add=0;
	int count=intframWords,ret;

	uint8_t *buffer=(uint8_t*)malloc(len);
	if(!buffer)
	{
		printf("Failed format buf\n");
		return -1;
	}
int son=0;

	while (count>0)
	{
		if(lbuffer!=NULL)
				memcpy(buffer,lbuffer,len); //Copy whatever was passed
			else
				memset(buffer,valor,len);  //Should be done only once

		if (count>len)
		{
			son+=len;
		//	printf("Format add %d len %d count %d val %d\n",add,len,count,valor);
			ret=writeMany(add,buffer,len);
			if (ret!=0)
				return ret;
		}
		else
		{
			son+=count;
		//	printf("FinalFormat add %d len %d val %d\n",add,count,valor);
			ret=writeMany(add,buffer,count);
			if (ret!=0)
				return ret;
		}
		count-=len;
		add+=len;
	}

//	verify
	uint32_t monton=0,total;
	if(all)
	{
		count=intframWords;
		add=0;
		uint8_t *lbuffer=buffer; //copy it
		while(count>0)
		{
			buffer=lbuffer;
			memset(buffer,0,len);

			if(count>len)
				monton=len;
			else
				monton=count;
			ret=readMany(add,buffer,monton);
			if(ret!=0)
			{
				printf("Verify failed HW\n");
				return ret;
			}
//compare
			total=0;
			for (int a=0;a<monton;a++)
			total+= *(buffer++);
			if (total!=0)
			{
				printf("Verify Logic failed Where %d Total %d\n",add,total);
				return -1;
			}

			add+=monton;
			count-=monton;
		}
		buffer=lbuffer;			//for free
	}
//	printf("Verify Ok\n");
	if(buffer)
		free(buffer);
	return ESP_OK;
}

int FramSPI::formatSlow(uint8_t valor)
{
	uint32_t add=0;
	int count=intframWords;
	while (count>0)
	{
		write8(add,valor);
		count--;
		add++;
	}
	return ESP_OK;
}


int FramSPI::formatMeter(uint8_t cual)
{
	uint32_t add;
	if (cual>MAXDEVSS)
		return -1;		//OB

	int count=DATAEND-BEATSTART+LLONG,ret;
	add=count*cual+BEATSTART;

	void *buf=malloc(count);
	if(!buf)
		return -1;
	bzero(buf,count);
	ret=writeMany(add,(uint8_t*)buf,count);
	if(buf)
		free(buf);
	return ret;

}

// Meter Data Management

int FramSPI::write_bytes(uint8_t meter,uint32_t add,uint8_t*  desde,uint32_t cuantos)
{
	int ret;
	if (meter>MAXDEVSS)
		return -1;		//OB
	add+=METERSIZE*meter+SCRATCHEND-BEATSTART;//skip header SCRATCHEND then start from 0 within each Meter by substracting BEATSTART
	// printf("WriteF %x - %d\n",add,add);
	ret=writeMany(add,desde,cuantos);
	return ret;
}

int FramSPI::read_bytes(uint8_t meter,uint32_t add,uint8_t*  donde,uint32_t cuantos)
{
	if (meter>MAXDEVSS)
		return -1;		//OB
	add+=METERSIZE*meter+SCRATCHEND-BEATSTART; //skip header SCRATCHEND then start from 0 within each Meter by substracting BEATSTART
	// printf("ReadF %x - %d\n",add,add);
	int ret;
	ret=readMany(add,donde,cuantos);
	return ret;
}

int FramSPI::write_guard(uint32_t  value)
{
	int ret;
	uint32_t badd=GUARDM;
	ret=writeMany(badd,(uint8_t*)&value,LLONG);
	return ret;
}

int FramSPI::write_beat(uint8_t meter, uint32_t value)
{
	int ret;
	if (meter>MAXDEVSS)
		return -1;		//OB
	uint32_t badd=BEATSTART;
	ret=write_bytes(meter,badd,(uint8_t* )&value,MWORD);
	return ret;
}

int FramSPI::write_lifedate(uint8_t meter, uint32_t value)
{
	int ret;
	uint32_t badd=LIFEDATE;
	if (meter>MAXDEVSS)
		return -1;		//OB

	ret=write_bytes(meter,badd,(uint8_t* )&value,LLONG);
	return ret;
}

int FramSPI::write_lifekwh(uint8_t meter, uint32_t value)
{
	int ret;
	uint32_t badd=LIFEKWH;
	if (meter>MAXDEVSS)
		return -1;		//OB
	ret=write_bytes(meter,badd,(uint8_t* )&value,LLONG);
	return ret;
}

int FramSPI::write_beatlife(uint8_t meter, uint32_t value)
{
	int ret;
	uint32_t badd=BEATLIFE;
	if (meter>MAXDEVSS)
		return -1;		//OB
	ret=write_bytes(meter,badd,(uint8_t* )&value,LLONG);
	return ret;
}

int FramSPI::write_kwhstart(uint8_t meter, uint32_t value)
{
	int ret;
	uint32_t badd=MKWHSTART;
	if (meter>MAXDEVSS)
		return -1;		//OB
	ret=write_bytes(meter,badd,(uint8_t* )&value,LLONG);
	return ret;
}

int FramSPI::write_month(uint8_t meter,uint8_t month,uint16_t value)
{
	int ret;

	uint32_t badd=MONTHSTART+month*MWORD;
	if (meter>MAXDEVSS || month>11)
	{
		printf("OB Mont %d %d\n",meter,month);
		return -1;		//OB
	}

	ret=write_bytes(meter,badd,(uint8_t* )&value,MWORD);
	return ret;
}

int FramSPI::write_mid(uint8_t meter,uint8_t *mid,uint16_t len)
{
	int ret;

	uint32_t badd=MID;
	if(len>12)
		len=12;	//truncate

	ret=write_bytes(meter,badd,mid,len);
	return ret;
}

int FramSPI::write_day(uint8_t meter,uint16_t days,uint16_t value)
{
	int ret;
	uint32_t badd=DAYSTART+days*MWORD;
	if (meter>MAXDEVSS || days>366)
		return -1;		//OB

	ret=write_bytes(meter,badd,(uint8_t* )&value,MWORD);
	return ret;
}

int FramSPI::write_meter(uint8_t meter,uint8_t *mmeter,uint16_t len)
{
	int ret;

	uint32_t badd=BEATSTART;

	ret=write_bytes(meter,badd,mmeter,len);
	return ret;
}


int FramSPI::read_guard(uint8_t*  value)
{
	int ret;
	uint32_t badd=GUARDM;
	ret=readMany(badd,value,LLONG);
	return ret;
}


int FramSPI::read_lifedate(uint8_t meter, uint8_t*  value)
{
	int ret;
	uint32_t badd=LIFEDATE;
	if (meter>MAXDEVSS)
		return -1;		//OB
	ret=read_bytes(meter,badd,value,LLONG);
	return ret;
}

int FramSPI::read_lifekwh(uint8_t meter, uint8_t*  value)
{
	int ret;
	uint32_t badd=LIFEKWH;
	if (meter>MAXDEVSS )
		return -1;		//OB
	ret=read_bytes(meter,badd,value,LLONG);
	return ret;
}

int FramSPI::read_beatlife(uint8_t meter, uint8_t*  value)
{
	int ret;
	uint32_t badd=BEATLIFE;
	if (meter>MAXDEVSS )
		return -1;		//OB
	ret=read_bytes(meter,badd,value,LLONG);
	return ret;
}

int FramSPI::read_kwhstart(uint8_t meter, uint8_t*  value)
{
	int ret;
	uint32_t badd=MKWHSTART;
	if (meter>MAXDEVSS )
		return -1;		//OB
	ret=read_bytes(meter,badd,value,LLONG);
	return ret;
}

int FramSPI::read_beat(uint8_t meter, uint8_t*  value)
{
	int ret;
	uint32_t badd=BEATSTART;
	if (meter>MAXDEVSS )
			return -1;		//OB
	ret=read_bytes(meter,badd,value,MWORD);

	return ret;
}

int FramSPI::read_month(uint8_t meter,uint8_t month,uint8_t*  value)
{
	int ret;
	uint32_t badd=MONTHSTART+month*MWORD;
	if (meter>MAXDEVSS || month>11)
	{
		printf("read OB Mont %d %d\n",meter,month);
			return -1;		//OB
	}
	ret=read_bytes(meter,badd,value,MWORD);

	return ret;
}

int FramSPI::read_mid(uint8_t meter,uint8_t*  value,uint8_t len)
{
	int ret;
	uint32_t badd=MID;

	ret=read_bytes(meter,badd,value,12);

	return ret;
}

int FramSPI::read_day(uint8_t meter,uint16_t days,uint8_t*  value)
{
	int ret;
	uint32_t badd=DAYSTART+days*MWORD;
	if (meter>MAXDEVSS || days>366)
			return -1;		//OB
	ret=read_bytes(meter,badd,value,MWORD);

	return ret;
}


int FramSPI::read_meter(uint8_t meter,uint8_t*  value,uint16_t len)
{
	int ret;
	uint32_t badd=BEATSTART;

	ret=read_bytes(meter,badd,value,len);

	return ret;
}