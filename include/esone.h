/*-----------------------------------------------------------------------------
 *  Copyright (c) 1998      TRIUMF Data Acquistion Group
 *  Please leave this header in any reproduction of that distribution
 * 
 *  TRIUMF Data Acquisition Group, 4004 Wesbrook Mall, Vancouver, B.C. V6T 2A3
 *  Email: online@triumf.ca           Tel: (604) 222-1047  Fax: (604) 222-1074
 *         amaudruz@triumf.ca
 * -----------------------------------------------------------------------------
 *  
 *  Description : Esone Standard calls. 
 *  Requires : 
 *  Application : Used in any camac driver
 *  Author:  Pierre-Andre Amaudruz Data Acquisition Group
 *
 *  $Log$
 *  Revision 1.2  2000/04/17 17:33:59  pierre
 *  - First round of doc++ comments.
 *
 *  Revision 1.1  1999/02/22 18:55:10  pierre
 *  - esone standard
 *
 *  Revision 1.3  1998/10/13 07:04:29  midas
 *  Added Log in header
 *
 *  Previous revision 1.0  1998        Pierre	 Initial revision
 *---------------------------------------------------------------------------*/

#ifndef INLINE
#if defined( _MSC_VER )
#define INLINE __inline
#elif defined(__GNUC__)
#define INLINE __inline__
#else 
#define INLINE
#endif
#endif

#define EXTERNAL extern 

/* make functions under WinNT dll exportable */
#if defined(_MSC_VER) && defined(MIDAS_DLL)
#define EXPRT __declspec(dllexport)
#else
#define EXPRT
#endif

/*------------------------------------------------------------------*/

