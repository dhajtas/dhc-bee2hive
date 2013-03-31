#include <avr/io.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <inttypes.h>

#include "include/hw.h"
#include "include/1wire.h"
//#include "routines.h"

#define		OW_PORT		PORT(I2C_P)

//-----------------------------------------------------------------------------------------------//
//		Global variables
//-----------------------------------------------------------------------------------------------//

OWIRE_t OWbuff[OW_NUM];
uint8_t EE_ow_mask EEMEM;
uint8_t EE_ow_error EEMEM;
OWIRE_t EE_ow_id[OW_NUM] EEMEM;
//-----------------------------------------------------------
//-----------------------------------------------------------

uint8_t ow_Init(void)
{
 register uint8_t maskIn, maskOut;

	clr_owBuffer();
 	maskIn = OW_bm;
 	maskOut = (ow_reset(maskIn) & maskIn);
// 	ow_rstprt(maskOut);		//pre xmega netreba ak wired and setting pouzite 
 	return(maskOut);
}

//-----------------------------------------------------------

void clr_owBuffer(void)
{
 uint8_t l,m;

 	for(l=0;l<OW_NUM;l++)
 	{
 		for(m=0;m<8;m++)
 			OWbuff[l].data[m]=0x00;
 	}
}

//-----------------------------------------------------------

uint8_t GetDallasID(uint8_t j)
{
 uint8_t l, pokus = 0;
 uint8_t dallas,error;
					//??????'vysledkom je 128bytov v buffry (pole[16,8]), vzdy 8x1.byte,8x2.byte....
					//nevolat Write -> problem s ptrlast. radsej priamo volat Mem_WRITE a Mem_WAIT!
	dallas = ow_Init();			//mask.DS1820 | mask.DS2450;
	error = dallas;
	do
	{
		owire(0x33,error);//OWbuff,0x08,0);	//read ROM, zapisuj do 1Wbuffra, citaj 9bytov, na konci vypni elektriku
		for(l = 0;l<8;l++)
		{
			ow_inp(OWbuff,l,error);		//zacinaj ukladat data vzdy od 1. dallasu byte "l"
		}
		ow_reset(error);
		error = CheckCRC_8(OWbuff,dallas,8); 	//checkCRC
		pokus++;
	}while(error && (pokus < 5));			//cyklus, kym error !=0 alebo try=3 (tri pokusy o spravne nacitanie)
 	ow_rstprt(dallas);

	if(pokus == 5)	//ak skoncil citanie neuspesne ->chyba na linke v maske error
	{
		return(0);
	}

	if(!j)				//ak ma zapisovat do pamate
	{
							//maska dallasov do pamate
		eeprom_write_byte(&EE_ow_mask, dallas);

							//maska CRC chyb do pamate
		eeprom_write_byte(&EE_ow_error,error);		//low byte

		for(l=0;l<OW_NUM;l++)
		{
			eeprom_write_block(&OWbuff[l], &EE_ow_id[l], 8);
	 	}
	}

	return(dallas);
}

//-----------------------------------------------------------

void owire(uint8_t command, uint8_t dallas)		//, OWIRE buffer[16], uint8_t size, uint8_t power)
{
// register uint8_t l;			//owire len zabezpecuje zaciatok komunikacie...

	ow_reset(dallas);
	ow_setprt(dallas);
	if(command != 0x33)
		ow_outp(0xCC,dallas); // skip ROM - only if single dallas is on line...
	ow_outp(command,dallas);
/*	for(l = 0;l<size;l++)
	{
		ow_inp(buffer,l);	//zacinaj ukladat data vzdy od 1. dallasu byte "l"
		//buffer+=16;		//toto by malo byt vyriesene pridanim pola
	}
	ow_reset(mask.OW);
	if(!power)
	{
	 	ow_rstprt(mask.OW);
	}
*/
}

//-----------------------------------------------------------

void ow_outp(register uint8_t data, register uint8_t dallas)
{
  register uint8_t l;

	for(l=0;l<8;l++)
	{
		ow_sendbit(dallas,data);
		data = data>>1;
	}
}

//-----------------------------------------------------------

void ow_inp(OWIRE_t *buffer,register uint8_t x, register uint8_t dallas)		// bude iba 1 dallas!!
{
 register uint8_t data;
 register uint8_t l,m;

	for(l=0;l<8;l++)
	{
		data = ow_recbit(dallas);
		data = data & dallas;
		for(m=OW_NUM;m>0;m--)
		{
			buffer[m-1].data[x] >>=1;				//priprava dat v buffri na prichod nasl.bitu
			buffer[m-1].data[x] &=0xFE;				//vynuluj nulty bit (co ak bol 1?)
			if(data)
				buffer[0].data[x] |= 0x80;			//pripisanie nulteho bitu (prave zapisovany dallas)

			data <<=1;						//posun prijatych dat o 1 do ????prava???? do lava??? -> nasl. dallas
		}
	}
}

//-----------------------------------------------------------

uint8_t ow_inp_1(uint8_t dallas)
{
 uint8_t pom;
 uint8_t l, data = 0;

	for(l=0;l<8;l++)
	{
		data >>=1;				//posun prijatych dat o 1 do prava -> nasl. dallas
		pom = ow_recbit(dallas) & dallas;
		if(pom)
			data |= 0x80;
	}
	return(data);
}

//-----------------------------------------------------------

