/*-----------------------------------------------------------------------------
 *  Copyright (c) 1998      TRIUMF Data Acquistion Group
 *  Please leave this header in any reproduction of that distribution
 * 
 *  TRIUMF Data Acquisition Group, 4004 Wesbrook Mall, Vancouver, B.C. V6T 2A3
 *  Email: online@triumf.ca           Tel: (604) 222-1047  Fax: (604) 222-1074
 *         amaudruz@triumf.ca
 * -----------------------------------------------------------------------------
 *  
 *  Description : Midas Camac Standard calls. 
 *  Requires : 
 *  Application : Used in any camac driver
 *  Author:  Pierre-Andre Amaudruz Data Acquisition Group
 *
 *  $Log$
 *  Revision 1.8  2001/08/21 09:32:02  midas
 *  Added cam_lam_wait
 *
 *  Revision 1.7  2001/08/13 11:25:34  midas
 *  Added some new functions
 *
 *  Revision 1.6  2000/08/10 07:49:04  midas
 *  Added client name together with frontend name in cam_init_rpc
 *
 *  Revision 1.5  2000/04/17 17:38:10  pierre
 *  - First round of doc++ comments
 *
 *  Revision 1.4  1999/02/19 21:59:59  pierre
 *  - Moved came_xxx to esone
 *
 *  Revision 1.3  1998/10/13 07:04:29  midas
 *  Added Log in header
 *
 *
 *  Previous revision 1.0  1998        Pierre	 Initial revision
 *                    1.1  JUL 98      SR      Added 8-bit functions, BYTE definition
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

/* make functions under WinNT dll exportable */
#if defined(_MSC_VER) && defined(MIDAS_DLL)
#define EXPRT __declspec(dllexport)
#else
#define EXPRT
#endif

#define EXTERNAL extern

#ifndef MIDAS_TYPE_DEFINED
#define MIDAS_TYPE_DEFINED

typedef unsigned char      BYTE;
typedef unsigned short int WORD;

#ifdef __alpha
typedef unsigned int       DWORD;
#else
typedef unsigned long int  DWORD;
#endif

#define SUCCESS  1

#endif /* MIDAS_TYPE_DEFINED */

/*------------------------------------------------------------------*/

