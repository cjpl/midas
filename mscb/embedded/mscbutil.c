/********************************************************************\

  Name:         mscbmain.asm
  Created by:   Stefan Ritt

  Contents:     Various utility functions for MSCB protocol

  $Log$
  Revision 1.1  2002/07/05 08:12:46  midas
  Moved files

\********************************************************************/

#include "mscb.h"
#include <intrins.h>

/*
;--------------------------------------------------------------------
;
; CRC-8 code calculation, adds value in A to running CRC code 
; in CRC_CODE
;
; Modifies: DPTR,CRC_CODE
;
;--------------------------------------------------------------------

CRC8_DATA:       
        db 000h, 05eh, 0bch, 0e2h, 061h, 03fh, 0ddh, 083h
        db 0c2h, 09ch, 07eh, 020h, 0a3h, 0fdh, 01fh, 041h
        db 09dh, 0c3h, 021h, 07fh, 0fch, 0a2h, 040h, 01eh
        db 05fh, 001h, 0e3h, 0bdh, 03eh, 060h, 082h, 0dch
        db 023h, 07dh, 09fh, 0c1h, 042h, 01ch, 0feh, 0a0h
        db 0e1h, 0bfh, 05dh, 003h, 080h, 0deh, 03ch, 062h
        db 0beh, 0e0h, 002h, 05ch, 0dfh, 081h, 063h, 03dh
        db 07ch, 022h, 0c0h, 09eh, 01dh, 043h, 0a1h, 0ffh
        db 046h, 018h, 0fah, 0a4h, 027h, 079h, 09bh, 0c5h
        db 084h, 0dah, 038h, 066h, 0e5h, 0bbh, 059h, 007h
        db 0dbh, 085h, 067h, 039h, 0bah, 0e4h, 006h, 058h
        db 019h, 047h, 0a5h, 0fbh, 078h, 026h, 0c4h, 09ah
        db 065h, 03bh, 0d9h, 087h, 004h, 05ah, 0b8h, 0e6h
        db 0a7h, 0f9h, 01bh, 045h, 0c6h, 098h, 07ah, 024h
        db 0f8h, 0a6h, 044h, 01ah, 099h, 0c7h, 025h, 07bh
        db 03ah, 064h, 086h, 0d8h, 05bh, 005h, 0e7h, 0b9h
        db 08ch, 0d2h, 030h, 06eh, 0edh, 0b3h, 051h, 00fh
        db 04eh, 010h, 0f2h, 0ach, 02fh, 071h, 093h, 0cdh
        db 011h, 04fh, 0adh, 0f3h, 070h, 02eh, 0cch, 092h
        db 0d3h, 08dh, 06fh, 031h, 0b2h, 0ech, 00eh, 050h
        db 0afh, 0f1h, 013h, 04dh, 0ceh, 090h, 072h, 02ch
        db 06dh, 033h, 0d1h, 08fh, 00ch, 052h, 0b0h, 0eeh
        db 032h, 06ch, 08eh, 0d0h, 053h, 00dh, 0efh, 0b1h
        db 0f0h, 0aeh, 04ch, 012h, 091h, 0cfh, 02dh, 073h
        db 0cah, 094h, 076h, 028h, 0abh, 0f5h, 017h, 049h
        db 008h, 056h, 0b4h, 0eah, 069h, 037h, 0d5h, 08bh
        db 057h, 009h, 0ebh, 0b5h, 036h, 068h, 08ah, 0d4h
        db 095h, 0cbh, 029h, 077h, 0f4h, 0aah, 048h, 016h
        db 0e9h, 0b7h, 055h, 00bh, 088h, 0d6h, 034h, 06ah
        db 02bh, 075h, 097h, 0c9h, 04ah, 014h, 0f6h, 0a8h
        db 074h, 02ah, 0c8h, 096h, 015h, 04bh, 0a9h, 0f7h
        db 0b6h, 0e8h, 00ah, 054h, 0d7h, 089h, 06bh, 035h

CRC8_ADD:
        PUSH    ACC

        MOV     DPTR,#CRC8_DATA         ; Point To Table
        XRL     A,CRC_CODE              ; XOR In CRC
        MOVC    A,@A+DPTR               ; Get New CRC Byte
        MOV     CRC_CODE,A              ; Store Back

        POP     ACC
        RET

CRC8_ADD1:
        PUSH    ACC
        PUSH    DPL
        PUSH    DPH

        MOV     DPTR,#CRC8_DATA         ; Point To Table
        XRL     A,CRC_CODE              ; XOR In CRC
        MOVC    A,@A+DPTR               ; Get New CRC Byte
        MOV     CRC_CODE,A              ; Store Back

        POP     DPH
        POP     DPL
        POP     ACC
        RET

;--------------------------------------------------------------------
;
; Configure UART via timer 2 
;
; parameters:
;   A=1:9600,2:19200,3:28800,4:57600,5:115200,6:172800,7:345600 Baud
;
; Modifes: A,DPTR
;
;--------------------------------------------------------------------

BAUD_TABLE:     ; reload 100-x, x = 11.0592MHz/32/baud
        DB      100h-36,100h-18,100h-12,100h-6,100h-3,100h-2,100h-1

UART_INIT:
        MOV     SCON,#11010000b ; Mode 3, 9 bit, receive enable
;        MOV     SCON,#50H       ; Mode 1, 8 bit, receive enable

        MOV     TMOD,#00H       ; timer 0&1 no function
        MOV     T2CON,#00110100b; timer 2 RX+TX mode
        MOV     RCAP2H,#0FFh
        MOV     RCAP2L,#100h-36 ; 9600 baud by default
        SETB    TR2

        RET

SET_BAUD:
        CLR     TR2
        MOV     RCAP2H,#0FFh

        MOV     DPTR,#BAUD_TABLE
        DEC     A
        MOVC    A,@A+DPTR
        MOV     RCAP2L,A        ; set reload register from table
        SETB    TR2

        RET
*/

