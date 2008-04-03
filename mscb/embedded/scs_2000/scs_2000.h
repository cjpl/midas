/********************************************************************\

  Name:         scs_2000.h
  Created by:   Stefan Ritt


  Contents:     Header file for scs_2000 special functions

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
                                                     
} SCS_2000_MODULE;


/*---- Functions prototypes ----------------------------------------*/

void address_port(unsigned char addr, unsigned char port_no, 
                  unsigned char am, unsigned char clk_level);
void write_port(unsigned char addr, unsigned char port_no, unsigned char d);
void read_port(unsigned char addr, unsigned char port_no, unsigned char *pd);
void read_eeprom(unsigned char addr, unsigned char port_no, unsigned char *pd);
void write_eeprom(unsigned char addr, unsigned char port_no, unsigned char d);
unsigned char module_present(unsigned char addr, unsigned char port_no);
unsigned char verify_module(unsigned char addr, unsigned char port_no, unsigned char id);
unsigned char power_status(unsigned char addr);
void power_24V(unsigned char addr, unsigned char flag);
void power_beeper(unsigned char addr, unsigned char flag);
unsigned char is_master();
unsigned char slave_addr();
unsigned char is_present(unsigned char addr);

/*---- Drivers -----------------------------------------------------*/

unsigned char dr_dout_bits(unsigned char id, unsigned char cmd, unsigned char addr, unsigned char port, unsigned char chn, void *pd) reentrant;
unsigned char dr_dout_byte(unsigned char id, unsigned char cmd, unsigned char addr, unsigned char port, unsigned char chn, void *pd) reentrant;
unsigned char dr_din_bits(unsigned char id, unsigned char cmd, unsigned char addr, unsigned char port, unsigned char chn, void *pd) reentrant;
unsigned char dr_ltc2600(unsigned char id, unsigned char cmd, unsigned char addr, unsigned char port, unsigned char chn, void *pd) reentrant;
unsigned char dr_ad5764(unsigned char id, unsigned char cmd, unsigned char addr, unsigned char port, unsigned char chn, void *pd) reentrant;
unsigned char dr_ad7718(unsigned char id, unsigned char cmd, unsigned char addr, unsigned char port, unsigned char chn, void *pd) reentrant;
unsigned char dr_ads1256(unsigned char id, unsigned char cmd, unsigned char addr, unsigned char port, unsigned char chn, void *pd) reentrant;
unsigned char dr_capmeter(unsigned char id, unsigned char cmd, unsigned char addr, unsigned char port, unsigned char chn, void *pd) reentrant;
unsigned char dr_temp(unsigned char id, unsigned char cmd, unsigned char addr, unsigned char port, unsigned char chn, void *pd) reentrant;
unsigned char dr_lhe(unsigned char id, unsigned char cmd, unsigned char addr, unsigned char port, unsigned char chn, void *pd) reentrant;

