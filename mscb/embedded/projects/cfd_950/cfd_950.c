/********************************************************************\

  Name:         cfd_950.c
  Created by:   Ueli Hartmann 21.08.2006


  Contents:     Application specific (user) part of
                Midas Slow Control Bus protocol
                for CFD-950/VME Contant Fraction Discriminator

  $Id$

\********************************************************************/

#include <stdio.h>
#include "mscbemb.h"

extern bit FREEZE_MODE;
extern bit DEBUG_MODE;

extern unsigned char idata _flkey; // needed for eeprom flash

/* declare number of sub-addresses to framework */
unsigned char idata _n_sub_addr = 1;

char code node_name[] = "CFD-950";

  sbit SR_CLOCK   = P1 ^ 4;    // Shift register clock
  sbit SR_STROBE  = P1 ^ 5;    // Storage register clock
  sbit SR_DATAO   = P1 ^ 6;    // Serial data out
  sbit SR_DATAI   = P1 ^ 7;    // Serial data in
  sbit SR_OE      = P1 ^ 3;    // Output enable

#define GAIN      0   // gain for internal PGA

/*---- Define variable parameters returned to the CMD_GET_INFO command ----*/

/* data buffer (mirrored in EEPROM) */

#define N_ADC 8
 // #define N_DAC 8
#define N_THR 8
#define N_WALK 8
#define N_WIDTH 8
#define N_DELAY 8

struct {
 
  short voltage[N_ADC];
  unsigned char button;
  unsigned char channelout;
  unsigned char modeout;
  unsigned short thr[N_THR];
  unsigned short walk[N_WALK];
  unsigned short width[N_WIDTH];
  unsigned char delay[N_DELAY];
  unsigned short walk_onoff;
  unsigned short width_onoff;
  unsigned short valueout;
  unsigned char input;

} xdata user_data;