/*---- Refresh watchdog --------------------------------------------*/

void watchdog_refresh(void)
{
#ifdef CPU_ADUC812
  WDR1 = 1;
  WDR2 = 1;
#endif;

#ifdef CPU_C8051F000
  WDTCN = 0xA5;
#endif
}

/*------------------------------------------------------------------*\

  Delay functions for 11.0520 MHz quartz

\*------------------------------------------------------------------*/

#ifdef CPU_ADUC812
#endif

#ifdef CPU_C8051F000

void delay_ms(unsigned int ms)
{
unsigned int i;

  for (i=0 ; i<ms ; i++)
    delay_us(1000);
}

void delay_us(unsigned int us)
{
unsigned char i;
unsigned int  remaining_us;

  if (us <= 250)
    {
    for (i=(unsigned char)us ; i>0 ; i--)
      {
      _nop_();
      }
    }
  else
    {
    remaining_us = us;
    while (remaining_us > 250)
      {
      delay_us(250);
      remaining_us -= 250;
      }
    if (us > 0)
      delay_us(remaining_us);
    }

}
#endif

/*
;--------------------------------------------------------------------
;
; EEPROM functions for reading writing flash EEPROM
;
; Parameters: 
;   A   parameter index 0..N
;   R0  pointer to data
;   R1  data length (1..4)
;
; Modifies:
;   A,R0,R1,R2
;
;--------------------------------------------------------------------

IF(CPU_ADUC812)

EEPROM_WRITE:
        MOV     EADRL,A         ; select page
        MOV     ECON,#5         ; erase page
        MOV     EDATA1,@R0
        INC     R0
        MOV     EDATA2,@R0
        INC     R0
        MOV     EDATA3,@R0
        INC     R0
        MOV     EDATA4,@R0
        INC     R0
        MOV     ECON,#2         ; write page
        RET

EEPROM_READ:
        MOV     EADRL,A         ; set page
        MOV     ECON,#1         ; read page
        MOV     A,R1
        DEC     A
        MOV     @R0,EDATA1
        JZ      EEPROM_RET
        INC     R0
        DEC     A
        MOV     @R0,EDATA2
        JZ      EEPROM_RET
        INC     R0
        DEC     A
        MOV     @R0,EDATA3
        JZ      EEPROM_RET
        INC     R0
        DEC     A
        MOV     @R0,EDATA4
EEPROM_RET:
        RET

ENDIF

IF(CPU_C8051F000)

EEPROM_WRITE:
        ; copy page to RAM at 128
        PUSH    DPL
        PUSH    DPH
        PUSH    B
        PUSH    ACC
        MOV     B,R0
        MOV     R0,#80h
        MOV     DPTR,#7F80h
        MOV     R1,#80h
EEW_LOOP1:
        MOV     A,R1
        MOVC    A,@A+DPTR
        MOV     @R1,A
        INC     R1
        DJNZ    R0,EEW_LOOP1

        ; copy data into RAM page
        POP     ACC
        MOV     R0,B

        RL      A
        RL      A
        ADD     A,#80h
        MOV     R1,A
        MOV     R2,#4
EEW_LOOP2:
        MOV     A,@R0
        MOV     @R1,A
        INC     R1
        INC     R0
        DJNZ    R2,EEW_LOOP2

        ; erase page
        MOV     A,FLSCL
        ANL     A,#0F0h
        ORL     A,#00001000b    ; set timer for 11.052 MHz clock
        MOV     FLSCL,A
        MOV     PSCTL,#3        ; allow write+erase
        MOV     DPTR,#8000h
        MOVX    @DPTR,A

        ; write page
        MOV     PSCTL,#1        ; allow write

        MOV     R0,#80h
        MOV     DPTR,#8000h
        MOV     R1,#80h
EEW_LOOP3:
        MOV     A,@R1
        MOVX    @DPTR,A
        INC     R1
        INC     DPL
        DJNZ    R0,EEW_LOOP3

        MOV     PSCTL,#0        ; don't allow write+erase

        POP     B
        POP     DPH
        POP     DPL
        RET

EEPROM_READ:
        MOV     DPTR,#8000h     ; read from 128-byte EEPROM page
        RL      A
        RL      A
EER_LOOP:
        PUSH    ACC
        MOVC    A,@A+DPTR
        MOV     @R0,A
        POP     ACC
        INC     R0
        INC     A
        DJNZ    R1,EER_LOOP
        RET

ENDIF
*/
/*------------------------------------------------------------------*\

  LCD functions for HD44780/KS0066 compatible LCD displays

\*------------------------------------------------------------------*/

