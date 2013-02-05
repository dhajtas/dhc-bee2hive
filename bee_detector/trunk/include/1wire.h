#ifndef	OWIRE_H
#define OWIRE_H

//-----------------------------------------------------------------------------------------------//
//		structure definitions 
//-----------------------------------------------------------------------------------------------//

typedef struct OWIRE
{
	uint8_t data[8];
} OWIRE;

//-----------------------------------------------------------------------------------------------//
//		Global variables
//-----------------------------------------------------------------------------------------------//

extern OWIRE OWbuff[D_NUM];

//-----------------------------------------------------------------------------------------------//
//		routines
//-----------------------------------------------------------------------------------------------//


port_width ow_Init(void);

void clr_owBuffer(void);

uint8_t GetDallasID(uint8_t);

void owire(uint8_t, port_width);	// OWIRE*, uint8_t, uint8_t);

port_width ow_reset(port_width);

void ow_rstprt(port_width);

void ow_outp(uint8_t, port_width);

void ow_inp(OWIRE*, uint8_t, port_width);

uint8_t ow_inp_1(port_width);

void ow_sendbit(port_width, uint8_t);

port_width ow_recbit(port_width);

port_width CheckCRC_8(OWIRE*, port_width, uint8_t);	//uint8_t,

void delay_us(uint16_t);

uint8_t CRC_8(uint8_t, uint8_t);

uint16_t CRC_16(uint16_t, uint8_t);

#ifndef  F_CPU
#define  F_CPU		              8000000   /* The cpu clock frequency in Hertz */
#endif

#define ONE_WIRE_RECOVERY_TIME_US     2       /* The time for the pullup resistor to get the line high */
#define ONE_WIRE_RESET_TIME_US        1000    /* The total reset pulse time */
#define ONE_WIRE_PRESENSE_TIME_US     70     /* The time of the presence pulse detection */
#define ONE_WIRE_WRITE_SLOT_TIME_US   90      /* The total time of the WRITE SLOT  */ 
#define ONE_WIRE_READ_SLOT_TIME_US    60      /* The total time of the READ SLOT  */ 

/* Accurate Delay macros */
#ifndef DELAY_S_ACCURATE
/* 4 cpu cycles per loop + 1 overhead when a constant is passed. */
#define DELAY_S_ACCURATE(x)  ({ unsigned short number_of_loops=(unsigned short)x; \
                              	__asm__ volatile ( "L_%=:           \n\t"         \
                                                   "sbiw %A0,1      \n\t"         \
                                                   "brne L_%=       \n\t"         \
                                                   : /* NO OUTPUT */              \
                                                   : "w" (number_of_loops)        \
                                                 );                               \
                             })                               

#endif /* #ifndef DELAY_S_ACCURATE */

#ifndef DELAY_US
#define DELAY_US(us)                DELAY_S_ACCURATE( (((us*1000L)/(1000000000/F_CPU))-1)/4 )  
#endif

#ifndef DELAY_L_ACCURATE
/* 6 cpu cycles per loop + 3 overhead when a constant is passed. */
#define DELAY_L_ACCURATE(x)  ({ unsigned long number_of_loops=(unsigned long)x;   \
                                __asm__ volatile ( "L_%=: \n\t"                   \
                                                   "subi %A0,lo8(-(-1)) \n\t"     \
                                                   "sbci %B0,hi8(-(-1)) \n\t"     \
                                                   "sbci %C0,hlo8(-(-1)) \n\t"    \
                                                   "sbci %D0,hhi8(-(-1)) \n\t"    \
                                                   "brne L_%= \n\t"               \
                                                   : /* NO OUTPUT */              \
                                                   : "w" (number_of_loops)        \
                                                 );                               \
                             })                                      

#endif  /* #ifndef DELAY_L_ACCURATE */

#ifndef TIME_L1_MS
#define TIME_L1_MS    ( 1*(F_CPU/6000) )    /* MUST USE A LONG FOR COUNTING TO BE ACCURATE  */
#endif
#ifndef DELAY_MS
#define DELAY_MS(ms)  DELAY_L_ACCURATE((TIME_L1_MS*ms))
#endif

