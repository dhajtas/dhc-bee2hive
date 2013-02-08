#include <avr/io.h>
#include <inttypes.h>

#include "data_max.h"
#include "1wire.h"
#include "at25256.h"
#include "rsc0.h"
#include "routines.h"
#include "ds2450.h"

//-----------------------------------------------------------------------------------------------//
//		Global variables
//-----------------------------------------------------------------------------------------------//

OWIRE OWbuff[D_NUM];

//-----------------------------------------------------------
//-----------------------------------------------------------

port_width ow_Init(void)
{
 register port_width maskIn, maskOut;

	clr_owBuffer();
#ifdef DS_3
 	maskIn = ~(mask.ADCM | mask.SHTM);
#else
 	maskIn = ~(((uint16_t)mask.ADCM <<8) | mask.DIOM | mask.SHTM);
#endif  //DS_3
 	VOUTon;
 	maskOut = (ow_reset(maskIn) & maskIn);
 	ow_rstprt(maskOut);
 	VOUToff;
 	return(maskOut);
}

//-----------------------------------------------------------

void clr_owBuffer(void)
{
 uint8_t l,m;

 	for(l=0;l<D_NUM;l++)
 	{
 		for(m=0;m<8;m++)
 			OWbuff[l].data[m]=0x00;
 	}
}

//-----------------------------------------------------------

uint8_t GetDallasID(uint8_t j)
{
 MEMPTR ptr;
 uint8_t l, m, pom, checksum = 0, pokus = 0;
 port_width dallas,error;
					//??????'vysledkom je 128bytov v buffry (pole[16,8]), vzdy 8x1.byte,8x2.byte....
					//nevolat Write -> problem s ptrlast. radsej priamo volat Mem_WRITE a Mem_WAIT!
	dallas = ow_Init();			//mask.DS1820 | mask.DS2450;
	error = dallas;
	VOUTon;
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
	VOUToff;

	if(pokus == 5)	//ak skoncil citanie neuspesne ->chyba na linke v maske error
	{
	}

	for(l=0;l<D_NUM;l++)	//zistovanie typu dallasu podla ID
	{
		mask.DS1820 >>= 1;
		mask.DS2450 >>= 1;
		switch(OWbuff[l].data[0])
		{
			case 0x10:
				mask.DS1820 |= 0x8000;
				mask.DS2450 &= 0x7FFF;
				break;
			case 0x20:
				mask.DS1820 &= 0x7FFF;
				mask.DS2450 |= 0x8000;
				break;
			default:
				mask.DS1820 &= 0x7FFF;
				mask.DS2450 &= 0x7FFF;
				break;
		}
	}

	if(!j)				//ak ma zapisovat do pamate
	{
		ptr.adr = 0x002C;
		ptr.page = 0x00;
		for(l=0;l<32;l+=8)
		{
			pom = (uint8_t)(DateTime>>l);
			Mem_WRITE(pom,&ptr,0);
			Add_Adr(&ptr,1);
			Mem_WAIT();
		}
							//maska dallasov do pamate
		Mem_WRITE((uint8_t)dallas,&ptr,0);		//low byte
		Add_Adr(&ptr,1);
		Mem_WAIT();
		pom = (uint8_t)(dallas>>8);
		Mem_WRITE(pom,&ptr,0);		//high byte
		Add_Adr(&ptr,1);
		Mem_WAIT();
							//maska CRC chyb do pamate
		Mem_WRITE((uint8_t)error,&ptr,0);		//low byte
		Add_Adr(&ptr,1);
		Mem_WAIT();
		pom = (uint8_t)(error>>8);
		Mem_WRITE(pom,&ptr,0);		//high byte
		Add_Adr(&ptr,1);
		Mem_WAIT();

		for(l=0;l<D_NUM;l++)
		{
	 		for(m=0;m<8;m++)
	 		{
	 			Mem_WRITE(OWbuff[l].data[m],&ptr,0);
	 			Add_Adr(&ptr,1);
	 			Mem_WAIT();
	 		}
	 	}
	 	checksum = 0;
	}
	else
	{
	 	checksum = Ser_com_txw(dallas);		//posle masku najdenych dallasov
	 	checksum += Ser_com_txw(error);		//posle masku chyb pri komunikacii (CRC)
		for(l=0;l<D_NUM;l++)
		{
	 		for(m=0;m<8;m++)
	 		{
//	 			checksum += Write(OWbuff[l].data[m],j,0);	//naco?
	 			checksum += Ser_com_txb(OWbuff[l].data[m]);
	 		}
	 	}
	}
	if(mask.DS2450)
	{
		InitDS2450(mask.DS2450);
	}
	return(checksum);
}

