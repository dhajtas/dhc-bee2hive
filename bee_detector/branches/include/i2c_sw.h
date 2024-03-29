#ifndef	I2C_SW_H
#define I2C_SW_H


#define		I2C_PORT		PORT(I2C_P)
#define		SDA_bm			SDA0|SDA1|SDA2|SDA3|SDA4|SDA5
#define		SCL_bm			SCL0|SCL1

//-----------------------------------------------------------------------------------------------//
//		structure definitions 
//-----------------------------------------------------------------------------------------------//

//-----------------------------------------------------------------------------------------------//
//		Global variables
//-----------------------------------------------------------------------------------------------//

//-----------------------------------------------------------------------------------------------//
//		routines
//-----------------------------------------------------------------------------------------------//


void I2C_connection_reset(uint8_t);

uint8_t I2C_send8_t(uint8_t,uint8_t);

uint8_t I2C_rcv8_t(uint8_t, uint8_t, uint8_t*);

void I2C_TRX_start(uint8_t);

void I2C_send_bit(uint8_t, uint8_t);

uint8_t I2C_rcv_bit(uint8_t);

//void SHT_send_ack(uint8_t, uint8_t);

//uint8_t SHT_rcv_ack(uint8_t);

void I2C_init(void);

void I2C_clk(uint8_t);

void I2C_sw_data_dir(uint8_t, uint8_t);


// only SHT # 0, 2, 3 will be serviced for DS3 device (0 - internal SHT port)

#endif //I2C_SW_H