MSCB_INFO_VAR code vars[] = {
  2, UNIT_VOLT, PRFX_MILLI, 0, 0, "+1.0V",     &user_data.voltage[0],
  2, UNIT_VOLT, PRFX_MILLI, 0, 0, "+2.5V",     &user_data.voltage[1],
  2, UNIT_VOLT, PRFX_MILLI, 0, 0, "+3.3V",     &user_data.voltage[2],
  2, UNIT_VOLT, PRFX_MILLI, 0, 0, "-5.0V",     &user_data.voltage[3],
  2, UNIT_VOLT, PRFX_MILLI, 0, 0, "Voltage4",  &user_data.voltage[4],
  2, UNIT_VOLT, PRFX_MILLI, 0, 0, "Voltage5",  &user_data.voltage[5],
  2, UNIT_VOLT, PRFX_MILLI, 0, 0, "Voltage6",  &user_data.voltage[6],
  2, UNIT_VOLT, PRFX_MILLI, 0, 0, "Walk",  	   &user_data.voltage[7],
  1, UNIT_BYTE, PRFX_NONE, 0, 0,  "BUTTON",	   &user_data.button,
  1, UNIT_BYTE, PRFX_NONE, 0, 0,  "CHANNEL",   &user_data.channelout,
  1, UNIT_BYTE, PRFX_NONE, 0, 0,  "MODE",	   &user_data.modeout,
  2, UNIT_VOLT, PRFX_MILLI, 0, 0, "THR_CH1",   &user_data.thr[0],
  2, UNIT_VOLT, PRFX_MILLI, 0, 0, "THR_CH2",   &user_data.thr[1],
  2, UNIT_VOLT, PRFX_MILLI, 0, 0, "THR_CH3",   &user_data.thr[2],
  2, UNIT_VOLT, PRFX_MILLI, 0, 0, "THR_CH4",   &user_data.thr[3],
  2, UNIT_VOLT, PRFX_MILLI, 0, 0, "THR_CH5",   &user_data.thr[4],
  2, UNIT_VOLT, PRFX_MILLI, 0, 0, "THR_CH6",   &user_data.thr[5],
  2, UNIT_VOLT, PRFX_MILLI, 0, 0, "THR_CH7",   &user_data.thr[6],
  2, UNIT_VOLT, PRFX_MILLI, 0, 0, "THR_CH8",   &user_data.thr[7],
  2, UNIT_VOLT, PRFX_MILLI, 0, 0, "WALK_CH1",  &user_data.walk[0],
  2, UNIT_VOLT, PRFX_MILLI, 0, 0, "WALK_CH2",  &user_data.walk[1],
  2, UNIT_VOLT, PRFX_MILLI, 0, 0, "WALK_CH3",  &user_data.walk[2],
  2, UNIT_VOLT, PRFX_MILLI, 0, 0, "WALK_CH4",  &user_data.walk[3],
  2, UNIT_VOLT, PRFX_MILLI, 0, 0, "WALK_CH5",  &user_data.walk[4],
  2, UNIT_VOLT, PRFX_MILLI, 0, 0, "WALK_CH6",  &user_data.walk[5],
  2, UNIT_VOLT, PRFX_MILLI, 0, 0, "WALK_CH7",  &user_data.walk[6],
  2, UNIT_VOLT, PRFX_MILLI, 0, 0, "WALK_CH8",  &user_data.walk[7],
  2, UNIT_VOLT, PRFX_MILLI, 0, 0, "WIDTH_C1",  &user_data.width[0],
  2, UNIT_VOLT, PRFX_MILLI, 0, 0, "WIDTH_C2",  &user_data.width[1],
  2, UNIT_VOLT, PRFX_MILLI, 0, 0, "WIDTH_C3",  &user_data.width[2],
  2, UNIT_VOLT, PRFX_MILLI, 0, 0, "WIDTH_C4",  &user_data.width[3],
  2, UNIT_VOLT, PRFX_MILLI, 0, 0, "WIDTH_C5",  &user_data.width[4],
  2, UNIT_VOLT, PRFX_MILLI, 0, 0, "WIDTH_C6",  &user_data.width[5],
  2, UNIT_VOLT, PRFX_MILLI, 0, 0, "WIDTH_C7",  &user_data.width[6],
  2, UNIT_VOLT, PRFX_MILLI, 0, 0, "WIDTH_C8",  &user_data.width[7],
  1, UNIT_BYTE, PRFX_NONE, 0, 0,  "DELAY_C1",  &user_data.delay[0],
  1, UNIT_BYTE, PRFX_NONE, 0, 0,  "DELAY_C2",  &user_data.delay[1],
  1, UNIT_BYTE, PRFX_NONE, 0, 0,  "DELAY_C3",  &user_data.delay[2],
  1, UNIT_BYTE, PRFX_NONE, 0, 0,  "DELAY_C4",  &user_data.delay[3],
  1, UNIT_BYTE, PRFX_NONE, 0, 0,  "DELAY_C5",  &user_data.delay[4],
  1, UNIT_BYTE, PRFX_NONE, 0, 0,  "DELAY_C6",  &user_data.delay[5],
  1, UNIT_BYTE, PRFX_NONE, 0, 0,  "DELAY_C7",  &user_data.delay[6],
  1, UNIT_BYTE, PRFX_NONE, 0, 0,  "DELAY_C8",  &user_data.delay[7],
  2, UNIT_BYTE, PRFX_NONE, 0, 0,  "WALK_A_M",  &user_data.walk_onoff,
  2, UNIT_BYTE, PRFX_NONE, 0, 0,  "WIDTH_BU",  &user_data.width_onoff,

  0
};

MSCB_INFO_VAR *variables = vars;

unsigned char output;

/********************************************************************\

  Application specific init and inout/output routines

\********************************************************************/

void user_write(unsigned char index) reentrant;
void send_dispmode(unsigned short display_char, unsigned char mod, unsigned char digit);
void ser_write_byte(short b);
void ser_write_longbyte(unsigned long b);
void ser_strob();
void ser_clock(void);

/*---- User init function ------------------------------------------*/

