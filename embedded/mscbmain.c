/********************************************************************\

  Name:         mscbmain.asm
  Created by:   Stefan Ritt

  Contents:     Midas Slow Control Bus protocol main program

  $Log$
  Revision 1.3  2002/07/08 08:51:39  midas
  Test eeprom functions

  Revision 1.2  2002/07/05 15:27:28  midas
  *** empty log message ***

  Revision 1.1  2002/07/05 08:12:46  midas
  Moved files

\********************************************************************/

#include <stdio.h>
#include "mscb.h"

/* bit definitions */
sbit LED =               P3^4;   // red LED on SCS100/SCS101
sbit RS485_ENABLE =      P3^5;   // enable bit for MAX1483

/* GET_INFO attributes */
#define GET_INFO_GENERAL 0
#define GET_INFO_CHANNEL 1
#define GET_INFO_CONF    2

/* Channel attributes */
#define SIZE_8BIT        1
#define SIZE_16BIT       2
#define SIZE_24BIT       3
#define SIZE_32BIT       4

/* Address modes */
#define ADDR_NONE        0
#define ADDR_NODE        1
#define ADDR_GROUP       2
#define ADDR_ALL         3

/*------------------------------------------------------------------*/

/* variables in internal RAM */

//DSEG    AT      0030h

char          in_buf[10], out_buf[8];
unsigned char i_in, last_i_in, final_i_in, n_out, i_out, cmd_len;
unsigned char crc_code, addr_mode, n_channel, n_conf, tmp[4];

unsigned int  node_addr, group_addr, wd_counter;

/*------------------------------------------------------------------*/

/* bit variables in internal RAM */

unsigned char bdata CSR;         // byte address of CSR consisting of bits below 

sbit DEBUG_MODE     = CSR ^ 0;   // debugging mode
sbit SYNC_MODE      = CSR ^ 1;   // turned on in SYNC mode
sbit FREEZE_MODE    = CSR ^ 2;   // turned on in FREEZE mode
sbit WD_RESET       = CSR ^ 3;   // got rebooted by watchdog reset

bit use_bit9;                    // use 9th bit in communication
bit addressed;                   // true if node addressed
bit debug_new_i, debug_new_o;    // used for LCD debug output



