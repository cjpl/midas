/********************************************************************\

  Name:         scd_600.c
  Created by:   Stefan Ritt


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol 
                for SCS-600 Digital I/O

  $Log$
  Revision 1.2  2002/11/22 15:43:03  midas
  Made user_write reentrant

  Revision 1.1  2002/11/20 12:05:14  midas
  Initila revision

  Revision 1.6  2002/10/28 14:26:30  midas
  Changes from Japan

  Revision 1.5  2002/10/22 15:06:18  midas
  Removed temporary OE test

  Revision 1.4  2002/10/16 15:24:38  midas
  Added units in descriptor

  Revision 1.3  2002/10/09 11:06:46  midas
  Protocol version 1.1

  Revision 1.2  2002/10/03 15:31:53  midas
  Various modifications

  Revision 1.1  2002/07/12 15:20:08  midas
  Initial revision

\********************************************************************/

#include <stdio.h>
#include "mscb.h"

extern bit DEBUG_MODE;

char code node_name[] = "SCS-600-P1";


sbit SR_CLOCK   = P0 ^ 2;    // Shift register clock
sbit SR_STROBE  = P0 ^ 5;    // Storage register clock
sbit SR_DATAO   = P0 ^ 6;    // Serial data out
sbit SR_DATAI   = P0 ^ 7;    // Serial data in
sbit SR_OE      = P0 ^ 3;    // Output enable
sbit SR_READB   = P0 ^ 4;    // Serial data readback

/*---- Define channels and configuration parameters returned to
       the CMD_GET_INFO command                                 ----*/

/* data buffer (mirrored in EEPROM) */

struct {
  unsigned char out[8];
  unsigned char in;
} idata user_data;

struct {
  float power[8];
} idata user_conf;

MSCB_INFO_CHN code channel[] = {
  1, UNIT_BOOLEAN, 0, 0, 0, "Out0",   &user_data.out[0],
  1, UNIT_BOOLEAN, 0, 0, 0, "Out1",   &user_data.out[1],
  1, UNIT_BOOLEAN, 0, 0, 0, "Out2",   &user_data.out[2],
  1, UNIT_BOOLEAN, 0, 0, 0, "Out3",   &user_data.out[3],
  1, UNIT_BOOLEAN, 0, 0, 0, "Out4",   &user_data.out[4],
  1, UNIT_BOOLEAN, 0, 0, 0, "Out5",   &user_data.out[5],
  1, UNIT_BOOLEAN, 0, 0, 0, "Out6",   &user_data.out[6],
  1, UNIT_BOOLEAN, 0, 0, 0, "Out7",   &user_data.out[7],
  0
};

MSCB_INFO_CHN code conf_param[] = {
  4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "Power0", &user_conf.power[0],
  4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "Power1", &user_conf.power[1],
  4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "Power2", &user_conf.power[2],
  4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "Power3", &user_conf.power[3],
  4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "Power4", &user_conf.power[4],
  4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "Power5", &user_conf.power[5],
  4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "Power6", &user_conf.power[6],
  4, UNIT_PERCENT, 0, 0, MSCBF_FLOAT, "Power7", &user_conf.power[7],
  0
};

unsigned char output;

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

#pragma NOAREGS

void user_write(unsigned char channel) reentrant;

/*---- User init function ------------------------------------------*/

void user_init(void)
{
unsigned char i;

  PRT1CF = 0xFF;  // push-pull for P1
  /* initialize configuration data */

  if (user_conf.power[0] <= 0 || user_conf.power[0] > 100)
    {
    for (i=0 ; i<8 ; i++)
      {
      user_data.out[i] = 0;
      user_conf.power[i] = 100;
      }
    eeprom_flash();
    }
}


/*---- User write function -----------------------------------------*/

void user_write(unsigned char channel) reentrant
{
  if (channel);
}

/*---- User read function ------------------------------------------*/

unsigned char user_read(unsigned char channel)
{
  if (channel);
  return 0;
}

/*---- User write config function ----------------------------------*/

void user_write_conf(unsigned char channel) reentrant
{
  if (channel);
}

/*---- User read config function -----------------------------------*/

void user_read_conf(unsigned char channel)
{
  if (channel);
}

/*---- User function called vid CMD_USER command -------------------*/

unsigned char user_func(unsigned char idata *data_in,
                        unsigned char idata *data_out)
{
  /* echo input data */
  data_out[0] = data_in[0];
  data_out[1] = data_in[1];
  return 2;
}

/*---- Apply power settings as a fraction of a second --------------*/

unsigned char cycle;
unsigned char ca[8];

//#define BIT(i) (0x80 >> i)   // "reverse" order
#define BIT(i) (1 << i)      // normal order

void set_power(void)
{
unsigned char        i;
static unsigned long on_time;
float                frac;
unsigned long        expired;

  /* turn output off after on-time expired */
  for (i=0 ; i<8 ; i++)
    if (user_conf.power[i] < 100)
      {
      expired = time() - on_time;
      if (expired >= (unsigned long) (user_conf.power[i]))
        {
        frac = user_conf.power[i] - (unsigned long) (user_conf.power[i]);

        if (frac == 0 || expired >= (unsigned long) (user_conf.power[i])+1)
          {
          output &= ~BIT(i);
          }
        else if (cycle > 0)
          {
          if ((float)ca[i] / cycle > frac)
            {
            output &= ~BIT(i);
            }
          else
            {
            ca[i]++;
            }
          }
        else 
          {
          ca[i]++;
          }
        }
      }

  /* turn all outputs on every second */
  if (time() - on_time >= 100)
    {
    on_time = time();
    for (i=0 ; i<8 ; i++)
      if (user_conf.power[i] > 0 && user_data.out[i])
        output |= BIT(i);

    cycle = (cycle + 1) % 10;

    if (cycle == 0)
      for (i=0 ; i<8 ; i++)
        ca[i] = 0;
    }

  /* set outputs according to main switch */
  for (i=0 ; i<8 ; i++)
    if (user_conf.power[i] >= 100)
      {
      if (user_data.out[i])
        output |= BIT(i);
      else
        output &= ~BIT(i);
      }

  /* write output to P1 */
  P1 = output;
}

/*---- User loop function ------------------------------------------*/

void user_loop(void)
{
unsigned char i;
static unsigned long last = 300;
static bit first = 1;

  /* set output according to power */
  set_power();

  if (!DEBUG_MODE && time() > last+30)
    {
    if (first)
      {
      lcd_clear();
      first = 0;
      }

    last = time();

    lcd_goto(0, 0);
    printf("OUT: ");
    for (i=0 ; i<8 ; i++)
      printf("%c", user_data.out[i] ? '1' : '0');
    }
}

