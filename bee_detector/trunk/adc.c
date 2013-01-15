#include <inttypes.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#include "hw.h"
#include "adc.h"
#include "ffft.h"

volatile uint8_t ADC_Status;
volatile uint16_t ADC_Cal[12];
volatile uint8_t ADC_CH0_PIN, ADC_CH1_PIN;

/*---------------------------------------------------------------------------*/
/*		ADC interrup routine												 */
/*---------------------------------------------------------------------------*/

//#if ADC_INT_ENABLE == 1

//volatile int16_t ADC_Buffer[10];
//volatile uint16_t ADC_Index;
/*
ISR(ADC_INT0)
{
//	const prog_int16_t *window = tbl_window;
	static uint16_t adc_i = 0; 
	static uint8_t adc_deci = 0;
	int16_t v, vv;
//  urobit kalibraciu 0 ako jedno cele meranie a vypocitat priemernu hodnotu - ulozit do eeprom?	
//	v = ADCA.CH0RES - 0x0874;	// 0V = 0x0C8... 1/2 je o cca 200dec vyssie ako 0x7FF
	v = ADCA.CH0RES;			// 0V = 0x0C8... 1/2 je o cca 200dec vyssie ako 0x7FF
	Signal[0][adc_i] += v;
//	PORTD.OUTTGL = _BV(LED0);		//sampling frequency test
//	65kHz sampling frequency; with clk div by 2, sampling frequenci 32.4kHz...
	
//	vv = FFT_Fmuls_f(v, pgm_read_word(window + adc_i));
//	Bfly_buffer[adc_i].r = vv;
//	Bfly_buffer[adc_i].i = vv;
	adc_deci++;
	if(adc_deci == 4)
	{
		Signal[0][adc_i] >>= 1;		//Decimation by 2
		Signal[0][adc_i] -= ADC_Cal[ADC_CH0_PIN];
		adc_i++;
		adc_deci = 0;
	}
	
	if(adc_i == FFT_N)
	{
		adc_i = 0;
		ADC_Status |= ADC0_FINISHED;
//		ADCA.CTRLB &= ~ADC_FREERUN_bm;									 		//disable freerunning... (if in freerunning mode)
		ADCA.CH0.INTCTRL = ADC_CH_INTMODE_COMPLETE_gc | ADC_CH_INTLVL_OFF_gc;		//0xFC;  	//disable interrupt from CH0
	}
}
*/

ISR(ADC_INT1)
{
//	const prog_int16_t *window = tbl_window;
	static uint16_t adc_i = 0;
	static uint8_t adc_deci = 0;
	int16_t v, vv;
	//  urobit kalibraciu 0 ako jedno cele meranie a vypocitat priemernu hodnotu - ulozit do eeprom?
	//	v = ADCA.CH0RES - 0x0874;	// 0V = 0x0C8... 1/2 je o cca 200dec vyssie ako 0x7FF
	v = ADCA.CH0RES;
	vv = ADCA.CH1RES;				// 0V = 0x0C8... 1/2 je o cca 200dec vyssie ako 0x7FF
	Signal[0][adc_i] += v;
	Signal[1][adc_i] += vv;
	//	PORTD.OUTTGL = _BV(LED0);		//sampling frequency test
	//	65kHz sampling frequency; with clk div by 2, sampling frequenci 32.4kHz...
	
	//	vv = FFT_Fmuls_f(v, pgm_read_word(window + adc_i));
	//	Bfly_buffer[adc_i].r = vv;
	//	Bfly_buffer[adc_i].i = vv;
	adc_deci++;
	if(adc_deci == 4)
	{
		Signal[0][adc_i] >>= 1;		//Decimation by 2
		Signal[0][adc_i] -= ADC_Cal[ADC_CH0_PIN];
		Signal[1][adc_i] >>= 1;		//Decimation by 2
		Signal[1][adc_i] -= ADC_Cal[ADC_CH0_PIN+1];
		adc_i++;
		adc_deci = 0;
	}
	
	if(adc_i == FFT_N)
	{
		adc_i = 0;
		ADC_Status |= ADC1_FINISHED|ADC0_FINISHED;
		ADCA.CTRLB &= ~ADC_FREERUN_bm;									 		//disable freerunning... (if in freerunning mode)
		ADCA.CH1.INTCTRL = ADC_CH_INTMODE_COMPLETE_gc | ADC_CH_INTLVL_OFF_gc;		//0xFC;  	//disable interrupt from CH1
	}
}
//#endif //ADC_INT_ENABLE

