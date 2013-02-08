#include <avr/io.h>
#include <inttypes.h>

#include "data_max.h"
#include "alarm.h"
#include "rscom0.h"
#include "routines.h"
#include "1wire.h"
#include "myeeprom.h"
#include "init.h"
#include "at25256.h"

//-----------------------------------------------------------------------------------------------//
//		Global variables
//-----------------------------------------------------------------------------------------------//

uint8_t uart_buffer[32];
volatile uint8_t uart_head;
volatile uint8_t uart_tail;
uint8_t SPEED, BULK;

//-----------------------------------------------------------------------------------------------//
//		routines
//-----------------------------------------------------------------------------------------------//

//----------------------------------------------------
//Initialisation of the port0

void Ser_com_Init(uint8_t speed)
{
	cli();
	outp(0x00,UBRR0H);
	outp(speed,UBRR0L);
	outp(0xC0,UCSR0A);
	outp(0xD8,UCSR0B);	//B8; vymenil som utxcie za udrie enable TX, RX and ISRs in UART
	outp(0x06,UCSR0C);
	inp(UDR0); 		//vycitanim UDR zrusi pripadny int pre rx
	sei();
	stat_TX &= 0xC0;
	stat_TX |= 0x04;	//							TX.2 = 1
	Ser_com_Reinit();
}

//---------------------------------------------------
//reinitialisation ot the port0

void Ser_com_Reinit(void)
{
	uart_head = 0;
	uart_tail = 0;
	stat_RX = 0x21;		//0x21;set comm thread, RX buff empty, URX
	if(!PWR)		// Ak PWR = 0; zapni prijem na rs485?
	{
		R485on;		
		T485off;
	}
}

//---------------------------------------------------
//Services global
//---------------------------------------------------

void Ser_com(void)
{
 register uint8_t service;
 
	service = Ser_com_rxb(); 

	if(!(stat_RX & 0x08))			//if not bulk				!RX.3?
	{
	 	if(!(stat_STAT & 0x01))		//if not measuring			!STAT.0?
	 	{
	   		Bulk_outmeas(service);
	  		SC_outmeas(service);
	 	}
	 	Bulk_inmeas(service);
 	 	SC_inmeas(service);
 	}
	else
	{					//else bulk
       		if(!(stat_STAT & 0x01))		//if not measuring			!STAT.0?
       			Bulk_outmeas(service);
	 	Bulk_inmeas(service);
	}
}

/*----------------------------------------------------------------------------------
	Services out of measurement
	
*/

