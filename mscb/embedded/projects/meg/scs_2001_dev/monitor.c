/********************************************************************\

  Name:         monitor.c
  Created by:   Stefan Ritt


  Contents:     Library of SCS-2001 power monitor and management

  $Id: scs_2001_lib.c 4816 2010-09-09 06:47:30Z ritt $

\********************************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <intrins.h>
#include "mscbemb.h"
#include "scs_2001.h"

/*------------------------------------------------------------------*/

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

/*------------------------------------------------------------------*/

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

/*------------------------------------------------------------------*/

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

/*------------------------------------------------------------------*/

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

/*------------------------------------------------------------------*/

void monitor_clear(unsigned char addr)
{
   monitor_address(addr, 0x09, 0); // Clear alarm for all channels
   /* remove CS */
   OPT_STR = 1;
   DELAY_CLK;
}

/*------------------------------------------------------------------*/

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

/*------------------------------------------------------------------*/

unsigned char is_master()
{
   unsigned char xdata status;

   read_csr(0, &status);
   if (status & 0x80)
      return 0;

   return 1;
}

/*------------------------------------------------------------------*/

unsigned char slave_addr()
{
   unsigned char xdata status;

   read_csr(0, &status);
   if (status & 0x80)
      return status & 0x0F;

   return 0;
}

/*------------------------------------------------------------------*/

unsigned char is_present(unsigned char addr)
{
   unsigned char xdata status;

   read_csr(addr, &status);
   return status != 0xFF && status != 0x00;
}
