/*
 * bee_detector.c
 *
 * Created: 22. 12. 2012 22:39:34
 *  Author: dundee
 */ 

#define F_CPU	32000000

#include <stdio.h>
#include <inttypes.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/eeprom.h>

#include "include/hw.h"
#if RSCOM == 1
#include "include/uart.h"
#endif		//RSCOM
#include "include/adc.h"
#include "include/ffft.h"
#include "include/rtc.h"
#include "include/sd_routines.h"
#include "include/FAT32.h"
#include "include/spi.h"
#include "include/routines.h"
#include "include/sht1x.h"
#include "include/1wire.h"
#include "include/ds1820.h"

volatile uint8_t Status;
volatile uint8_t SD_Status;

uint16_t Spectrum[FFT_N/2];
complex_t Bfly_buffer[FFT_N];
int16_t Signal[2][FFT_N];
//uint16_t Mask_MIC, Mask_SHT, Mask_DS;
MASK_t Mask;
//volatile complex_t Bfly_buffer[FFT_N];
//volatile int16_t Signal[2][FFT_N];

uint8_t Resets __attribute__ ((section (".noinit")));
uint16_t EE_cal[12] EEMEM;
MASK_t EE_mask EEMEM;

void Debug_Init(void);
void CCPWrite(volatile uint8_t * address, uint8_t value);

