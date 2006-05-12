/********************************************************************\

  Name:         scs_2000_lib.c
  Created by:   Stefan Ritt


  Contents:     Library of plug-in modules for SCS-2000

  $Id: scs_2000_lib.c 2874 2005-11-15 08:47:14Z ritt $

\********************************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <intrins.h>
#include "mscb.h"
#include "scs_2000.h"

/*---- Port definitions ----*/

sbit OPT_CLK    = P3 ^ 0;
sbit OPT_ALE    = P3 ^ 1;
sbit OPT_STR    = P3 ^ 2;
sbit OPT_DATAO  = P3 ^ 3;

sbit OPT_DATAI  = P3 ^ 4;
sbit OPT_STAT   = P3 ^ 5;
sbit OPT_SPARE1 = P3 ^ 6;
sbit OPT_SPARE2 = P3 ^ 7;

/*---- List of modules ---------------------------------------------*/

unsigned char dr_dout_bits(unsigned char id, unsigned char cmd, unsigned char addr, unsigned char port, unsigned char chn, void *pd) reentrant;
unsigned char dr_dout_byte(unsigned char id, unsigned char cmd, unsigned char addr, unsigned char port, unsigned char chn, void *pd) reentrant;
unsigned char dr_din_bits(unsigned char id, unsigned char cmd, unsigned char addr, unsigned char port, unsigned char chn, void *pd) reentrant;
unsigned char dr_ltc2600(unsigned char id, unsigned char cmd, unsigned char addr, unsigned char port, unsigned char chn, void *pd) reentrant;
unsigned char dr_ad7718(unsigned char id, unsigned char cmd, unsigned char addr, unsigned char port, unsigned char chn, void *pd) reentrant;
unsigned char dr_ads1256(unsigned char id, unsigned char cmd, unsigned char addr, unsigned char port, unsigned char chn, void *pd) reentrant;

MSCB_INFO_VAR code vars_id01[] =
   { 1, UNIT_BYTE,    0,          0, 0,           "P%Out",   1, 0, 255, 1   };

MSCB_INFO_VAR code vars_uin[] =
   { 4, UNIT_VOLT,    0,          0, MSCBF_FLOAT, "P%Uin#",  8 };

MSCB_INFO_VAR code vars_uout[] =
   { 4, UNIT_VOLT,    0,          0, MSCBF_FLOAT, "P%Uout#", 8, 0, 30,  0.5 };

MSCB_INFO_VAR code vars_iin[] =
   { 4, UNIT_AMPERE,  PRFX_MILLI, 0, MSCBF_FLOAT, "P%Iin#",  8 };

MSCB_INFO_VAR code vars_iout[] =
   { 4, UNIT_AMPERE,  PRFX_MILLI, 0, MSCBF_FLOAT, "P%Iout#", 8, 0, 20,  0.1 };

MSCB_INFO_VAR code vars_din[] =
   { 1, UNIT_BOOLEAN, 0,          0,           0, "P%Din#",  8 };

MSCB_INFO_VAR code vars_dout[] =
   { 1, UNIT_BOOLEAN, 0,          0,           0, "P%Dout#", 8, 0,  1,  1 };

MSCB_INFO_VAR code vars_relais[] =
   { 1, UNIT_BOOLEAN, 0,          0,           0, "P%Rel#",  4, 0,  1,  1 };

MSCB_INFO_VAR code vars_optout[] =
   { 1, UNIT_BOOLEAN, 0,          0,           0, "P%Out#",  4, 0,  1,  1 };