void ADC_Init(void)
{
	uint8_t ii=0;
	ADC0_PORT.DIR = 0x00; 						// configure a2d port (PORTF) as input so we can receive analog signals
	ADC1_PORT.DIR = 0x00; 						// configure a2d port (PORTF) as input so we can receive analog signals
	PORTCFG.MPCMASK = 0xFF;						// copy settings to all pins
	ADC0_PORT.PIN0CTRL = 0x07; 					// make sure pull-up resistors are turned off, digital input buffer disabled

	for(ii=0;ii<12;ii++)						// clear zero point for all possible channels
	{
		ADC_Cal[ii] = 0;
	}

	ADCA.CTRLA = ADC_ENABLE_bm | ADC_FLUSH_bm;	// enable ADC, no DMA controll, no conversion start, flush pipeline 
	ADCA.CTRLB = ADC_IMPMODE_LOWIMP_gc | ADC_CURRLIMIT_NO_gc | ADC_RESOLUTION_12BIT_gc;		// freerunning at this time?
	ADCA.REFCTRL = ADC_REFSEL_VCC_gc | ADC_BANDGAP_bm;		// enable bandgap, reference = VCC/1.6
	ADCA.PRESCALER = ADC_PRESCALER_DIV256_gc;				// clk/512 - 31.25kSps @ 16MHz; clk/256 - 125kSps @ 32MHz, decimated by 2 - 13bit @ 31.25kSps
	ADCA.EVCTRL = ADC_SWEEP_01_gc;							// freerunning channels 0 1
//  ADCA.EVCTRL = 		//event control	
//  ADCA.EVCTRL = 		//event control	
//#if ADC_INT_ENABLE == 1
	ADCA.CH0.CTRL = ADC_CH_INPUTMODE_SINGLEENDED_gc;	// selects single-ended conversion
	ADCA.CH1.CTRL = ADC_CH_INPUTMODE_SINGLEENDED_gc;	// selects single-ended conversion
	ADC_chswitch(0,1);
//	ADCA.CH0.INTCTRL = ADC_CH_INTMODE_COMPLETE_gc | ADC_CH_INTLVL_HI_gc;		// enable interrupt & free running mode
// only interrupt on channel 1, channel 0 done 1 ADC clk before - given by design
	ADCA.CH1.INTCTRL = ADC_CH_INTMODE_COMPLETE_gc | ADC_CH_INTLVL_HI_gc;		// enable interrupt & free running mode
	PMIC.CTRL |= PMIC_HILVLEN_bm;			// enable low level interrupts

// 	Preparing for DMA transfers in future
//	DMA.CTRL = DMA_ENABLE_bm;
//	DMA.CTRL |= DMA_PRIMODE_CH0123_gc;
	
//	DMA.CH0.CTRL = DMA_CH_RESET_bm;
//	while (DMA.CH0.CTRL & DMA_CH_RESET_bm);

//	DMA.CH0.CTRLA = DMA_CH_SINGLE_bm | DMA_CH_BURSTLEN_2BYTE_gc;
//	DMA.CH0.CTRLB |= DMA_CH_TRNINTLVL_HI_gc;
//	DMA.CH0.ADDRCTRL = DMA_CH_SRCRELOAD_BURST_gc | DMA_CH_SRCDIR_INC_gc | DMA_CH_DESTRELOAD_TRANSACTION_gc | DMA_CH_DESTDIR_INC_gc;
//	DMA.CH0.TRIGSRC = DMA_CH_TRIGSRC_ADCA_CH0_gc;
//	DMA.CH0.SRCADR = &ADCA.CH0RESL;
//	DMA.CH0.DESTADR = &Signal[0][0];
		
//	DMA.CH0.CTRLA |= DMA_CH_ENABLE_bm;


	ADCA.CTRLB |= ADC_FREERUN_bm;							// start freerun - no start necessary???
//	ADCA.CTRLA |= ADC_CH0START_bm || ADC_CH1START_bm;					// first conversion needs to be started, free running continues...
//	ADCA.CH0.CTRL |= ADC_CH_START_bm;				// equivalent to previous one...
	ADC_Status = 0;
//#endif //ADC_INT_ENABLE	
}

