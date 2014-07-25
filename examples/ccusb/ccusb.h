#ifndef _CCUSB_INCLUDE_H
#define _CCUSB_INCLUDE_H

#include <stdio.h>
#include <string.h>

#define D16 0x0000   //!< Data16
#define D24 0x4000   //!< Data 24
#define RNAFD16(n,a,f) (D16 | (n<<9) | (a<<5) | f) //!< Macro for Read D16
#define RNAFD24(n,a,f) (D24 | (n<<9) | (a<<5) | f) //!< Macro for Read D24
#define MARKER         (D16 | (0<<9) | (0<<5) | 16)
#define RD16    1  //!< Read D16
#define MRAD16  2  //!< Multiple Read on A D16
#define MRND16  3  //!< Multiple Read on N D16
#define RD24    11 //!< Read D24
#define MRAD24  12 //!< Multiple Read on A D24
#define MRND24  13 //!< Multiple Read on N D24
#define CD16    21 //!< Command D16
#define WMARKER 50 //!< Marker

//
//---------------------------------------------------------------------
/**
 * \brief Stack builder 
 *
 * To be used in the Begin_of_run for camac list definition
 * Will be loaded in the CCUSB at end of BOR
 *
 * \param mode CAMAC action type
 * \param n Station number
 * \param a Station subaddress
 * \param f CAMAC function
 * \param d data
 * \param *stack stack list 
 */
int StackFill(int mode, int n, int a, int f, int d, long int *stack) {
  
  int lvar=0;

  // New entry
  switch (mode) {
  case RD16:  // Read 
  case CD16:  // Command (same as read)
    stack[0] += 1;  lvar = stack[0];
    stack[lvar] = RNAFD16(n, a, f);
    break;
  case MRAD16:
    for (int i=0;i<d;i++) {
      int aa = a+i;
      stack[0] += 1;  lvar = stack[0];
      stack[lvar] = RNAFD16(n, aa, f);
    }
    break;
  case MRND16:
    for (int i=0;i<d;i++) {
      int nn = n+i;
      stack[0] += 1;  lvar = stack[0];
      stack[lvar] = RNAFD16(nn, a, f);
    }
    break;
  case RD24:
    stack[0] += 1;  lvar = stack[0];
    stack[lvar] = RNAFD24(n, a, f);
    break;
  case MRAD24:
    for (int i=0;i<d;i++) { 
      int aa = a+i;
      stack[0] += 1;  lvar = stack[0];
      stack[lvar] = RNAFD24(n, aa, f);
    }
    break;
  case MRND24:
    for (int i=0;i<d;i++) { 
      int nn = n+i;
      stack[0] += 1;  lvar = stack[0];
      stack[lvar] = RNAFD24(nn, a, f);
    }
    break;
  case WMARKER:
    stack[0] += 1; lvar = stack[0];
    stack[lvar] = MARKER; 
    stack[0] += 1; lvar = stack[0];
    stack[lvar] = d; 
    break;
  default:
    break;
  }

  return stack[0];
}

//
//---------------------------------------------------------------------
/**
   To be called before the StackFill()
 */
void StackCreate(long int *stack) {
  stack[0] = 0;
}

//
//---------------------------------------------------------------------
/**
   To be called after StackFill()
 */
void StackClose(long int *stack) {
  // nothing to do for now
}

#endif // _CCUSB_INCLUDE_H