/* make functions callable from a C++ program */
#ifdef __cplusplus
extern "C" {
#endif
EXTERNAL INLINE void EXPRT cam8i(const int c, const int n, const int a, const int f, BYTE *d);

/** cam16i()
    16 bits input.
    @memo 16 bits read.
    @param c crate number (0..)
    @param n station number (0..30)
    @param a sub-address (0..15)
    @param f function (0..7)
    @param d data read out data
    @return void
*/
EXTERNAL INLINE void EXPRT cam16i(const int c, const int n, const int a, const int f, WORD *d);

/** cam24i()
    24 bits input.
    @memo 24 bits read.
    @param c crate number (0..)
    @param n station number (0..30)
    @param a sub-address (0..15)
    @param f function (0..7)
    @param d data read out data
    @return void
*/
EXTERNAL INLINE void EXPRT cam24i(const int c, const int n, const int a, const int f, DWORD *d);

/** cam8i\_q()
    8 bits input with Q response.
    @memo 8 bits read with X, Q response.
    @param c crate number (0..)
    @param n station number (0..30)
    @param a sub-address (0..15)
    @param f function (0..7)
    @param d data read out data
    @param x X response (0:failed,1:success)
    @param q Q resonpse (0:no Q, 1: Q)
    @return void
*/
EXTERNAL INLINE void EXPRT cam8i_q(const int c, const int n, const int a, const int f, BYTE *d, int *x, int *q);

/** cam16i\_q()
    16 bits input with Q response.
    @memo 16 bits read with X, Q response.
    @param c crate number (0..)
    @param n station number (0..30)
    @param a sub-address (0..15)
    @param f function (0..7)
    @param d data read out data
    @param x X response (0:failed,1:success)
    @param q Q resonpse (0:no Q, 1: Q)
    @return void
*/
EXTERNAL INLINE void EXPRT cam16i_q(const int c, const int n, const int a, const int f, WORD *d, int *x, int *q);

/** cam24i\_q()
    24 bits input with Q response.
    @memo 24 bits read with X, Q response.
    @param c crate number (0..)
    @param n station number (0..30)
    @param a sub-address (0..15)
    @param f function (0..7)
    @param d data read out data
    @param x X response (0:failed,1:success)
    @param q Q resonpse (0:no Q, 1: Q)
    @return void
*/
EXTERNAL INLINE void EXPRT cam24i_q(const int c, const int n, const int a, const int f, DWORD *d, int *x, int *q);
EXTERNAL INLINE void EXPRT cam8i_r(const int c, const int n, const int a, const int f, BYTE **d, const int r);

/** cam16i\_r()
    Repeat 16 bits input.
    @memo Repeat 16 bits read r times.
    @param c crate number (0..)
    @param n station number (0..30)
    @param a sub-address (0..15)
    @param f function (0..7)
    @param d data read out data
    @param r repeat time
    @return void
*/
EXTERNAL INLINE void EXPRT cam16i_r(const int c, const int n, const int a, const int f, WORD **d, const int r);

/** cam24i\_r()
    Repeat 24 bits input.
    @memo Repeat 24 bits read r times.
    @param c crate number (0..)
    @param n station number (0..30)
    @param a sub-address (0..15)
    @param f function (0..7)
    @param d data read out
    @param r repeat time
    @return void
*/
EXTERNAL INLINE void EXPRT cam24i_r(const int c, const int n, const int a, const int f, DWORD **d, const int r);

/** cam8i\_rq()
    Repeat 8 bits input with Q stop.
    @memo Repeat 16 bits read r times while Q.
    @param c crate number (0..)
    @param n station number (0..30)
    @param a sub-address (0..15)
    @param f function (0..7)
    @param d pointer to data read out
    @param r repeat time
    @return void
*/
EXTERNAL INLINE void EXPRT cam8i_rq(const int c, const int n, const int a, const int f, BYTE **d, const int r);

/** cam16i\_rq()
    Repeat 16 bits input with Q stop.
    @memo Repeat 16 bits read r times while Q.
    @param c crate number (0..)
    @param n station number (0..30)
    @param a sub-address (0..15)
    @param f function (0..7)
    @param d pointer to data read out
    @param r repeat time
    @return void
*/
EXTERNAL INLINE void EXPRT cam16i_rq(const int c, const int n, const int a, const int f, WORD **d, const int r);

/** cam24i\_rq
    Repeat 24 bits input with Q stop.
    @memo Repeat 24 bits read r times while Q.
    @param c crate number (0..)
    @param n station number (0..30)
    @param a sub-address (0..15)
    @param f function (0..7)
    @param d pointer to data read out
    @param r repeat time
    @return void
*/
EXTERNAL INLINE void EXPRT cam24i_rq(const int c, const int n, const int a, const int f, DWORD **d, const int r);

/** cam8i\_sa
    Read the given CAMAC address and increment the sub-address by one. Repeat r times.
    \\ {\bf Examples:} \begin{verbatim}
    BYTE pbkdat[4];
    cam8i_sa(crate, 5, 0, 2, &pbkdat, 4);
    \end{verbatim} equivalent to : \begin{verbatim} 
    cam8i(crate, 5, 0, 2, &pbkdat[0]);
    cam8i(crate, 5, 1, 2, &pbkdat[1]);
    cam8i(crate, 5, 2, 2, &pbkdat[2]);
    cam8i(crate, 5, 3, 2, &pbkdat[3]);
    \end{verbatim}
    @memo Scan read sub-address (8 bit).
    @param c crate number (0..)
    @param n station number (0..30)
    @param a sub-address (0..15)
    @param f function (0..7)
    @param d pointer to data read out
    @param r number of consecutive sub-address to read
    @return void
*/
EXTERNAL INLINE void EXPRT cam8i_sa(const int c, const int n, const int a, const int f, BYTE **d, const int r);

/** cam16i\_sa
    Read the given CAMAC address and increment the sub-address by one. Repeat r times.
    \\ {\bf Examples:} \begin{verbatim}
    WORD pbkdat[4];
    cam16i_sa(crate, 5, 0, 2, &pbkdat, 4);
    \end{verbatim} equivalent to : \begin{verbatim} 
    cam16i(crate, 5, 0, 2, &pbkdat[0]);
    cam16i(crate, 5, 1, 2, &pbkdat[1]);
    cam16i(crate, 5, 2, 2, &pbkdat[2]);
    cam16i(crate, 5, 3, 2, &pbkdat[3]);
    \end{verbatim}
    @memo Scan read sub-address (16 bit).
    @param c crate number (0..)
    @param n station number (0..30)
    @param a sub-address (0..15)
    @param f function (0..7)
    @param d pointer to data read out
    @param r number of consecutive sub-address to read
    @return void
*/
EXTERNAL INLINE void EXPRT cam16i_sa(const int c, const int n, const int a, const int f, WORD **d, const int r);

/** cam24i\_sa()
    Read the given CAMAC address and increment the sub-address by one. Repeat r times.
    \\ {\bf Examples:} \begin{verbatim}
    DWORD pbkdat[8];
    cam24i\_sa(crate, 5, 0, 2, &pbkdat, 8);
    \end{verbatim} equivalent to : \begin{verbatim} 
    cam24i(crate, 5, 0, 2, &pbkdat[0]);
    cam24i(crate, 6, 0, 2, &pbkdat[1]);
    cam24i(crate, 7, 0, 2, &pbkdat[2]);
    cam24i(crate, 8, 0, 2, &pbkdat[3]);
    \end{verbatim}
    @memo Scan read sub-address (24 bit).
    @param c crate number (0..)
    @param n station number (0..30)
    @param a sub-address (0..15)
    @param f function (0..7)
    @param d pointer to data read out
    @param r number of consecutive sub-address to read
    @return void
*/
EXTERNAL INLINE void EXPRT cam24i_sa(const int c, const int n, const int a, const int f, DWORD **d, const int r);

/** cam8i\_sn()
    Read the given CAMAC address and increment the station number by one. Repeat r times.
    \\ {\bf Examples:} \begin{verbatim}
    BYTE pbkdat[4];
    cam8i\_sa(crate, 5, 0, 2, &pbkdat, 4);
    \end{verbatim} equivalent to : \begin{verbatim} 
    cam8i(crate, 5, 0, 2, &pbkdat[0]);
    cam8i(crate, 6, 0, 2, &pbkdat[1]);
    cam8i(crate, 7, 0, 2, &pbkdat[2]);
    cam8i(crate, 8, 0, 2, &pbkdat[3]);
    \end{verbatim}
    @memo Scan read station (8 bit).
    @param c crate number (0..)
    @param n station number (0..30)
    @param a sub-address (0..15)
    @param f function (0..7)
    @param d pointer to data read out
    @param r number of consecutive station to read
    @return void
*/
EXTERNAL INLINE void EXPRT cam8i_sn(const int c, const int n, const int a, const int f, BYTE **d, const int r);

/** cam16i\_sn()
    Read the given CAMAC address and increment the station number by one. Repeat r times.
    \\ {\bf Examples:} \begin{verbatim}
    WORD pbkdat[4];
    cam16i\_sa(crate, 5, 0, 2, &pbkdat, 4);
    \end{verbatim} equivalent to : \begin{verbatim} 
    cam16i(crate, 5, 0, 2, &pbkdat[0]);
    cam16i(crate, 6, 0, 2, &pbkdat[1]);
    cam16i(crate, 7, 0, 2, &pbkdat[2]);
    cam16i(crate, 8, 0, 2, &pbkdat[3]);
    \end{verbatim}
    @memo Scan read station (16 bit).
    @param c crate number (0..)
    @param n station number (0..30)
    @param a sub-address (0..15)
    @param f function (0..7)
    @param d pointer to data read out
    @param r number of consecutive station to read
    @return void
*/
EXTERNAL INLINE void EXPRT cam16i_sn(const int c, const int n, const int a, const int f, WORD **d, const int r);

/** cam24i\_sn()
    Read the given CAMAC address and increment the station number by one. Repeat r times.
    \\ {\bf Examples:} \begin{verbatim}
    DWORD pbkdat[4];
    cam24i\_sa(crate, 5, 0, 2, &pbkdat, 4);
    \end{verbatim} equivalent to : \begin{verbatim} 
    cam24i(crate, 5, 0, 2, &pbkdat[0]);
    cam24i(crate, 6, 0, 2, &pbkdat[1]);
    cam24i(crate, 7, 0, 2, &pbkdat[2]);
    cam24i(crate, 8, 0, 2, &pbkdat[3]);
    \end{verbatim}
    @memo Scan read station (24 bit).
    @param c crate number (0..)
    @param n station number (0..30)
    @param a sub-address (0..15)
    @param f function (0..7)
    @param d pointer to data read out
    @param r number of consecutive station to read
    @return void
*/
EXTERNAL INLINE void EXPRT cam24i_sn(const int c, const int n, const int a, const int f, DWORD **d, const int r);

/** Same as cam16i()
*/
EXTERNAL INLINE void EXPRT cami(const int c, const int n, const int a, const int f, WORD *d);

/** cam8o()
    Write data to given CAMAC address.
    @memo Write 8 bits data.
    @param c crate number (0..)
    @param n station number (0..30)
    @param a sub-address (0..15)
    @param f function (16..31)
    @param d data to be written to CAMAC
    @return void
*/
EXTERNAL INLINE void EXPRT cam8o(const int c, const int n, const int a, const int f, BYTE  d);

/** cam16o()
    Write data to given CAMAC address.
    @memo Write 16 bits data.
    @param c crate number (0..)
    @param n station number (0..30)
    @param a sub-address (0..15)
    @param f function (16..31)
    @param d data to be written to CAMAC
    @return void
*/
EXTERNAL INLINE void EXPRT cam16o(const int c, const int n, const int a, const int f, WORD  d);

/** cam24o()
    Write data to given CAMAC address.
    @memo Write 24 bits data.
    @param c crate number (0..)
    @param n station number (0..30)
    @param a sub-address (0..15)
    @param f function (16..31)
    @param d data to be written to CAMAC
    @return void
*/
EXTERNAL INLINE void EXPRT cam24o(const int c, const int n, const int a, const int f, DWORD  d);

/** cam8o\_q()
    Write data to given CAMAC address with Q response.
    @memo Write 8 bits data with Q.
    @param c crate number (0..)
    @param n station number (0..30)
    @param a sub-address (0..15)
    @param f function (16..31)
    @param d data to be written to CAMAC
    @param x X response (0:failed,1:success)
    @param q Q resonpse (0:no Q, 1: Q)
    @return void
*/
EXTERNAL INLINE void EXPRT cam8o_q(const int c, const int n, const int a, const int f, BYTE d, int *x, int *q);

/** cam16o\_q()
    Write data to given CAMAC address with Q response.
    @memo Write 16 bits data with Q.
    @param c crate number (0..)
    @param n station number (0..30)
    @param a sub-address (0..15)
    @param f function (16..31)
    @param d data to be written to CAMAC
    @param x X response (0:failed,1:success)
    @param q Q resonpse (0:no Q, 1: Q)
    @return void
*/
EXTERNAL INLINE void EXPRT cam16o_q(const int c, const int n, const int a, const int f, WORD d, int *x, int *q);

/** cam24o\_q()
    Write data to given CAMAC address with Q response.
    @memo Write 24 bits data with Q.
    @param c crate number (0..)
    @param n station number (0..30)
    @param a sub-address (0..15)
    @param f function (16..31)
    @param d data to be written to CAMAC
    @param x X response (0:failed,1:success)
    @param q Q response (0:no Q, 1: Q)
    @return void
*/
EXTERNAL INLINE void EXPRT cam24o_q(const int c, const int n, const int a, const int f, DWORD d, int *x, int *q);

/** cam8o\_r()
    Repeat write data to given CAMAC address r times.
    @memo Repeat Write 8 bits data.
    @param c crate number (0..)
    @param n station number (0..30)
    @param a sub-address (0..15)
    @param f function (16..31)
    @param d data to be written to CAMAC
    @return void
*/
EXTERNAL INLINE void EXPRT cam8o_r(const int c, const int n, const int a, const int f, BYTE *d, const int r);

/** cam16o\_r()
    Repeat write data to given CAMAC address r times.
    @memo Repeat Write 16 bits data.
    @param c crate number (0..)
    @param n station number (0..30)
    @param a sub-address (0..15)
    @param f function (16..31)
    @param d data to be written to CAMAC
    @return void
*/
EXTERNAL INLINE void EXPRT cam16o_r(const int c, const int n, const int a, const int f, WORD *d, const int r);

/** cam24o\_r()
    Repeat write data to given CAMAC address r times.
    @memo Repeat Write 24 bits data.
    @param c crate number (0..)
    @param n station number (0..30)
    @param a sub-address (0..15)
    @param f function (16..31)
    @param d data to be written to CAMAC
    @return void
*/
EXTERNAL INLINE void EXPRT cam24o_r(const int c, const int n, const int a, const int f, DWORD *d, const int r);

/** Same as cam16o()
*/
EXTERNAL INLINE void EXPRT camo(const int c, const int n, const int a, const int f, WORD d);

/** camc\_chk()
    Crate presence check.
    @memo Crate presence check.
    @param c crate number (0..)
    @return 0:Success, -1:No CAMAC response
*/
EXTERNAL INLINE int  EXPRT camc_chk(const int c);

/** camc()
    CAMAC command (no data).
    @memo CAMAC command.
    @param c crate number (0..)
    @param n station number (0..30)
    @param a sub-address (0..15)
    @param f function (8..15, 24..31)
    @return void
*/
EXTERNAL INLINE void EXPRT camc(const int c, const int n, const int a, const int f);

/** camc\_q()
    CAMAC command with Q response (no data).
    @memo CAMAC command with Q.
    @param c crate number (0..)
    @param n station number (0..30)
    @param a sub-address (0..15)
    @param f function (8..15, 24..31)
    @param q Q response (0:no Q, 1:Q)
    @return void
*/
EXTERNAL INLINE void EXPRT camc_q(const int c, const int n, const int a, const int f, int *q);

/** camc\_sa()
    Scan CAMAC command on sub-address.
    @memo Scan command on sub-address.
    @param c crate number (0..)
    @param n station number (0..30)
    @param a sub-address (0..15)
    @param f function (8..15, 24..31)
    @param r number of consecutive sub-address to read
    @return void
*/
EXTERNAL INLINE void EXPRT camc_sa(const int c, const int n, const int a, const int f, const int r);

/** camc\_sn()
    Scan CAMAC command on station.
    @memo Scan command on station.
    @param c crate number (0..)
    @param n station number (0..30)
    @param a sub-address (0..15)
    @param f function (8..15, 24..31)
    @param r number of consecutive station to read
    @return void
*/
EXTERNAL INLINE void EXPRT camc_sn(const int c, const int n, const int a, const int f, const int r);

/** cam\_init()
    Initialize CAMAC for access.
    @memo CAMAC initilize.
    @return 1: success
*/
EXTERNAL INLINE int  EXPRT cam_init(void);
EXTERNAL INLINE int  EXPRT cam_init_rpc(char *host_name, char *exp_name, char *fe_name, char *client_name, char *rpc_server);

/** cam\_exit()
    Close CAMAC accesss.
    @memo Close CAMAC.
*/
EXTERNAL INLINE void EXPRT cam_exit(void);

/** cam\_inhibit\_set()
    Set Crate inhibit.
    @memo Set Crate inhibit.
    @param c crate number (0..)
    @return void
*/
EXTERNAL INLINE void EXPRT cam_inhibit_set(const int c);

/** cam\_inhibit\_clear()
    Clear Crate inhibit.
    @memo Clear Crate inhibit.
    @param c crate number (0..)
    @return void
*/
EXTERNAL INLINE void EXPRT cam_inhibit_clear(const int c);

/** cam\_inhibit\_test()
    Test Crate Inhibit.
    @memo Test Crate inhibit.
    @param c crate number (0..)
    @return 1 for set, 0 for cleared
*/
EXTERNAL INLINE int EXPRT cam_inhibit_test(const int c);

/** cam\_crate\_clear()
    Issue CLEAR to crate.
    @memo Clear Crate.
    @param c crate number (0..)
    @return void
*/
EXTERNAL INLINE void EXPRT cam_crate_clear(const int c);

/** cam\_crate\_zinit()
    Issue Z to crate.
    @memo Z Crate.
    @param c crate number (0..)
    @return void
*/
EXTERNAL INLINE void EXPRT cam_crate_zinit(const int c);

/** cam\_lam\_enable()
    Enable LAM generation for given station to the Crate controller.
    It doesn't enable the LAM of the actual station itself.
    @memo Enable LAM (Crate Controller).
    @param c crate number (0..)
    @param n LAM station
    @return void
*/
EXTERNAL INLINE void EXPRT cam_lam_enable(const int c, const int n);

/** cam\_lam\_disable()
    Disable LAM generation for given station to the Crate controller.
    It doesn't disable the LAM of the actual station itself.
    @memo Disable LAM (Crate Controller).
    @param c crate number (0..)
    @param n LAM station
    @return void
*/
EXTERNAL INLINE void EXPRT cam_lam_disable(const int c, const int n);

/** cam\_lam\_read()
    Reads in lam the lam pattern of the entire crate.
    @memo Read LAM of crate controller.
    @param c crate number (0..)
    @param lam LAM pattern of the crate
    @return void
*/
EXTERNAL INLINE void EXPRT cam_lam_read(const int c, DWORD *lam);

/** cam\_lam\_clear()
    Clear the LAM register of the crate controller.
    It doesn't clear the LAM of the particular station.
    @memo Clear LAM register (Crate Controller).
    @param c crate number (0..)
    @param lam LAM pattern of the crate
    @param n LAM station
    @return void
*/
EXTERNAL INLINE void EXPRT cam_lam_clear(const int c, const int n);

/** cam\_lam\_wait()
    Wait for a LAM to occur with a certain timeout. Return
    crate and station if LAM occurs.
    @memo Wait for LAM.
    @param c crate number (0..)
    @param lam LAM pattern with a bit set for the station which generated the LAM 
    @param millisec If there is no LAM after this timeout, the routine returns
    @return 1 if LAM occured, 0 else
*/
EXTERNAL INLINE int EXPRT cam_lam_wait(int *c, DWORD *n, const int millisec);

EXTERNAL INLINE void EXPRT cam_interrupt_enable (const int c);
EXTERNAL INLINE void EXPRT cam_interrupt_disable(const int c);
EXTERNAL INLINE int  EXPRT cam_interrupt_test   (const int c);
EXTERNAL INLINE void EXPRT cam_interrupt_attach (void (*isr)(void));
EXTERNAL INLINE void EXPRT cam_interrupt_detach (void);
EXTERNAL INLINE void EXPRT cam_glint_enable     (void);
EXTERNAL INLINE void EXPRT cam_glint_disable    (void);
EXTERNAL INLINE void EXPRT cam_glint_attach     (int lam, void (*isr)(void));
EXTERNAL INLINE void EXPRT cam_glint_detach(void);

#ifdef __cplusplus
}
#endif