uint8_t ADC_chswitch(uint8_t channel0, uint8_t channel1)
{
	ADC_CH0_PIN = channel0;
	ADC_CH1_PIN = channel1;
	if (ADC_CH0_PIN > 10)
		ADC_CH0_PIN = 0;
	if (ADC_CH1_PIN > 11)
		ADC_CH1_PIN = 1;
	ADCA.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN0_gc + (ADC_CH0_PIN<<3);
	ADCA.CH1.MUXCTRL = ADC_CH_MUXPOS_PIN0_gc + (ADC_CH1_PIN<<3);
	return(ADC_CH0_PIN | (ADC_CH1_PIN<<4));
}


ADC_CH_MUXPOS
uint16_t ADC_cal(uint8_t channel)
{
	uint16_t i;
	uint32_t cal = 0;
	
	for(i=0;i<FFT_N;i++)
	{
		cal += Signal[channel][i];
	}
	cal = cal/FFT_N;
	return((uint16_t)cal);
}

/*
#if ADC_INT_ENABLE == 0
#if ADC_CAPTURE_INPLACE == 1

void ADC_Capture (uint8_t channel, complex_t *bfly_buffer)
{
	uint16_t i;
	uint16_t v;
	const prog_int16_t *window = tbl_window;
	

	ADMUX = 0x40 + channel;				// selects single-ended conversion on pin PF0 ~ PF7 (ADC0 ~ ADC7)
										// selects AVCC as Vref
										// selects right adjust of ADC result
										
	ADCSRA |= _BV(ADFR);				// enable free running mode - fixed sample rate											
	
	for (i = 0; i < FFT_N; i++)		// averaging the ADC results
	{
//		ADCSRA |= _BV(ADSC); 			// start a conversion by writing a one to the ADSC bit (bit 6)

		while(!(ADCSRA & _BV(ADIF))); 	// wait for conversion to complete: ADIF = 1

		v = fmuls_f(ADC - 32768, pgm_read_word_near(window));

		bfly_buffer->r = v;
		bfly_buffer->i = v;
		bfly_buffer++;
		window++;
	}
	ADCSRA &= ~(_BV(ADFR));			//stop free running
}

#else
void ADC_Capture (uint8_t channel, uint16_t *voltage)
{
	uint16_t i;

	ADMUX = 0x40 + channel;				// selects single-ended conversion on pin PF0 ~ PF7 (ADC0 ~ ADC7)
										// selects AVCC as Vref
										// selects right adjust of ADC result

	ADCSRA |= _BV(ADFR);				// enable free running mode - fixed sample rate											
												
	for (i = 0; i < FFT_N; i++)		// averaging the ADC results
	{
//		ADCSRA |= _BV(ADSC); 			// start a conversion by writing a one to the ADSC bit (bit 6)
		while(!(ADCSRA & _BV(ADIF))); 	// wait for conversion to complete: ADIF = 1

		*(voltage++) = ADC;
	}
	ADCSRA &= ~(_BV(ADFR));			//stop free running

}
#endif //inplace
#endif //adc_int_enable
*/
