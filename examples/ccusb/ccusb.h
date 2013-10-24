#ifndef _CCUSB_INCLUDE_H
#define _CCUSB_INCLUDE_H

#include <stdio.h>
#include <string.h>

#define D16 0x0000
#define D24 0x4000
#define RNAFD16(n,a,f) (D16 | (n<<9) | (a<<5) | f)
#define RNAFD24(n,a,f) (D24 | (n<<9) | (a<<5) | f)
#define MARKER         (D16 | (0<<9) | (0<<5) | 16)
#define RD16    1
#define MRAD16  2
#define MRND16  3
#define RD24    11
#define MRAD24  12
#define MRND24  13
#define CD16    21
#define CND16   23
#define WMARKER 50

//
//---------------------------------------------------------------------
int StackFill(int mode, int n, int a, int f, int d, long int *stack) {

  int lvar=0;

  // New entry

  switch (mode) {
  case RD16:
  case CD16:
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

void StackCreate(long int *stack) {
  stack[0] = 0;
}

void StackClose(long int *stack) {
  // nothing to do for now
}

#endif // _CCUSB_INCLUDE_H
