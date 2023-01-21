
#ifndef framDef_h
#define framDef_h

//#define HOST
#define TXL 				SOC_SPI_MAXIMUM_BUFFER_SIZE
#define MAXDEVSUP           (8)
#define MAXDEVSS            (8)
#define MWORD				(2)
#define LLONG               (4)
// Fram Data Map
#define FRAMDATE			0                       
#define GUARDM				(FRAMDATE+LLONG)        
#define SCRATCH          	(GUARDM+LLONG)          
#define SCRATCHEND        	(SCRATCH+100)         
//medidores 
#define BEATSTART           (SCRATCHEND)           
#define MID                 (BEATSTART+MWORD)      
#define MAXAMP              (MID+12)      
#define MINAMP              (MAXAMP+MWORD)      
#define BPK                 (MINAMP+MWORD)                
#define BEATLIFE            (BPK+MWORD)                 
#define MKWHSTART           (BEATLIFE+LLONG)        
#define MUPDATE             (MKWHSTART+LLONG)       
#define LIFEKWH             (MUPDATE+LLONG)       
#define LIFEDATE            (LIFEKWH+LLONG)         
#define MONTHSTART          (LIFEDATE+LLONG)        
#define DAYSTART            (MONTHSTART+MWORD*12)   
#define DATAEND             (DAYSTART+(366*MWORD)) 
                                                             
#define TOTALFRAM			((DATAEND-BEATSTART)*MAXDEVSS+BEATSTART)

#endif /* framDef_h */