int main(void)
{
 	uint8_t x, save_count = 0, error = 0, pin0, pin1;
 	uint8_t write_restart = 0;
	int16_t ow_temp = 0;
 	//	uint16_t i;

 	uint8_t filename[13];
 	DATE_t *sysdate;
 	TIME_t *systime;

 	Status = 0;
 	Resets++;
	Mask.MIC = 0;
	Mask.SHT = 0;
	Mask.DS  = 0;
	
	
 	CCPWrite(&PMIC.CTRL, 0x00);			// make sure IVSEL is zero - interrupts in application section
 	CCPWrite(&WDT.CTRL, 0x29);				// make sure WDT is disabled, timeout 8s (0x28+0x01(CEN))
 	CCPWrite(&WDT.WINCTRL, 0x29);			// make sure windowWDT is disabled, timeout 8s (0x28+0x01(CEN))

 	OSC.CTRL |= OSC_RC32MEN_bm;				// enable internal 32MHz oscillator
 	while(OSC.STATUS & OSC_RC32MRDY_bm);	// wait until stable operation
 	//	CCP = 0xD8;								//CPU_CCP_IOREG_gc;		//alow write to protectet register
 	//	CLK.CTRL = CLK_RC32MHZ_gc;				//change clock to 32MHz

 	CCPWrite( &CLK.CTRL, CLK_RC32MHZ_gc);
 //	CCPWrite( &CLK.PSCTRL, CLK_PSADIV_2_gc);

 	OSC.CTRL &= ~OSC_RC2MEN_bm;				// disable internal 2MHz oscillator
 	
 	Debug_Init();

 	SD_Init_hw();
 	LED_PORT.OUTSET = _BV(LED0); 	//switching ON the LED (for testing purpose only)
 	_delay_ms(500);
 	SPI_Init();
 	LED_PORT.OUTSET = _BV(LED1); 	//switching ON the LED (for testing purpose only)
 	_delay_ms(500);
 	RTC_Init();
 	LED_PORT.OUTSET = _BV(LED2); 	//switching ON the LED (for testing purpose only)
	 
	//start the DLL for stabilization of the main oscillator
	OSC.DFLLCTRL = 0x00;
	DFLLRC32M.CTRL = DFLL_ENABLE_bm;
	
 	_delay_ms(500);
 	sei();						// global interrupts enable
 	
 	sysdate = RTC_GetDate();
 	systime = RTC_GetTime();
 	LED_PORT.OUTCLR = _BV(LED0); 	//switching off the LED (for testing purpose only)

 	LED_PORT.OUTCLR = _BV(LED1); 	//switching ON the LED (for testing purpose only)
 	
 	for(x=0;x<FFT_N/2;x++)		//vymaz buffer pre spectrum
 	{
	 	Spectrum[x]=0;
 	}


 	ADC_Init();
	for (x=0;x<6;x++)
	{
		while(!(ADC_Status & ADC0_FINISHED));
		ADC_Cal[x*2] = ADC_cal(0);
		ADC_Status &= ~ADC0_FINISHED;
		ADC_Cal[x*2+1] = ADC_cal(1); 
		ADC_Status &= ~ADC1_FINISHED;
		while(!(ADCA.INTFLAGS));	//???					// test if pending conversion is finished...
		ADCA.INTFLAGS = 0xFF;								// clear any int flags
		ADC_chswitch(x+2,x+3);								//switch to other channel pair
		ADCA.CH1.INTCTRL = ADC_CH_INTMODE_COMPLETE_gc | ADC_CH_INTLVL_HI_gc;		// enable interrupt & free running mode
		ADCA.CTRLB |= ADC_FREERUN_bm;						//enable freerunning... (if in freerunning mode)
		pin0 = 0;
		pin1 = 1;
	}
	
	eeprom_write_block(ADC_Cal, EE_cal, 24);
	
			
 	LED_PORT.OUTCLR = _BV(LED2);
 	
 	x = 0;
 	
 	while(1)
 	{
	 	
	 	if(SD_DETECT_PORT.IN & _BV(SD_CD))					// ak bola vybrata karta
	 	{
		 	SD_Status &= SD_WRITE_BREAK;
		 	LED_PORT.OUTCLR = _BV(LED1);
		 	SD_DETECT_PORT.INT0MASK = _BV(SD_CD);			// INT0 pins setup
	 	}
	 	
	 	
	 	if(SD_Status == SD_PRESENT)
	 	{
		 	error=FAT32_InitFS();
		 	
		 	switch(error)
		 	{
			 	case 	0:
			 				if(readCfgFile(filename,0))
			 				{
#if RSCOM == 1
				 				printf_P(PSTR("NEW CFG LOADED"));
#endif
				 				Resets = 0;
			 				}
#if RSCOM == 1
			 				else
			 				{
				 				printf_P(PSTR("No config detected"));
				 				printf_P(PSTR("OLD CFG USED"));
			 				}
#endif
							SHT_init();
							if(Mask.DS)
								Mask.DS = GetDallasID(1);			// zapis do EEprom masku dallasov a dallasID /debug/
							eeprom_write_block(&Mask, &EE_mask, sizeof(MASK_t)); //zapis do EEprom vsetky masky /debug/
			 				LED_PORT.OUTSET = _BV(LED1);
			 				SD_Status |= SD_READY | SD_FS_READY;

			 				break;

			 	case	1:
#if RSCOM == 1
			 				printf_P(PSTR("No SD card detected"));
			 				printf_P(PSTR("OLD CFG USED"));
#endif
			 				SD_Status |= SD_ERROR;
			 				break;
			 	case	2:
#if RSCOM == 1
			 				printf_P(PSTR("No FAT32 detected"));
			 				printf_P(PSTR("OLD CFG USED"));
#endif
			 				SD_Status |= SD_READY;
			 	default:	break;
		 	}

	 	}
	 	
	 	if(SD_Status & SD_WRITE_BREAK)
	 	{
		 	if(SWITCH_PORT.IN & _BV(DS0))		// if switch released
		 	{
			 	SWITCH_PORT.INT0MASK = _BV(DS0);		// INT0 pins setup
			 	SD_Status &= ~SD_WRITE_BREAK;
			 	if(Status & MEASUREMENT)
			 	LED_PORT.OUTSET = _BV(LED2);
		 	}
		 	if(SD_Status & SD_MEASUREMENT)		// if active saving...
		 	{
			 	LED_PORT.OUTSET = _BV(LED0);
			 	SD_Status &= ~SD_MEASUREMENT;
			 	writeSpectrum(1,FFT_N/2,Spectrum,0);		//end of file
			 	closeFile(0);
			 	LED_PORT.OUTCLR = _BV(LED2)|_BV(LED0);
		 	}
	 	}

	 	
	 	if(ADC_Status & ADC0_FINISHED)	//merat kontinualne, kazdu minutu ulozit 2x12 merani (vsetky vstupy 2x) - pri 24h vydrzi karta 100dni
	 	{
		 	if(SD_Status & SD_WRITE)
		 	{
			 	if(!(SD_Status & SD_WRITE_BREAK))
			 	{
				 	if(!(SD_Status & SD_MEASUREMENT))
				 	{
						write_restart++;
					 	sprintf_P(filename,PSTR("B%02d%02d%02d.d%02d"),(sysdate->y - 20), sysdate->m, sysdate->d, write_restart);
					 	openFile(filename,0,0);
					 	SD_Status |= SD_MEASUREMENT;
				 	}
				 	if(save_count < 24)		//24 zapisov pri 128 prvkoch * 2B = 12 sektorov = necele 2 clustre (4 x 512 bytov chyba do 2 clustrov)
				 	{
					 	FFT_Input(&Signal[0][0], Bfly_buffer);
						FFT_Execute(Bfly_buffer);
		 				FFT_Output(Bfly_buffer,Spectrum);
						if(Mask.MIC & (0x0001 << pin0))
					 		writeSpectrum(0,FFT_N/2,Spectrum,0);
						else
							writeZeroes(0,FFT_N/2,0);
					 	save_count++;
						 
					 	FFT_Input(&Signal[1][0], Bfly_buffer);
					 	FFT_Execute(Bfly_buffer);
					 	FFT_Output(Bfly_buffer,Spectrum);
						if(Mask.MIC & (0x0001 << pin1))
							writeSpectrum(0,FFT_N/2,Spectrum,0);
						else
							writeZeroes(0,FFT_N/2,0);

					 	save_count++;
						pin0 = search_mask(pin1,Mask.MIC);
						pin1 = search_mask(pin0,Mask.MIC);
						ADC_chswitch(pin0,pin1);			
				 	}
				 	else
				 	{
						 if(Mask.SHT)
						 {
//							SHT_meas(SHT_MEAS_TEMP);						//meraj teplotu a vlhkost na SHT
//							SHT_meas(SHT_MEAS_HUM);
//							writeSpectrum(0,sizeof(SHT_t)*12,(uint16_t*)SHTbuff,0);	// uloz cely SHT buffer (60 bytov) (este ostava 1988 bytov do 2 clustrov )
						 }	
//						 else
						 	writeZeroes(0,sizeof(SHT_t)*12,0);						// uloz nuly ak nie je SHT (60 bytov) (este ostava 1988 bytov do 2 clustrov )				
						if(Mask.DS)
						{
							ConvertT(Mask.DS);
							_delay_ms(800);
							Read1820(Mask.DS);
							ow_temp = ReadTemp(0);						// only 1 dallas connected
							writeSpectrum(0,2,(uint16_t*)&ow_temp,0);					// uloz dallas teplotu (2 bytes)
						}	
						else
							writeZeroes(0,2,0);					// uloz dallas teplotu (2 bytes)				
												
						writeSpectrum(0,sizeof(MASK_t),(uint16_t*)&Mask,0);		// uloz masky (8 bytov);
						writeZeroes(0, FFT_N/2 - ((sizeof(SHT_t)*12)+sizeof(MASK_t)+2),0);	//dopln do nasobku dlzky spectra nejake data...
					 	save_count = 0;
					 	SD_Status &= ~SD_WRITE;
					 	LED_PORT.OUTCLR = _BV(LED0);					//led off
//						pin0 = search_mask(11,Mask_MIC);				//najdi prvy kanal na meranie - nasledujuca skupina merani zacina opat od zaciatku
//						pin1 = search_mask(pin0,Mask_MIC);				//najdi nasledujuci kanal na meranie
//						ADC_chswitch(pin0,pin1);						//prepni vstupy ADC
						ADC_chswitch(0,1);								//prepni vstupy ADC
						// meraj SHT a DS1820 (ukladaj vsetky, konverziu kontroluj podla masky...)
				 	}
			 	}
		 	}

		 	ADC_Status &= ~(ADC0_FINISHED|ADC1_FINISHED);
		 	
		 	//			ADCSRA |= _BV(ADFR)|_BV(ADIE);
		 	//			ADCSRA |= _BV(ADSC);
	 		ADCA.INTFLAGS = 0xFF;								// clear any int flags
//		 	ADCA.CH0.INTCTRL = ADC_CH_INTMODE_COMPLETE_gc | ADC_CH_INTLVL_HI_gc;
		 	ADCA.CH1.INTCTRL = ADC_CH_INTMODE_COMPLETE_gc | ADC_CH_INTLVL_HI_gc;	// INT len na kanale 1
		 	ADCA.CTRLB |= ADC_FREERUN_bm;									 		//enable freerunning... (if in freerunning mode)
//		 	ADCA.CH0.CTRL |= ADC_CH_START_bm;					// Start first conversion...
//			ADCA.CTRLA |= ADC_CH0START_bm || ADC_CH1START_bm;	// start the conversion
	 	}
	 	
	 	if(Status & RTC_UPDATE)
	 	{
		 	RTC_DateTime();
		 	Status &= ~RTC_UPDATE;
		 	if(Status & MEASUREMENT)
		 	{
			 	//				if(systime->s == 0)						//RTC_UPDATE is every minute - no need to check seconds, just measure & write
			 	//				{
				 	//					if(Time.m != old_m)
				 	//					{
					 	//						old_m = Time.m;
					 	if(SD_Status & SD_FS_READY)
					 	{
						 	SD_Status |= SD_WRITE;
						 	LED_PORT.OUTSET = _BV(LED0);		// Led on when writing
					 	}
				 	//					}
			 	//				}
			 	
			 	if(RTC_CmpTime(systime, &ATime[1]))
			 	{
				 	Status &= ~MEASUREMENT;
				 	if(SD_Status & SD_MEASUREMENT)
				 	{
					 	SD_Status &= ~SD_MEASUREMENT;
					 	writeSpectrum(1,FFT_N/2,Spectrum,0);		//end of file
					 	closeFile(0);
						write_restart = 0;
				 	}
				 	LED_PORT.OUTCLR = _BV(LED2);
			 	}
		 	}
		 	else
		 	{
			 	switch(RTC_CmpTime(systime, &ATime[0]))
			 	{
				 	case 2:										// ak je cas vacsi ako zaciatok merania
				 			if(RTC_CmpTime(systime, &ATime[1]))	// ak je cas vacsi ako koniec merania tak nic nerob
				 				break;
				 	case 1:										// ak je cas presne alebo ide zhora...
				 			if(SD_Status & SD_FS_READY)
				 			{
					 			sprintf_P(filename,PSTR("B%02d%02d%02d.d%02d"),(sysdate->y - 20), sysdate->m, sysdate->d, write_restart);
					 			openFile(filename,0,0);
					 			SD_Status |= SD_MEASUREMENT;
				 			}
				 			Status |= MEASUREMENT;
//tst							SHT_meas_dummy();
				 	//							old_m = Time.m;
				 			LED_PORT.OUTSET = _BV(LED2);
				 	case 0:	break;
			 	}
		 	}
	 	}
 	}

 	return(0);
}

void Debug_Init(void)
{
	LED_PORT.DIRSET = _BV(LED0)|_BV(LED1)|_BV(LED2);
	LED_PORT.OUTCLR = _BV(LED0)|_BV(LED1)|_BV(LED2);
	LED_PORT.OUTSET = _BV(LED0)|_BV(LED1)|_BV(LED2);
	_delay_ms(1000);
	LED_PORT.OUTCLR = _BV(LED0)|_BV(LED1)|_BV(LED2);
}

void CCPWrite( volatile uint8_t * address, uint8_t value )
{
	AVR_ENTER_CRITICAL_REGION( );
	volatile uint8_t * tmpAddr = address;
	#ifdef RAMPZ
	RAMPZ = 0;
	#endif
	asm volatile(
	"movw r30,  %0"	      "\n\t"
	"ldi  r16,  %2"	      "\n\t"
	"out   %3, r16"	      "\n\t"
	"st     Z,  %1"       "\n\t"
	:
	: "r" (tmpAddr), "r" (value), "M" (CCP_IOREG_gc), "i" (&CCP)
	: "r16", "r30", "r31"
	);

	AVR_LEAVE_CRITICAL_REGION( );
}