//-----------------------------------------------------------

void owire(uint8_t command, port_width dallas)		//, OWIRE buffer[16], uint8_t size, uint8_t power)
{
// register uint8_t l;			//owire len zabezpecuje zaciatok komunikacie...

	ow_reset(dallas);
	if(command != 0x33)
		ow_outp(0xCC,dallas);
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

void ow_outp(register uint8_t data, register port_width dallas)
{
  register uint8_t l;

	for(l=0;l<8;l++)
	{
		ow_sendbit(dallas,data);
		data = data>>1;
	}
}

//-----------------------------------------------------------

void ow_inp(OWIRE *buffer,register uint8_t x, register port_width dallas)
{
 register port_width data;
 register uint8_t l,m;

	for(l=0;l<8;l++)
	{
		data = ow_recbit(dallas);
		data = data & dallas;
		for(m=D_NUM;m>0;m--)
		{
			buffer[m-1].data[x] >>=1;				//priprava dat v buffri na prichod nasl.bitu
//			buffer[m].data[x] &=0xFE;				//vynuluj nulty bit (co ak bol 1?)
#ifdef DS_3
			buffer[m-1].data[x] |= data & 0x80;			//pripisanie nulteho bitu (prave zapisovany dallas)
#else
			buffer[m-1].data[x] |= (uint8_t)(data>>8) & 0x80;	//pripisanie nulteho bitu (prave zapisovany dallas)
#endif  //DS_3
			data <<=1;						//posun prijatych dat o 1 do ????prava???? do lava??? -> nasl. dallas
		}
	}
}

//-----------------------------------------------------------

uint8_t ow_inp_1(port_width dallas)
{
 port_width pom;
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

port_width CheckCRC_8(OWIRE *buffer, port_width dallas, uint8_t count)	//uint8_t base, na zaciatok listu ak spolocne aj pre CRC_16
{
 uint8_t CRCbuf;	//ak bude spolocny check aj pre CRC_16 tak uint16_t
 port_width error = 0;
 uint8_t l,m,data;

 	for(m=0;m<D_NUM;m++)
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
#ifdef DS_3
 			        error |= 0x80;
#else
 				error |= 0x8000;
#endif  //DS_3
 		}
 		dallas >>= 1;
 	}
 	return(error);
}

//-----------------------------------------------------------

uint8_t CRC_8(uint8_t CRC, uint8_t x)
{
 uint8_t l;

 	for(l=0;l<8;l++)
 	{
 		if((x^CRC) & 0x01)
 		{
			CRC ^= 0x18;
			CRC >>= 1;
			CRC |= 0x80;
		}
		else
			CRC >>= 1;
		x >>= 1;
	}
	return(CRC);
}

//-----------------------------------------------------------

uint16_t CRC_16(uint16_t CRC, uint8_t x)
{
 uint16_t l;

 	for(l=0;l<8;l++)
 	{
 		if((x^CRC) & 0x0001)
 		{
			CRC ^= 0x4002;
			CRC >>= 1;
			CRC |= 0x8000;
		}
		else
			CRC >>= 1;
		x >>= 1;

 	}
	return(CRC);
}

//-----------------------------------------------------------

void delay_us(register uint16_t time)
{
	DELAY_US(time);
}

#ifdef DS_3
#include "1wire_03.c"
#else
#include "1wire_02.c"
#endif  //DS_3

