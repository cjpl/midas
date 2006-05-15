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
#define MC_SPECIAL      4

typedef struct {
  unsigned char id;                                  // unique module ID, 0xFF for uninitialized 
  char *name;                                        // module name
  MSCB_INFO_VAR *var;                                // list of variables
  unsigned char (*driver)(unsigned char id,          // driver for init/read/wrie
                  unsigned char cmd,                                
                  unsigned char addr, unsigned char port, 
                  unsigned char chn , void *) reentrant;  
                                                     
} SCS_2000_MODULE;


/*---- Functions prototypes ----------------------------------------*/

void address_port(unsigned char addr, unsigned char port_no, 
                  unsigned char am, unsigned char clk_level) reentrant;
void write_port(unsigned char addr, unsigned char port_no, unsigned char d) reentrant;
void read_port(unsigned char addr, unsigned char port_no, unsigned char *pd) reentrant;
void read_eeprom(unsigned char addr, unsigned char port_no, unsigned char *pd) reentrant;
void write_eeprom(unsigned char addr, unsigned char port_no, unsigned char d) reentrant;
unsigned char module_present(unsigned char addr, unsigned char port_no) reentrant;