#define LCD P2 // LCD display connected to port2

sbit LCD_RS     = LCD ^ 1;
sbit LCD_R_W    = LCD ^ 2;
sbit LCD_E      = LCD ^ 3;

sbit LCD_DB4    = LCD ^ 4;
sbit LCD_DB5    = LCD ^ 5;
sbit LCD_DB6    = LCD ^ 6;
sbit LCD_DB7    = LCD ^ 7;

sbit LCD_CLK    = LCD ^ 7; // pins for 4021 shift register
sbit LCD_P_W    = LCD ^ 6;
sbit LCD_SD     = LCD ^ 0;

bit lcd_present;

lcd_out(unsigned char d, bit df)
{
  if (!lcd_present)
    return;

#ifdef CPU_C8051F000
  /* switch LCD_DB7 to input */
  // PRT2CF = 0x7F;
#endif
  LCD = 0xFC;       // data input R/W=High
  delay_us(1);
  while (LCD_DB7);  // loop if busy

#ifdef CPU_C8051F000
  /* switch LCD_DB7 to output */
  // PRT2CF = 0xFF;
#endif

  LCD = 0;
  delay_us(1);

  if (df)
    LCD = (d & 0xF0) | 0x02; 
  else
    LCD = (d & 0xF0); 
  delay_us(1);

  LCD_E = 1;
  delay_us(1);
  LCD_E = 0;
  delay_us(1);

  if (df)
    LCD = (d << 4) | 0x02; 
  else
    LCD = (d << 4); 
  delay_us(1);

  LCD_E = 1;
  delay_us(1);
  LCD_E = 0;
  delay_us(1);
}

void lcd_setup()
{
  LCD = 0;
  delay_ms(15);
  LCD = 0x30;
  
  LCD_E = 1;
  delay_us(1);
  LCD_E = 0;
  delay_ms(5);

  LCD_E = 1;
  delay_us(1);
  LCD_E = 0;
  delay_ms(1);

  LCD_E = 1;
  delay_us(1);
  LCD_E = 0;

  LCD = 0x20; // set 4-bit interface
  LCD_E = 1;
  delay_us(1);
  LCD_E = 0;

  /* test if LCD present */
  LCD = 0xFC; // data input R/W=High
  delay_us(10);
  if (!LCD_DB7)
    {
    lcd_present = 0;
    return;
    }

  lcd_present = 1;

  lcd_out(0x2C, 0); // select 2 lines, big font
  lcd_out(0x0C, 0); // display on
  lcd_out(0x01, 0); // clear display
  lcd_out(0x06, 0); // entry mode
}

void lcd_clear()
{
  lcd_out(0x01, 0);
}

void lcd_goto(char x, char y)
{
  /* goto position x(0..19), y(0..1) */

  lcd_out((x & 0x0F) | (0x80) | ((y & 0x01) << 6), 0);
}

char putchar(char c)
{
  lcd_out(c, 1);
  return c;
}

void lcd_puts(char *str)
{
  while (*str)
    lcd_out(*str++, 1);
}



/*------------------------------------------------------------------*\

  Input function for SCS-LCD1
  Returns: switch states in ACC

\*------------------------------------------------------------------*/

char scs_lcd1_read()
{
char i, d;

  LCD_CLK = 0;  // enable input
  LCD_SD = 1;

  LCD_P_W = 1;  // latch input
  delay_us(1);
  LCD_P_W = 0;

  for (i=d=0 ; i<8 ; i++)
    {
    d |= LCD_SD;
    d = (d | LCD_SD) << 1;

    LCD_CLK = 1;
    delay_us(1);
    LCD_CLK = 0;
    delay_us(1);
    }

  return d;
}