/* Definition of the recovery time delay maro. Accuracy is great and it is needed */
#define OW_RECOVERY_TIME            ( ((ONE_WIRE_RECOVERY_TIME_US*1000L)/(1000000000/F_CPU)) )

#if  OW_RECOVERY_TIME<= 0
#error " RECOVERY TIME TO SMALL "

#elif  OW_RECOVERY_TIME== 1
#define DELAY_OW_RECOVERY_TIME()     __asm__ volatile("nop")

#elif OW_RECOVERY_TIME== 2
#define DELAY_OW_RECOVERY_TIME()     __asm__ volatile("rjmp _PC_+0")

#elif OW_RECOVERY_TIME== 3
#define DELAY_OW_RECOVERY_TIME()     { __asm__ volatile("nop"); __asm__ volatile("rjmp _PC_+0"); }

#elif OW_RECOVERY_TIME== 4
#define DELAY_OW_RECOVERY_TIME()     {  __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); }

#elif OW_RECOVERY_TIME== 5
#define DELAY_OW_RECOVERY_TIME()     {  __asm__ volatile("nop");         __asm__ volatile("rjmp _PC_+0"); \
                                        __asm__ volatile("rjmp _PC_+0");                                  \
                                     }
#elif OW_RECOVERY_TIME == 6
#define DELAY_OW_RECOVERY_TIME()     {  __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); \
                                        __asm__ volatile("rjmp _PC_+0");                                  \
                                     }
#elif OW_RECOVERY_TIME == 7
#define DELAY_OW_RECOVERY_TIME()     {  __asm__ volatile("nop");         __asm__ volatile("rjmp _PC_+0"); \
                                        __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); \
                                     }
#elif OW_RECOVERY_TIME == 8
#define DELAY_OW_RECOVERY_TIME()     {  __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); \
                                        __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); \
                                     }
#elif OW_RECOVERY_TIME == 9
#define DELAY_OW_RECOVERY_TIME()     {  __asm__ volatile("nop");         __asm__ volatile("rjmp _PC_+0"); \
                                        __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); \
                                        __asm__ volatile("rjmp _PC_+0");                                  \
                                     }
#elif OW_RECOVERY_TIME == 10
#define DELAY_OW_RECOVERY_TIME()     {  __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); \
                                        __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); \
                                        __asm__ volatile("rjmp _PC_+0");                                  \
                                     }
#elif OW_RECOVERY_TIME == 11
#define DELAY_OW_RECOVERY_TIME()     {  __asm__ volatile("nop");         __asm__ volatile("rjmp _PC_+0"); \
                                        __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); \
                                        __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); \
                                     }
#elif OW_RECOVERY_TIME == 12
#define DELAY_OW_RECOVERY_TIME()     {  __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); \
                                        __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); \
                                        __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); \
                                     }
#elif OW_RECOVERY_TIME == 13
#define DELAY_OW_RECOVERY_TIME()     {  __asm__ volatile("nop");         __asm__ volatile("rjmp _PC_+0"); \
                                        __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); \
                                        __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); \
                                        __asm__ volatile("rjmp _PC_+0");                                  \
                                     }
#elif OW_RECOVERY_TIME == 14
#define DELAY_OW_RECOVERY_TIME()     {  __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); \
                                        __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); \
                                        __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); \
                                        __asm__ volatile("rjmp _PC_+0");                                  \
                                     }
#elif OW_RECOVERY_TIME == 15
#define DELAY_OW_RECOVERY_TIME()     {  __asm__ volatile("nop");         __asm__ volatile("rjmp _PC_+0"); \
                                        __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); \
                                        __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); \
                                        __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); \
                                     }
#elif OW_RECOVERY_TIME == 16
#define DELAY_OW_RECOVERY_TIME()     {  __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); \
                                        __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); \
                                        __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); \
                                        __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); \
                                     }