void user_init(unsigned char i)
{
  unsigned short a;	//  wert a
  unsigned short b;	//  wert b
  unsigned long d;	//  wert d
  unsigned char m;	//  mode


  AMX0CF = 0x00;  // select single ended analog inputs
  ADC0CF = 0xE0;  // 16 system clocks, gain 1
  ADC0CN = 0x80;  // enable ADC

  REF0CN = 0x03;  // enable internal reference
  DAC0CN = 0x80;  // enable DAC0
  DAC1CN = 0x80;  // enable DAC1

  PRT1CF = 0x7F;  // P1 on push-pull

   /* init shift register lines */
  SR_OE = 1;
  SR_CLOCK = 0;
  SR_DATAO = 0;
  SR_DATAI = 1; // prepare for input


  /* initialize configuration data */
  delay_ms(2000);
  send_dispmode(' ', 1, 3);
  send_dispmode('P', 1, 2);
  send_dispmode('S', 1, 1);
  send_dispmode('I', 1, 0);
  delay_ms(500);
  send_dispmode(' ', 1, 3);
  send_dispmode('C', 1, 2);
  send_dispmode('F', 1, 1);
  send_dispmode('D', 1, 0);
  delay_ms(500);
  send_dispmode(' ', 1, 3);
  send_dispmode('9', 1, 2);
  send_dispmode('5', 1, 1);
  send_dispmode('0', 1, 0);
  delay_ms(500);
  send_dispmode('V', 1, 3);
  send_dispmode('1', 1, 2);
  send_dispmode('.', 1, 1);
  send_dispmode('0', 1, 0);

  m = 1; // Mode Threshold
  P1 = m & 0x07;

  for (i=0 ; i<8 ; i++) { // Init Threshold
    a = ((user_data.thr[i]) * 4); //* shift left 2 bit
    a = (a & 0x0FFC);             //* wert_th maskieren
    b = (i * 0x1000);             //* Adresse setzen
    a = (a + b);                  //* Adresse und Wert zusammenfgen

    ser_write_byte(a);	
    ser_strob();
    SR_OE = 0;                    //* ser. ausgeben
  }

  m = 2; // Mode Walk
  P1 = m & 0x07;

  for (i=0 ; i<8 ; i++) { // Init Walk
    a = ((user_data.walk[i]) * 4); //* shift left 2 bit
    a = (a & 0x0FFC);              //* wert_th maskieren
    b = (i * 0x1000);              //* Adresse setzen
    a = (a + b);                   //* Adresse und Wert zusammenfgen

    ser_write_byte(a);	
    ser_strob();
    SR_OE = 0;                     //* ser. ausgeben
  }

  m = 4; // Mode Width
  P1 = m & 0x07;

  for (i=0 ; i<8 ; i++) { // Init Width
    a = ((user_data.width[i]) * 4); //* shift left 2 bit
    a = (a & 0x0FFC);               //* wert_th maskieren
    b = (i * 0x1000);               //* Adresse setzen
    a = (a + b);                    //* Adresse und Wert zusammenfgen

    ser_write_byte(a);	
    ser_strob();
    SR_OE = 0;                      //* ser. ausgeben
  }

  m = 3;
  P1 = m & 0x07; // Init Walk on off

  ser_write_byte(user_data.walk_onoff);
  ser_strob();
  SR_OE = 0;

  m = 5;
  P1 = m & 0x07; // Init Width on off

  ser_write_byte(user_data.width_onoff);
  ser_strob();
  SR_OE = 0;

  m = 6; // Mode Delay
  P1 = m & 0x07;
  d = 0;

  for (i=0; i<8; i++) { // Init Delay
    d = d<<4;                   //* 4-bit nach links schieben
    d = d | user_data.delay[i]; //* delay mit 32-bit adieren
  }

  ser_write_longbyte(d);
  ser_strob();
  SR_OE = 0;

  m = 1; // zurueck Mode TH
  P1 = m & 0x07;

}

/*---- User write function -----------------------------------------*/

#pragma NOAREGS

void user_write(unsigned char index) reentrant
{
  if (index);
}

/*---- User read function ------------------------------------------*/

unsigned char user_read(unsigned char index)
{
  if (index);

  return 0;
}

/*---- User function called vid CMD_USER command -------------------*/

unsigned char user_func(unsigned char *data_in,
                        unsigned char *data_out)
{
  /* echo input data */
  data_out[0] = data_in[0];
  data_out[1] = data_in[1];
  return 2;
}

/*------------------------------------------------------------------*/

