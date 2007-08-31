/********************************************************************\

  Name:         scs_2000_led.h
  Created by:   Stefan Ritt


  Contents:     Header file for scs_2000 special functions

  $Id: scs_2000.h 2874 2005-11-15 08:47:14Z ritt $

\********************************************************************/

/*---- Modules structure -------------------------------------------*/

/* module commands */
#define MC_INIT   1
#define MC_WRITE  2
#define MC_READ   3
#define MC_FREQ   4
#define MC_PWIDTH 5

/*---- Functions prototypes ----------------------------------------*/

void address_port(unsigned char addr, unsigned char port_no, 
                  unsigned char am, unsigned char clk_level) reentrant;
void write_port(unsigned char addr, unsigned char port_no, unsigned char d) reentrant;
void read_port(unsigned char addr, unsigned char port_no, unsigned char *pd) reentrant;
void read_eeprom(unsigned char addr, unsigned char port_no, unsigned char *pd) reentrant;
void write_eeprom(unsigned char addr, unsigned char port_no, unsigned char d) reentrant;
unsigned char module_present(unsigned char addr, unsigned char port_no) reentrant;
unsigned char power_mgmt(unsigned char addr, unsigned char reset) reentrant;
unsigned char verify_module(unsigned char addr, unsigned char port_no, unsigned char id) reentrant;

unsigned char dr_pulser(unsigned char id, unsigned char cmd, unsigned char addr, 
                        unsigned char port, unsigned char chn, void *pd) reentrant;

