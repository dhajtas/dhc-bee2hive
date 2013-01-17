
#ifndef HW_H_
#define HW_H_

#ifndef GLUE
	#define GLUE(a, b)     a##b
	#define PORT(x)        GLUE(PORT, x)
	#define SPI(x)         GLUE(SPI, x)
	#define UART(x)        GLUE(UART, x)
#endif	//GLUE

#define AVR_ENTER_CRITICAL_REGION( ) uint8_t volatile saved_sreg = SREG; \
                                     cli();

#define AVR_LEAVE_CRITICAL_REGION( ) SREG = saved_sreg;


extern volatile uint8_t Status;
extern volatile uint8_t SD_Status;
//extern volatile uint8_t COM_Status;

#define BIGAVR				0
#define RSCOM				0

//Status

#define RTC_UPDATE			0x40
#define FFT_DONE			0x20
#define DATA_BANK			0x10
#define MEASUREMENT			0x02
#define MAIN_ERROR			0x01

//SD_Status
#define SD_ERROR			0x80
#define SD_WRITE_BREAK		0x20
#define SD_MEASUREMENT		0x10
#define SD_WRITE			0x08
#define SD_FS_READY			0x04
#define SD_READY			0x02
#define SD_PRESENT			0x01

#define SD_SPI_PORT			0
#define POWER_DOWN			0x04			//waiting for crd
#define POWER_ADC			0x02			//measuring batt
#define POWER_SAVE			0x06			//waiting for measurement start
#define POWER_EXTENDED		0x0E			//in regular measurement - maybe IDLE?
#define POWER_IDLE			0x00

#define PMIC_INTLVL_OFF_gc  0 
#define PMIC_INTLVL_LO_gc   1
#define PMIC_INTLVL_MED_gc  2
#define PMIC_INTLVL_HI_gc   3

#define CLK_RC32MHZ_gc		1

#define ADC0_P				A
#define ADC1_P				B
#define ADC_C				0
#define ADC_DIFF			0
#define ADC_INT_ENABLE		1
#define ADC_INT0			ADCA_CH0_vect
#define ADC_INT1			ADCA_CH1_vect
#define ADC_INT2			ADCA_CH2_vect
#define ADC_INT3			ADCA_CH3_vect

#define LED_PORT			PORTD
#define LED0				2
#define LED1				1
#define LED2				3

#define SWITCH_PORT			PORTR
#define DS0					PIN1
#define DS1					PIN0
#define SW0_INT				PORTR_INT0_vect

#define SD_CTRL_PORT		D					// SD card defs from mmcconf.h
#define SD_CS				PIN4
#define SD_CD				PIN0				// SD card detect - active low
#define SD_CD_INT			PORTD_INT0_vect

#define SPI0_P				D
#define SPI0_MOSI			PIN5
#define SPI0_MISO			PIN6
#define SPI0_SS				PIN4
#define SPI0_SCK			PIN7
#define SPI0_int			SPID_INT_vect

#define RTC_INT				RTC_OVF_vect 

#define RS485_POWER			PIN0
#define RS485_POWER_PORT	PORTE

		

//#define CYCLES_PER_US ((F_CPU+500000)/1000000) 	// cpu cycles per microsecond from global.h

#endif		// HW_H_