SCS_2000_MODULE code scs_2000_module[] = {
  /* 0x01-0x1F misc. */
  { 0x01, "LED-Debug",       vars_id01,   dr_dout_byte   },
  { 0x02, "LED-Pulser",      vars_uout,   dr_ltc2600     },

  /* 0x20-0x3F digital in  */
  { 0x20, "Din 5V",          vars_din,    dr_din_bits    },

  /* 0x40-0x5F digital out */
  { 0x40, "Dout 5V",         vars_dout,   dr_dout_bits   },
  { 0x41, "Relais",          vars_relais, dr_dout_bits   },
  { 0x42, "OptOut",          vars_optout, dr_dout_bits   },

  /* 0x60-0x7F analog in  */
  { 0x60, "Uin 0-2.5V",      vars_uin,    dr_ad7718      },
  { 0x61, "Uin +-10V",       vars_uin,    dr_ad7718      },
  { 0x62, "Iin 0-2.5mA",     vars_iin,    dr_ad7718      },
  { 0x63, "Iin 0-25mA",      vars_iin,    dr_ad7718      },

  { 0x64, "Uin 0-2.5V Fast", vars_uin,    dr_ads1256     },
  { 0x65, "Uin +-10V Fast",  vars_uin,    dr_ads1256     },
  { 0x66, "Iin 0-2.5mA Fast",vars_iin,    dr_ads1256     },
  { 0x67, "Iin 0-25mA Fast", vars_iin,    dr_ads1256     },

  /* 0x80-0x9F analog out */
  { 0x80, "UOut 0-2.5V",     vars_uout,   dr_ltc2600     },
  { 0x81, "UOut +-10V",      vars_uout,   dr_ltc2600     },
  { 0x82, "IOut 0-2.5mA",    vars_iout,   dr_ltc2600     },
  { 0x83, "IOut 0-25mA",     vars_iout,   dr_ltc2600     },

  { 0 }
};

/*---- Serial functions --------------------------------------------*/

#pragma NOAREGS

#define DELAY_CLK { \
_nop_(); _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); \
_nop_(); _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); }

#define CLOCK  { DELAY_CLK; OPT_CLK = 1; DELAY_CLK; OPT_CLK = 0; }
#define STROBE { OPT_STR = 1; DELAY_CLK; OPT_CLK = 1; DELAY_CLK; OPT_CLK = 0; OPT_STR = 0; }

#define AM_READ_PORT    0 // switch port to input and read pin value
#define AM_READ_REG     1 // readback port output register
#define AM_WRITE_PORT   2 // write to port output register
#define AM_READ_CSR     3 // read CPLD CSR (buzzer etc)
#define AM_WRITE_CSR    4 // write CPLD CSR
#define AM_RW_SERIAL    5 // read/write to serial device on port
#define AM_RW_EEPROM    6 // read/write to eeprom on port

/* address port 0-7 in CPLD, with address modifier listed above */
void address_port(unsigned char addr, unsigned char port_no, unsigned char am, unsigned char clk_level) reentrant
{
unsigned char i;

   OPT_ALE = 1;
   OPT_CLK = 0;
   OPT_STR = 0;

   /* address unit */
   for (i=0 ; i<4 ; i++) {
      OPT_DATAO = ((addr << i) & 8) > 0;
      CLOCK;
   }


   /* address port */
   for (i=0 ; i<3 ; i++) {
      OPT_DATAO = ((port_no << i) & 4) > 0;
      CLOCK;
   }

   /* address modifier */
   for (i=0 ; i<3 ; i++) {
      OPT_DATAO = ((am << i) & 4) > 0;
      CLOCK;
   }

   STROBE;
   OPT_DATAO = 0;
   OPT_CLK = clk_level;
   OPT_STR = 1;  // keeps /CS high
   OPT_ALE = 0;  // CLK gets set
   DELAY_CLK;
   OPT_STR = 0;  // set /CS low
}

/* write 8 bits to port output */
void write_port(unsigned char addr, unsigned char port_no, unsigned char d) reentrant
{
unsigned char i;

   address_port(addr, port_no, AM_WRITE_PORT, 0);

   /* send data */
   for (i=0 ; i<8 ; i++) {
      OPT_DATAO = (d & 0x80) > 0;
      CLOCK;
      d <<= 1;
   }
   OPT_DATAO = 0;
   STROBE;
}