/*
STR_NADR:
        DB      'Node:',0
STR_GADR:
        DB      ' Group:',0
STR_WD:
        DB      'WD  :',0

*/
void setup(void)
{
unsigned char i;

#ifdef CPU_C8051F000
  
  XBR0 = 0x04; // Enable RX/TX
  XBR1 = 0x00;
  XBR2 = 0x40; // Enable crossbar

  /* Port configuration (1 = Push Pull Output) */
  PRT0CF = 0xFF;  // P0: TX = Push Pull
  PRT1CF = 0xFF;  // P1
  PRT2CF = 0x00;  // P2  Open drain for 5V LCD
  PRT3CF = 0x00;  // P3

  /* Disable watchdog (for LCD setup with delays) */
  WDTCN = 0xDE;
  WDTCN = 0xAD;

  /* Select external quartz oscillator */
  OSCXCN = 0x66;  // Crystal mode, Power Factor 22E6
  OSCICN = 0x08;  // CLKSL=1 (external)

#endif

  /* Blink LED */
  for (i=0 ; i<3 ; i++)
    {
    LED = 1;
    delay_ms(100);
    LED = 0;
    delay_ms(200);
    }

  /* init memory */
  DEBUG_MODE = 0;
  CSR = 0;
  LED = 0;
  RS485_ENABLE = 0;

  uart_init(BD_345600);

  /* retrieve EEPROM data */
  eeprom_read(&node_addr, 6, 0);

  lcd_setup();
  lcd_clear();
  lcd_goto(0, 0);
  //printf("Node: %04X Group: %02X", node_addr, group_addr);
}
/*
; reset IO's


; configure UART

        CALL    UART_INIT
        ;MOV     A,#1            ; 9600 baud
        MOV     A,#7            ; set baud rate
        CALL    SET_BAUD

        SETB    ES              ; enable serial interrupt
        CLR     PS              ; serial interrupt low priority
        SETB    EA              ; general interrupt enable
        CLR     RB8             ; clear read bit9
        
; call user configuration

        CALL    USER_INIT

; count channels

COUNT_CHANNELS:
        MOV     DPTR,#CHANNEL_INFO
        CLR     A
        MOV     R0,#0
CH_LOOP:
        MOV     A,R0
        MOV     B,#12           ; each channel info has 12 bytes
        MUL     AB
        MOVC    A,@A+DPTR
        INC     R0
        JNZ     CH_LOOP
        DEC     R0
        MOV     N_CHANNEL,R0

; get configuration parameters from EEPROM

INIT_CONF:
        MOV     DPTR,#CONF_PARAM; count config. param.
        MOV     R3,#0
IC_LOOP1:
        MOV     A,R3
        MOV     B,#12
        MUL     AB
        MOVC    A,@A+DPTR
        INC     R3
        JNZ     IC_LOOP1
        DEC     R3
        MOV     N_CONF,R3

        MOV     R4,#0
IC_LOOP2:
        MOV     A,R4
        ADD     A,#2            ; parameters start at page 2
        MOV     R0,#TMP
        MOV     R1,#4
        CALL    EEPROM_READ     ; get data from EEPROM

        MOV     A,R4
        MOV     R0,#TMP
        CALL    WRITE_CONF      ; call user configuration
        INC     R4
        DJNZ    R3,IC_LOOP2

; configure watchdog

        MOV     R0,#WD_COUNTER  ; read watchdog counter
        MOV     R1,#2
        MOV     A,#1
        CALL    EEPROM_READ

IF(CPU_ADUC812)
        JNB     WDS,NO_WD_RESET ; check if reset by watchdog
ENDIF
IF(CPU_C8051F000)
        MOV     A,RSTSRC
        ANL     A,#7Fh
        CJNE    A,#8,NO_WD_RESET ; check if reset by watchdog
ENDIF

        SETB    WD_RESET        ; set flag in node status

        MOV     A,WD_COUNTER
        ADD     A,#1            
        MOV     WD_COUNTER,A
        MOV     A,WD_COUNTER+1
        ADDC    A,#0
        MOV     WD_COUNTER+1,A

        MOV     A,#1
        MOV     R0,#WD_COUNTER
        MOV     R1,#2
        CALL    EEPROM_WRITE

NO_WD_RESET:                    ; output watchdog resets
        MOV     A,#0
        MOV     B,#1
        CALL    LCD_GOTO
        MOV     DPTR,#STR_WD
        CALL    LCD_PUTS
        MOV     A,WD_COUNTER+1
        CALL    LCD_HEX
        MOV     A,WD_COUNTER
        CALL    LCD_HEX

IF(CPU_ADUC812)
        MOV     WDCON,#0E0h     ; 2.048 second timeout period
        SETB    WDE             ; enable watchdog timer
ENDIF
IF(CPU_C8051F000)
        MOV     WDTCN,#00000111b ; 95ms (@11.0592MHz)
ENDIF

        RET

;--------------------------------------------------------------------
; Debug output

DEBUG_OUTPUT:
        JNB     DEBUG_NEW_I,DEBUG_NO_NEW_I
        CALL    DEBUG_DISP_I
        CLR     DEBUG_NEW_I

DEBUG_NO_NEW_I:        
        JNB     DEBUG_NEW_O,DEBUG_NO_NEW_O
        CALL    DEBUG_DISP_O
        CLR     DEBUG_NEW_O

DEBUG_NO_NEW_O:        
        RET
        
DEBUG_DISP_I:
        CALL    LCD_CLEAR
        MOV     A,#0
        MOV     B,#0
        CALL    LCD_GOTO
        
        MOV     A,I_IN          ; use I_IN if non-zero
        JNZ     DEBUG_I
        
        MOV     A,CMD_LEN       ; else use CMD_LEN for last command

DEBUG_I:
        MOV     R1,A
        MOV     R2,#0
DEBUG_CHAR_I:
        MOV     A,#IN_BUF
        ADD     A,R2
        MOV     R0,A
        MOV     A,@R0           ; output single char
        CALL    LCD_HEX
        MOV     A,#' '
        CALL    LCD_PUTC
        INC     R2
        DJNZ    R1,DEBUG_CHAR_I
        
        RET
        
DEBUG_DISP_O:
        MOV     A,#0
        MOV     B,#1
        CALL    LCD_GOTO
        
        MOV     A,N_OUT
        JZ      DEBUG_O_RET
        MOV     R1,A
        MOV     R2,#0
DEBUG_CHAR_O:
        MOV     A,#OUT_BUF
        ADD     A,R2
        MOV     R0,A
        MOV     A,@R0           ; output single char
        CALL    LCD_HEX
        MOV     A,#' '
        CALL    LCD_PUTC
        INC     R2
        DJNZ    R1,DEBUG_CHAR_O
        
DEBUG_O_RET:        
        RET
        
;--------------------------------------------------------------------
; Serial interrupt

SERIAL_INT:
        PUSH    PSW
        PUSH    ACC
        PUSH    DPL
        PUSH    DPH

        CLR     RS1             ; Select register bank 1
        SETB    RS0
        
        JNB     TI,SER_READ     ; if TI not set, RI must be set

SER_WRITE:
        CLR     TI              ; clear TI flag
        
        INC     I_OUT           ; increment output counter
        
        MOV     A,N_OUT
        CJNE    A,I_OUT,NOT_EMPTY

        MOV     I_OUT,#0        ; send buffer empty, clear pointer
        CALL    SERIAL_STOP     ; disable RS485 driver
        SETB    DEBUG_NEW_O     ; indicate new debug output
        JMP     SER_RET

NOT_EMPTY:
        MOV     A,#OUT_BUF      ; get buffer pointer
        ADD     A,I_OUT
        MOV     R0,A
        MOV     SBUF,@R0        ; send character
        
        JMP     SER_RET

SER_READ:  
        JNB     USE_BIT9,SER_STORE ; skip bit test if not configured
        JNB     RB8,NO_BIT9     ; no bit 9 received
        CLR     RB8
        JMP     SER_STORE
        
NO_BIT9:                        ; return if not addressed
        JB      ADDRESSED,SER_STORE
        CLR     RI
        MOV     I_IN,#0         ; reset buffer counter
        JMP     SER_RET

SER_STORE:                      ; store received byte in buffer
        MOV     A,#IN_BUF       ; get buffer pointer
        ADD     A,I_IN
        MOV     R0,A
        MOV     A,SBUF          ; read character
        CLR     RI              ; clear read flag
        MOV     @R0,A           ; store character

        MOV     A,I_IN          ; check if 1st character
        JNZ     NOT_FIRST

        MOV     CRC_CODE,#0     ; initialize CRC code on first byte

        MOV     A,IN_BUF        ; derive command length (+CRC)
        ANL     A,#07h
        INC     A
        INC     A
        MOV     CMD_LEN,A

NOT_FIRST:
        INC     I_IN            ; increment buffer offset
        SETB    DEBUG_NEW_I     ; indicate new data in buffer
        MOV     A,I_IN          ; test for buffer overflow
        CJNE    A,#10,NO_OVER

        MOV     I_IN,#0         ; reset buffer
        SJMP    SER_RET         ; don't interprete command

NO_OVER:                        ; check if command complete
        MOV     A,I_IN
        CJNE    A,CMD_LEN,NOT_COMPLETE ; not all bytes received yet
        SJMP    CMD_COMPLETE

NOT_COMPLETE:
        MOV     A,@R0
        CALL    CRC8_ADD
        SJMP    SER_RET

CMD_COMPLETE:  
        MOV     A,@R0
        CJNE    A,CRC_CODE,CLEAR_BUF  ; return if wrong CRC code

        MOV     A,IN_BUF
        ANL     A,#11100000b    ; check if address command
        JZ      IS_ADDRESS

NO_ADDRESS:        
        JNB     ADDRESSED,CLEAR_BUF ; return if not addressed

IS_ADDRESS:
        CALL    INTERPRETE_COMMAND

CLEAR_BUF:
        MOV     I_IN,#0         ; clear input buffer

SER_RET:        
        POP     DPH
        POP     DPL
        POP     ACC
        POP     PSW             ; selects old register bank
        RETI

; start output of serial buffer, first character in ACC
; enable RS-485 driver

SERIAL_START:

        SETB    RS485_ENABLE    ; enable RS385 driver
        MOV     SBUF,OUT_BUF    ; send first byte
        RET

; stop output of serial buffer
; disable RS-485 driver

SERIAL_STOP:           
        CLR     RS485_ENABLE
        RET

;--------------------------------------------------------------------
; interprete command

; command table -----------------
CMD_TABLE:
        AJMP    S_INVAL         ; 0x00
        AJMP    S_NODE_ADDRESS  ; 0x01
        AJMP    S_GROUP_ADDRESS ; 0x02
        AJMP    S_PING          ; 0x03

        AJMP    S_INIT          ; 0x04
        AJMP    S_GET_INFO      ; 0x05
        AJMP    S_SET_ADDR      ; 0x06
        AJMP    S_SET_BAUD      ; 0x07

        AJMP    S_FREEZE        ; 0x08
        AJMP    S_SYNC          ; 0x09
        AJMP    S_TRANSP        ; 0x0A
        AJMP    S_USER          ; 0x0B

        AJMP    S_INVAL         ; 0x0C
        AJMP    S_INVAL         ; 0x0D
        AJMP    S_INVAL         ; 0x0E
        AJMP    S_INVAL         ; 0x0F

        AJMP    S_WRITE_NA      ; 0x10
        AJMP    S_WRITE_ACK     ; 0x11
        AJMP    S_WRITE_CONF    ; 0x12
        AJMP    S_WRITE_CONF_PERM ; 0x13

        AJMP    S_READ          ; 0x14
        AJMP    S_READ_CONF     ; 0x15
        AJMP    S_WRITE_BLOCK   ; 0x16
        AJMP    S_READ_BLOCK    ; 0x17

        AJMP    S_INVAL         ; 0x18
        AJMP    S_INVAL         ; 0x19
        AJMP    S_INVAL         ; 0x1A
        AJMP    S_INVAL         ; 0x1B

        AJMP    S_INVAL         ; 0x1C
        AJMP    S_INVAL         ; 0x1D
        AJMP    S_INVAL         ; 0x1E
        AJMP    S_INVAL         ; 0x1F


INTERPRETE_COMMAND:
        
        CLR     F0              ; used for similar commands
        MOV     A,IN_BUF        ; get command
        ANL     A,#11111000b    ; mask command
        RR      A               ; use command as table index
        RR      A
        MOV     DPTR,#CMD_TABLE
        JMP     @A+DPTR

S_INVAL:
        RET

;-------------- Addressing commands

S_NODE_ADDRESS:                 ; check for 8-bit node addressing
        MOV     A,IN_BUF
        CJNE    A,#CMD_ADDR_NODE8,NO_N8
        
        MOV     A,IN_BUF+1
        CJNE    A,NODE_ADDR,NOT_ADDRESSED
        MOV     A,NODE_ADDR+1
        JNZ     NOT_ADDRESSED
        SJMP    NODE_ADDRESSED
NO_N8:                          ; check for 16-bit node addressing
        CJNE    A,#CMD_ADDR_NODE16,INVAL_ADDRESS
        MOV     A,IN_BUF+1
        CJNE    A,NODE_ADDR,NOT_ADDRESSED
        MOV     A,IN_BUF+2
        CJNE    A,NODE_ADDR+1,NOT_ADDRESSED
        SJMP    NODE_ADDRESSED

S_GROUP_ADDRESS:                ; check for 8-bit group addressing
        MOV     A,IN_BUF
        CJNE    A,#CMD_ADDR_GRP8,NO_G8
        MOV     A,IN_BUF+1
        CJNE    A,GROUP_ADDR,NOT_ADDRESSED
        MOV     A,GROUP_ADDR+1
        JNZ     NOT_ADDRESSED
        SJMP    GROUP_ADDRESSED
NO_G8:                          ; check for 16-bit group addressing
        CJNE    A,#CMD_ADDR_GRP16,NO_G16
        MOV     A,IN_BUF+1
        CJNE    A,GROUP_ADDR,NOT_ADDRESSED
        MOV     A,IN_BUF+2
        CJNE    A,GROUP_ADDR+1,NOT_ADDRESSED
        SJMP    GROUP_ADDRESSED

NO_G16:
        CJNE    A,#CMD_ADDR_BC,INVAL_ADDRESS ; check for broadcast
        
        MOV     ADDR_MODE,#ADDR_ALL
        SETB    ADDRESSED
        CLR     LED             ; turn LED on
        RET

S_PING:
        MOV     A,IN_BUF        ; check for 8-bit ping addressing
        CJNE    A,#CMD_PING8,NO_P8
        
        MOV     A,IN_BUF+1
        CJNE    A,NODE_ADDR,NOT_ADDRESSED
        MOV     A,NODE_ADDR+1
        JNZ     NOT_ADDRESSED
        SJMP    PING_REPLY
NO_P8:                          ; check for 16-bit ping addressing
        CJNE    A,#CMD_PING16,INVAL_ADDRESS
        MOV     A,IN_BUF+1
        CJNE    A,NODE_ADDR,NOT_ADDRESSED
        MOV     A,IN_BUF+2
        CJNE    A,NODE_ADDR+1,NOT_ADDRESSED
        SJMP    PING_REPLY

INVAL_ADDRESS:       
        RET

NODE_ADDRESSED:
        MOV     ADDR_MODE,#ADDR_NODE
        SETB    ADDRESSED
        CLR     LED             ; turn LED on

        RET
        
GROUP_ADDRESSED:
        MOV     ADDR_MODE,#ADDR_GROUP
        SETB    ADDRESSED
        CLR     LED             ; turn LED on
        RET

PING_REPLY:
        MOV     OUT_BUF,#CMD_ACK
        MOV     N_OUT,#1
        CALL    SERIAL_START
        SJMP    NODE_ADDRESSED

NOT_ADDRESSED:                  ; node deselected
        MOV     ADDR_MODE,#ADDR_NONE
        CLR     ADDRESSED
        SETB    LED             ; turn LED off
        RET

;-------------- Non-data commands

S_INIT:                         ; do soft reset
        MOV     PSW,#0
        MOV     A,#0
        PUSH    ACC             ; start at 0000
        PUSH    ACC
        RETI
        
S_GET_INFO:
        CLR     ES              ; temporarily disable serial interrupt
        MOV     CRC_CODE,#0

        MOV     A,IN_BUF        ; get channel
        CJNE    A,#CMD_GET_INFO,GET_CHN_INFO
        JMP     GET_GENERAL_INFO

GET_CHN_INFO:                   ; return info about channel
        MOV     A,IN_BUF+1      ; channel type (1=write,2=conf)

        CJNE    A,#GET_INFO_CHANNEL,GET_CONF_PARAM
        MOV     DPTR,#CHANNEL_INFO
        SJMP    GOTO_CHN

GET_CONF_PARAM:
        MOV     DPTR,#CONF_PARAM

        ; go to selected channel
GOTO_CHN:
        MOV     R0,IN_BUF+2
        MOV     R1,#0
CHN_LOOP:
        MOV     A,R1
        MOVC    A,@A+DPTR
        JNZ     CHN_VALID       ; check if end of channel list
        JMP     INVAL_CHN
        
CHN_VALID:        
        MOV     A,R0
        JZ      GET_CHN
        MOV     A,R1
        ADD     A,#12           ; go to next channel
        MOV     R1,A

        DJNZ    R0,CHN_LOOP

GET_CHN:
        MOV     A,#CMD_ACK+7    ; send acknowlege, variable data length
        CALL    CRC8_ADD1
        MOV     OUT_BUF,A
        CALL    SERIAL_START
        JNB     TI,$
        CLR     TI

        MOV     A,#12           ; send data length
        CALL    CRC8_ADD1
        MOV     SBUF,A
        JNB     TI,$
        CLR     TI

        MOV     A,R1            ; send channel width
        MOVC    A,@A+DPTR       
        CALL    CRC8_ADD1
        MOV     SBUF,A
        JNB     TI,$
        CLR     TI
        INC     R1

        MOV     A,R1            ; send physical units
        MOVC    A,@A+DPTR       
        CALL    CRC8_ADD1
        MOV     SBUF,A
        JNB     TI,$
        CLR     TI
        INC     R1

        MOV     A,R1            ; send status 
        MOVC    A,@A+DPTR       
        CALL    CRC8_ADD1
        MOV     SBUF,A
        JNB     TI,$
        CLR     TI
        INC     R1

        MOV     A,R1            ; send channel flags
        MOVC    A,@A+DPTR       
        CALL    CRC8_ADD1
        MOV     SBUF,A
        JNB     TI,$
        CLR     TI
        INC     R1

        MOV     R0,#8
CN_LOOP:
        MOV     A,R1
        MOVC    A,@A+DPTR       
        CALL    CRC8_ADD1
        MOV     SBUF,A
        JNB     TI,$
        CLR     TI
        INC     R1
        DJNZ    R0,CN_LOOP

        MOV     A,CRC_CODE
        MOV     SBUF,A
        JNB     TI,$
        CLR     TI
        CALL    SERIAL_STOP

INVAL_CHN:
        SETB    ES              ; re-enable serial interrupt
        RET

GET_GENERAL_INFO:               ; return general info

        MOV     A,#CMD_ACK+7    ; send acknowlege, variable data length
        CALL    CRC8_ADD
        MOV     OUT_BUF,A
        CALL    SERIAL_START
        JNB     TI,$
        CLR     TI

        MOV     A,#26           ; send data length
        CALL    CRC8_ADD
        MOV     SBUF,A
        JNB     TI,$
        CLR     TI

        MOV     DPTR,#VERSION   ; send protocol version
        CLR     A
        MOVC    A,@A+DPTR       
        CALL    CRC8_ADD
        MOV     SBUF,A
        JNB     TI,$
        CLR     TI

        MOV     A,CSR           ; send node status
        CALL    CRC8_ADD
        MOV     SBUF,A
        JNB     TI,$
        CLR     TI

        MOV     A,N_CHANNEL     ; send number of channels
        CALL    CRC8_ADD
        MOV     SBUF,A
        JNB     TI,$
        CLR     TI

        MOV     A,N_CONF        ; send number of configuration parameters
        CALL    CRC8_ADD
        MOV     SBUF,A
        JNB     TI,$
        CLR     TI

        MOV     A,NODE_ADDR     ; send node address
        CALL    CRC8_ADD
        MOV     SBUF,A
        JNB     TI,$
        CLR     TI
        MOV     A,NODE_ADDR+1
        CALL    CRC8_ADD
        MOV     SBUF,A
        JNB     TI,$
        CLR     TI

        MOV     A,GROUP_ADDR    ; send group address
        CALL    CRC8_ADD
        MOV     SBUF,A
        JNB     TI,$
        CLR     TI
        MOV     A,GROUP_ADDR+1
        CALL    CRC8_ADD
        MOV     SBUF,A
        JNB     TI,$
        CLR     TI

        MOV     A,WD_COUNTER    ; send #watchdog resets
        CALL    CRC8_ADD
        MOV     SBUF,A
        JNB     TI,$
        CLR     TI
        MOV     A,WD_COUNTER+1
        CALL    CRC8_ADD
        MOV     SBUF,A
        JNB     TI,$
        CLR     TI

        MOV     R0,#16
        MOV     R1,#0
NN_LOOP:
        MOV     A,R1
        MOV     DPTR,#NODE_NAME ; send node name
        MOVC    A,@A+DPTR       
        CALL    CRC8_ADD
        MOV     SBUF,A
        JNB     TI,$
        CLR     TI
        JZ      NN_EOS          ; don't increment if end of string
        INC     R1
NN_EOS:
        DJNZ    R0,NN_LOOP

        MOV     A,CRC_CODE
        MOV     SBUF,A
        JNB     TI,$
        CLR     TI

        CALL    SERIAL_STOP
        SETB    ES              ; re-enable serial interrupt
        RET

S_SET_ADDR:
        MOV     NODE_ADDR,IN_BUF+1 ; copy addr to RAM
        MOV     NODE_ADDR+1,IN_BUF+2
        MOV     GROUP_ADDR,IN_BUF+3
        MOV     GROUP_ADDR+1,IN_BUF+4

        MOV     A,#0            ; write address to EEPROM, page 0
        MOV     R0,#IN_BUF+1
        MOV     R1,#4
        CALL    EEPROM_WRITE

        MOV     A,#0
        MOV     B,#0
        CALL    LCD_GOTO
        MOV     DPTR,#STR_NADR
        CALL    LCD_PUTS
        MOV     A,NODE_ADDR+1
        CALL    LCD_HEX
        MOV     A,NODE_ADDR
        CALL    LCD_HEX

        MOV     DPTR,#STR_GADR
        CALL    LCD_PUTS
        MOV     A,GROUP_ADDR+1
        CALL    LCD_HEX
        MOV     A,GROUP_ADDR
        CALL    LCD_HEX
        MOV     A,#0
        MOV     B,#0
        CALL    LCD_GOTO
        
        RET

S_SET_BAUD:        
        MOV     A,IN_BUF+1
        CALL    SET_BAUD
        RET

S_FREEZE:
        MOV     A,IN_BUF+1
        JZ      FREEZE_OFF
        SETB    FREEZE_MODE
        RET
FREEZE_OFF:
        CLR     FREEZE_MODE
        RET

S_SYNC:
        MOV     A,IN_BUF+1
        JZ      SYNC_OFF
        SETB    SYNC_MODE
        RET
SYNC_OFF:
        CLR     SYNC_MODE
        RET

S_TRANSP:                       ; TBD
        RET

;-------------- User function with parameter exchange

S_USER:
        MOV     R0,#IN_BUF+1    ; input parameters
        MOV     A,I_IN
        MOV     R1,#OUT_BUF+1   ; return parameters
        CALL    USER_FUNC

        MOV     R1,A            ; save #bytes
        MOV     R2,A

        MOV     CRC_CODE,#0     ; initialize CRC code
        MOV     A,#CMD_ACK      ; add data length to CMD_ACK
        ADD     A,R1
        MOV     OUT_BUF,A       ; set first return byte
        CALL    CRC8_ADD
        MOV     R0,#OUT_BUF+1   ; calculate CRC for remaining params
USER_CRC:
        MOV     A,R2
        JZ      USER_FIN
        
        MOV     A,@R0
        CALL    CRC8_ADD
        INC     R0
        DJNZ    R2,USER_CRC

USER_FIN:
        MOV     @R0,CRC_CODE
        MOV     N_OUT,R1
        INC     N_OUT           ; add one for CMD_ACK
        INC     N_OUT           ; add one for CRC

        CALL    SERIAL_START
        RET

;-------------- Write with/without acknowledge

S_WRITE_ACK:
        SETB    F0              ; remember acknowledge
S_WRITE_NA:
        MOV     A,IN_BUF+1      ; channel
        MOV     R0,#IN_BUF+2    ; data
        CALL    WRITE
        JB      F0,WRITE_ACK
        RET
WRITE_ACK:
        MOV     OUT_BUF,#CMD_ACK
        MOV     OUT_BUF+1,CRC_CODE ; reply with original CRC code
        MOV     N_OUT,#2
        CALL    SERIAL_START
        RET

;-------------- Write configuration parameter

S_WRITE_CONF_PERM:
        SETB    F0              ; remember perm
S_WRITE_CONF:                   ; 8-bit conf param
        MOV     A,IN_BUF+1
        MOV     R0,#IN_BUF+2
        CJNE    A,#0FFh,CONF_CHN
        
        ; write bits in CSR
        MOV     A,IN_BUF+2
        MOV     C,ACC.0
        MOV     DEBUG_MODE,C
        JNB     DEBUG_MODE,NO_CONF_PERM
        CALL    LCD_CLEAR       ; clear LCD on entering debug mode
        SJMP    NO_CONF_PERM
        
CONF_CHN:       
        CALL    WRITE_CONF

CONF_REPLY:
        JNB     F0,NO_CONF_PERM
        ; write parameter to EEPROM
        MOV     A,IN_BUF+1
        ADD     A,#2            ; start with page #2
        MOV     R0,#IN_BUF+2
        CALL    EEPROM_WRITE

NO_CONF_PERM:
        MOV     OUT_BUF,#CMD_ACK
        MOV     OUT_BUF+1,CRC_CODE ; reply with original CRC code
        MOV     N_OUT,#2
        CALL    SERIAL_START
        RET

;-------------- Read data

S_READ_CONF:
        SETB    F0
S_READ:
        MOV     A,IN_BUF+1
        MOV     R0,#OUT_BUF+1   ; leave space for CMD_ACK

        JB      F0,READ_CNF     ; F0 distinguishes read and read_conf
        CALL    READ
        SJMP    READ_RDY
READ_CNF:
        CALL    READ_CONF

READ_RDY:
        MOV     R1,A            ; save #bytes
        MOV     R2,A

        MOV     CRC_CODE,#0     ; initialize CRC code
        MOV     A,#CMD_ACK      ; add data length to CMD_ACK
        ADD     A,R1
        MOV     OUT_BUF,A       ; set first return byte
        CALL    CRC8_ADD
        MOV     R0,#OUT_BUF+1   ; calculate CRC for remaining params
READ_CRC:
        MOV     A,R2
        JZ      READ_FIN
        
        MOV     A,@R0
        CALL    CRC8_ADD
        INC     R0
        DJNZ    R2,READ_CRC

READ_FIN:
        MOV     @R0,CRC_CODE
        MOV     N_OUT,R1
        INC     N_OUT           ; add one for CMD_ACK
        INC     N_OUT           ; add one for CRC

        MOV     A,#CMD_ACK
        ADD     A,R1
        CALL    SERIAL_START
        RET
        
;-------------- Write block

S_WRITE_BLOCK:
        RET

;-------------- Read block

S_READ_BLOCK:
        RET
*/

void main(void)
{
  setup();

  do
    {
    watchdog_refresh();
/*
    user_loop();
    if (debug_mode)
      debug_output();
*/
    } while (1);
  
}
