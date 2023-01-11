#include "includes.h"

#ifndef _FramSPI_H_
#define _FramSPI_H_

#define MB85RC_DEFAULT_ADDRESS          (0x50) /* 1010 + A2 + A1 + A0 = 0x50 default */
#define MB85RC_SLAVE_ID                 (0xF8)
#define ACK 							true
#define NAK 							false
#define MBRSPI_WREN 					0x06
#define MBRSPI_WRDI 					0x04
#define MBRSPI_RDSR 					0x05
#define MBRSPI_WRSR 					0x01
#define MBRSPI_READ 					0x03
#define MBRSPI_WRITE 					0x02
#define MBRSPI_RDID 					0x9f
#define MBRSPI_FSTRD 					0x0c


typedef struct
{
    uint8_t       state;
    uint8_t		  free;
    uint16_t      meter;
    uint32_t      life;
    uint16_t      month;
    uint16_t      day;
    uint16_t      cycle;
    uint16_t      hora;
    uint16_t      mesg,diag,horag;
    uint16_t      yearg;
} meterspi;


class FramSPI {
public:
    FramSPI(void);
    int 		sendCmd (uint8_t cmd);
    uint8_t		readStatus();
    int  		writeStatus(uint8_t streg);
    bool 		begin(int MOSI, int MISO, int CLK, int CS,SemaphoreHandle_t *framSem);
    int	        write8 (uint32_t framAddr, uint8_t value);
    int     	read8  (uint32_t framAddr, uint8_t *donde);
    void        getDeviceID(uint16_t *manufacturerID, uint16_t *productID);
    int         writeBulk (uint32_t framAddr, uint8_t *valores,uint32_t son);
    int 		format(uint8_t valor, uint8_t *buffer,uint32_t len,bool all);
    int 		formatSlow(uint8_t valor);

    int			formatMeter(uint8_t cual);
    int 		writeMany (uint32_t framAddr, uint8_t *valores,uint32_t son);
    int			readMany (uint32_t framAddr, uint8_t *valores,uint32_t son);
    int         write_bytes(uint8_t meter,uint32_t add,uint8_t*  desde,uint32_t cuantos);
    int         read_bytes(uint8_t meter,uint32_t add,uint8_t*  donde,uint32_t cuantos);
    
    int	        write_beat(uint8_t medidor, uint32_t value);
    int	        write_lifekwh(uint8_t medidor, uint32_t value);
    int	        write_beatlife(uint8_t medidor, uint32_t value);
    int	        write_month(uint8_t medidor,uint8_t month,uint16_t value);
    int	        write_day(uint8_t medidor,uint16_t yearDay,uint16_t value);
    int	        write_guard(uint32_t value);
    int	        write_lifedate(uint8_t medidor, uint32_t value);
    int         write_mid(uint8_t meter,uint8_t *mid,uint16_t len);
    int         write_kwhstart(uint8_t meter, uint32_t value);
    int         write_meter(uint8_t meter,uint8_t *mid,uint16_t len);

    int	        read_beat(uint8_t medidor, uint8_t*  value);
    int	        read_month(uint8_t medidor,uint8_t month,uint8_t*  value);
    int	        read_day(uint8_t medidor,uint16_t yearDay,uint8_t*  value);
    int	        read_guard(uint8_t*  value);
    int	        read_lifedate(uint8_t medidor,uint8_t*  value);
    int	        read_lifekwh(uint8_t medidor, uint8_t*  value);
    int	        read_beatlife(uint8_t medidor, uint8_t*  value);
    int         read_mid(uint8_t meter,uint8_t*  value, uint8_t len);
    int         read_kwhstart(uint8_t meter, uint8_t*  value);
    int         read_meter(uint8_t meter, uint8_t*  value,uint16_t len);

public:
    bool _framInitialised;
    spi_device_handle_t spi;
    uint8_t addressBytes;
    uint32_t intframWords,maxSpeed;
    uint16_t manufID,prodId;
    bool setw;

    void	setWrite(void);

};

#endif
