/********************************************************************\

  Name:         scs_2001.h
  Created by:   Stefan Ritt


  Contents:     Header file for scs_2001 special functions

  $Id: scs_2000.h 2874 2005-11-15 08:47:14Z ritt $

\********************************************************************/

/*---- Modules structure -------------------------------------------*/

/* module commands */
#define MC_INIT         1
#define MC_WRITE        2
#define MC_READ         3
#define MC_GETDEFAULT   4

typedef struct {
  unsigned char id;                                  // unique module ID, 0xFF for uninitialized 
  char *name;                                        // module name
  MSCB_INFO_VAR *var;                                // list of variables
  unsigned char n_var;                               // number of variable sets
  unsigned char (*driver)(unsigned char id,          // driver for init/read/wrie
                  unsigned char cmd,                                
                  unsigned char addr, unsigned char port, 
                  unsigned char chn , void *) reentrant;  
                                                     
} SCS_2001_MODULE;

/*---- Port definitions  ----------------------------------------*/

sbit OPT_CLK    = P3 ^ 0;
sbit OPT_ALE    = P3 ^ 1;
sbit OPT_STR    = P3 ^ 2;
sbit OPT_DATAO  = P3 ^ 3;

sbit OPT_DATAI  = P3 ^ 4;
sbit OPT_STAT   = P3 ^ 5;
sbit OPT_SPARE1 = P3 ^ 6;
sbit OPT_SPARE2 = P3 ^ 7;

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

/*---- Functions prototypes ----------------------------------------*/

void address_port(unsigned char addr, unsigned char port_no, unsigned char am);
void address_port1(unsigned char addr, unsigned char port_no, 
                   unsigned char am, unsigned char clk_level);
void write_port(unsigned char addr, unsigned char port_no, unsigned char d);
void read_port(unsigned char addr, unsigned char port_no, unsigned char *pd);
void read_eeprom(unsigned char addr, unsigned char port_no, unsigned char *pd);
void write_eeprom(unsigned char addr, unsigned char port_no, unsigned char d);
unsigned char module_present(unsigned char addr, unsigned char port_no);
unsigned char verify_module(unsigned char addr, unsigned char port_no, unsigned char id);
void power_beeper(unsigned char addr, unsigned char flag);
void read_csr(unsigned char addr, unsigned char *pd);
void write_csr(unsigned char addr, unsigned char d);
unsigned char is_present(unsigned char addr);
unsigned char is_master();
unsigned char slave_addr();

void monitor_init(unsigned char addr);
void monitor_read(unsigned char uaddr, unsigned char cmd, unsigned char raddr, unsigned char *pd, unsigned char nbytes);
void monitor_clear(unsigned char addr);

/*---- Drivers -----------------------------------------------------*/

unsigned char dr_dout_bits(unsigned char id, unsigned char cmd, unsigned char addr, unsigned char port, unsigned char chn, void *pd) reentrant;
unsigned char dr_dout_byte(unsigned char id, unsigned char cmd, unsigned char addr, unsigned char port, unsigned char chn, void *pd) reentrant;
unsigned char dr_din_bits(unsigned char id, unsigned char cmd, unsigned char addr, unsigned char port, unsigned char chn, void *pd) reentrant;
unsigned char dr_ltc2600(unsigned char id, unsigned char cmd, unsigned char addr, unsigned char port, unsigned char chn, void *pd) reentrant;
unsigned char dr_ad5764(unsigned char id, unsigned char cmd, unsigned char addr, unsigned char port, unsigned char chn, void *pd) reentrant;
unsigned char dr_ad7718(unsigned char id, unsigned char cmd, unsigned char addr, unsigned char port, unsigned char chn, void *pd) reentrant;
unsigned char dr_ads1256(unsigned char id, unsigned char cmd, unsigned char addr, unsigned char port, unsigned char chn, void *pd) reentrant;
unsigned char dr_capmeter(unsigned char id, unsigned char cmd, unsigned char addr, unsigned char port, unsigned char chn, void *pd) reentrant;
unsigned char dr_temp8(unsigned char id, unsigned char cmd, unsigned char addr, unsigned char port, unsigned char chn, void *pd) reentrant;
unsigned char dr_temp2(unsigned char id, unsigned char cmd, unsigned char addr, unsigned char port, unsigned char chn, void *pd) reentrant;
unsigned char dr_ad590(unsigned char id, unsigned char cmd, unsigned char addr, unsigned char port, unsigned char chn, void *pd) reentrant;
unsigned char dr_lhe(unsigned char id, unsigned char cmd, unsigned char addr, unsigned char port, unsigned char chn, void *pd) reentrant;
unsigned char dr_floatin(unsigned char id, unsigned char cmd, unsigned char addr, unsigned char port, unsigned char chn, void *pd) reentrant;
unsigned char dr_hv(unsigned char id, unsigned char cmd, unsigned char addr, unsigned char port, unsigned char chn, void *pd) reentrant;

