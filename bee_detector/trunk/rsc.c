#include <avr/io.h>
#include <inttypes.h>

#include "data_max.h"
#include "alarm.h"
#include "rsc.h"
#include "routines.h"
#include "1wire.h"
#include "myeeprom.h"
#include "init.h"
#include "at25256.h"

//-----------------------------------------------------------------------------------------------//
//		Global variables
//-----------------------------------------------------------------------------------------------//

//-----------------------------------------------------------------------------------------------//
//		routines
//-----------------------------------------------------------------------------------------------//

/*----------------------------------------------------------------------------------
	Services out of measurement
	
*/

void SC_outmeas(uint8_t service, uint8_t port)
{
 uint8_t pom, checksum;
	switch(service)
	{
	  	case 0x01:					// Clear Data
	  		ptrlast = ptrDS;
	  		ptrhead.adr = 0x0000;
	  		ptrhead.page = 0x00;
	  		checksum = Repeat(service,0x01,port);
	  		PCheck(checksum,port);
	  		break; 
	  	case 0x03:					//Set Identity
	  		sign.TYPE = Ser_com_rxb(port);
	  		sign.SN = Ser_com_rxb(port);
			checksum = Repeat(service,0x05,port);
			checksum += Ser_com_txb(sign.TYPE, port);
			checksum += Ser_com_txb(sign.SN, port);
			checksum += Ser_com_txb(sign.PRGM, port);
			checksum += Ser_com_txb(sign.VER, port);
			PCheck(checksum,port);
			break;
	  	case 0x08:					//Copy Memory
	  		pom = Ser_com_rxb(port);
	  		stat_STAT |= 0x02;			//			STAT.1 = 1
	  		if(pom == 1)
	  		 stat_STAT |= 0x04;
	  		else
	  		 stat_STAT &= 0xFB;
	  		checksum = Repeat(service,0x02,port);
	  		checksum += Ser_com_txb(pom, port);
	  		PCheck(checksum,port);
			break;
	  	case 0x09:					//CHECK PERIPHERALS
	  		InitPorts(); 				//"???????????"
			GetDallasID(0);				//1wire porty dallas inicializovane
			SHT_init();				// initialise SHT sensor
			Mem_Init();				//init seriovych eeprom
	  		checksum = Repeat(service,0x01,port);
			PCheck(checksum,port);
			break;
	  	case 0x18:					//GET DALLAS ID
	  		checksum = Repeat(service,133,port);
	  		checksum += GetDallasID(1);		//1wire porty dallas inicializovane		
			PCheck(checksum,port);
			break;  	
	  	case 0x22:					//Set Battery Alarms
			batt.LOW = Ser_com_rxw(port);
			batt.SAVE = Ser_com_rxw(port);
			batt.MIN = Ser_com_rxw(port);
			batt.DTSAVE = 0;
			mode |= 0x80;				//			mode.7 = 1
	  		checksum = Repeat(service,0x01,port);
			PCheck(checksum,port);
			break;
	  	case 0x24:					//Set DS
	  		Ser_com_rxptr(&ptrDS,port);
			if((ptrlast.page < ptrDS.page) && (ptrlast.adr < ptrDS.adr))
			 	ptrlast = ptrDS;
	  		checksum = Repeat(service,0x01,port);
			PCheck(checksum,port);
			break;
	  	case 0x26:					//Set Analogue outputs
	  		PWMbuff = Ser_com_rxw(port);		//??????
	  		checksum = Repeat(service,0x01,port);
			PCheck(checksum,port);
			break;
	  	case 0x27:					//Set Time Raster
	  		TR = Ser_com_rxb(port);
	  		checksum = Repeat(service,0x01,port);
			PCheck(checksum,port);
			InitRTC();
			break;
	  	case 0x2A:					//Set Measuring mode
	  		EMP = Ser_com_rxb(port);
	  		EMPR = Ser_com_rxw(port);
	  		pom = Ser_com_rxb(port);
	  		if(pom & 0x01)			//		pom.0?
	  			mode |= 0x80;		//		mode.7 = 1 povolene meranie baterii
	  		else
	  			mode &= ~0x80;		//		mode.7 = 0
	  		if(pom & 0x10)			//		pom.4?
	  			mode |= 0x40;		//		mode.6 = 1 povolene ukladanie PTR
	  		else
	  			mode &= ~0x40;		//		mode.6 = 0
	  		if(pom & 0x02)			//		pom.1?
	  			mode |= 0x20;		//		mode.5 = 1 povolene nabijanie
	  		else
	  			mode &= ~0x20;		//		mode.5 = 0
	  			
	  		checksum = Repeat(service,0x05,port);
			checksum += Ser_com_txb(EMP,port);
			checksum += Ser_com_txw(EMPR,port);
			checksum += Ser_com_txb(mode,port);
			PCheck(checksum,port);
			break;
		case 0x2B:					//
			mask.ADCM = Ser_com_rxb(port);
			mask.DIOM = Ser_com_rxw(port);
			mask.SHTM = Ser_com_rxw(port);
			InitOutPorts();
	  		checksum = Repeat(service,0x06,port);
			checksum += Ser_com_txb(mask.ADCM,port);
			checksum += Ser_com_txw(mask.DIOM,port);
			checksum += Ser_com_txw(mask.SHTM,port);
			PCheck(checksum,port);
			break;
		case 0x2C:					//
			Ser_com_rxptr(&ptrlast,port);
			Ser_com_rxptr(&ptrhead,port);
			checksum = Repeat(service,0x07,port);
			checksum += Ser_com_txptr(&ptrlast,port);
			checksum += Ser_com_txptr(&ptrhead,port);
			PCheck(checksum,port);
			break;
		case 0x2D:					//
			Autosend = Ser_com_rxb(port);
			checksum = Repeat(service,0x02,port);
			checksum += Ser_com_txb(Autosend,port);
			PCheck(checksum,port);
			break;
		default:
			break;
	}
}