/* make functions callable from a C++ program */
#ifdef __cplusplus
extern "C" {
#endif

/* -- ESONE standard ---------------------------------------------------*/
/** cdreg
    Control Declaration REGister.

    Compose an external address from BCNA for later use.
    Accessing CAMAC through ext could be faster if the external address is
    memory mapped to the processor (hardware dependent). Some CAMAC controller
    do not have this option see \Ref{appendix D: Supported hardware}. In this case
    @memo External Address register.
    @param ext external address
    @param b branch number (0..7)
    @param c crate number (0..)
    @param n station number (0..30)
    @param a sub-address (0..15)
    @return void
*/
EXTERNAL INLINE void cdreg(int *ext, const int b, const int c, const int n, const int a);

/** cssa
    Control Short Operation.

    16 bit operation on a given external CAMAC address.

    The range of the f is hardware dependent. The number indicated below are for
    standard ANSI/IEEE Std (758-1979) \\
    Execute cam16i for f<8, cam16o for f>15, camc\_q for (f>7 or f>23)
    
    @memo 16 bit function.
    @param f function code (0..31)
    @param ext external address
    @param d data word
    @param q Q response
    @return void
*/
EXTERNAL INLINE void cssa (const int f, int ext, unsigned short *d, int *q);

/** cfsa
    Control Full Operation.

    24 bit operation on a given external CAMAC address.

    The range of the f is hardware dependent. The number indicated below are for
    standard ANSI/IEEE Std (758-1979) \\
    Execute cam24i for f<8, cam24o for f>15, camc\_q for (f>7 or f>23)
    
    @memo 24 bit function.
    @param f function code (0..31)
    @param ext external address
    @param d data long word
    @param q Q response
    @return void
*/
EXTERNAL INLINE void cfsa (const int f, const int ext, unsigned long *d, int *q);

/** cccc
    Control Crate Clear.

    Generate Crate Clear function. Execute cam\_crate\_clear()
    
    @memo Crate Clear.
    @param ext external address
    @return void
*/
EXTERNAL INLINE void cccc (const int ext);

/** cccz
    Control Crate Z.

    Generate Dataway Initialize. Execute cam\_crate\_zinit()
    
    @memo Crate Z.
    @param ext external address
    @return void
*/
EXTERNAL INLINE void cccz (const int ext);

/** ccci
    Control Crate I.

    Set or Clear Dataway Inhibit, Execute cam\_inhinit\_set() /clear()
    which is equivalent to c=c,n=30,a=9,f26/f24
    
    @memo Crate I.
    @param ext external address
    @param l action l=0 -> Clear I, l=1 -> Set I
    @return void
*/
EXTERNAL INLINE void ccci (const int ext, int l);

/** cccd
    Control Crate D.

    Enable or Disable Crate Demand. Execute c=c,n=30,a=10,f=26 for enable
    , f24 for disable.
    \\ {\bf Has not been tested}
    
    @memo Crate D.
    @param ext external address
    @param l action l=0 -> Clear D, l=1 -> Set D
    @return void
*/
EXTERNAL INLINE void cccd (const int ext, int l);

/** ctcd
    Control Test Crate D.

    Test Crate Demand. Execute c=c,n=30,a=10,f=27.
    \\ {\bf Has not been tested}
    
    @memo Test Crate D.
    @param ext external address
    @param l D cleared -> l=0, D set -> l=1
    @return void
*/
EXTERNAL INLINE void ctcd (const int ext, int *l);

/** cdlam
    Control Declare LAM.

    Declare LAM, Identical to cdreg.
    \\ {\bf Has not been tested}
    
    @memo Declare LAM.
    @param lam external LAM address
    @param b branch number (0..7)
    @param c crate number (0..)
    @param n station number (0..30)
    @param a sub-address (0..15)
    @param inta[2] implementation dependent
    @return void
*/
EXTERNAL INLINE void cdlam(int *lam, const int b, const int c, const int n, const int a, const int inta[2]);

/** ctgl
    Control Test Demand Present.

    Test the Graded LAM register. Execute c=c,n=30,a=11,f=27.
    \\ {\bf Has not been tested}
    
    @memo Test GL.
    @param lam external LAM register address
    @param l  l !=0 if any LAM is set.
    @return void
*/
EXTERNAL INLINE void ctgl (const int lam, int *l);

/** cclm
    Control Crate LAM.

    Enable or Disable LAM. Execute F24 for disable, F26 for enable.
    \\ {\bf Has not been tested}
    
    @memo dis/enable LAM.
    @param lam external address
    @param l action l=0 -> disable LAM , l=1 -> enable LAM
    @return void
*/
EXTERNAL INLINE void cclm (const int lam, int l);

/** cclc
    Control Clear LAM.
    Clear the LAM of th estaion pointer by the lam address.
    \\ {\bf Has not been tested}

    @memo Clear LAM.
    @param lam external address
    @return void
*/
EXTERNAL INLINE void cclc (const int lam);

/** ctlm
    Test LAM.

    Test the LAM of the station pointed by lam. Performs an F10
    \\ {\bf Has not been tested}
    
    @memo Test LAM.
    @param lam external address
    @param l No LAM-> l=0, LAM present-> l=1
    @return void
*/
EXTERNAL INLINE void ctlm (const int lam, int *l);

/** cfga
    Control Full (24bit) word General Action.
    \\ {\bf Has not been tested}

    @memo General external address scan function.
    @param f function code
    @param exta[] external address array
    @param intc[] data array
    @param qa[] Q response array
    @param cb[] control block array \\
    cb[0] : number of function to perform \\
    cb[1] : returned number of function performed
    @return void
*/
EXTERNAL INLINE void cfga (int f[], int exta[], int intc[], int qa[], int cb[]);

/** cfmad
    Control Full (24bit) Address Q scan.

    Scan all sub-address while Q=1 from a0..a15 max from address extb[0] and store
    corresponding data in intc[]. If Q=0 while A<15 or A=15 then cross station boundary is applied
    (n-> n+1) and sub-address is reset (a=0). Perform action until either cb[0] action are performed
    or current external address exceeds extb[1].

    {\it implementation of cb[2] for LAM recognition is not implemented.}
    \\ {\bf Has not been tested}
    
    @memo Address scan function.
    @param f function code
    @param extb[] external address array \\
    extb[0] : first valid external address \\
    extb[1] : last valid external address
    @param intc[] data array
    @param qa[] Q response array
    @param cb[] control block array \\
    cb[0] : number of function to perform \\
    cb[1] : returned number of function performed
    @return void
*/
EXTERNAL INLINE void cfmad(int f, int extb[], int intc[], int cb[]);

/** cfubc
    Control Full (24bit) Block Repeat with Q-stop.

    Execute function f on address ext with data intc[] while Q.
    \\ {\bf Has not been tested}
    
    @memo Repeat function Q-stop.
    @param f function code
    @param ext external address array
    @param intc[] data array
    @param cb[] control block array \\
    cb[0] : number of function to perform \\
    cb[1] : returned number of function performed
    @return void
*/
EXTERNAL INLINE void cfubc(const int f, int ext, int intc[], int cb[]);

/** cfubr
    Control Full (24bit) Block Repeat.

    Execute function f on address ext with data intc[] if Q.
    If noQ keep current intc[] data. Repeat cb[0] times. 
    \\ {\bf Has not been tested}
    @memo  Repeat function.
    @param f function code
    @param ext external address array
    @param intc[] data array
    @param cb[] control block array \\
    cb[0] : number of function to perform \\
    cb[1] : returned number of function performed
    @return void
*/
EXTERNAL INLINE void cfubr(const int f,int ext,int intc[],int cb[]);
/*-----------------------------------------------------------------------*/
