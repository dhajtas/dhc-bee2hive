
#include <avr/io.h>
#include <inttypes.h>

#include "data_max.h"
#include "1wire.h"
#include "ds1820.h"


void ConvertT(port_width dallas)		//zaciatok merania pre DS1820
{
	owire(0x44,dallas);		//start measurement,1Wbuff ptr, read 0 bytes, do not switch off power after finish)
	ow_reset(dallas);
}

//----------------------------------------------------------
//	Read the data from 1W devices

port_width Read1820(port_width error)
{
 uint8_t l, m, pokus=0;
 port_width errormask = (port_width)0x0001;

//???;*'vysledkom je 16 bytov v buffroch: 8x1.byte+8x2.byte...
	do
	{
		owire(0xBE,error);		//read scratch, save it to 1Wbuff, read 9 bytes, switch off power after finish)
		for(l = 0;l<8;l++)
		{
			ow_inp(OWbuff,l,error);	//zacinaj ukladat data vzdy od 1. dallasu byte "l"
		}
		ow_reset(error);
		error = CheckCRC_8(OWbuff,error,8); 	//checkCRC
		pokus++;
	}while(error && (pokus < 5));			//cyklus, kym error !=0 alebo try=3 (tri pokusy o spravne nacitanie)
  	ow_rstprt(error);
  	
  	if(error)		//ak bola chyba v comm, tak data pri chybe = 0xFF
  	{
  		for(l=0;l<D_NUM;l++)
  		{
  			if(error & errormask)
  			{
  				for(m=0;m<8;m++)
  					OWbuff[l].data[m] = 0xFF;
  			}
  			errormask<<=1;	
  		}
  	}
  	return(error);
}
//--------------------------------------------------------------


uint16_t ReadTemp(uint8_t number)
{
 register uint16_t pomw,rem = 0;
 register uint8_t count,pom;
 
	pomw = (uint16_t)OWbuff[number].data[0];
	pomw = pomw<<7;		//posun do horneho bytu, ale MSB bude znamienko a povodne LSB sa prepise (0.5 stupna)
	if(!OWbuff[number].data[1])
		pomw |= 0x8000;	//ak kladne tak 1, zaporne 0 (koli porovnavaniu velkosti v ramci unsigned!)	
				//							pomw.15 = 1
	rem = (uint16_t)OWbuff[number].data[6];
	rem = rem <<4;		//reminder * 16 - na zvacsenie presnosti prevodu
	count = OWbuff[number].data[7];
	if(count != 0xFF)
	{
		if(count == 0x00)
			pom = 0;	//ak je count 0 tak nedeli a vysledok je 0 -> error pri deleni
		else	
			pom = rem/count;
		pomw = pomw | (uint16_t)pom;
	}
	else
		pomw = 0xFFFF;		//ak bola chyba a data su 0xFF ta vysledok bude tiez 0xFFFF!
	return(pomw);
}

//-----------------------------------------------------------