//-------------------------------------------------------------------------------
// Services within the measurement


void SC_inmeas(uint8_t service, uint8_t port)
{
 uint8_t outcnt, pom, checksum, pomw;
 
	switch(service)
	{
	  	case 0x02:
	  		checksum = Repeat(service,0x05,port);
			checksum += Ser_com_txb(sign.TYPE,port);
			checksum += Ser_com_txb(sign.SN,port);
			checksum += Ser_com_txb(sign.PRGM,port);
			checksum += Ser_com_txb(sign.VER,port);
			PCheck(checksum,port);
			break;
	  	case 0x11:
	  		pom = Ser_com_rxb(port);
	  		checksum = Repeat(service,0x08,port);
	  		checksum += Ser_com_txb(pom,port);
	  		checksum += Ser_com_txw(alarm[pom].MASK,port);
	  		checksum += Ser_com_txw(alarm[pom].BOTTOM,port);
	  		checksum += Ser_com_txw(alarm[pom].TOP,port);
			PCheck(checksum,port);
			break;
	  	case 0x13:
	  		checksum = Repeat(service,0x05,port);
			checksum += Ser_com_txdw(DateTime,port);
			PCheck(checksum,port);
			break;		
	  	case 0x14:
	  		checksum = Repeat(service,0x04,port);
			checksum += Ser_com_txptr(&ptrDS,port);
			PCheck(checksum,port);
			break;
	  	case 0x15:
	  		pomw = (uint16_t)mask.ADCM;
#ifndef DS_3
	  		pomw &= 0xFF7F;			//PWM nebudem zahrnovat do vystupnych dat!
	  		pomw = pomw<<8;
#endif  //DS_3
	  		pomw = pomw | (uint16_t)mask.DS1820 | (uint16_t)mask.DS2450;
	  		outcnt = 0;
	  		for(pom=0;pom<D_NUM;pom++)
	  		{
	  			if(pomw & 0x0001)	//				pomw.0?
	  				outcnt++;
	  			pomw >>= 1;
	  		}
	  		outcnt = outcnt<<1;		//pom = pom*2
	  		outcnt += 3;
	  		if(mask.DIOM)
	  			outcnt += 2;
			if(mask.SHTM)
			{
				pomw = mask.SHTM;
				for(pom=0;pom<4;pom++)
				{
					if(pomw & 0x0001)	//				pomw.0?
						outcnt += 4;
					pomw >>= 4;
				}
			}
			checksum = Repeat(service,outcnt,port);
	  		pomw = (uint16_t)mask.ADCM;
#ifndef DS_3
	  		pomw &= 0xFF7F;			//???PWM nebudem zahrnovat do vystupnych dat!
	  		pomw = pomw<<8;
#endif  //DS_3
	  		pomw = pomw | (uint16_t)mask.DS1820 | (uint16_t)mask.DS2450 | (uint16_t)mask.DIOM | mask.SHTM;
	  		checksum += Ser_com_txw(pomw,port);
	  		checksum += StoreData(1,port);
	  		PCheck(checksum,port);
	  		break;
	  	case 0x17:
	  		checksum = Repeat(service,0x02,port);
			checksum += Ser_com_txb(TR,port);
			PCheck(checksum,port);
			break;
	  	case 0x19:
	  		checksum = Repeat(service,0x03,port);
	  		checksum += Ser_com_txw(stat_PIP,port);
	  		PCheck(checksum,port);
	  		break;
	  	case 0x1A:
			mode &= 0xFC;			//vynuluj LSB 2 bity
			if(stat_STAT & 0x01)		//				STAT.0?
	  			mode |= 0x01;		// mode.0 = stat.STAT.0
			if(stat_STAT & 0x20)		//				STAT.5?
				mode |= 0x02;		// mode.1 = stat.STAT.5
			if(Mem_Init() & 0xF0)
			 	mode |= 0x08;		//				mode.3 = 1
			else
			 	mode &= 0xF7;		//				mode.3 = 0

			pomw = BattMeas(1);
	  		checksum = Repeat(service,0x11,port);
	  		checksum += Ser_com_txb(mode,port);
	  		checksum += Ser_com_txptr(&ptrlast,port);
	  		checksum += Ser_com_txptr(&ptrhead,port);
	  		checksum += Ser_com_txb(EMP,port);
	  		checksum += Ser_com_txw(EMPR,port);
	  		checksum += Ser_com_txw(pomw,port);		//baterky
	  		checksum += Ser_com_txdw(batt.DTSAVE,port);
	  		PCheck(checksum,port);
	  		break;
	  	case 0x1B:				//get masks
	  		checksum = Repeat(service,0x0A,port);
	  		checksum += Ser_com_txb(mask.ADCM,port);
	  		checksum += Ser_com_txw(mask.DIOM,port);
	  		checksum += Ser_com_txw(mask.DS1820,port);
	  		checksum += Ser_com_txw(mask.DS2450,port);
	  		checksum += Ser_com_txw(mask.SHTM,port);
			PCheck(checksum,port);
	  		break;
	  	case 0x1D:
	  		checksum = Repeat(service,0x02,port);
			checksum += Ser_com_txb(Autosend,port);
			PCheck(checksum,port);
			break;
	  	case 0x21:
	  		pom = Ser_com_rxb(port);
	  		alarm[pom].MASK = Ser_com_rxb(port);
	  		alarm[pom].BOTTOM = Ser_com_rxb(port);
	  		alarm[pom].TOP = Ser_com_rxb(port);
			checksum = Repeat(service,0x01,port);
			PCheck(checksum,port);
			break;
	  	case 0x23:
			DateTime = Ser_com_rxdw(port);
	  		checksum = Repeat(service,0x01,port);
			PCheck(checksum,port);
			break;
	  	case 0x25:
	  		break;
	  	case 0x29:				// Shut-up
	  		stat_PIP = 0;
	  		stat_STAT &= 0xBF;	//					STAT.6 = 0
	  		checksum = Repeat(service,0x01,port);
			PCheck(checksum,port);
			break;
	  	case 0x31:				// Read Int. mem
			ReadMem(service,0,port);
			break;
	  	case 0x32:				// Read Ext. mem
			ReadMem(service,1,port);
			break;
	  	case 0x41:				// Write int. mem
			WriteMem(service,0,port);
			break;
	  	case 0x42:				// Write Ext mem
			WriteMem(service,1,port);
			break;
		case 0x57:				// Toogle Charging status after next measurement 
			stat_WD ^= 0x10;		// stat.WD.4 = 1/0
			stat_WD &= ~0x20;		// vypni fast charge (vzdy) Ak bude FCHRG povolene zapne sa samo za chvilu
			break;
		default:
			break;
	}
}

