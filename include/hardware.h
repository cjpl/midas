/********************************************************************\

  Name:         HARDWARE.H
  Created by:   Stefan Ritt

  Contents:     Type ID definitions and structured for specific
                hardware.

  $Id$

\********************************************************************/

/* Hardware data types. Can be used to create MIDAS banks. Should
   be or'ed with standard data types like TID_DWORD | TID_LRS1882 */

#define TID_LRS1882    (1<<8)   /* LeCroy 1882 fastbus ADC */
#define TID_LRS1877    (2<<8)   /* LeCroy 1877 fastbus TDC */
#define TID_PCOS3      (3<<8)   /* LeCroy PCOS 3 wire chambers */
#define TID_LRS1875    (4<<8)   /* LeCroy 1875 fastbus TDC */

/* Hardware data records */

typedef struct {
   WORD data:12;
    WORD:4;
   WORD channel:7;
   WORD range:1;
   WORD event:3;
   WORD geo_addr:5;
} LRS1882_DATA;

typedef struct {
   WORD data:16;
   WORD edge:1;
   WORD channel:7;
   WORD buffer:2;
   WORD parity:1;
   WORD geo_addr:5;
} LRS1877_DATA;

typedef struct {
   WORD count:11;
   WORD buffer:3;
    WORD:1;
   WORD parity:1;
    WORD:11;
   WORD geo_addr:5;
} LRS1877_HEADER;

typedef struct {
  WORD data     : 12;
  WORD          : 4;
  WORD channel  : 6;
  WORD          : 1;
  WORD range    : 1;
  WORD event    : 3;
  WORD geo_addr : 5;
} LRS1875_DATA;

typedef struct {
  WORD          : 8;
  WORD count    : 6; 
  WORD          : 2; 
  WORD crate    : 8; 
  WORD tag      : 3;
  WORD geo_addr : 5; 
} CAEN775_HEADER;

typedef struct {
  WORD data     : 12;
  WORD overflow : 1;
  WORD underflow: 1;
  WORD valid    : 1;
  WORD          : 1;
  WORD channel  : 6;
  WORD          : 2;
  WORD tag      : 3;
  WORD geo_addr : 5;
} CAEN775_DATA;