#elif OW_RECOVERY_TIME == 17
#define DELAY_OW_RECOVERY_TIME()     {  __asm__ volatile("nop");         __asm__ volatile("rjmp _PC_+0"); \
                                        __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); \
                                        __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); \
                                        __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); \
                                        __asm__ volatile("rjmp _PC_+0");                                  \
                                     }
#elif OW_RECOVERY_TIME == 18
#define DELAY_OW_RECOVERY_TIME()     {  __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); \
                                        __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); \
                                        __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); \
                                        __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); \
                                        __asm__ volatile("rjmp _PC_+0");                                  \
                                     }
#elif OW_RECOVERY_TIME == 19
#define DELAY_OW_RECOVERY_TIME()     {  __asm__ volatile("nop");         __asm__ volatile("rjmp _PC_+0"); \
                                        __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); \
                                        __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); \
                                        __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); \
                                        __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); \
                                     }
#elif OW_RECOVERY_TIME == 20
#define DELAY_OW_RECOVERY_TIME()     {  __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); \
                                        __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); \
                                        __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); \
                                        __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); \
                                        __asm__ volatile("rjmp _PC_+0"); __asm__ volatile("rjmp _PC_+0"); \
                                     }


#elif  OW_RECOVERY_TIME >= 21

#define  OW_RECOVERY_TIME_REAL  ((OW_RECOVERY_TIME/4)*4)

#if  OW_RECOVERY_TIME - OW_RECOVERY_TIME_REAL == 0

#define DELAY_OW_RECOVERY_TIME()     DELAY_S_ACCURATE( ((OW_RECOVERY_TIME-1)/4) )

#elif OW_RECOVERY_TIME - OW_RECOVERY_TIME_REAL == 1

#define DELAY_OW_RECOVERY_TIME()     { DELAY_S_ACCURATE( ((OW_RECOVERY_TIME-1)/4) ); \
                                       __asm__ volatile("nop");                      \
                                     } 

#elif OW_RECOVERY_TIME - OW_RECOVERY_TIME_REAL == 2

#define DELAY_OW_RECOVERY_TIME()     { DELAY_S_ACCURATE( ((OW_RECOVERY_TIME-1)/4) ); \
                                       __asm__ volatile("rjmp _PC_+0");              \
                                     }
#elif OW_RECOVERY_TIME - OW_RECOVERY_TIME_REAL == 3

#define DELAY_OW_RECOVERY_TIME()     { DELAY_S_ACCURATE( ((OW_RECOVERY_TIME-1)/4) ); \
                                       __asm__ volatile("rjmp _PC_+0");              \
                                       __asm__ volatile("nop");                      \
                                     } 

#endif /* #if  OW_RECOVERY_TIME - OW_RECOVERY_TIME_REAL == 0 */



#endif  /* #if  OW_RECOVERY_TIME<= 0 -> #elif  OW_RECOVERY_TIME >= 21 */


/*
#define ow_read_bit()   ({ unsigned char x=0;                                  \
                           ONE_WIRE_DQ_0();                                    \
                           DELAY_OW_RECOVERY_TIME();                           \
                           ONE_WIRE_DQ_1();                                    \
                           DELAY_US(13);                                       \
                           if(bit_is_set(ONE_WIRE_PIN_REG, ONE_WIRE_PIN)) x=1; \
                           DELAY_US(ONE_WIRE_READ_SLOT_TIME_US);               \
                           x;                                                  \
                        }) 

#define ow_write_bit(x) ({   if(x)                                             \
                              {                                                \
                                 ONE_WIRE_DQ_0();                              \
                                 DELAY_OW_RECOVERY_TIME();                     \
                                 ONE_WIRE_DQ_1();                              \
                                 DELAY_US(ONE_WIRE_WRITE_SLOT_TIME_US);        \
                              }                                                \
                             else{                                             \
                                    ONE_WIRE_DQ_0();                           \
                                    DELAY_US(ONE_WIRE_WRITE_SLOT_TIME_US);     \
                                    ONE_WIRE_DQ_1();                           \
                                    DELAY_OW_RECOVERY_TIME();                  \
                                 }                                             \
                        })

*/
#endif //OWIRE_H
