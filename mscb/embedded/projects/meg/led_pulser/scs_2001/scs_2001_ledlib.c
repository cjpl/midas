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
#include "mscbemb.h"
#include "scs_2001_led.h"

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
void write_csr(unsigned char addr, unsigned char csr_no, unsigned char d)
{
unsigned char xdata i;

   address_port(addr, csr_no, AM_WRITE_CSR);

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
void read_csr(unsigned char addr, unsigned char csr_no, unsigned char *pd)
{
unsigned char xdata i, d;

   address_port(addr, csr_no, AM_READ_CSR);

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

/*---- LTC2600 16-bit DAC ------------------------------------------*/

unsigned char dr_ltc2600(unsigned char id, unsigned char cmd, unsigned char addr, 
                         unsigned char port, unsigned char chn, void *pd)
{
float value;
unsigned short d;
unsigned char i;

   if (cmd == MC_WRITE) {

      value = *((float *)pd);
   
      /* convert value to DAC counts */
      if (id == 0x02)
         d = value * 65535;      // 0-1
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

   return 1;
}

/*---- LED pulser --------------------------------------------------*/

unsigned char xdata port_reg[8];

unsigned char dr_pulser(unsigned char cmd, unsigned char addr, 
                        unsigned char port, unsigned char chn, void *pd)
{
float value;
unsigned char d;
unsigned long f;

   if (chn || pd); // suppress compiler warning

   if (cmd == MC_INIT) {
      /* clear port register */
      port_reg[port] = 0;
      write_port(addr, port, 0);

      /* set DAC to zero */
      value = 0;
      dr_ltc2600(0x02, MC_WRITE, addr, port, chn, &value);
   }

   if (cmd == MC_WRITE) {
      value = *((float *)pd);
      dr_ltc2600(0x02, MC_WRITE, addr, port, chn, &value);

      /* turn LED pulser on/off */
      if (value == 0)
         port_reg[port] &= ~(1 << chn);
      else
         port_reg[port] |= (1 << chn);
      write_port(addr, port, port_reg[port]);
   }

   if (cmd == MC_FREQ) {
     f = *((unsigned long*) pd);
     write_csr(addr, CSR_PULFREQ0, f & 0xFF);
     f >>= 8;
     write_csr(addr, CSR_PULFREQ1, f & 0xFF);
     f >>= 8;
     write_csr(addr, CSR_PULFREQ2, f & 0xFF);
     f >>= 8;
     write_csr(addr, CSR_PULFREQ3, f & 0xFF);
     f >>= 8;
   }

   if (cmd == MC_PWIDTH) {
     d = *((unsigned char*) pd);
     if (d > 20)
        d = 20;
     write_csr(addr, CSR_PWIDTH, d);
   }

   return 1;   
}