/* read 8 bits from port pins */
void read_port(unsigned char addr, unsigned char port_no, unsigned char *pd) reentrant
{
unsigned char i, d;

   address_port(addr, port_no, AM_READ_PORT, 0);

   /* strobe input */
   STROBE;

   /* read data */
   for (i=0 ; i<8 ; i++) {
      CLOCK;
      d = (d << 1) | OPT_DATAI;
   }
   *pd = d;
}

/* read 8 bits from port output register */
void read_port_reg(unsigned char addr, unsigned char port_no, unsigned char *pd) reentrant
{
unsigned char i, d;

   address_port(addr, port_no, AM_READ_REG, 0);

   /* strobe input */
   STROBE;

   /* read data */
   for (i=0 ; i<8 ; i++) {
      CLOCK;
      d = (d << 1) | OPT_DATAI;
   }
   *pd = d;
}

/* write to CSR */
void write_csr(unsigned char addr, unsigned char csr_no, unsigned char d) reentrant
{
unsigned char i;

   address_port(addr, csr_no, AM_WRITE_CSR, 0);

   /* send data */
   for (i=0 ; i<8 ; i++) {
      OPT_DATAO = (d & 0x80) > 0;
      CLOCK;
      d <<= 1;
   }
   OPT_DATAO = 0;
   STROBE;
}

/* read from CSR */
void read_csr(unsigned char addr, unsigned char csr_no, unsigned char *pd) reentrant
{
unsigned char i, d;

   address_port(addr, csr_no, AM_READ_CSR, 0);

   /* strobe input */
   STROBE;

   /* read data */
   for (i=0 ; i<8 ; i++) {
      CLOCK;
      d = (d << 1) | OPT_DATAI;
   }
   *pd = d;
}

/* check if module is plugged in */
unsigned char module_present(unsigned char addr, unsigned char port_no) reentrant
{
unsigned char i;

   address_port(addr, port_no, AM_RW_EEPROM, 0);

   /* READ command */
   OPT_DATAO = 1;
   CLOCK;
   OPT_DATAO = 1;
   CLOCK;
   OPT_DATAO = 0;
   CLOCK;

   /* address 0 */
   for (i=0 ; i<6 ; i++) {
      OPT_DATAO = 0;
      CLOCK;
   }

   /* check for dummy zero readout */
   return OPT_DATAI == 0;
}

/* read 8 bits from eeprom on specific port */
void read_eeprom(unsigned char addr, unsigned char port_no, unsigned char *pd) reentrant
{
unsigned char i;
unsigned short d;

   address_port(addr, port_no, AM_RW_EEPROM, 0);

   /* READ command */
   OPT_DATAO = 1;
   CLOCK;
   OPT_DATAO = 1;
   CLOCK;
   OPT_DATAO = 0;
   CLOCK;

   /* address 0 */
   for (i=0 ; i<6 ; i++) {
      OPT_DATAO = 0;
      CLOCK;
   }

   /* read 16 bit data */
   for (i=0 ; i<16 ; i++) {
      CLOCK;
      d = (d << 1) | OPT_DATAI;
   }

   /* return only lower 8 bits */
   *pd = (d >> 8);
}

/* write 8 bits to eeprom on specific port */
void write_eeprom(unsigned char addr, unsigned char port_no, unsigned char d) reentrant
{
unsigned char i;
unsigned short t;

   address_port(addr, port_no, AM_RW_EEPROM, 0);

   /* EWEN command */
   OPT_DATAO = 1;
   CLOCK;
   OPT_DATAO = 0;
   CLOCK;
   OPT_DATAO = 0;
   CLOCK;

   for (i=0 ; i<6 ; i++) {
      OPT_DATAO = 1;
      CLOCK;
   }

   /* remove CS to strobe */
   OPT_STR = 1;
   DELAY_CLK;
   OPT_STR = 0;

   /* WRITE command */
   OPT_DATAO = 1;
   CLOCK;
   OPT_DATAO = 0;
   CLOCK;
   OPT_DATAO = 1;
   CLOCK;

   for (i=0 ; i<6 ; i++) {
      OPT_DATAO = 0;
      CLOCK;
   }

   /* write 16 bit data */
   for (i=0 ; i<16 ; i++) {
      OPT_DATAO = (d & 0x80) > 0;
      CLOCK;
      d <<= 1;
   }

   /* remove CS to strobe */
   OPT_STR = 1;
   DELAY_CLK;
   OPT_STR = 0;

   /* wait until BUSY high, should be around 6ms */
   for (t=0 ; t<10000 ; t++) {
      if (OPT_DATAI == 1)
         break;
      DELAY_US(1);
   }

}