/*---------------------------------------------------------------------------------
	Bulk services within the measurement
*/

void Bulk_inmeas(uint8_t service, uint8_t port)
{
 uint8_t pom, checksum;
 
 	switch(service)
 	{
 		case 0x0A:			//validate config 0=restore config/1=save config
 			pom = Ser_com_rxb(port);
 			if(pom)
 			  	SaveConfig();
 			else
 			 	RestoreConfig();
 			checksum = Repeat(service,0x01,port);
 			PCheck(checksum,port);
 			break;
 		case 0x20:
 			SPEED = Ser_com_rxb(port);
 			Ser_com_Init(SPEED,port);		//hned aj preinicializovat com port
 			break;
 		case 0x2F:			//set localID to 01
 			LocalID = 0x01;
 			break;
 		case 0x50:
 			if(stat_RX & 0x20)	//if buffer empty			RX.5?
 			{
 				stat_STAT = stat_STAT & (~0x11);
			}
 			else
 			{
 				DTStop = Ser_com_rxdw();
 			 	stat_STAT |= 0x10;	//				STAT.4 = 1	
			}
			checksum = Repeat(service,0x01,port);
			PCheck(checksum,port);
			mode &= 0xFE;		//					mode.0 = 0
 			if(stat_STAT & 0x10)	//					STAT.4?
			 SaveMode();
 			else
 			 SaveConfig(); 			 
 			break;
 		case 0x52:
 			stat_STAT &= 0xDF;	//					STAT.5 = 0
 			checksum = Repeat(service,0x01,port);
 			PCheck(checksum,port);
 			break;
 		case 0x53:
 			stat_STAT |= 0x20;	//					STAT.5 = 1
 			if(!(stat_RX & 0x08))	//					!RX.3?
 			{
 			 checksum = Repeat(service,0x01,port);
 			 PCheck(checksum,port);
 			}
  			stat_RX &= 0xFE;	//po skonceni moze ist spat		RX.0 = 0
			break;
 		case 0x56:
 			stat_MEAS = 0x01;
 			break;
 		case 0xEE:
			break;
#ifndef DS_3
		case 0xF0:			//servisna rutina - nabijanie: data 1 byte
			checksum = Repeat(service,0x02,port);
 			switch(Ser_com_rxb(port))
 			{
 				case 0x00:
 					CHRGoff;
 					FCHRGoff;
 					stat_WD &= ~0x30;
 					Ser_com_txb(0x00,port);
 					break;
 				case 0x01:
 					CHRGon;
 					FCHRGoff;
 					stat_WD |= 0x10;
 					stat_WD &= ~0x20;
 					Ser_com_txb(0x01,port);
 					break;
 				case 0x02:
 					FCHRGon;
 					stat_WD |= 0x20;
 					Ser_com_txb(0x02,port);
 					break;
 				default:
 					Ser_com_txb(0xFF,port);
 					break;
 			}
 			PCheck(checksum,port);
			break;
#endif  //DS_3
		default:
			break;
 	}
}

