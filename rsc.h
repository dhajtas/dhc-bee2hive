#ifndef	RSCOM_H
#define RSCOM_H

//-----------------------------------------------------------------------------------------------//
//		structure definitions 
//-----------------------------------------------------------------------------------------------//

//-----------------------------------------------------------------------------------------------//
//		Global variables
//-----------------------------------------------------------------------------------------------//

//-----------------------------------------------------------------------------------------------//
//		routines
//-----------------------------------------------------------------------------------------------//

void SC_outmeas(uint8_t);

void SC_inmeas(uint8_t);

void Bulk_inmeas(uint8_t);

void Bulk_outmeas(uint8_t);

uint8_t Repeat(uint8_t, uint8_t);

void PCheck( uint8_t);

uint32_t Ser_com_rxdw(uint8_t);

void Ser_com_rxptr(MEMPTR*, uint8_t);

uint16_t Ser_com_rxw(uint8_t);

uint8_t Ser_com_rxb(uint8_t);

uint8_t Ser_com_txdw( uint32_t, uint8_t);

uint8_t Ser_com_txptr(MEMPTR*, uint8_t);

uint8_t Ser_com_txw( uint16_t, uint8_t);

uint8_t Ser_com_txb( uint8_t, uint8_t);

#endif	//RSCOM_H