/*---- Bitwise output/input ----------------------------------------*/

unsigned char xdata pattern[8];

unsigned char dr_dout_bits(unsigned char id, unsigned char cmd, unsigned char addr, 
                           unsigned char port, unsigned char chn, void *pd) reentrant
{
unsigned char port_dir;

   if (id || chn || pd); // suppress compiler warning

   if (cmd == MC_INIT) {
      /* read previous pattern */
      read_port_reg(addr, port, &pattern[port]);

      /* switch port to output */
      read_csr(addr, 0, &port_dir);
      port_dir |= (1 << port);
      write_csr(addr, 0, port_dir);
   }

   if (cmd == MC_WRITE) {
      if (*((unsigned char *)pd))
         pattern[port] |= (1 << chn);
      else
         pattern[port] &= ~(1 << chn);

      write_port(addr, port, pattern[port]);
   }

   return 1;   
}

unsigned char dr_din_bits(unsigned char id, unsigned char cmd, unsigned char addr, 
                          unsigned char port, unsigned char chn, void *pd) reentrant
{
unsigned char d;

   if (id || chn || pd); // suppress compiler warning

   if (cmd == MC_READ) {
      if (*((unsigned char *)pd))
         pattern[port] |= (1 << chn);
      else
         pattern[port] &= ~(1 << chn);

      read_port(addr, port, &d);
      *((unsigned char *)pd) = (d & (1 << chn)) > 0;
   }

   return 1;   
}

unsigned char dr_dout_byte(unsigned char id, unsigned char cmd, unsigned char addr, 
                           unsigned char port, unsigned char chn, void *pd) reentrant
{
unsigned char port_dir;

   if (id || chn || pd); // suppress compiler warning

   if (cmd == MC_INIT) {
      /* switch port to output */
      read_csr(addr, 0, &port_dir);
      port_dir |= (1 << port);
      write_csr(addr, 0, port_dir);
   }

   if (cmd == MC_WRITE)
      write_port(addr, port, *((unsigned char *)pd));

   return 1;   
}

/*---- LTC2600 16-bit DAC ------------------------------------------*/

unsigned char dr_ltc2600(unsigned char id, unsigned char cmd, unsigned char addr, 
                         unsigned char port, unsigned char chn, void *pd) reentrant
{
float value;
unsigned short d;
unsigned char i;

   if (cmd == MC_WRITE) {
      value = *((float *)pd);
   
      /* convert value to DAC counts */
      if (id == 0x82)
         d = value/2.5 * 65535;  // 0-2.5mA
      else if (id == 0x83)
         d = value/25.0 * 65535; // 0-25mA
   
      address_port(addr, port, AM_RW_SERIAL, 0);
    
      /* EWEN command */
      for (i=0 ; i<4 ; i++) {
         OPT_DATAO = (i > 1);
         CLOCK;
      }
   
      for (i=0 ; i<4 ; i++) {
         OPT_DATAO = (chn & 8) > 0;
         CLOCK;
         chn <<= 1;
      }
   
      for (i=0 ; i<16 ; i++) {
         OPT_DATAO = (d & 0x8000) > 0;
         CLOCK;
         d <<= 1;
      }
   
      /* remove CS to update outputs */
      OPT_ALE = 1; 
   }

   return 1;
}

/*---- AD7718 24-bit sigma-delta ADC -------------------------------*/