/*-----------------------------------------------------------------------------
	Bulk services out of the measurement
*/

void Bulk_outmeas(uint8_t service, uint8_t port)
{
 uint8_t pom, pomb, checksum, testspeed;
 
  	switch(service)
 	{
 		case 0x04:
 			pom = Ser_com_rxb(port);
 			if(sign.TYPE == pom)
 			{
 			 pom = LocalID;
 			 LocalID = sign.SN;
 			 checksum = Repeat(service,0x02,port);
 			 checksum += Ser_com_txb(sign.SN,port);
 			 LocalID = pom;
 			 PCheck(checksum,port);
 			}
 			break;
 		case 0x0F:
 			pom = Ser_com_rxb(port);
 			if(port)
			{
 				Ser_com1_Init(pom);
				pomb = Comratetest1();
			}
			else
			{
			        Ser_com0_Init(pom);
				pomb = Comratetest0();
			}
			if(pomb)
				testspeed = pom;
			stat_MEAS &= 0xFB;		//To to nastavi!		MEAS.2 = 0
 			if(port)
   	 			Ser_com1_Init(SPEED1);
			else
				Ser_com0_Init(SPEED0);
			break;
 		case 0x10:
 		    checksum = Repeat(service,0x02,port);
 		    checksum += Ser_com_txb(testspeed,port);
 		    PCheck(checksum,port);
 		    break;
 		case 0x28:
 			pom = Ser_com_rxb(port);
 			if(sign.TYPE == pomb)
 			{
 				pom = Ser_com_rxb(port);
 			 	if(pom == sign.SN)
 			 	{
 			  		LocalID = Ser_com_rxb(port);
 			  		checksum = Repeat(service,0x01,port);
 			  		PCheck(checksum,port);
 			  	}	
			}
 			break;
  		case 0x51:
  		        if(port)
  		                pom=stat_RX1;
			else
			        pom=stat_RX0;
			        
 			if(pom & 0x20)			//if buffer empty		RX.5?
 			{
 				stat_STAT &= 0xF7;	//				STAT.3 = 0			
 				stat_STAT = stat_STAT | 0x81;
 				MP = 0;
 				MPR = 0;
 			}
 			else
 			{
 			 	DTStart = Ser_com_rxdw(port);
 			 	stat_STAT |= 0x08;	//				STAT.3 = 1
			}
			if(!(stat_STAT & 0x01))		//				!STAT.0?
				pom = 0x00;
			else
				pom = 0x01;
 			checksum = Repeat(service,0x02,port);
 			checksum += Ser_com_txb(pom,port);
 			PCheck(checksum,port);
 			mode |= 0x01;			//				mode.0 = 1
 			SaveMode();
 			break;
 		default:
 			break;
 	}
}