void SC_outmeas(uint8_t service)
{
 uint8_t pom, checksum;
	switch(service)
	{
	  	case 0x01:					// Clear Data
	  		ptrlast = ptrDS;
	  		ptrhead.adr = 0x0000;
	  		ptrhead.page = 0x00;
	  		checksum = Repeat(service,0x01);
	  		PCheck(checksum);
	  		break; 
	  	case 0x03:					//Set Identity
	  		sign.TYPE = Ser_com_rxb();
	  		sign.SN = Ser_com_rxb();
			checksum = Repeat(service,0x05);
			checksum += Ser_com_txb(sign.TYPE);
			checksum += Ser_com_txb(sign.SN);
			checksum += Ser_com_txb(sign.PRGM);
			checksum += Ser_com_txb(sign.VER);
			PCheck(checksum);
			break;
	  	case 0x08:					//Copy Memory
	  		pom = Ser_com_rxb();
	  		stat_STAT |= 0x02;			//			STAT.1 = 1
	  		if(pom == 1)
	  		 stat_STAT |= 0x04;
	  		else
	  		 stat_STAT &= 0xFB;
	  		checksum = Repeat(service,0x02);
	  		checksum += Ser_com_txb(pom);
	  		PCheck(checksum);
			break;
	  	case 0x09:					//CHECK PERIPHERALS
	  		InitPorts(); 				//"???????????"
			GetDallasID(0);				//1wire porty dallas inicializovane
			Mem_Init();				//init seriovych eeprom
	  		checksum = Repeat(service,0x01);
			PCheck(checksum);
			break;
	  	case 0x18:					//GET DALLAS ID
	  		checksum = Repeat(service,0x85);
	  		checksum += GetDallasID(1);		//1wire porty dallas inicializovane		
			PCheck(checksum);	
			break;  	
	  	case 0x22:					//Set Battery Alarms
			batt.LOW = Ser_com_rxw();
			batt.SAVE = Ser_com_rxw();
			batt.MIN = Ser_com_rxw();
			batt.DTSAVE = 0;
			mode |= 0x80;				//			mode.7 = 1
	  		checksum = Repeat(service,0x01);
			PCheck(checksum);
			break;
	  	case 0x24:					//Set DS
	  		Ser_com_rxptr(&ptrDS);
			if((ptrlast.page < ptrDS.page) && (ptrlast.adr < ptrDS.adr))
			 	ptrlast = ptrDS;
	  		checksum = Repeat(service,0x01);
			PCheck(checksum);
			break;
	  	case 0x26:					//Set Analogue outputs
	  		PWMbuff = Ser_com_rxw();		//??????
	  		checksum = Repeat(service,0x01);
			PCheck(checksum);
			break;
	  	case 0x27:					//Set Time Raster
	  		TR = Ser_com_rxb();
	  		checksum = Repeat(service,0x01);
			PCheck(checksum);
			InitRTC();
			break;
	  	case 0x2A:					//Set Measuring mode
	  		EMP = Ser_com_rxb();
	  		EMPR = Ser_com_rxw();
	  		pom = Ser_com_rxb();
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
	  			
	  		checksum = Repeat(service,0x05);
			checksum += Ser_com_txb(EMP);
			checksum += Ser_com_txw(EMPR);
			checksum += Ser_com_txb(mode);
			PCheck(checksum);
			break;
		case 0x2B:					//  Set input masks
			mask.ADCM = Ser_com_rxb();
			mask.DIOM = Ser_com_rxw();
			mask.SHTM = Ser_com_rxw();
			InitOutPorts();
	  		checksum = Repeat(service,0x06);
			checksum += Ser_com_txb(mask.ADCM);
			checksum += Ser_com_txw(mask.DIOM);
			checksum += Ser_com_txw(mask.SHTM);
			PCheck(checksum);
			break;
		case 0x2C:					//  Set ptrLast, ptrHead
			Ser_com_rxptr(&ptrlast);
			Ser_com_rxptr(&ptrhead);
			checksum = Repeat(service,0x07);
			checksum += Ser_com_txptr(&ptrlast);
			checksum += Ser_com_txptr(&ptrhead);
			PCheck(checksum);
			break;
		case 0x2D:					//  Set autosend (after each measurement the raw data to be send via COMport)
			Autosend = Ser_com_rxb();
			checksum = Repeat(service,0x02);
			checksum += Ser_com_txb(Autosend);
			PCheck(checksum);
			break;
		default:
			break;
	}
}

//-------------------------------------------------------------------------------
// Services within the measurement