void adc_read()
{
  static unsigned char channel = 0;
  static unsigned long value;
  static unsigned char cnt = 0;
  unsigned char i;
  float u;

  if (cnt == 0) {
    /* switch to new channel */
    channel = (channel + 1) % N_ADC;
    AMX0SL = channel & 0x0F;
    ADC0CF = 0xE0 | (GAIN & 0x07);  // 16 system clocks and gain

    value = 0;
  }

  /* integrate 16 times */
  for (i=0 ; i < 16 ; i++) {
    DISABLE_INTERRUPTS;

    ADCINT = 0;
    ADBUSY = 1;
    while (!ADCINT);  // wait until conversion ready, does NOT work with ADBUSY!

    ENABLE_INTERRUPTS;

    value += (ADC0L | (ADC0H << 8));
    yield();
  }

  cnt++;

  if (cnt == 0) { /* average 16*256 = 4096 times */
    value /= 256;

    /* convert to milli volts */
    u = value  / 65536.0 * 2.5 * 1000;

    DISABLE_INTERRUPTS;
    user_data.voltage[channel] = u;
    ENABLE_INTERRUPTS;
  }
}


/*---- Clock external shift registers to read and output data 16-Bit ------*/

void ser_write_byte(short b)
{
  unsigned char i;

  SR_OE = 1;

  /* first shift register 16 Bit (buttons) */
  for(i=0;i<16;i++) {
    SR_DATAO = ((b & (0x8000>>i))==0);

    delay_us(10);
    SR_CLOCK = 1;
    delay_us(10);
    SR_CLOCK = 0;
    delay_us(10);
  }
}

/*---- Clock external shift registers to read and output data 32-Bit ------*/

void ser_write_longbyte(unsigned long b)
{
  unsigned char i;

  SR_OE = 1;

  /* first shift register 32 Bit (buttons) */
  for(i=0;i<32;i++) {
    SR_DATAO = ((b & (1l<<i))==0); //* 1l fr long

    delay_us(10);
    SR_CLOCK = 1;
    delay_us(10);
    SR_CLOCK = 0;
    delay_us(10);
  }
}

 /* strobe for output  */

void ser_strob()
{
  SR_STROBE = 1;
  delay_us(10);
  SR_STROBE = 0;
  delay_us(10);

  /* after first cycle, enable outputs */
}

/*------------------------  (mode 7)  send to DISPLAY  -------------------------*/

void send_display(unsigned short display_char, unsigned char mod)
{
  unsigned char mode_hold; // mode save
  unsigned short c;
  unsigned char i;
  unsigned short dig;

  mode_hold = mod; // mode save
  mod = 7;         // set mode for channel
  P1 = mod & 0x07; // an port1 mode 0 ausgeben

  dig = 1000;
  for (i=0; i<4; i++) {
    c = display_char / dig;
    display_char = display_char % dig;
    dig /= 10;
    ser_write_byte(((unsigned short)(3-i) << 8) | (c+'0'));
    ser_strob();
    SR_OE = 0;
  }

  mod = mode_hold; // mode zurckschreiben
  P1 = mod & 0x07; // an port1 mode ausgeben
}
/*------------------------------- Mode 7 send to Display "char"  -----------------------------------*/

void send_dispmode(unsigned short display_char, unsigned char mod, unsigned char digit)
{
  unsigned char mode_hold; // mode save
  unsigned short c;

  mode_hold = mod; // mode save
  mod = 7;         // set mode for channel
  P1 = mod & 0x07; // an port1 mode 0 ausgeben

  c = display_char;	
  ser_write_byte(((unsigned short)(digit) << 8) | (c));
  ser_strob();
  SR_OE = 0;

/* ------------------------------------------------------------------ 	*/

// ser_write_byte(display_char);
// ser_strob();
// SR_OE = 0;
  mod = mode_hold; // mode zurckschreiben
  P1 = mod & 0x07; // an port1 mode ausgeben
}


/*------------------------------- button read -----------------------------------*/



