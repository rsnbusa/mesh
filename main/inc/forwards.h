#ifndef FORWARDS_H
#define FORWARDS_H

void set_senddata_timer();
void kbd();
void displayManager(void *pArg);
void mdelay(uint32_t a);
uint32_t xmillis(void);
int cmdRestore(void *argument);
int cmdFormat(void *argument);
int cmdFormatMeter(void *argument);
int cmdInitMeter(void *argument);
int cmdMetrics(void *argument);
int cmdController(void *argument);
int cmdUpdate(void *argument);
int cmdErase(void *argument);
#endif
