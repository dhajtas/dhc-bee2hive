#ifndef RTC_H_
#define RTC_H_

#define RTC_UPDATE		0x40

typedef struct TIME_struct
{
	uint8_t h;
	uint8_t m;
	uint8_t s;
} TIME_t;

typedef struct DATE_struct
{
	uint8_t d;
	uint8_t m;
	uint8_t y;
} DATE_t;


//extern DATE Date __attribute__ ((section (".noinit")));
//extern TIME Time __attribute__ ((section (".noinit")));

extern DATE_t ADate[2] __attribute__ ((section (".noinit")));
extern TIME_t ATime[2] __attribute__ ((section (".noinit")));

void RTC_Init(void);

void RTC_DateTime(void);

uint8_t RTC_CmpTime(TIME_t *time1, TIME_t *time2);

DATE_t* RTC_GetDate(void);

TIME_t* RTC_GetTime(void);

void RTC_SetDateTime(DATE_t *date, TIME_t *time);

#endif	//RTC_H_
