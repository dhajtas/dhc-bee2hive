//	RTC routines bazed on hw.h //

#include <avr/interrupt.h>
//test
#include <avr/io.h>
#include "spi.h"
//endtest
#include <inttypes.h>
#include <stdio.h>

#include "hw.h"
#include "rtc.h"


DATE_t ADate[2] __attribute__ ((section (".noinit")));
DATE_t Date __attribute__ ((section (".noinit")));
TIME_t ATime[2] __attribute__ ((section (".noinit")));
TIME_t Time __attribute__ ((section (".noinit")));

//----------------------------------------------------------------------
//-------------------- Routines ----------------------------------------
//----------------------------------------------------------------------

ISR(RTC_INT)
{
	Status |= RTC_UPDATE;
//	PORTD.OUTTGL = _BV(LED0);
	
//	sei();
}


void RTC_DateTime(void)
{
//	Time.s++;					// on xmega interrupt each minute...
//	if(Time.s==60)
//	{
//		Time.s = 0;
		Time.m++;
		if(Time.m==60)
		{
			Time.m = 0;
			Time.h++;
			if(Time.h==24)
			{
				Time.h = 0;
				Date.d++;
				switch(Date.m)
				{
					case	2:	if(Date.d>28)
								{
									if(Date.y%4)
									{
										Date.d = 1;
										Date.m++;
									}
									else if(Date.d==30)
									{
										Date.d = 1;
										Date.m++;
									}
									
								}
								break;
					case	4:
					case	6:
					case	9:
					case	11:	if(Date.d==31)
								{
									Date.d = 1;
									Date.m++;
								}
								break;
					default	:	if(Date.d==32)
								{
									Date.d = 1;
									Date.m++;
								}
				}
				if(Date.m==13)
				{
					Date.m = 1;
					Date.y++;
				}
			}
		}
//	}
}

void RTC_Init(void)
{
	OSC.CTRL |= OSC_RC32KEN_bm;				// enable internal 32kHz oscillator
	while(OSC.STATUS & OSC_RC32KRDY_bm);	// wait until stable operation of the 32kHz internal oscillator
	CLK.RTCCTRL = CLK_RTCSRC_RCOSC_gc | CLK_RTCEN_bm;		// prescaler on internal 32kHz oscillator as RTC clk source
	
	RTC.PER = 60;							// count 60 seconds ???
	RTC.CTRL = RTC_PRESCALER_DIV1024_gc;	// start the RTC at 1Hz clk

	while(RTC.STATUS);						// wait for update

	RTC.INTCTRL = PMIC_INTLVL_LO_gc;		// overflow interrupt will be set by periode compare
	PMIC.CTRL |= PMIC_LOLVLEN_bm;			// enable low level interrupts
}

uint8_t RTC_CmpTime(TIME_t *time1, TIME_t *time2)
{
	if(time1->h == time2->h)
	{
		if(time1->m == time2->m)
		{
//			if(time1->s == time2->s)
//			{
//				return(1);
//			}
//			else
//			{
//				if(time1->s > time2->s)
//				{
//					return(2);
//				}
//				else
//				{
//					return(0);
//				}
//			}
			return(1);
		}
		else
		{
			if(time1->m > time2->m)
				return(2);
			else
				return(0);
		}
	}
	
	if(time1->h > time2->h)
		return(2);
	else
		return(0);
}

DATE_t* RTC_GetDate(void)
{
	return(&Date);
}

TIME_t* RTC_GetTime(void)
{
	Time.s = RTC.CNTL;
	return(&Time);
}

void RTC_SetDateTime(DATE_t *date, TIME_t *time)
{
	Time = *time;
	Date = *date;
	RTC.CNTL = time->s;
	return;
}