void button_read()
{
  unsigned char b;
  unsigned char m;
  unsigned char i;

  static unsigned char channel = 0; //  status channel 1 - 8
  unsigned char chtemp;             //  channel temp
  static unsigned char mode = 0;    //  status mode 1 - 7
  unsigned char mode_hold;          // mode save
  unsigned short wert_th;           //  wert an ser out
  unsigned short wert_walk;         //  wert an ser out
  unsigned short wert_width;        //  wert an ser out
  unsigned long wert_delay = 0;     //  wert an ser out
  unsigned short dac_ad;            //  DAC address
  unsigned short wert_display;      //  wert an display

  b = P0;
  b = b & 0x3C;
  user_data.button = b;

  if (b == 0x3C) // no botton pressed
    return;

  /* ------------------   read channel botton  -------------------------- */

  if (b == 0x1C) {
    channel = (channel + 1) % 8;
    mode_hold = mode;        // mode save
    mode = 0;                // set mode for channel
    P1 = mode & 0x07;        // an port1 mode 0 ausgeben
    ser_write_byte(channel); // channel ser_out
    SR_OE = 0;
    mode = mode_hold;        // mode zurckschreiben
    P1 = mode & 0x07;        // an port1 mode ausgeben
    delay_ms(300);
  }

  user_data.channelout = channel;

  /* ---------------------   read mode botton   ---------------------------- */

  if (b == 0x2C) {
    mode = (mode + 1) % 7;
    delay_ms(300);
  }

  user_data.modeout = mode;
  m = mode;
  P1 = m & 0x07;

  switch(mode) {

    /*----------------------------  mode 0 Save to EEPROM ------------------------------*/

    case 0:
      send_dispmode('P', 1, 3);
      send_dispmode('R', 1, 2);
      send_dispmode('O', 1, 1);
      send_dispmode('M', 1, 0);

      if (b == 0x34) { // botton INC for save to EEPROM

        _flkey = 0xF1;  // needed otherwise eeprom_flash is not allowed 
        eeprom_flash();

        send_dispmode('s', 1, 3);
        send_dispmode('a', 1, 2);
        send_dispmode('v', 1, 1);
        send_dispmode('e', 1, 0);
        delay_ms(500);
        send_dispmode(' ', 1, 3);
        send_dispmode(' ', 1, 2);
        send_dispmode('o', 1, 1);
        send_dispmode('k', 1, 0);
        delay_ms(500);
      }
      break;


    /*----------------------------  mode 1 Threshold ------------------------------*/

    case 1:
      if (b == 0x34) {
        user_data.thr[channel] += 1;
        if (user_data.thr[channel] == 1024)
          user_data.thr[channel] = 0;
        delay_ms(100);
      }

      if (b == 0x38) {
        if (user_data.thr[channel] == 0)
          user_data.thr[channel] = 1024;
        user_data.thr[channel] -= 1;

        delay_ms(100);
      }
      send_display(user_data.thr[channel], mode);

      if (mode == 1) {
        wert_th = ((user_data.thr[channel]) * 4); //* shift left 2 bit
        wert_th = (wert_th & 0x0FFC);             //* wert_th maskieren
        dac_ad = (channel * 0x1000);              //* Adresse setzen
        wert_th = (wert_th + dac_ad);             //* Adresse und Wert zusammenfgen
      }

      if ((b == 0x34) || (b == 0x38)) { // botton INC DEC
        ser_write_byte(wert_th); //* ser. ausgeben
        ser_strob();
        SR_OE = 0;
      }

      break;

    /*----------------------------  mode 2 Walk ------------------------------*/

    case 2:
      if (b == 0x34) {
        user_data.walk[channel] += 1;
        if (user_data.walk[channel] == 1024)
          user_data.walk[channel] = 0;
        delay_ms(100);
      }

      if (b == 0x38) {
        if (user_data.walk[channel] == 0)
          user_data.walk[channel] = 1024;
        user_data.walk[channel] -=1;

        delay_ms(100);
      }
      send_display(user_data.walk[channel], mode);

      if (mode == 2) {
        wert_walk = ((user_data.walk[channel]) * 4); //*shift left
        wert_walk = (wert_walk & 0x0FFC);
        dac_ad = (channel * 0x1000);
        wert_walk = (wert_walk + dac_ad);
       }

       if ((b == 0x34) || (b == 0x38)) { // botton INC DEC
         ser_write_byte(wert_walk);
         ser_strob();
         SR_OE = 0;
       }

       break;

    /*----------------------------  mode 3 Walk ON OFF ------------------------------*/

    case 3:
      if (b == 0x34) {
        (user_data.walk_onoff) = (0x0000);
        delay_ms(500);
      }

      if (b == 0x38) {
        (user_data.walk_onoff) = (0xFFFF);
        delay_ms(500);
      }

      if ((user_data.walk_onoff >0) || (b == 0x38)) {
        send_dispmode('M', mode, 3);
        send_dispmode('A', mode, 2);
        send_dispmode('N', mode, 1);
        send_dispmode('.', mode, 0);
      }

      if ((user_data.walk_onoff == 0) || (b == 0x34)) { //* char "A"
        send_dispmode('A', mode, 3);
        send_dispmode('U', mode, 2);
        send_dispmode('T', mode, 1);
        send_dispmode('O', mode, 0);
      }

      if (mode == 3) {
        wert_walk = (user_data.walk_onoff);
        wert_walk = (wert_walk & 0xFFFF);
      }

      if ((b == 0x34) || (b == 0x38)) { // botton INC DEC
        ser_write_byte(wert_walk);
        ser_strob();
        SR_OE = 0;
      }

      break;

    /*----------------------------  mode 4 Width ------------------------------*/

    case 4:
      if (b == 0x34) {
        user_data.width[channel] += 1;
        if (user_data.width[channel] == 1024)
          user_data.width[channel] = 0;
        delay_ms(100);
      }

      if (b == 0x38) {
        if (user_data.width[channel] == 0)
          user_data.width[channel] = 1024;
        user_data.width[channel] -= 1;

        delay_ms(100);
      }
      send_display(user_data.width[channel], mode);

      if (mode == 4) {
        wert_width = ((user_data.width[channel]) * 4); //*shift left
        wert_width = (wert_width & 0x0FFC);
        dac_ad = (channel * 0x1000);
        wert_width = (wert_width + dac_ad);
      }

      if ((b == 0x34) || (b == 0x38)) { // botton INC DEC
        ser_write_byte(wert_width);
        ser_strob();
        SR_OE = 0;
      }

      break;

    /*----------------------------  mode 5 Width ON OFF ------------------------------*/

    case 5:
      if (b == 0x34) {
        user_data.width_onoff = 0xFFFF;
        delay_ms(500);
      }

      if (b == 0x38) {
        user_data.width_onoff = 0x0000;
        delay_ms(500);
      }

      if ((user_data.width_onoff >0) || (b == 0x34)) { //* char "B"
        send_dispmode('B', mode, 3);
        send_dispmode('L', mode, 2);
        send_dispmode('K', mode, 1);
        send_dispmode('.', mode, 0);
      }

      if ((user_data.width_onoff  == 0) || (b == 0x38)) { //* char "U"
        send_dispmode('U', mode, 3);
        send_dispmode('P', mode, 2);
        send_dispmode('D', mode, 1);
        send_dispmode('T', mode, 0);
      }

      if (mode == 5) {
        wert_width = user_data.width_onoff;
        wert_width = (wert_width & 0xFFFF);
      }

      if ((b == 0x34) || (b == 0x38)) { // botton INC DEC
        ser_write_byte(wert_width);
        ser_strob();
        SR_OE = 0;
      }

      break;

    /*----------------------------  mode 6 Delay ------------------------------*/

    case 6:
      if (b == 0x34) {
        user_data.delay[channel] += 1;
        if (user_data.delay[channel] == 16)
          user_data.delay[channel] = 0;
        // delay_ms(300);
      }

      if (b == 0x38) {
        if (user_data.delay[channel] == 0)
          user_data.delay[channel] = 16;
        user_data.delay[channel] -= 1;

        // delay_ms(300);
      }

      wert_display = user_data.delay[channel]; //* Wert an Dispaly

      wert_display = wert_display * 5 + 10;
      wert_display = wert_display * 100;

      send_display(wert_display, mode);
      send_dispmode('n', mode, 1);
      send_dispmode('s', mode, 0);

      if (mode == 6) {
        chtemp = channel; //* channel speichern
        channel = 0;      //* channel auf ch1 setzen
      }

      for (i=0; i<8; i++) { //* Zaehler, channel 1-8
        wert_delay = wert_delay<<4; //* 4-bit nach links schieben
        wert_delay = wert_delay | user_data.delay[i];//* delay mit 32-bit adieren
      }
      channel = chtemp; //* alter channel zurckladen


      if ((b == 0x34) || (b == 0x38)) { // botton INC DEC
        ser_write_longbyte(wert_delay);
        ser_strob();
        SR_OE = 0;
        delay_ms(300);
      }

      break;
  }
}

/*----------------------------  end mode  ------------------------------*/


/* ------------------------------------------------------------------ */

void user_loop(void)
{
  /* read temperature */
  adc_read();

  /* read buttons */
  button_read();

}