/*-----------------------------------------------------------------------------
	first part of the answering routine
*/

uint8_t Repeat(uint8_t service, uint8_t outcnt, uint8_t port)
{
 uint8_t checksum, pom;
 if(port==0)
 {
#ifndef DS_3
 	R485off;			//bez ohladu na napajanie vypni prijimac 485
#endif  //DS_3
	if(stat_RX & 0x08)		//						RX.3?
	{
	 	cbi(UCSR0B,RXEN);		//pocas bulk cakania na time slot vypni RX
	 	pom = SPEED>>1;
	 	pom++;
		my_delay(LocalID,pom);
	 	while(!(stat_TX & 0x10));	//					!TX.4?
	 	stat_TX &= 0xEF;		//					TX.4 = 0
	 	stat_MEAS &= 0xFB;		//pretoze v Txx dojde k nastaveniu!	MEAS.2 = 0
#ifdef DS_3
		T485;
#else
		if(!PWR)
	 		T485on;			//switch on TX driver on RS485 if external power
#endif  //DS_3
 	 	sbi(UCSR0B,RXEN);
 	 	Ser_com_txb(LocalID,port);
	 	return(0);
	}
 }
#ifdef DS_3
	T485;
#else
	if(!PWR)
 		T485on;			//switch on TX driver on RS485 if external power
#endif  //DS_3
	Ser_com_txb((uint8_t)0x55,port);
					//ak sa bude do check sum pocitat aj LocalID
	checksum = Ser_com_txb(LocalID,port);
	checksum += Ser_com_txb(outcnt,port);
	checksum += Ser_com_txb(service,port);
	return(checksum);
}

/*---------------------------------------------------------------------------------
	Finishing part of the answering routine
*/

void PCheck(uint8_t checksum, uint8_t port)
{
	checksum = ~checksum + 0x01;
    if(port)
    {
	Ser_com_txb(checksum,port);
	while(!(stat_TX1 & 0x04));	// !TX.2?
    }
    else
    {
	if(!(stat_RX0 & 0x08))		//ak bulk, tak preskoc rcall			!RX.3?
	 	Ser_com_txb(checksum,port);
	while(!(stat_TX0 & 0x04));	// !TX.2?
    }
#ifdef DS_3
	R485;
#else
	T485off;			//vypni vysielac 485
#endif  //DS_3
}

/*--------------------------------------------------------------------------------
	Receive the DWORD
*/

uint32_t Ser_com_rxdw(uint8_t port)
{
 uint32_t data;
 uint16_t pomw;
 
 	pomw = Ser_com_rxw(port);
 	data = (uint32_t)pomw;
 	pomw = Ser_com_rxw(port);
 	data = data | (((uint32_t)pomw)<<16);
 	return(data);
}

/*--------------------------------------------------------------------------------
	Receive the MEMPTR
*/


void Ser_com_rxptr(MEMPTR *data, uint8_t port)
{
  	(*data).adr = Ser_com_rxw(port);
 	(*data).page = Ser_com_rxb(port);
}

/*--------------------------------------------------------------------------------
	Receive the WORD
*/


uint16_t Ser_com_rxw(uint8_t port)
{
 uint16_t data;
 uint8_t pom;
 
 	pom = Ser_com_rxb(port);
 	data = (uint16_t)pom;
 	pom = Ser_com_rxb(port);
 	data = data | (((uint16_t)pom)<<8); 
 	return(data);
}

/*--------------------------------------------------------------------------------
	Receive the BYTE
*/

