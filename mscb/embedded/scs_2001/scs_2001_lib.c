/********************************************************************\

  Name:         scs_2001_lib.c
  Created by:   Stefan Ritt


  Contents:     Library of plug-in modules for SCS-2001

  $Id: scs_2001_lib.c 4169 2008-04-03 13:35:06Z ritt@PSI.CH $

\********************************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <intrins.h>
#include "mscbemb.h"
#include "scs_2001.h"

char code svn_rev_lib[] = "$Rev: 4169 $";

#define N_PORT 24 /* maximal one master and two slave modules */

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

MSCB_INFO_VAR code vars_bout[] =
   { 1, UNIT_BYTE,    0,          0,           0, "P%Out",   (void xdata *)1, 0, 0, 255, 1 };

MSCB_INFO_VAR code vars_uin[] =
   { 4, UNIT_VOLT,    0,          0, MSCBF_FLOAT, "P%Uin#",  (void xdata *)8, 4 };

MSCB_INFO_VAR code vars_diffin[] =
   { 4, UNIT_VOLT,    0,          0, MSCBF_FLOAT, "P%Uin#",  (void xdata *)4, 4 };

MSCB_INFO_VAR code vars_uin_range[] = {
   { 4, UNIT_VOLT,    0,          0, MSCBF_FLOAT, "P%Uin#",  (void xdata *)8, 4 },
   { 1, UNIT_BYTE,    0,          0, MSCBF_HIDDEN,"P%Range", (void xdata *)0 },
};

MSCB_INFO_VAR code vars_uout[] =
   { 4, UNIT_VOLT,    0,          0, MSCBF_FLOAT, "P%Uout#", (void xdata *)8, 2, 0, 30,  0.5 };

MSCB_INFO_VAR code vars_iin[] =
   { 4, UNIT_AMPERE,  PRFX_MILLI, 0, MSCBF_FLOAT, "P%Iin#",  (void xdata *)8, 4 };

MSCB_INFO_VAR code vars_cin[] =
   { 4, UNIT_FARAD,   PRFX_NANO,  0, MSCBF_FLOAT, "P%Cin#",  (void xdata *)4, 4 };

MSCB_INFO_VAR code vars_temp[] = {
   { 4, UNIT_CELSIUS, 0,          0, MSCBF_FLOAT, "P%T#",    (void xdata *)8, 2, 0, 0 },
   { 4, UNIT_AMPERE,  PRFX_MILLI, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "P%Excit", (void xdata *)0, 1 },
};

MSCB_INFO_VAR code vars_iout[] =
   { 4, UNIT_AMPERE,  PRFX_MILLI, 0, MSCBF_FLOAT, "P%Iout#", (void xdata *)8, 2, 0, 20,  0.1 };

MSCB_INFO_VAR code vars_lhe[] = {
   { 4, UNIT_AMPERE,  PRFX_MILLI, 0, MSCBF_FLOAT | MSCBF_HIDDEN, "P%Excit", (void xdata *)0, 1, 0, 100,  1 },
   { 4, UNIT_PERCENT, 0,          0, MSCBF_FLOAT | MSCBF_HIDDEN, "P%LOfs",  (void xdata *)0, 2 },
   { 4, UNIT_FACTOR,  0,          0, MSCBF_FLOAT | MSCBF_HIDDEN, "P%LGain",  (void xdata *)0, 2 },
   { 2, UNIT_SECOND,  0,          0, MSCBF_HIDDEN, "P%Toff",  (void xdata *)0, 0 },
   { 2, UNIT_SECOND,  0,          0, MSCBF_HIDDEN, "P%Ton",   (void xdata *)0, 0 },

   { 4, UNIT_PERCENT, 0,          0, MSCBF_FLOAT, "P%Lin",  (void xdata *)0, 1 },
};

MSCB_INFO_VAR code vars_din[] =
   { 1, UNIT_BOOLEAN, 0,          0,           0, "P%Din#",  (void xdata *)8 };

MSCB_INFO_VAR code vars_optin[] =
   { 1, UNIT_BOOLEAN, 0,          0,           0, "P%Din#",  (void xdata *)4 };

MSCB_INFO_VAR code vars_dout[] =
   { 1, UNIT_BOOLEAN, 0,          0,           0, "P%Dout#", (void xdata *)8, 0,  1,  1 };

MSCB_INFO_VAR code vars_relais[] =
   { 1, UNIT_BOOLEAN, 0,          0,           0, "P%Rel#",  (void xdata *)4, 0,  1,  1 };

MSCB_INFO_VAR code vars_optout[] =
   { 1, UNIT_BOOLEAN, 0,          0,           0, "P%Out#",  (void xdata *)4, 0,  1,  1 };

