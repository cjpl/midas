/********************************************************************\

  Name:         rtc.c
  Created by:   Stefan Ritt

  Contents:     Real time clock routines for SCS-2001
                using DS1302 chip

  $Id: rtc.c 4909 2010-12-09 10:47:33Z ritt $

\********************************************************************/

#include <intrins.h>
#include <string.h>
#include <stdio.h>
#include "mscbemb.h"

sbit RTC_IO  = P1 ^ 2;
sbit RTC_CLK = P1 ^ 3;

/*------------------------------------------------------------------*/

void rtc_output(unsigned char d)
{
   unsigned char idata i;

   for (i=0 ; i<8 ; i++) {
      RTC_IO = d & 0x01;
      delay_us(10);
      RTC_CLK = 1;
      delay_us(10);
      RTC_CLK = 0;

      d >>= 1;
   }
}

/*------------------------------------------------------------------*/

unsigned char rtc_read_byte(unsigned char adr)
{
   unsigned char idata i, d, m;

   RTC_CLK = 0;

#ifdef SCS_2000
   SFRPAGE = DAC1_PAGE;
   DAC1L = 0xFF;
   DAC1H = 0x0F;
#else
   SFRPAGE = DAC0_PAGE;
   DAC0L = 0xFF;
   DAC0H = 0x0F;
#endif

   delay_us(10); // wait for DAC

   /* switch port to output */
   SFRPAGE = CONFIG_PAGE;
   P1MDOUT |= 0x04; 

   rtc_output(adr);

   /* switch port to input */
   SFRPAGE = CONFIG_PAGE;
   P1MDOUT &= ~ 0x04;
   RTC_IO = 1;

   delay_us(10);
   for (i=d=0,m=1 ; i<8 ; i++) {
      if (RTC_IO)
         d |= m;
      RTC_CLK = 1;
      delay_us(10);
      RTC_CLK = 0;
      delay_us(10);
      m <<= 1;
   }

#ifdef SCS_2000
   SFRPAGE = DAC1_PAGE;
   DAC1L = 0;
   DAC1H = 0;
#else
   SFRPAGE = DAC0_PAGE;
   DAC0L = 0;
   DAC0H = 0;
#endif

   delay_us(10); // wait for DAC

   return d;
}

/*------------------------------------------------------------------*/

void rtc_read(unsigned char d[6])
{
   unsigned char idata i, j, b, m;

   RTC_CLK = 0;

#ifdef SCS_2000
   SFRPAGE = DAC1_PAGE;
   DAC1L = 0xFF;
   DAC1H = 0x0F;
#else
   SFRPAGE = DAC0_PAGE;
   DAC0L = 0xFF;
   DAC0H = 0x0F;
#endif

   delay_us(10); // wait for DAC

   /* switch port to output */
   SFRPAGE = CONFIG_PAGE;
   P1MDOUT |= 0x04; 

   rtc_output(0xBF); // burst read

   /* switch port to input */
   SFRPAGE = CONFIG_PAGE;
   P1MDOUT &= ~ 0x04;
   RTC_IO = 1;

   delay_us(10); // wait for RTC output

   for (j=0 ; j<7 ; j++) {
      for (i=b=0,m=1 ; i<8 ; i++) {
         if (RTC_IO)
            b |= m;
         RTC_CLK = 1;
         delay_us(10);
         RTC_CLK = 0;
         delay_us(10);
         m <<= 1;
      }

      if (j<3)
        d[5-j] = b;
      else if (j < 5)
        d[j-3] = b;
      else if (j == 6)
        d[2] = b;
   }

#ifdef SCS_2000
   SFRPAGE = DAC1_PAGE;
   DAC1L = 0;
   DAC1H = 0;
#else
   SFRPAGE = DAC0_PAGE;
   DAC0L = 0;
   DAC0H = 0;
#endif

   delay_us(10); // wait for DAC
}

/*------------------------------------------------------------------*/