uint8_t CheckCRC_8(OWIRE_t *buffer, uint8_t dallas, uint8_t count)	//uint8_t base, na zaciatok listu ak spolocne aj pre CRC_16
{
 uint8_t CRCbuf;	//ak bude spolocny check aj pre CRC_16 tak uint16_t
 uint8_t error = 0;
 uint8_t l,m,data;

 	for(m=0;m<OW_NUM;m++)
 	{
 		error >>= 1;
 		if(dallas & 0x01)
 		{
 			CRCbuf = 0;
// 			if(base == 8)
// 			{
 				for(l=0;l<count;l++)
 				{
 					data = buffer[m].data[l];
 					//(uint8_t)
					CRCbuf = CRC_8((uint8_t)CRCbuf,(uint8_t)data);
 				}
// 			}
// 			else
// 			{
// 				for(l=0;l<count,l++)
// 				{
// 					data = buffer[m].data[l];
//					CRCbuf = CRC_16(CRCbuf,data);
// 				}
// 			}
 			if(CRCbuf)

 			        error |= 0x80;

 		}
 		dallas >>= 1;
 	}
 	return(error);
}

//-----------------------------------------------------------

uint8_t CRC_8(uint8_t crc, uint8_t x)
{
 uint8_t l;

 	for(l=0;l<8;l++)
 	{
 		if((x^crc) & 0x01)
 		{
			crc ^= 0x18;
			crc >>= 1;
			crc |= 0x80;
		}
		else
			crc >>= 1;
		x >>= 1;
	}
	return(crc);
}

//-----------------------------------------------------------

uint16_t CRC_16(uint16_t crc, uint8_t x)
{
 uint16_t l;

 	for(l=0;l<8;l++)
 	{
 		if((x^crc) & 0x0001)
 		{
			crc ^= 0x4002;
			crc >>= 1;
			crc |= 0x8000;
		}
		else
			crc >>= 1;
		x >>= 1;

 	}
	return(crc);
}

//-----------------------------------------------------------

//void delay_us(register uint16_t time)
//{
//	DELAY_US(time);
//}

//-----------------------------------------------------------

void ow_sendbit(register uint8_t dallas,register uint8_t data)
{

	OW_PORT.OUTCLR = dallas;			//clear 1w lines low byte

	cli();

	//	DELAY_OW_RECOVERY_TIME();		//pre 8535
	_delay_us(ONE_WIRE_RECOVERY_TIME_US);	//pre mega128

	if(data & 0x01)				//				!data.0
	{
		OW_PORT.OUTSET = dallas;			//if send 1, set the lines
	}

	_delay_us(ONE_WIRE_WRITE_SLOT_TIME_US);
	
	sei();
	OW_PORT.OUTSET = dallas;				// set the 1W lines
}

//-----------------------------------------------------------

uint8_t ow_recbit(register uint8_t dallas)
{
	register uint8_t pom1;
	
	cli();					//disable interrupts (time critical)
	
	OW_PORT.OUTCLR = dallas;			//clear 1w lines low byte
	
	//	DELAY_OW_RECOVERY_TIME();		//pre 8535
	_delay_us(ONE_WIRE_RECOVERY_TIME_US);	//pre mega128

	OW_PORT.OUTSET = dallas;				//active pull-up najprv vnutit tvrdu jeddnotku ako vystup
	OW_PORT.DIRCLR = dallas;				//a potom prepnut na vstup s pull-up odporom - mozno bude potrebne pre zrychlenie hrany (+ pred zaciatkom 1w komunikacie zmenit nastavenie daneho pinu na totem pole + pullup)
	
	_delay_us(13);
	
	pom1 = OW_PORT.IN;  			//low byte
	sei();
	_delay_us(ONE_WIRE_READ_SLOT_TIME_US);

	OW_PORT.DIRSET = dallas;				//return ports to init state for 1w comm (outpu, H on pin)
	return(pom1);	//
}

//-----------------------------------------------------------

uint8_t ow_reset(register uint8_t dallas)
{

	OW_PORT.OUTSET = dallas;		//set 1w lines low byte
	_delay_us(250);				//na odstranenie dummy resetu (pred tymto boli draty na 0 takze hned pride pulz a potom sa uz nic nedeje :-( )

	OW_PORT.OUTCLR = dallas;			//clear 1w lines low byte
	_delay_us(550);

	cli();							//disable interrupts
	OW_PORT.OUTSET = dallas;			//set 1w lines low byte (wired AND and passive pull-up)
	_delay_us(70);

	dallas = OW_PORT.IN;				//citaj presence pulse -> 0 znamena pritomny
	_delay_us(300);
	
	dallas = dallas | ~(OW_PORT.IN);			//ak tam skutocne je, 0 musi po case zmiznut.
	sei();			//enable interrupts
	_delay_us(250);

	return(~dallas);			//inverted result is in zl
}

//-----------------------------------------------------------

void ow_setprt(register uint8_t dallas)
{
	PORTCFG.MPCMASK = dallas;
	OW_PORT.PIN0CTRL = PORT_OPC_PULLUP_gc;	// wired-and and pull-up set for all SDA lines (should be output???)
	OW_PORT.DIRSET = dallas;		//wired AND output + pullup... len pre xmega
	OW_PORT.OUTSET = dallas;	
	return;
}

void ow_rstprt(register uint8_t dallas)
{
	PORTCFG.MPCMASK = dallas;
	OW_PORT.PIN0CTRL = PORT_OPC_WIREDANDPULL_gc;	// wired-and and pull-up set for all SDA lines (should be output???)
	OW_PORT.DIRSET = dallas;		//wired AND output + pullup... len pre xmega
	OW_PORT.OUTSET = dallas;
	return;
}