SCS_2001_MODULE code scs_2001_module[] = {
  /* 0x01-0x1F misc. */
  { 0x01, "LED-Debug",       vars_bout,   1, dr_dout_byte   },
  { 0x02, "LED-Pulser",      NULL,        1, NULL           },

  /* 0x20-0x3F digital in  */
  { 0x20, "Din 5V",          vars_din,    1, dr_din_bits    },
  { 0x21, "OptIn",           vars_optin,  1, dr_din_bits    },
  { 0x22, "CPLD",            vars_din,    1, dr_din_bits    },

  /* 0x40-0x5F digital out */
  { 0x40, "Dout 5V/24V",     vars_dout,   1, dr_dout_bits   },
  { 0x41, "Relais",          vars_relais, 1, dr_dout_bits   },
  { 0x42, "OptOut",          vars_optout, 1, dr_dout_bits   },

  /* 0x60-0x7F analog in  */
  { 0x60, "Uin 0-2.5V",      vars_uin,    1, dr_ad7718      },
  { 0x61, "Uin +-10V",       vars_uin,    1, dr_ad7718      },
  { 0x62, "Iin 0-2.5mA",     vars_iin,    1, dr_ad7718      },
  { 0x63, "Iin 0-25mA",      vars_iin,    1, dr_ad7718      },

  { 0x64, "Uin 0-2.5V Fst",  vars_uin,    1, dr_ads1256     },
  { 0x65, "Uin +-10V Fast",  vars_uin,    1, dr_ads1256     },
  { 0x66, "Iin 0-2.5mA Fst", vars_iin,    1, dr_ads1256     },
  { 0x67, "Iin 0-25mA Fst",  vars_iin,    1, dr_ads1256     },
  { 0x68, "Uin +-10V Diff",  vars_diffin, 1, dr_ad7718      },

  { 0x70, "Cin 0-10nF",      vars_cin,    1, dr_capmeter    },
  { 0x71, "Cin 0-1uF",       vars_cin,    1, dr_capmeter    },

  { 0x72, "PT100",           vars_temp,   2, dr_temp        },
  { 0x73, "PT1000",          vars_temp,   2, dr_temp        },

  /* 0x80-0x9F analog out */
  { 0x80, "UOut 0-2.5V",     vars_uout,   1, dr_ltc2600     },
  { 0x81, "UOut +-10V",      vars_uout,   1, dr_ad5764      },
  { 0x82, "IOut 0-2.5mA",    vars_iout,   1, dr_ltc2600     },
  { 0x83, "IOut 0-25mA",     vars_iout,   1, dr_ltc2600     },
  { 0x84, "Liq.He level",    vars_lhe,    5, dr_lhe         },

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
#define AM_WRITE_DIR    3 // set port direction (1=output, 0=input)
#define AM_READ_CSR     4 // read CPLD CSR:  bit0: beeper; bit4-6: firmware; bit7:slave
#define AM_WRITE_CSR    5 // write CPLD CSR
#define AM_RW_SERIAL    6 // read/write to serial device on port
#define AM_RW_EEPROM    7 // read/write to eeprom on port
#define AM_RW_MONITOR   8 // read/write to power monitor MAX1253

/* address port 0-7 in CPLD, with address modifier listed above */
void address_port1(unsigned char addr, unsigned char port_no, unsigned char am, 
                  unsigned char clk_level)
{
unsigned char xdata i;

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
   for (i=0 ; i<4 ; i++) {
      OPT_DATAO = ((am << i) & 8) > 0;
      CLOCK;
   }

   STROBE;
   OPT_DATAO = 0;
   OPT_CLK = clk_level;
   OPT_STR = 1;         // keeps /CS high
   OPT_ALE = 0;
   DELAY_CLK;
   OPT_STR = 0;         // set /CS low
}

/* address port 0-7 in CPLD, with address modifier listed above */
void address_port(unsigned char addr, unsigned char port_no, unsigned char am)
{
   address_port1(addr, port_no, am, 0); // default values for clk level
}

/* write 8 bits to port directio (1=output, 0=input */
void write_dir(unsigned char addr, unsigned char port_no, unsigned char d)
{
unsigned char xdata i;

   address_port(addr, port_no, AM_WRITE_DIR);

   /* send data */
   for (i=0 ; i<8 ; i++) {
      OPT_DATAO = (d & 0x80) > 0;
      CLOCK;
      d <<= 1;
   }
   OPT_DATAO = 0;
   STROBE;
}

/* write 8 bits to port output */
void write_port(unsigned char addr, unsigned char port_no, unsigned char d)
{
unsigned char xdata i;

   address_port(addr, port_no, AM_WRITE_PORT);

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
void read_port(unsigned char addr, unsigned char port_no, unsigned char *pd)
{
unsigned char xdata i, d;

   address_port(addr, port_no, AM_READ_PORT);

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
void read_port_reg(unsigned char addr, unsigned char port_no, unsigned char *pd)
{
unsigned char xdata i, d;

   address_port(addr, port_no, AM_READ_REG);

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
void write_csr(unsigned char addr, unsigned char d)
{
unsigned char xdata i;

   address_port(addr, 0, AM_WRITE_CSR);

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
void read_csr(unsigned char addr, unsigned char *pd)
{
unsigned char xdata i, d;

   address_port(addr, 0, AM_READ_CSR);

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
unsigned char module_present(unsigned char addr, unsigned char port_no)
{
unsigned char xdata id;

   read_eeprom(addr, port_no, &id);
   return id > 0;
}

/* read 8 bits from eeprom on specific port */
void read_eeprom(unsigned char addr, unsigned char port_no, unsigned char *pd)
{
unsigned char xdata i;
unsigned short xdata d;

   address_port(addr, port_no, AM_RW_EEPROM);

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

   /* check if EEPROM present */
   if (OPT_DATAI == 1) {
      *pd = 0;
      return;
   }

   /* read 16 bit data */
   for (i=0 ; i<16 ; i++) {
      CLOCK;
      d = (d << 1) | OPT_DATAI;
   }

   /* return only lower 8 bits */
   *pd = (d >> 8);
}

/* verify that certain module is plugged into specific port */
unsigned char verify_module(unsigned char addr, unsigned char port_no, unsigned char id)
{
unsigned char xdata real_id, xaddr, xport_no, xid;

   xaddr = addr;
   xport_no = port_no;
   xid = id;
   if (!module_present(xaddr, xport_no))
      return 0;

   read_eeprom(xaddr, xport_no, &real_id);
   return real_id == xid;
}

/* write 8 bits to eeprom on specific port */
void write_eeprom(unsigned char addr, unsigned char port_no, unsigned char d)
{
unsigned char xdata i;
unsigned short xdata t;

   address_port(addr, port_no, AM_RW_EEPROM);

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
      DELAY_US_REENTRANT(1);
   }

}

/* read 16 bits from MAX1253 power monitor */
void monitor_read(unsigned char uaddr, unsigned char cmd, unsigned char raddr, unsigned char *pd, unsigned char nbytes)
{
unsigned char xdata i;

   /* address monitor on unit address */
   address_port(uaddr, 0, AM_RW_MONITOR);

   /* write command */
   for (i=0 ; i<4 ; i++) {
      OPT_DATAO = (cmd & 0x08) > 0;
      CLOCK;
      cmd <<= 1;
   }

   /* write register address */
   for (i=0 ; i<4 ; i++) {
      OPT_DATAO = (raddr & 0x08) > 0;
      CLOCK;
      raddr <<= 1;
   }

   /* read data */
   for (i=0 ; i<nbytes*8 ; i++) {
      if (i>0 && i%8 == 0)
         pd++;
      OPT_CLK = 0;
      DELAY_CLK;
      *pd = (*pd << 1) | OPT_DATAI;
      OPT_CLK = 1; 
      DELAY_CLK; 
   }

   /* remove CS */
   OPT_STR = 1;
   DELAY_CLK;
}

/* address certain register in MAX1253 power monitor */
void monitor_address(unsigned char uaddr, unsigned char cmd, unsigned char raddr)
{
unsigned char xdata i;

   /* remove any previous CS */
   OPT_STR = 1;
   DELAY_CLK;
   OPT_STR = 0;
   DELAY_CLK;

   /* address monitor on unit address */
   address_port(uaddr, 0, AM_RW_MONITOR);

   /* write command */
   for (i=0 ; i<4 ; i++) {
      OPT_DATAO = (cmd & 0x08) > 0;
      CLOCK;
      cmd <<= 1;
   }

   /* write register address */
   for (i=0 ; i<4 ; i++) {
      OPT_DATAO = (raddr & 0x08) > 0;
      CLOCK;
      raddr <<= 1;
   }
}

/* write single byte to MAX1253 power monitor */
void monitor_write(unsigned char d)
{
unsigned char xdata i;

   /* write 8 bit data */
   for (i=0 ; i<8 ; i++) {
      OPT_DATAO = (d & 0x80) > 0;
      CLOCK;
      d <<= 1;
   }
}

void monitor_init(unsigned char addr)
{
   unsigned char xdata i;
   unsigned short xdata d;

   monitor_address(addr, 0x07, 0); // Reset

   monitor_address(addr, 0x0A, 0); // Write data register 0
   monitor_write(0x00); // 0 deg. C
   monitor_write(0x00);

   monitor_address(addr, 0x0A, 2); // Write data register 2
   monitor_write(0x00); // 0V
   monitor_write(0x00);

   for (i=0 ; i<7 ; i++) {
      monitor_address(addr, 0x0C, i); // Write channel configuration registers

      if (i==0)
         d = 100.0*8; // 100 deg. C
      else if (i==2)
         d = (unsigned short) (1.5/14000.0*2700.0/2.5*4096.0); // 1.5A limit
      else
         d = 0xFFF;

      monitor_write(d >> 4); // Upper threshold
      monitor_write((d & 0x0F) << 4);
      monitor_write(0x00);   // Lower threshold
      monitor_write(0x00);
      monitor_write(0x3A);   // 4 faults, 1024 averaging
   }

   monitor_address(addr, 0x0D, 0); // Write global configuration registers
   monitor_write(0xFF); // Enable all channels except AIN5-AIN7
   monitor_write(0xFF);
   monitor_write(0x00); // Single ended input
   monitor_write(0x00);
   monitor_write(0x14); // INT push-pull active low, auto scan, external reference
}

void monitor_clear(unsigned char addr)
{
   monitor_address(addr, 0x09, 0); // Clear alarm for all channels
   /* remove CS */
   OPT_STR = 1;
   DELAY_CLK;
}

void power_beeper(unsigned char addr, unsigned char flag)
{
   unsigned char xdata status;

   read_csr(addr, &status);

   if (flag)
      status |= 0x01;
   else
      status &= ~0x01;

   write_csr(addr, status);

}

unsigned char is_master()
{
   unsigned char xdata status;

   read_csr(0, &status);
   if (status & 0x80)
      return 0;

   return 1;
}

unsigned char slave_addr()
{
   unsigned char xdata status;

   read_csr(0, &status);
   if (status & 0x80)
      return status & 0x0F;

   return 0;
}

unsigned char is_present(unsigned char addr)
{
   unsigned char xdata status;

   read_csr(addr, &status);
   return status != 0xFF && status != 0x00;
}

/*---- Bitwise output/input ----------------------------------------*/

unsigned char xdata pattern[N_PORT];

unsigned char dr_dout_bits(unsigned char id, unsigned char cmd, unsigned char addr, 
                           unsigned char port, unsigned char chn, void *pd) reentrant
{
unsigned char d;

   if (id || chn || pd); // suppress compiler warning

   if (cmd == MC_INIT) {

      /* default pattern */
      pattern[addr*8+port] = 0;

      /* retrieve previous pattern if not reset by power on */
      SFRPAGE = LEGACY_PAGE;
      if ((RSTSRC & 0x02) == 0)
         read_port_reg(addr, port, &pattern[addr*8+port]);

      /* set default value before switching to putput */   
      write_port(addr, port, pattern[addr*8+port]);

      /* switch port to output */
      write_dir(addr, port, 0xFF);
   }

   if (cmd == MC_WRITE) {

      d = *((unsigned char *)pd);

      if (d)
         pattern[addr*8+port] |= (1 << chn);
      else
         pattern[addr*8+port] &= ~(1 << chn);

      write_port(addr, port, pattern[addr*8+port]);
   }

   return 0;   
}

unsigned char dr_din_bits(unsigned char id, unsigned char cmd, unsigned char addr, 
                          unsigned char port, unsigned char chn, void *pd) reentrant
{
unsigned char d;

   if (id || chn || pd); // suppress compiler warning

   if (cmd == MC_READ) {
      if (*((unsigned char *)pd))
         pattern[addr*8+port] |= (1 << chn);
      else
         pattern[addr*8+port] &= ~(1 << chn);

      read_port(addr, port, &d);
      *((unsigned char *)pd) = (d & (1 << chn)) > 0;
      return 1;
   }

   return 0;   
}

unsigned char dr_dout_byte(unsigned char id, unsigned char cmd, unsigned char addr, 
                           unsigned char port, unsigned char chn, void *pd) reentrant
{
   if (id || chn || pd); // suppress compiler warning

   if (cmd == MC_INIT) {
      /* switch port to output */
      write_dir(addr, port, 0xFF);
   }

   if (cmd == MC_WRITE)
      write_port(addr, port, *((unsigned char *)pd));

   return 0;   
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
      if (id == 0x02)
         d = value/100 * 65535;  // 0-100%
      else if (id == 0x82)
         d = value/2.5 * 65535;  // 0-2.5mA
      else if (id == 0x83)
         d = value/25.0 * 65535; // 0-25mA
   
      address_port(addr, port, AM_RW_SERIAL);
    
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

   return 0;
}

/*---- AD5764 16-bit DAC -------------------------------------------*/

unsigned char dr_ad5764(unsigned char id, unsigned char cmd, unsigned char addr, 
                        unsigned char port, unsigned char chn, void *pd) reentrant
{
float value;
unsigned short d;
unsigned char i;

   if (id);

   if (cmd == MC_WRITE) {

      value = *((float *)pd);

      if (value > 10)
         value = 10;
      if (value < -10)
         value = -10;
   
      /* convert value to DAC counts */
      d = (value+10)/20 * 65535 + 0.5;  // +-10V
   
      address_port(addr, port, AM_RW_SERIAL);
    
      /* R/!W = 0 */
      OPT_DATAO = 0;
      CLOCK;
   
      /* DB22 = 0 */
      OPT_DATAO = 0;
      CLOCK;

      /*---- first DAC ----*/

      if (chn < 4) {
         /* DAC get NOP */
         for (i=0 ; i<22 ; i++)
            CLOCK;
      } else {
         /* REG2 = 0 */
         OPT_DATAO = 0;
         CLOCK;
         /* REG1 = 1 */
         OPT_DATAO = 1;
         CLOCK;
         /* REG0 = 0 */
         OPT_DATAO = 0;
         CLOCK;

         /* A2 = 0 */
         OPT_DATAO = 0;
         CLOCK;
         /* A1 */
         OPT_DATAO = (chn & 0x02) > 0;
         CLOCK;
         /* A0 */
         OPT_DATAO = (chn & 0x01) > 0;
         CLOCK;
      
         for (i=0 ; i<16 ; i++) {
            OPT_DATAO = (d & 0x8000) > 0;
            CLOCK;
            d <<= 1;
         }
      }
   
      /*---- second DAC ----*/

      /* R/!W = 0 */
      OPT_DATAO = 0;
      CLOCK;
   
      /* DB22 = 0 */
      OPT_DATAO = 0;
      CLOCK;

      if (chn > 3) {
         /* DAC get NOP */
         for (i=0 ; i<22 ; i++)
            CLOCK;
      } else {
         /* REG2 = 0 */
         OPT_DATAO = 0;
         CLOCK;
         /* REG1 = 1 */
         OPT_DATAO = 1;
         CLOCK;
         /* REG0 = 0 */
         OPT_DATAO = 0;
         CLOCK;

         /* A2 = 0 */
         OPT_DATAO = 0;
         CLOCK;
         /* A1 */
         OPT_DATAO = (chn & 0x02) > 0;
         CLOCK;
         /* A0 */
         OPT_DATAO = (chn & 0x01) > 0;
         CLOCK;
      
         for (i=0 ; i<16 ; i++) {
            OPT_DATAO = (d & 0x8000) > 0;
            CLOCK;
            d <<= 1;
         }
      }

      /* remove CS to update both DAC outputs */
      OPT_ALE = 1; 
   }

   return 0;
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

unsigned char xdata ad7718_cur_chn[N_PORT];
unsigned char xdata ad7718_range[N_PORT];
float code ad7718_full_range[] = { 0.02, 0.04, 0.08, 0.16, 0.32, 0.64, 1.28, 2.56 };

unsigned char dr_ad7718(unsigned char id, unsigned char cmd, unsigned char addr, 
                        unsigned char port, unsigned char chn, void *pd) reentrant
{
float value;
unsigned long d;
unsigned char status;

   if (cmd == MC_INIT) {
      address_port1(addr, port, AM_RW_SERIAL, 1);
      ad7718_write(AD7718_FILTER, 82);                // SF value for 50Hz rejection (2 Hz)
      //ad7718_write(AD7718_FILTER, 13);                // SF value for faster conversion (12 Hz)
      ad7718_write(AD7718_MODE, 3);                   // continuous conversion
      DELAY_US_REENTRANT(100);

      /* default range */
      ad7718_range[addr*8+port] = 0x07;               // +2.56V range

      /* start first conversion */
      ad7718_write(AD7718_CONTROL, (0 << 4) | 0x08 | ad7718_range[addr*8+port]);  // Channel 0, unipolar
      ad7718_cur_chn[addr*8+port] = 0;
   }

   if (cmd == MC_WRITE && chn == 8) {
      ad7718_range[addr*8+port] = *((unsigned char *)pd);
   }

   if (cmd == MC_READ) {

      if (chn > 7)
         return 0;

      /* return if ADC busy */
      read_port(addr, port, &status);
      if ((status & 1) > 0)
         return 0;

      /* return if not current channel */
      if (chn != ad7718_cur_chn[addr*8+port])
         return 0;

      address_port1(addr, port, AM_RW_SERIAL, 1);
    
      /* read 24-bit data */
      ad7718_read(AD7718_ADCDATA, &d);

      /* start next conversion */
      if (id == 0x68)
         ad7718_cur_chn[addr*8+port] = (ad7718_cur_chn[addr*8+port] + 1) % 4;
      else
         ad7718_cur_chn[addr*8+port] = (ad7718_cur_chn[addr*8+port] + 1) % 8;

      ad7718_write(AD7718_CONTROL, (ad7718_cur_chn[addr*8+port] << 4) | 0x08 | ad7718_range[addr*8+port]);

      /* convert to volts */
      value = ad7718_full_range[ad7718_range[addr*8+port]]*((float)d / (1l<<24));
   
      /* apply range */
      if (id == 0x60)
         value *= 1;
      else if (id == 0x61)
         value = (value/2.5)*20.0 - 10;
      else if (id == 0x62)
         value *= 1;
      else if (id == 0x63)
         value *= 10;
      else if (id == 0x68)
         value = (value/2.5)*20.0 - 10;

      /* round result to significant digits */
      if (id == 0x61 || id == 0x63 || id == 0x68) {
         value = (long)(value*1E5+0.5)/1E5;
      } else {
         value = (long)(value*1E6+0.5)/1E6;
      }

      *((float *)pd) = value;
      return 4;
   }

   return 0;
}

/*---- PT100/1000 via AD7718 ---------------------------------------*/

static float xdata exc_current[N_PORT];
static float xdata temp_base_value[N_PORT];
static unsigned char xdata temp_cur_chn[N_PORT];

unsigned char dr_temp(unsigned char id, unsigned char cmd, unsigned char addr, 
                      unsigned char port, unsigned char chn, void *pd) reentrant
{
float value, new_base_value;
unsigned char status;
unsigned long d;

   if (cmd == MC_INIT) {
      address_port1(addr, port, AM_RW_SERIAL, 1);
      ad7718_write(AD7718_FILTER, 82);                // SF value for 50Hz rejection (2 Hz)
      //ad7718_write(AD7718_FILTER, 13);                // SF value for faster conversion (12 Hz)
      ad7718_write(AD7718_MODE, 3);                   // continuous conversion
      DELAY_US_REENTRANT(100);

      /* start first conversion */
      ad7718_write(AD7718_CONTROL, (0 << 4) | 0x0F);  // Channel 0, +2.56V range
      temp_cur_chn[addr*8+port] = 0;
      exc_current[port] = 1;
   }

   if (cmd == MC_WRITE && chn == 8) {
      exc_current[port] = *((float *)pd);
      if (exc_current[port] < 0.01 || exc_current[port] > 10) {
         exc_current[port] = 1;
         *((float *)pd) = 1;
      }
   }

   if (cmd == MC_READ) {

      if (chn > 7)
         return 0;

      /* return if ADC busy */
      read_port(addr, port, &status);
      if ((status & 1) > 0)
         return 0;

      /* return if not current channel */
      if (chn != temp_cur_chn[addr*8+port])
         return 0;

      address_port1(addr, port, AM_RW_SERIAL, 1);
    
      /* read 24-bit data */
      ad7718_read(AD7718_ADCDATA, &d);

      /* start next conversion */
      temp_cur_chn[addr*8+port] = (temp_cur_chn[addr*8+port] + 1) % 8;
      ad7718_write(AD7718_CONTROL, (temp_cur_chn[addr*8+port] << 4) | 0x0F);  // next chn, +2.56V range

      /* convert to volts */
      value = 2.56*((float)d / (1l<<24));

      new_base_value = value;

      /* subtract base value */
      if (chn > 0)
         value -= temp_base_value[addr*8+port];

      /* remember current value as next base value */
      temp_base_value[addr*8+port] = new_base_value;

      /* convert to Ohms (1mA excitation) */
      value /= (exc_current[port]*0.001);

      /* convert PT1000 to PT100 ??? */
      if (id == 0x73)
         value /= 10;

      /* convert to Kelvin, coefficients obtained from table fit (www.lakeshore.com) */
      value = 5.232935E-7 * value * value * value + 
              0.0009 * value * value + 
              2.357 * value + 28.288;

      /* convert to Celsius */
      value -= 273.15;

      /* round result to significant digits */
      value = ((long)(value*1E1+0.5))/1E1;
   
      if (value > 999)
         value = 999.9;
      if (value < -200)
         value = -999.9;

      *((float *)pd) = value;
      return 4;
   }

   return 1;
}

/*---- ADS1256 24-bit fast sigma-delta ADC -------------------------*/

unsigned char xdata ads1256_cur_chn[N_PORT];

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

   DELAY_US(5); // Datasheet: 50 tau_clkin = 1/8MHz * 50

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

unsigned char dr_ads1256(unsigned char id, unsigned char cmd, unsigned char addr, 
                         unsigned char port, unsigned char chn, void *pd) reentrant
{
float value;
unsigned long d;

   if (cmd == MC_INIT) {
      address_port(addr, port, AM_RW_SERIAL);

      ads1256_write(ADS1256_STATUS, 0x02);  // Enable buffer
      ads1256_write(ADS1256_ADCON,  0x01);  // Clock Out OFF, PGA=2

      //ads1256_write(ADS1256_DRATE,  0x23);  // 10 SPS
      //ads1256_write(ADS1256_DRATE,  0x82);  // 100 SPS
      //ads1256_write(ADS1256_DRATE,  0xA1);  // 1000 SPS
      //ads1256_write(ADS1256_DRATE,  0xB0);  // 2000 SPS
      //ads1256_write(ADS1256_DRATE,  0xC0);  // 3750 SPS
      ads1256_write(ADS1256_DRATE,  0xD0);  // 7500 SPS
      //ads1256_write(ADS1256_DRATE,  0xF0);  // 30000 SPS

      /* do self calibration */
      ads1256_cmd(ADS1256_SELFCAL);

      /* wait for /DRDY to go low after self calibration */
      for (d = 0 ; d<100000 ; d++) {
         if (OPT_STAT == 0)
            break;
         DELAY_US_REENTRANT(1);
      }

      /* start first conversion */
      ads1256_write(ADS1256_MUX, (0 << 4) | 0x0F);
      ads1256_cmd(ADS1256_SYNC);
      ads1256_cmd(ADS1256_WAKEUP);
      ads1256_cur_chn[addr*8+port] = 0;
   }

   if (cmd == MC_READ) {

      if (chn > 7)
         return 0;

      address_port(addr, port, AM_RW_SERIAL);

      /* wait for /DRDY to go low after conversion */
      for (d = 0 ; d<10000 ; d++) {
         if (OPT_STAT == 0)
            break;
         DELAY_US_REENTRANT(1);
      }
      if (d == 10000)
         return 0;

      /* return if not current channel */
      if (chn != ads1256_cur_chn[addr*8+port])
         return 0;

      /* start next conversion on next channel */
      ads1256_cur_chn[addr*8+port] = (ads1256_cur_chn[addr*8+port] + 1) % 8;

      ads1256_write(ADS1256_MUX, (ads1256_cur_chn[addr*8+port] << 4) | 0x0F); // Select single ended positive input
      ads1256_cmd(ADS1256_SYNC);  // Trigger new conversion
      ads1256_cmd(ADS1256_WAKEUP);

      /* read 24-bit data */
      ads1256_read(&d);

      /* expand to signed long (32-bit) */
      value = (signed long)d << 8;

      /* convert to volts, PGA=2 (+-2.5V range) */
      value = 5*(value / 0xFFFFFFFF);
   
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
         value = ((long)(value*1E4+0.5))/1E4;
      } else {
         value = ((long)(value*1E5+0.5))/1E5;
      }
   
      *((float *)pd) = value;
      return 4;
   }

   return 1;
}

/*---- Capacitance meter via TLC555 timer IC -----------------------*/

unsigned char dr_capmeter(unsigned char id, unsigned char cmd, unsigned char addr, 
                          unsigned char port, unsigned char chn, void *pd) reentrant
{
unsigned char i, b, m;
unsigned long v;
float c;

   if (cmd == MC_READ) {

      if (chn > 3)
         return 0;

      m = (1 << chn);

      SFRPAGE = TMR3_PAGE;
      /* average 10 times */
      for (i = v = 0 ; i < 10 ; i++) {
   
         /* init timer 3 */
         TMR3L = 0;
         TMR3H = 0;
   
         TMR3CF = 0;               // SYSCLK/12
         TMR3CN = (1 << 2);        // Enable Timer 3
   
         /* trigger NE555 */
         address_port(addr, port, AM_RW_SERIAL); // CS0 low
         address_port(addr, port, AM_READ_PORT); // CS0 high
   
         /* wait until NE555 output goes low again */
         do
            {
            watchdog_refresh(0);
            read_port(addr, port, &b);
         } while ((b & m) != 0x00 && (TMR3CN & 0x80) == 0);
   
         /* stop timer 3 */
         TMR3CN = 0;

         /* accumulate timers */
         v += TMR3H * 256 + TMR3L;
      }
   
      /* check for overvlow */
      if ((TMR3CN & 0x80) == 1) {
         if (id == 0x70)
            c = 9.999999;
         else
            c = 999.9999;
      } else {

         /* time in us, clock = 98MHz/12 */
         c = (float) v / i / 98 * 12;
      
         /* time in s */
         c /= 1E6;
      
         /* convert to nF via t = 1.1 * R * C */
         if (id == 0x70) {
            c = c / 1.1 / 560000 * 1E9;  // 560k Ohm
            c -= 0.078;                  // general offset 78pF
         } else {
            c = c / 1.1 / 5600 * 1E9;    // 5600 Ohm
            c -= 7.8;                    // general offset 7.8nF
         }
      
         /* stip off unsignificant digits */
         c = ((long) (c * 1000 + 0.5)) / 1000.0;
      }
      
      *((float *)pd) = c;
      return 4;
   }

   return 1;
}

/*---- Liquid helium level meter --------*/

static float xdata lhe_excit[N_PORT];
static float xdata lhe_ofs[N_PORT];
static float xdata lhe_gain[N_PORT];
static unsigned short xdata lhe_toff[N_PORT];
static unsigned short xdata lhe_ton[N_PORT];
static unsigned long xdata lhe_last[N_PORT];
static unsigned char xdata lhe_on[N_PORT];

unsigned char dr_lhe(unsigned char id, unsigned char cmd, unsigned char addr, 
                     unsigned char port, unsigned char chn, void *pd) reentrant
{
float value;
unsigned char status, b;
unsigned long d;
unsigned short s;
unsigned char i, idx;


   if (id || chn || pd); // suppress compiler warning

   idx = addr*8 + port;

   if (cmd == MC_GETDEFAULT) {
      if (chn == 0) {
         lhe_excit[idx] = 75;
         *((float *)pd) = 75;
      } else if (chn == 1) {
         lhe_ofs[idx] = 0;
         *((float *)pd) = 0;
      } else if (chn == 2) {
         lhe_gain[idx] = 1;
         *((float *)pd) = 1;
      } else if (chn == 3) {
         lhe_toff[idx] = 60;
         *((unsigned short *)pd) = 60;
      } else if (chn == 4) {
         lhe_ton[idx] = 5;
         *((unsigned short *)pd) = 5;
      }
      return 0;
   }

   /* handle switching on/off */
   if (lhe_on[idx] == 0) {
      if (time() - lhe_last[idx] > lhe_toff[idx]*100) {
         lhe_on[idx] = 1;
         lhe_last[idx] = time();
         value = lhe_excit[idx];
         dr_lhe(id, MC_WRITE, addr, port, 0, &value);
      }
   } else {
      if (time() - lhe_last[idx] > lhe_ton[idx]*100) {
         lhe_on[idx] = 0;
         lhe_last[idx] = time();
         value = 0;
         dr_lhe(id, MC_WRITE, addr, port, 0, &value);
      }
   }

   if (cmd == MC_INIT) {
      /* configure serial interface to LT2600 */
      write_dir(addr, port, 0x0E);  // bit0: input (/RDY); 
                                    // bit1: /CS2
                                    // bit2: SDI2
                                    // bit3: CLK2
      write_port(addr, port, 0x0E); // all high

      /* configure AD7718 */
      address_port1(addr, port, AM_RW_SERIAL, 1);
      ad7718_write(AD7718_FILTER, 82);                // SF value for 50Hz rejection (2 Hz)
      ad7718_write(AD7718_MODE, 3);                   // continuous conversion
      DELAY_US_REENTRANT(100);

      /* start first conversion */
      ad7718_write(AD7718_CONTROL, (0 << 4) | (0x07));  // Channel 0, Bipolar, +-2.56V range

      lhe_on[idx] = 1;
      lhe_last[idx] = time();
   }

   if (cmd == MC_WRITE) {

      if (chn == 0 && *((float *)pd) > 0)
         lhe_excit[idx] = *((float *)pd);
      else if (chn == 1)
         lhe_ofs[idx] = *((float *)pd);
      else if (chn == 2)
         lhe_gain[idx] = *((float *)pd);
      else if (chn == 3)
         lhe_toff[idx] = *((unsigned short *)pd);
      else if (chn == 4)
         lhe_ton[idx] = *((unsigned short *)pd);

      if (chn > 0)
         return 0;

      value = *((float *)pd);
   
      if (value < 0)
         value = 0;
      if (value > 100)
         value = 100;

      /* convert value to DAC counts */
      s = value/100.0 * 65535; // 0-100mA
   
      write_port(addr, port, 0); // /CS2 active

      /* EWEN command */
      for (i=0 ; i<4 ; i++) {
         b = (i > 1)?(1<<2):0;               // SDI2
         write_port(addr, port, b);
         write_port(addr, port, b | (1<<3)); // CLK2 high
         write_port(addr, port, b);          // CLK2 low
      }
   
      for (i=0 ; i<4 ; i++) {
         b = ((chn & 8) > 0)?(1<<2):0;       // SDI2
         write_port(addr, port, b);
         write_port(addr, port, b | (1<<3)); // CLK2 high
         write_port(addr, port, b);          // CLK2 low
         chn <<= 1;
      }
   
      for (i=0 ; i<16 ; i++) {
         b = ((s & 0x8000) > 0)?(1<<2):0;       // SDI2
         write_port(addr, port, b);
         write_port(addr, port, b | (1<<3)); // CLK2 high
         write_port(addr, port, b);          // CLK2 low
         s <<= 1;
      }

      write_port(addr, port, 0x0E); // /CS2 inactive
   }

   if (cmd == MC_READ) {

      if (chn == 0) {
         *((float *)pd) = lhe_excit[idx];
         return 4;
      } else if (chn == 1) {
         *((float *)pd) = lhe_ofs[idx];
         return 4;
      } else if (chn == 2) {
         *((float *)pd) = lhe_gain[idx];
         return 4;
      } else if (chn == 3) {
         *((unsigned short *)pd) = lhe_toff[idx];
         return 2;
      } else if (chn == 4) {
         *((unsigned short *)pd) = lhe_ton[idx];
         return 2;
      }

      if (chn != 4)
         return 0;

      /* return if ADC busy */
      read_port(addr, port, &status);
      if ((status & 1) > 0)
         return 0;

      address_port1(addr, port, AM_RW_SERIAL, 1);
    
      /* read 24-bit data */
      ad7718_read(AD7718_ADCDATA, &d);

      /* start next conversion */
      ad7718_write(AD7718_CONTROL, (0 << 4) | 0x07);  // Channel 0, bipolar, +-2.56V range

      /* convert to volts */
      value = 5.12*((float)d / (1l<<24))-2.56;

      /* invert input divider */
      value *= (1E6/82E3);

      /* convert to R */
      value = value / (lhe_excit[idx] / 1000.0); 

      /* convert to fill level (0%=0Ohm, 100%=270Ohm, 4.5Ohm/cm) */
      value = 100 - (value / 2.7);

      /* apply calibration */
      value = (value - lhe_ofs[idx]) * lhe_gain[idx];

      /* round result to significant digits */
      value = ((long)(value*1E5+0.5))/1E5;

      /* only measure if on for some time */
      if (lhe_on[idx] == 1 && time() - lhe_last[idx] > 100) {
         *((float *)pd) = value;
         return 4;
      }

      return 0;
   }

   return 1;
}


