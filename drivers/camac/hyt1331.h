/*
  Name:         hyt1331.h
  Created by:   P.Knowles
  Contents:     Hytec Camac interface card  parameters...

*/
#ifndef  _HYTEC_IO_HEADER
#define  _HYTEC_IO_HEADER

/* Maximal 4 PC cards                   */
#define MAX_HYTEC_CARDS 4           
/* Maximal 4 crates per card            */
#define MAX_CRATES_PER_CARD 4            

// IO base address for PC interfaces
extern WORD io_base[MAX_HYTEC_CARDS]; 
// IRQ for PC interfaces
extern BYTE irq[MAX_HYTEC_CARDS];     
// Switch 1D on HYT1331 crate controllers
extern int gbl_sw1d[MAX_HYTEC_CARDS][MAX_CRATES_PER_CARD]; 

#endif // _HYTEC_IO_HEADER
