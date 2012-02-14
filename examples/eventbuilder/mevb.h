/********************************************************************\
Name:         mevb.h
  Created by:   Pierre-Andre Amaudruz

  Contents:     Event builder header file

  $Id$

\********************************************************************/

#define EBUILDER(_name) char const *_name[] = {\
"[.]",\
"Number of Fragment = INT : 0",\
"User build = BOOL : n",\
"User Field = STRING : [64] 100",\
"Fragment Required = BOOL[2] :",\
"[0] y",\
"[1] y",\
"",\
NULL }


typedef struct {
   WORD event_id;
   WORD trigger_mask;
   INT  nfragment;
   char buffer[32];
   char format[32];
   BOOL user_build;
   char user_field[64];
   char hostname[64];
   BOOL *preqfrag;
   BOOL *received;
} EBUILDER_SETTINGS;

typedef struct {
   double events_sent;
   double events_per_sec_;
   double kbytes_per_sec_;
} EBUILDER_STATISTICS;

typedef struct {
   char buffer[32];
   char format[32];
   WORD event_id;
   WORD trigger_mask;
   INT type;
   INT hBuf;
   INT req_id;
   INT hStat;
   time_t time;
   INT timeout;
   DWORD serial;
   char *pfragment;
} EBUILDER_CHANNEL;

#define   EB_SUCCESS               1
#define   EB_COMPOSE_TIMEOUT      -1
#define   EB_ERROR              1001
#define   EB_USER_ERROR         1002
#define   EB_ABORTED            1003
#define   EB_SKIP               1004
#define   EB_BANK_NOT_FOUND        0
#define   TIMEOUT              10000
#define   MAX_CHANNELS           128
