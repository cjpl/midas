#ifndef _FDG_008_TSENSOR_H
#define _FDG_008_TSENSOR_H

// Prescale factor. Onlye one temperature measurement is performed per 
// "TEMP_PRESCALE" times  user_loop call.
#define     TEMP_PRESCALE   10

// --------------------------------------------------------
//  LM95231 related definitions
// --------------------------------------------------------
// SMBus slave address
#define ADDR_LM95231            0x19

// Register Map
#define REG_TEMP_STATUS         0x02
#define REG_TEMP_CONFIG         0x03
#define REG_TEMP_FILTER         0x06
#define REG_TEMP_TRUTHERM       0x07
#define REG_TEMP_ONESHOT        0x0F
#define REG_TEMP_MODEL          0x30
#define REG_TEMP_LOCAL_MSB      0x10
#define REG_TEMP_REMOTE1_MSB    0x11
#define REG_TEMP_REMOTE2_MSB    0x12
#define REG_TEMP_LOCAL_LSB      0x20
#define REG_TEMP_REMOTE1_LSB    0x21
#define REG_TEMP_REMOTE2_LSB    0x22
#define REG_TEMP_MANUFACTURER   0xFE
#define REG_TEMP_REVISION       0xFF

// --------------------------------------------------------
//  Function prototypes (public)
// --------------------------------------------------------
void    process_temperature(unsigned char mode);
void    store_temperature(unsigned char index, float temperature);

// --------------------------------------------------------
// C8051F310 related function prototype
// --------------------------------------------------------
unsigned char read_temperature_CPU(float * temperature);

// --------------------------------------------------------
// LM95231 related function prototypes
// --------------------------------------------------------
unsigned char init_LM95231 (unsigned char init);
unsigned char read_temperature_LM95231(unsigned char diode, float * temp);

// private functions
// single read/write cycle
void          write_LM95231(unsigned char address, unsigned char value);
unsigned char read_LM95231 (unsigned char address);

/*
// single 8-bit word read/write (primitive functions)
void          write_word_LM95231(unsigned char word, unsigned char ack);
void          read_word_LM95231(unsigned char * word, unsigned char ack);
*/

#endif