void SC_inmeas(uint8_t service)
{
 uint8_t outcnt, pom, checksum, pomw;
 
	switch(service)
	{
	  	case 0x02:
	  		checksum = Repeat(service,0x05);
			checksum += Ser_com_txb(sign.TYPE);
			checksum += Ser_com_txb(sign.SN);
			checksum += Ser_com_txb(sign.PRGM);
			checksum += Ser_com_txb(sign.VER);
			PCheck(checksum);
			break;
	  	case 0x11:
	  		pom = Ser_com_rxb();
	  		checksum = Repeat(service,0x08);
	  		checksum += Ser_com_txb(pom);
	  		checksum += Ser_com_txw(alarm[pom].MASK);
	  		checksum += Ser_com_txw(alarm[pom].BOTTOM);
	  		checksum += Ser_com_txw(alarm[pom].TOP);
			PCheck(checksum);
			break;
	  	case 0x13:
	  		checksum = Repeat(service,0x05);
			checksum += Ser_com_txdw(DateTime);
			PCheck(checksum);
			break;		
	  	case 0x14:
	  		checksum = Repeat(service,0x04);
			checksum += Ser_com_txptr(&ptrDS);
			PCheck(checksum);
			break;
	  	case 0x15:
	  		pomw = (uint16_t)mask.ADCM;
	  		pomw &= 0xFF7F;			//PWM nebudem zahrnovat do vystupnych dat!
	  		pomw = pomw<<8;
	  		pomw = pomw | mask.DS1820 | mask.DS2450;
	  		outcnt = 0;
	  		for(pom=0;pom<16;pom++)
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
			checksum = Repeat(service,outcnt);
	  		pomw = (uint16_t)mask.ADCM;
	  		pomw &= 0xFF7F;			//???PWM nebudem zahrnovat do vystupnych dat!
	  		pomw = pomw<<8;
	  		pomw = pomw | mask.DS1820 | mask.DS2450 | mask.DIOM | mask.SHTM;
	  		checksum += Ser_com_txw(pomw);
	  		checksum += StoreData(1);
	  		PCheck(checksum);
	  		break;
	  	case 0x17:
	  		checksum = Repeat(service,0x02);
			checksum += Ser_com_txb(TR);
			PCheck(checksum);
			break;
	  	case 0x19:
	  		checksum = Repeat(service,0x03);
	  		checksum += Ser_com_txw(stat_PIP);
	  		PCheck(checksum);
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
	  		checksum = Repeat(service,0x11);
	  		checksum += Ser_com_txb(mode);		
	  		checksum += Ser_com_txptr(&ptrlast);
	  		checksum += Ser_com_txptr(&ptrhead);
	  		checksum += Ser_com_txb(EMP);
	  		checksum += Ser_com_txw(EMPR);
	  		checksum += Ser_com_txw(pomw);		//baterky
	  		checksum += Ser_com_txdw(batt.DTSAVE);
	  		PCheck(checksum);
	  		break;
	  	case 0x1B:				//get masks
	  		checksum = Repeat(service,0x0A);
	  		checksum += Ser_com_txb(mask.ADCM);
	  		checksum += Ser_com_txw(mask.DIOM);
	  		checksum += Ser_com_txw(mask.SHTM);
	  		checksum += Ser_com_txw(mask.DS1820);
	  		checksum += Ser_com_txw(mask.DS2450);
	  		PCheck(checksum); 
	  		break;
	  	case 0x1D:
	  		checksum = Repeat(service,0x02);
			checksum += Ser_com_txb(Autosend);
			PCheck(checksum);
			break;
	  	case 0x21:
	  		pom = Ser_com_rxb();
	  		alarm[pom].MASK = Ser_com_rxb();
	  		alarm[pom].BOTTOM = Ser_com_rxb();
	  		alarm[pom].TOP = Ser_com_rxb();
			checksum = Repeat(service,0x01);
			PCheck(checksum);
			break;
	  	case 0x23:
			DateTime = Ser_com_rxdw();
	  		checksum = Repeat(service,0x01);
			PCheck(checksum);
			break;
	  	case 0x25:
	  		break;
	  	case 0x29:				// Shut-up
	  		stat_PIP = 0;
	  		stat_STAT &= 0xBF;	//					STAT.6 = 0
	  		checksum = Repeat(service,0x01);
			PCheck(checksum);
			break;
	  	case 0x31:				//Read Int. mem
			ReadMem(service,0);
			break;
	  	case 0x32:				//Read Ext. mem
			ReadMem(service,1);
			break;
	  	case 0x41:				// Write int. mem
			WriteMem(service,0);
			break;
	  	case 0x42:				// Write Ext mem
			WriteMem(service,1);
			break;
		case 0x57:				//	Toogle Charging status after next measurement 
			stat_WD ^= 0x10;		//	stat.WD.4 = 1/0
			stat_WD &= ~0x20;		// vypni fast charge (vzdy) Ak bude FCHRG povolene zapne sa samo za chvilu
			break;
		default:
			break;
	}
}

/*---------------------------------------------------------------------------------
	Bulk services within the measurement
*/

void Bulk_inmeas(uint8_t service)
{
 uint8_t pom, checksum;
 
 	switch(service)
 	{
 		case 0x0A:			//validate config 0=restore config/1=save config
 			pom = Ser_com_rxb();
 			if(pom)
 			  	SaveConfig();
 			else
 			 	RestoreConfig();
 			checksum = Repeat(service,0x01);
 			PCheck(checksum);
 			break;
 		case 0x20:
 			SPEED = Ser_com_rxb();
 			Ser_com_Init(SPEED);		//hned aj preinicializovat com port
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
			checksum = Repeat(service,0x01);
			PCheck(checksum);
			mode &= 0xFE;		//					mode.0 = 0
 			if(stat_STAT & 0x10)	//					STAT.4?
			 SaveMode();
 			else
 			 SaveConfig(); 			 
 			break;
 		case 0x52:
 			stat_STAT &= 0xDF;	//					STAT.5 = 0
 			checksum = Repeat(service,0x01);
 			PCheck(checksum);
 			break;
 		case 0x53:
 			stat_STAT |= 0x20;	//					STAT.5 = 1
 			if(!(stat_RX & 0x08))	//					!RX.3?
 			{
 			 checksum = Repeat(service,0x01);
 			 PCheck(checksum);
 			}
  			stat_RX &= 0xFE;	//po skonceni moze ist spat		RX.0 = 0
			break;
 		case 0x56:
 			stat_MEAS = 0x01;
 			break;
 		case 0xEE:
			break;
		case 0xF0:			//servisna rutina - nabijanie: data 1 byte
			checksum = Repeat(service,0x02);
 			switch(Ser_com_rxb())
 			{
 				case 0x00:
 					CHRGoff;
 					FCHRGoff;
 					stat_WD &= ~0x30;
 					Ser_com_txb(0x00);
 					break;
 				case 0x01:
 					CHRGon;
 					FCHRGoff;
 					stat_WD |= 0x10;
 					stat_WD &= ~0x20;
 					Ser_com_txb(0x01);
 					break;
 				case 0x02:
 					FCHRGon;
 					stat_WD |= 0x20;
 					Ser_com_txb(0x02);
 					break;
 				default:
 					Ser_com_txb(0xFF);
 					break;
 			}
 			PCheck(checksum);
			break;
		default:
			break;
 	}
}