/* AD7718 registers */
#define AD7718_STATUS     0
#define AD7718_MODE       1
#define AD7718_CONTROL    2
#define AD7718_FILTER     3
#define AD7718_ADCDATA    4
#define AD7718_ADCOFFSET  5
#define AD7718_ADCGAIN    6
#define AD7718_IOCONTROL  7

void ad7718_write(unsigned char a, unsigned char d) reentrant
{
   unsigned char i, m;

   OPT_DATAO = 0;
   OPT_ALE = 0;
   DELAY_CLK;

   /* write zeros to !WEN and R/!W */
   for (i=0 ; i<4 ; i++) {
      OPT_CLK   = 0;
      OPT_DATAO = 0;
      DELAY_CLK;
      OPT_CLK   = 1;
      DELAY_CLK;
   }

   /* register address */
   for (i=0,m=8 ; i<4 ; i++) {
      OPT_CLK   = 0;
      OPT_DATAO = (a & m) > 0;
      DELAY_CLK;
      OPT_CLK   = 1;
      DELAY_CLK;
      m >>= 1;
   }

   /* remove CS to strobe */
   OPT_STR = 1;
   DELAY_CLK;
   OPT_STR = 0;

   /* write to selected data register */
   for (i=0,m=0x80 ; i<8 ; i++) {
      OPT_CLK   = 0;
      OPT_DATAO = (d & m) > 0;
      DELAY_CLK;
      OPT_CLK   = 1;
      DELAY_CLK;
      m >>= 1;
   }

   OPT_ALE = 1;
   OPT_DATAO = 0;
   DELAY_CLK;
}

void ad7718_read(unsigned char a, unsigned long *d) reentrant
{
   unsigned char i, m;

   OPT_ALE = 0;
   DELAY_CLK;

   /* write zero to !WEN and one to R/!W */
   for (i=0 ; i<4 ; i++) {
      OPT_CLK   = 0;
      OPT_DATAO = (i == 1);
      DELAY_CLK;
      OPT_CLK   = 1;
      DELAY_CLK;
   }

   /* register address */
   for (i=0,m=8 ; i<4 ; i++) {
      OPT_CLK   = 0;
      OPT_DATAO = (a & m) > 0;
      DELAY_CLK;
      OPT_CLK   = 1;
      DELAY_CLK;
      m >>= 1;
   }

   /* remove CS to strobe */
   OPT_STR = 1;
   DELAY_CLK;
   OPT_STR = 0;

   /* read from selected data register */
   for (i=0,*d=0 ; i<24 ; i++) {
      OPT_CLK = 0;
      DELAY_CLK;
      *d = (*d << 1) | OPT_DATAI;
      OPT_CLK = 1; 
      DELAY_CLK; 
   }

   OPT_ALE = 1;
   DELAY_CLK;
}

unsigned char xdata ad7718_last_chn[8];

unsigned char dr_ad7718(unsigned char id, unsigned char cmd, unsigned char addr, 
                        unsigned char port, unsigned char chn, void *pd) reentrant
{
float value;
unsigned long d;
unsigned char status, next_chn;

   if (chn); // suppress compiler warning

   if (cmd == MC_INIT) {
      address_port(addr, port, AM_RW_SERIAL, 1);
      ad7718_write(AD7718_FILTER, 82);                // SF value for 50Hz rejection (2 Hz)
      //ad7718_write(AD7718_FILTER, 13);                // SF value for faster conversion (12 Hz)
      ad7718_write(AD7718_MODE, 3);                   // continuous conversion
      ad7718_write(AD7718_CONTROL, (0 << 4) | 0x0F);  // Chn. 0, +2.56V range
      ad7718_last_chn[port] = 0;
      DELAY_US(100);
   }

   if (cmd == MC_READ) {

      read_port(addr, port, &status);
      if ((status & 1) > 0)
         return 0;

      address_port(addr, port, AM_RW_SERIAL, 1);
    
      /* read 24-bit data */
      ad7718_read(AD7718_ADCDATA, &d);

      /* start next conversion */
      next_chn = (ad7718_last_chn[port] + 1) % 8;
      ad7718_write(AD7718_CONTROL, (next_chn << 4) | 0x0F);  // next chn, +2.56V range

      /* convert to volts */
      value = 2.56*((float)d / (1l<<24));
   
      /* apply range */
      if (id == 0x60)
         value *= 1;
      else if (id == 0x61)
         value = (value/2.5)*20.0 - 10;
      else if (id == 0x62)
         value *= 1;
      else if (id == 0x63)
         value *= 10;

      /* round result to significant digits */
      if (id == 0x61|| id == 0x63) {
         d = (unsigned long)(value*1E4+0.5);
         value = d/1E4;
      } else {
         d = (unsigned long)(value*1E5+0.5);
         value = d/1E5;
      }

      DISABLE_INTERRUPTS;
      *((float *)pd + ad7718_last_chn[port]) = value;
      ENABLE_INTERRUPTS;

      ad7718_last_chn[port] = next_chn;
   }

   return 1;
}