void rtc_write_byte(unsigned char adr, unsigned char d)
{
   RTC_CLK = 0;

#ifdef SCS_2000
   SFRPAGE = DAC1_PAGE;
   DAC1L = 0xFF;
   DAC1H = 0x0F;
#else
   SFRPAGE = DAC0_PAGE;
   DAC0L = 0xFF;
   DAC0H = 0x0F;
#endif

   delay_us(10); // wait for DAC

   /* switch port to output */
   SFRPAGE = CONFIG_PAGE;
   P1MDOUT |= 0x04; 

   rtc_output(adr);
   rtc_output(d);

#ifdef SCS_2000
   SFRPAGE = DAC1_PAGE;
   DAC1L = 0;
   DAC1H = 0;
#else
   SFRPAGE = DAC0_PAGE;
   DAC0L = 0;
   DAC0H = 0;
#endif

   delay_us(10); // wait for DAC
}

/*------------------------------------------------------------------*/

void rtc_write(unsigned char d[6])
{
   RTC_CLK = 0;

#ifdef SCS_2000
   SFRPAGE = DAC1_PAGE;
   DAC1L = 0xFF;
   DAC1H = 0x0F;
#else
   SFRPAGE = DAC0_PAGE;
   DAC0L = 0xFF;
   DAC0H = 0x0F;
#endif

   delay_us(10); // wait for DAC

   /* switch port to output */
   SFRPAGE = CONFIG_PAGE;
   P1MDOUT |= 0x04; 

   rtc_output(0xBE); // clock burst write

   rtc_output(d[5]);     // sec
   rtc_output(d[4]);     // min
   rtc_output(d[3]);     // hour
   rtc_output(d[0]);     // date
   rtc_output(d[1]);     // month
   rtc_output(1);        // weekday
   rtc_output(d[2]);     // year
   rtc_output(0);        // WP

#ifdef SCS_2000
   SFRPAGE = DAC1_PAGE;
   DAC1L = 0;
   DAC1H = 0;
#else
   SFRPAGE = DAC0_PAGE;
   DAC0L = 0;
   DAC0H = 0;
#endif

   delay_us(10); // wait for DAC
}

/*------------------------------------------------------------------*/

void rtc_write_item(unsigned char item, unsigned char d)
{
   switch (item) {
      case 0: rtc_write_byte(0x86, d); break; // day
      case 1: rtc_write_byte(0x88, d); break; // month
      case 2: rtc_write_byte(0x8C, d); break; // year
      case 3: rtc_write_byte(0x84, d); break; // hour
      case 4: rtc_write_byte(0x82, d); break; // minute
      case 5: rtc_write_byte(0x80, d); break; // second
   }
}

/*------------------------------------------------------------------*/

void rtc_conv_date(unsigned char d[6], char *str)
{
   if (d[0] == 0xFF) { // no clock mounted
      str[0] = str[1] = str[3] = str[4] = str[6] = str[7] = '?';
      str[2] = str[5] = '-';
      str[8] = 0;
      return;
   }
   str[0] = '0'+d[0]/0x10;
   str[1] = '0'+d[0]%0x10;
   str[2] = '-';

   str[3] = '0'+d[1]/0x10;
   str[4] = '0'+d[1]%0x10;
   str[5] = '-';

   str[6] = '0'+d[2]/0x10;
   str[7] = '0'+d[2]%0x10;
   str[8] = 0;
}

/*------------------------------------------------------------------*/

void rtc_conv_time(unsigned char d[6], char *str)
{
   if (d[0] == 0xFF) { // no clock mounted
      str[0] = str[1] = str[3] = str[4] = str[6] = str[7] = '?';
      str[2] = str[5] = ':';
      str[8] = 0;
      return;
   }
   str[0] = '0'+d[3]/0x10;
   str[1] = '0'+d[3]%0x10;
   str[2] = ':';

   str[3] = '0'+d[4]/0x10;
   str[4] = '0'+d[4]%0x10;
   str[5] = ':';

   str[6] = '0'+d[5]/0x10;
   str[7] = '0'+d[5]%0x10;
   str[8] = 0;
}

/*------------------------------------------------------------------*/

void rtc_print()
{
   unsigned char xdata d[6];
   char xdata str[10];

   rtc_read(d);
   if (d[0] != 0xFF) {
      rtc_conv_date(d, str);
      puts(str);
      puts("  ");
      rtc_conv_time(d, str);
      puts(str);
   }
}

/*------------------------------------------------------------------*/

unsigned char rtc_present()
{
   unsigned char idata d;

   d = rtc_read_byte(0);
   return d != 0xFF;
}

/*------------------------------------------------------------------*/

void rtc_init()
{
   /* remove write protection */
   rtc_write_byte(0x8E, 0);
}
