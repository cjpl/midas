/********************************************************************\
Name:         mevb.h
  Created by:   Pierre-Andre Amaudruz

  Contents:     Event builder header file
  $Log$
  Revision 1.3  2002/06/14 04:57:09  pierre
  revised for 1.9.x

\********************************************************************/

#define EBUILDER(_name) char *_name[] = {\
"[Settings]",\
"Event ID = WORD : 1",\
"Trigger mask = WORD : 1",\
"Buffer = STRING : [32] SYSTEM",\
"Format = STRING : [32] MIDAS",\
"User build = BOOL : n",\
"Event mask = DWORD : 3",\
"Hostname = STRING : [64] ",\
"",\
"[Statistics]",\
"Events sent = DOUBLE : 0",\
"Events per sec. = DOUBLE : 0",\
"kBytes per sec. = DOUBLE : 0",\
"",\
NULL }

#define EBUILDER_CHANNEL(_name) char *_name[] = {\
"[Frag1/Settings]",\
"Event ID = WORD : 1",\
"Trigger mask = WORD : -1",\
"Buffer = STRING : [32] BUF1",\
"Format = STRING : [32] MIDAS",\
"Event mask = DWORD : 1",\
"User Field = STRING : [64] ",\
"",\
"[Frag1/Statistics]",\
"Events sent = DOUBLE : 0",\
"Events per sec. = DOUBLE : 0",\
"kBytes per sec. = DOUBLE : 0",\
"",\
"[Frag2/Settings]",\
"Event ID = WORD : 2",\
"Trigger mask = WORD : -1",\
"Buffer = STRING : [32] BUF2",\
"Format = STRING : [32] MIDAS",\
"Event mask = DWORD : 2",\
"User Field = STRING : [64] ",\
"",\
"[Frag2/Statistics]",\
"Events sent = DOUBLE : 0",\
"Events per sec. = DOUBLE : 0",\
"kBytes per sec. = DOUBLE : 0",\
"",\
NULL }

typedef struct {
    WORD      event_id;
    WORD      trigger_mask;
    char      buffer[32];
    char      format[32];
    BOOL      user_build;
    DWORD     emask;
    char      hostname[64];
  } EBUILDER_SETTINGS;

typedef struct {
    WORD      event_id;
    WORD      trigger_mask;
    char      buffer[32];
    char      format[32];
    DWORD     emask;
    char      user_field[64];
  } EBUILDER_SETTINGS_CH;

typedef struct {
    double    events_sent;
    double    events_per_sec_;
    double    kbytes_per_sec_;
  } EBUILDER_STATISTICS;

typedef struct {
    char  name[32];
    INT   hBuf;
    INT   req_id;
    INT   hStat;
    INT   timeout;
    DWORD serial;
    char *pfragment;
    EBUILDER_SETTINGS_CH   set;
    EBUILDER_STATISTICS   stat;
} EBUILDER_CHANNEL;

#define   EB_SUCCESS               1
#define   EB_COMPOSE_TIMEOUT      -1
#define   EB_ERROR              1001
#define   TIMEOUT                 10
#define   MAX_CHANNELS             8