/*---- ADS1256 24-bit fast sigma-delta ADC -------------------------*/

/* ADS1256 commands */
#define ADS1256_WAKEUP      0x00
#define ADS1256_RDATA       0x01
#define ADS1256_RDATAC      0x03
#define ADS1256_SDATAC      0x0F
#define ADS1256_RREG        0x10
#define ADS1256_WREG        0x50
#define ADS1256_SELFCAL     0xF0
#define ADS1256_SELFOCAL    0xF1
#define ADS1256_SELFGCAL    0xF2
#define ADS1256_SYSOCAL     0xF3
#define ADS1256_SYSGCAL     0xF4
#define ADS1256_SYNC        0xFC
#define ADS1256_STANDBY     0xFD
#define ADS1256_RESET       0xFE

/* ADS1256 registers */
#define ADS1256_STATUS      0
#define ADS1256_MUX         1
#define ADS1256_ADCON       2
#define ADS1256_DRATE       3
#define ADS1256_IO          4

void ads1256_cmd(unsigned char c) reentrant
{
   unsigned char i;

   OPT_DATAO = 0;
   OPT_ALE = 0;
   DELAY_CLK;

   for (i=0 ; i<8 ; i++) {
      OPT_CLK   = 1;
      OPT_DATAO = (c & 0x80) > 0;
      DELAY_CLK;
      OPT_CLK   = 0;
      DELAY_CLK;
      c <<= 1;
   }

   OPT_ALE = 1;
   OPT_DATAO = 0;
   DELAY_CLK;
}

void ads1256_write(unsigned char a, unsigned char d) reentrant
{
   unsigned char i, c;

   OPT_DATAO = 0;
   OPT_ALE = 0;
   DELAY_CLK;

   /* WREG command */
   c = ADS1256_WREG + a;
   for (i=0 ; i<8 ; i++) {
      OPT_CLK   = 1;
      OPT_DATAO = (c & 0x80) > 0;
      DELAY_CLK;
      OPT_CLK   = 0;
      DELAY_CLK;
      c <<= 1;
   }

   /* data length-1 = 0 */
   c = ADS1256_WREG + a;
   for (i=0 ; i<8 ; i++) {
      OPT_CLK   = 1;
      OPT_DATAO = 0;
      DELAY_CLK;
      OPT_CLK   = 0;
      DELAY_CLK;
      c <<= 1;
   }

   /* write one byte for single register */
   for (i=0 ; i<8 ; i++) {
      OPT_CLK   = 1;
      OPT_DATAO = (d & 0x80) > 0;
      DELAY_CLK;
      OPT_CLK   = 0;
      DELAY_CLK;
      d <<= 1;
   }

   OPT_ALE = 1;
   OPT_DATAO = 0;
   DELAY_CLK;
}

