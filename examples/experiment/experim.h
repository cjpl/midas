/********************************************************************\

  Name:         experim.h
  Created by:   ODBedit program

  Contents:     This file contains C structures for the "Experiment"
                tree in the ODB and the "/Analyzer/Parameters" tree.

                Additionally, it contains the "Settings" subtree for
                all items listed under "/Equipment" as well as their
                event definition.

                It can be used by the frontend and analyzer to work
                with these information.

                All C structures are accompanied with a string represen-
                tation which can be used in the db_create_record function
                to setup an ODB structure which matches the C structure.

  Created on:   Fri Apr 11 12:58:35 2003

\********************************************************************/

#ifndef EXCL_TRIGGER

#define TEST_BANK_DEFINED

typedef struct {
  float     a;
  float     b;
  char      str[32];
} TEST_BANK;

#define TEST_BANK_STR(_name) char *_name[] = {\
"[.]",\
"a = FLOAT : 1.234",\
"b = FLOAT : 2.24923e+006",\
"str = STRING : [32] Hello",\
"",\
NULL }

#define ADC0_BANK_DEFINED

typedef struct {
  WORD      adc0;
  WORD      adc1;
  WORD      adc2;
  WORD      adc3;
} ADC0_BANK;

#define ADC0_BANK_STR(_name) char *_name[] = {\
"[.]",\
"adc0 = WORD : 0",\
"adc1 = WORD : 0",\
"adc2 = WORD : 0",\
"adc3 = WORD : 0",\
"",\
NULL }

#define TRIGGER_COMMON_DEFINED

typedef struct {
  WORD      event_id;
  WORD      trigger_mask;
  char      buffer[32];
  INT       type;
  INT       source;
  char      format[8];
  BOOL      enabled;
  INT       read_on;
  INT       period;
  double    event_limit;
  DWORD     num_subevents;
  INT       log_history;
  char      frontend_host[32];
  char      frontend_name[32];
  char      frontend_file_name[256];
} TRIGGER_COMMON;

#define TRIGGER_COMMON_STR(_name) char *_name[] = {\
"[.]",\
"Event ID = WORD : 1",\
"Trigger mask = WORD : 0",\
"Buffer = STRING : [32] SYSTEM",\
"Type = INT : 2",\
"Source = INT : 16777215",\
"Format = STRING : [8] MIDAS",\
"Enabled = BOOL : y",\
"Read on = INT : 257",\
"Period = INT : 500",\
"Event limit = DOUBLE : 0",\
"Num subevents = DWORD : 0",\
"Log history = INT : 0",\
"Frontend host = STRING : [32] pc810",\
"Frontend name = STRING : [32] Sample Frontend",\
"Frontend file name = STRING : [256] c:\online\frontend.c",\
"",\
NULL }

#endif

#ifndef EXCL_SCALER

#define SCALER_COMMON_DEFINED

typedef struct {
  WORD      event_id;
  WORD      trigger_mask;
  char      buffer[32];
  INT       type;
  INT       source;
  char      format[8];
  BOOL      enabled;
  INT       read_on;
  INT       period;
  double    event_limit;
  DWORD     num_subevents;
  INT       log_history;
  char      frontend_host[32];
  char      frontend_name[32];
  char      frontend_file_name[256];
} SCALER_COMMON;

#define SCALER_COMMON_STR(_name) char *_name[] = {\
"[.]",\
"Event ID = WORD : 2",\
"Trigger mask = WORD : 0",\
"Buffer = STRING : [32] SYSTEM",\
"Type = INT : 17",\
"Source = INT : 0",\
"Format = STRING : [8] MIDAS",\
"Enabled = BOOL : y",\
"Read on = INT : 377",\
"Period = INT : 10000",\
"Event limit = DOUBLE : 0",\
"Num subevents = DWORD : 0",\
"Log history = INT : 0",\
"Frontend host = STRING : [32] pc810",\
"Frontend name = STRING : [32] Sample Frontend",\
"Frontend file name = STRING : [256] c:\online\frontend.c",\
"",\
NULL }

#endif