uint8_t Ser_com_rxb(uint8_t port)
{
        uint8_t data;
        
 	if(stat_RX0 & 0x20)		//						RX.5?
 	{
		stat_RX0 |= 0x80;	//error (buffer is empty but read requested)	RX.7 = 1
		data = 0x00;
	}
	else
	{
		data = uart0_buffer[uart0_head];
		uart0_head++;
		uart0_head &= 0x1F;		//rotuj buffer na 32 bytoch
		if(uart0_head == uart0_tail)	//ak sa head = tail -> buffer je prazdny
			stat_RX0 |= 0x20;	//					RX.5 = 1
	}
	return(data);
}

/*{
 uint8_t data;
    if(port==0)
    {
 	if(stat_RX0 & 0x20)		//						RX.5?
 	{
		stat_RX0 |= 0x80;	//error (buffer is empty but read requested)	RX.7 = 1
		data = 0x00;
	}
	else
	{
		data = uart0_buffer[uart0_head];
		uart0_head++;
		uart0_head &= 0x1F;		//rotuj buffer na 32 bytoch
		if(uart0_head == uart0_tail)	//ak sa head = tail -> buffer je prazdny
			stat_RX0 |= 0x20;	//					RX.5 = 1
	}
    }
    else
    {
 	if(stat_RX1 & 0x20)		//						RX.5?
 	{
		stat_RX1 |= 0x80;	//error (buffer is empty but read requested)	RX.7 = 1
		data = 0x00;
	}
	else
	{
		data = uart1_buffer[uart1_head];
		uart1_head++;
		uart1_head &= 0x1F;		//rotuj buffer na 32 bytoch
		if(uart1_head == uart1_tail)	//ak sa head = tail -> buffer je prazdny
			stat_RX1 |= 0x20;	//					RX.5 = 1
	}
    }

	return(data);
}*/

/*--------------------------------------------------------------------------------
	Send the DWORD
*/

uint8_t Ser_com_txdw(uint32_t data, uint8_t port)
{
 uint8_t checksum;
 
 	checksum = Ser_com_txw((uint16_t)data, port);		//posli dolne 2 byty cez 16bitovy send
 	checksum += Ser_com_txw((uint16_t)(data>>16), port);	//posli horne 2 byty cez 16 bitoby send
 	return(checksum);
}


/*--------------------------------------------------------------------------------
	Send the MEMPTR
*/

uint8_t Ser_com_txptr(MEMPTR *data, uint8_t port)
{
 uint8_t checksum;

	checksum = Ser_com_txw((*data).adr, port);		//posli adresu as word
	checksum += Ser_com_txb((*data).page, port);		//posli horny page as byte
	return(checksum);
}

/*--------------------------------------------------------------------------------
	Send the WORD
*/

uint8_t Ser_com_txw(uint16_t data, uint8_t port)
{
 uint8_t checksum;
 
	checksum = Ser_com_txb((uint8_t)data, port);		//posli spodny byte
	checksum += Ser_com_txb((uint8_t)(data>>8), port);	//posli horny byte
	return(checksum);
}

/*--------------------------------------------------------------------------------
	Send the BYTE
*/

uint8_t Ser_com_txb(uint8_t data, uint8_t port)
{
	while(!((stat_TX0 & 0x04)));	//pokial sa neskoncilo posielanie predchadzajuceho bytu, tak cakaj
//&(stat_TX1 & 0x04)					//						!TX.2?
	UDR0 = data;			//poloz data do registra
	UDR1 = data;
	stat_TX0 &= 0xFB;		//zrus priznak TX buffer prazdny		TX.2 = 0
//	stat_TX1 &= 0xFB;
}

/*{
   if(port==0)
   {
	while(!(stat_TX0 & 0x04));	//pokial sa neskoncilo posielanie predchadzajuceho bytu, tak cakaj
					//						!TX.2?
	UDR0 = data;			//poloz data do registra
	stat_TX0 &= 0xFB;		//zrus priznak TX buffer prazdny		TX.2 = 0
   }
   else
   {
	while(!(stat_TX1 & 0x04));	//pokial sa neskoncilo posielanie predchadzajuceho bytu, tak cakaj
					//						!TX.2?
	UDR1 = data;			//poloz data do registra
	stat_TX1 &= 0xFB;		//zrus priznak TX buffer prazdny		TX.2 = 0
   }

	return(data);			//vrat to co posielal pre potreby vypoctu checksumu
}*/
 