void ads1256_read(unsigned long *d) reentrant
{
   unsigned char i, c;

   OPT_ALE = 0;
   DELAY_CLK;

   /* RDATA command */
   c = ADS1256_RDATA;
   for (i=0 ; i<8 ; i++) {
      OPT_CLK   = 1;
      OPT_DATAO = (c & 0x80) > 0;
      DELAY_CLK;
      OPT_CLK   = 0;
      DELAY_CLK;
      c <<= 1;
   }

   DELAY_US(10); // Datasheet: 50 tau_clkin = 1/8MHz * 50

   /* read from selected data register */
   for (i=0,*d=0 ; i<24 ; i++) {
      OPT_CLK = 1;
      DELAY_CLK;
      *d = (*d << 1) | OPT_DATAI;
      OPT_CLK = 0; 
      DELAY_CLK; 
   }

   OPT_ALE = 1;
   DELAY_CLK;
}

unsigned long xdata tmp_last = 0;

unsigned char dr_ads1256(unsigned char id, unsigned char cmd, unsigned char addr, 
                         unsigned char port, unsigned char chn, void *pd) reentrant
{
float value;
unsigned long d;
unsigned char status;

   if (chn); // suppress compiler warning

   if (cmd == MC_INIT) {
      address_port(addr, port, AM_RW_SERIAL, 0);

      ads1256_write(ADS1256_STATUS, 0x02);  // Enable buffer
      ads1256_write(ADS1256_ADCON,  0x01);  // Clock Out OFF, PGA=2

      //ads1256_write(ADS1256_DRATE,  0x23);  // 10 SPS
      //ads1256_write(ADS1256_DRATE,  0x82);  // 100 SPS
      //ads1256_write(ADS1256_DRATE,  0xA1);  // 1000 SPS
      ads1256_write(ADS1256_DRATE,  0xD0);  // 7500 SPS
      //ads1256_write(ADS1256_DRATE,  0xF0);  // 30000 SPS

      /* do self calibration */
      ads1256_cmd(ADS1256_SELFCAL);

      /* wait for /DRDY to go low after self calibration */
      for (d = 0 ; d<100000 ; d++) {
         if (OPT_STAT == 0)
            break;
         DELAY_US(1);
      }
   }

   if (cmd == MC_READ) {

      if (time() == tmp_last)
         return 0;

      tmp_last = time();
      /*
      if (time() - tmp_last > 10) {
         tmp_last = time();
         address_port(addr, port, AM_RW_SERIAL, 0);
         DELAY_US(100);
         if (tmp & 1) {
            ads1256_write(ADS1256_IO, 0x00);
         } else {
            ads1256_write(ADS1256_IO, 0x0F);
         }
         tmp++;
      }
      return 0;
      */

      /* read all 8 channels */
      for (chn = 0 ; chn < 8 ; chn++) {

         address_port(addr, port, AM_RW_SERIAL, 0);
         ads1256_write(ADS1256_MUX, (chn << 4) | 0x0F);  // Select single ended positive input
         ads1256_cmd(ADS1256_SYNC);                      // Trigger new conversion
         ads1256_cmd(ADS1256_WAKEUP);
   
         /* Wait for /DRDY with timeout */
         for (d=0 ; d<100000 ; d++) {
            if (OPT_STAT == 0)
               break;
            DELAY_US(1);
         }
         if ((status & 1) > 0)
            return 0;

         /* read 24-bit data */
         ads1256_read(&d);
   
         /* convert to volts, PGA=2 */
         value = 5*((float)d / (1l<<24));
      
         /* apply range */
         if (id == 0x64)
            value *= 1;
         else if (id == 0x65)
            value = (value/2.5)*20.0 - 10;
         else if (id == 0x66)
            value *= 1;
         else if (id == 0x67)
            value *= 10;
   
         /* round result to significant digits */
         if (id == 0x65|| id == 0x67) {
            d = (unsigned long)(value*1E4+0.5);
            value = d/1E4;
         } else {
            d = (unsigned long)(value*1E5+0.5);
            value = d/1E5;
         }
      
         DISABLE_INTERRUPTS;
         *((float *)pd + chn) = value;
         ENABLE_INTERRUPTS;
      }
   }

   return 1;
}

