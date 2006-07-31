/*********************************************************************

  Name:         mfbstr.h
  Created by:   Stefan Ritt

  Cotents:      MIDAS FASTBUS standard routines. Has to be combined
                with either LRS1821.C or STR340.C
                
  $Id$

*********************************************************************/

/*---- replacements if not running under MIDAS ---------------------*/

#ifndef MIDAS_TYPE_DEFINED
#define MIDAS_TYPE_DEFINED

typedef unsigned short int WORD;

#ifndef _MSC_VER
typedef unsigned int DWORD;
#endif

#define SUCCESS  1

#endif                          /* MIDAS_TYPE_DEFINED */

/* make functions under WinNT dll exportable */
#if defined(_MSC_VER) && defined(MIDAS_DLL)
#define EXPRT __declspec(dllexport)
#else
#define EXPRT
#endif


/*---- function declarations ---------------------------------------*/

/* make functions callable from a C++ program */
#ifdef __cplusplus
extern "C" {
#endif

   int EXPRT fb_init();
   void EXPRT fb_exit();
   int EXPRT fb_reset(void);

   int EXPRT fb_frd(int paddr, int saddr, DWORD * data);
   int EXPRT fb_frc(int paddr, int saddr, DWORD * data);
   int EXPRT fb_fwd(int paddr, int saddr, DWORD data);
   int EXPRT fb_fwc(int paddr, int saddr, DWORD data);
   int EXPRT fb_fwdm(int paddr, int saddr, DWORD data);
   int EXPRT fb_fwcm(int b_case, int paddr, int saddr, DWORD data);
   int EXPRT fb_frcm(int b_case, DWORD * data);
   int EXPRT fb_frdb(int paddr, int saddr, DWORD * data, int *count);
   int EXPRT fb_out(DWORD data);
   int EXPRT fb_in(void);
   void EXPRT fb_frdba(int paddr, int saddr, int count);
   int EXPRT fb_load_begin(int addr);
   int EXPRT fb_load_end(void);
   int EXPRT fb_execute(int addr, void *buffer, int *count);

#ifdef __cplusplus
}
#endif