/*-----------------------------------------------------------------------------
	Bulk services out of the measurement
*/

void Bulk_outmeas(uint8_t service)
{
 uint8_t pom, pomb, checksum, testspeed;
 
  	switch(service)
 	{
 		case 0x04:
 			pom = Ser_com_rxb();
 			if(sign.TYPE == pom)
 			{
 			 pom = LocalID;
 			 LocalID = sign.SN;
 			 checksum = Repeat(service,0x02);
 			 checksum += Ser_com_txb(sign.SN);
 			 LocalID = pom;
 			 PCheck(checksum);
 			}
 			break;
 		case 0x0F:
 			pom = Ser_com_rxb();
 			Ser_com_Init(pom);
			pomb = Comratetest();
			if(pomb)
				testspeed = pom;
			stat_MEAS &= 0xFB;		//To to nastavi!		MEAS.2 = 0
 			Ser_com_Init(SPEED);
			break;
 		case 0x10:
 		    checksum = Repeat(service,0x02);
 		    checksum += Ser_com_txb(testspeed);
 		    PCheck(checksum); 		    	
 		    break;
 		case 0x28:
 			pom = Ser_com_rxb();
 			if(sign.TYPE == pomb)
 			{
 				pom = Ser_com_rxb();
 			 	if(pom == sign.SN)
 			 	{
 			  		LocalID = Ser_com_rxb();
 			  		checksum = Repeat(service,0x01);
 			  		PCheck(checksum);
 			  	}	
			}
 			break;
  		case 0x51:
 			if(stat_RX & 0x20)		//if buffer empty		RX.5?
 			{
 				stat_STAT &= 0xF7;	//				STAT.3 = 0			
 				stat_STAT = stat_STAT | 0x81;
 				MP = 0;
 				MPR = 0;
 			}
 			else
 			{
 			 	DTStart = Ser_com_rxdw(); 
 			 	stat_STAT |= 0x08;	//				STAT.3 = 1
			}
			if(!(stat_STAT & 0x01))		//				!STAT.0?
				pom = 0x00;
			else
				pom = 0x01;
 			checksum = Repeat(service,0x02);
 			checksum += Ser_com_txb(pom);
 			PCheck(checksum);
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

uint8_t Repeat(uint8_t service, uint8_t outcnt)
{
 uint8_t checksum, pom;
 
 	R485off;			//bez ohladu na napajanie vypni prijimac 485
	if(stat_RX & 0x08)		//						RX.3?
	{
	 	cbi(UCSR0B,RXEN);		//pocas bulk cakania na time slot vypni RX
	 	pom = SPEED>>1;
	 	pom++;
		my_delay(LocalID,pom);
	 	while(!(stat_TX & 0x10));	//					!TX.4?
	 	stat_TX &= 0xEF;		//					TX.4 = 0
	 	stat_MEAS &= 0xFB;		//pretoze v Txx dojde k nastaveniu!	MEAS.2 = 0
		if(!PWR)
	 		T485on;			//switch on TX driver on RS485 if external power
 	 	sbi(UCSR0B,RXEN);
 	 	Ser_com_txb(LocalID);
	 	return(0);
	}
	if(!PWR)
 		T485on;			//switch on TX driver on RS485 if external power
	Ser_com_txb((uint8_t)0x55);
					//ak sa bude do check sum pocitat aj LocalID
	checksum = Ser_com_txb(LocalID);
	checksum += Ser_com_txb(outcnt);
	checksum += Ser_com_txb(service);
	return(checksum);
}

/*---------------------------------------------------------------------------------
	Finishing part of the answering routine
*/

void PCheck(uint8_t checksum)
{
	checksum = ~checksum + 0x01;
	if(!(stat_RX & 0x08))		//ak bulk, tak preskoc rcall			!RX.3?
	 	Ser_com_txb(checksum);
	while(!(stat_TX & 0x04));	//						!TX.2?
	T485off;			//vypni vysielac 485
}

/*--------------------------------------------------------------------------------
	Receive the DWORD
*/

uint32_t Ser_com_rxdw(void)
{
 uint32_t data;
 uint16_t pomw;
 
 	pomw = Ser_com_rxw();
 	data = (uint32_t)pomw;
 	pomw = Ser_com_rxw();
 	data = data | (((uint32_t)pomw)<<16);
 	return(data);
}

/*--------------------------------------------------------------------------------
	Receive the MEMPTR
*/


void Ser_com_rxptr(MEMPTR *data)
{
  	(*data).adr = Ser_com_rxw();
 	(*data).page = Ser_com_rxb();
}

/*--------------------------------------------------------------------------------
	Receive the WORD
*/


uint16_t Ser_com_rxw(void)
{
 uint16_t data;
 uint8_t pom;
 
 	pom = Ser_com_rxb();
 	data = (uint16_t)pom;
 	pom = Ser_com_rxb();
 	data = data | (((uint16_t)pom)<<8); 
 	return(data);
}

/*--------------------------------------------------------------------------------
	Receive the BYTE
*/

uint8_t Ser_com_rxb(void)
{
 uint8_t data;
 	if(stat_RX & 0x20)		//						RX.5?
 	{
		stat_RX |= 0x80;	//error (buffer is empty but read requested)	RX.7 = 1
		data = 0x00;
	}
	else
	{
		data = uart_buffer[uart_head];
		uart_head++;
		uart_head &= 0x1F;		//rotuj buffer na 32 bytoch
		if(uart_head == uart_tail)	//ak sa head = tail -> buffer je prazdny
			stat_RX |= 0x20;	//					RX.5 = 1
	}
	return(data);
}

/*--------------------------------------------------------------------------------
	Send the DWORD
*/

uint8_t Ser_com_txdw(uint32_t data)
{
 uint8_t checksum;
 
 	checksum = Ser_com_txw((uint16_t)data);		//posli dolne 2 byty cez 16bitovy send
 	checksum += Ser_com_txw((uint16_t)(data>>16));	//posli horne 2 byty cez 16 bitoby send
 	return(checksum);
}


/*--------------------------------------------------------------------------------
	Send the MEMPTR
*/

uint8_t Ser_com_txptr(MEMPTR *data)
{
 uint8_t checksum;

	checksum = Ser_com_txw((*data).adr);		//posli adresu as word
	checksum += Ser_com_txb((*data).page);		//posli horny page as byte
	return(checksum);
}

/*--------------------------------------------------------------------------------
	Send the WORD
*/

uint8_t Ser_com_txw(uint16_t data)
{
 uint8_t checksum;
 
	checksum = Ser_com_txb((uint8_t)data);		//posli spodny byte
	checksum += Ser_com_txb((uint8_t)(data>>8));	//posli horny byte
	return(checksum);
}

/*--------------------------------------------------------------------------------
	Send the BYTE
*/

uint8_t Ser_com_txb(uint8_t data)
{
	while(!(stat_TX & 0x04));	//pokial sa neskoncilo posielanie predchadzajuceho bytu, tak cakaj
					//						!TX.2?
	outp(data,UDR0);		//poloz data do registra
	stat_TX &= 0xFB;		//zrus priznak TX buffer prazdny		TX.2 = 0
	return(data);			//vrat to co posielal pre potreby vypoctu checksumu
}
 
/*--------------------------------------------------------------------------------
	Communication speed test
*/

uint8_t Comratetest(void)
{
 uint8_t pom,l;
 
	outp(0x58, UCSR0B);		//RXCIE disable - software control
	my_delay(14,0xC2);		//maximalna doba 350ms?
	stat_TX &= 0xEF;		//pretoze dojde k nastaveniu v T2		TX.4 = 0
	for(l = 0;l<10;l++)	//cakam 10 testovacich bytov
	{
		while(bit_is_clear(UCSR0A,7));	//softverove riadenie prijmu -> cakam na prijatie bytu
		{
			if(!i)			//ak medzi tym pretiekol cas, tak vrat neuspesny pokus
				return(0);
		}
		pom = inp(UDR0);		//vycitaj data
		if(pom != 0xAA)			//ak sa nerovnaju test. vzorke 0xAA tak zrus casovac
		{
			i = 1;
			while(i);
		 	return(0);		//vrat neuspesny pokus
		}
	}
	i = 1;			//ak preslo vsetkych 10 bytov, tak zrus casovac
	while(i);
	return(1);		// a vrat uspesny pokus
}
