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

  Created on:   Thu Feb  1 21:20:48 2007
  $Id$
\********************************************************************/

#ifndef EXCL_TRIGGER

#define TRIGGER_SETTINGS_DEFINED

typedef struct {
  struct {
    BOOL      external_trigger;
    BOOL      inverse_signal;
    INT       segment_size;
    INT       group_bitwise_all_0x3f_;
    BOOL      raw_data_suppress;
    BOOL      channel_suppress_enable;
    INT       active_channel_mask;
    INT       trigger_threshold;
    INT       hit_threshold;
    INT       pretrigger;
    INT       latency;
    INT       divisor;
    INT       k_coeff;
    INT       l_coeff;
    INT       m_coeff;
  } vf48;
  struct {
    WORD      threshold1[32];
    WORD      threshold2[32];
    WORD      threshold3[32];
    WORD      threshold4[32];
  } v792;
  struct {
    INT       leresolution;
    INT       windowoffset;
    INT       windowwidth;
  } v1190;
  struct {
    DWORD     post_trigger;
  } v1729;
} TRIGGER_SETTINGS;

#define TRIGGER_SETTINGS_STR(_name) char *_name[] = {\
"[vf48]",\
"External Trigger = BOOL : y",\
"Inverse Signal = BOOL : y",\
"Segment size = INT : 128",\
"Group(bitwise all:0x3F) = INT : 63",\
"Raw Data Suppress = BOOL : n",\
"Channel Suppress Enable = BOOL : n",\
"Active Channel Mask = INT : 255",\
"Trigger Threshold = INT : 20",\
"Hit Threshold = INT : 100",\
"PreTrigger = INT : 15",\
"Latency = INT : 14",\
"Divisor = INT : 2",\
"K-Coeff = INT : 60",\
"L-Coeff = INT : 72",\
"M-Coeff = INT : 16",\
"",\
"[v792]",\
"threshold1 = WORD[32] :",\
"[0] 1",\
"[1] 511",\
"[2] 511",\
"[3] 511",\
"[4] 511",\
"[5] 511",\
"[6] 511",\
"[7] 511",\
"[8] 511",\
"[9] 511",\
"[10] 511",\
"[11] 511",\
"[12] 511",\
"[13] 511",\
"[14] 511",\
"[15] 511",\
"[16] 511",\
"[17] 511",\
"[18] 511",\
"[19] 511",\
"[20] 511",\
"[21] 511",\
"[22] 511",\
"[23] 511",\
"[24] 511",\
"[25] 511",\
"[26] 511",\
"[27] 511",\
"[28] 511",\
"[29] 511",\
"[30] 511",\
"[31] 511",\
"threshold2 = WORD[32] :",\
"[0] 10",\
"[1] 10",\
"[2] 10",\
"[3] 511",\
"[4] 511",\
"[5] 511",\
"[6] 511",\
"[7] 511",\
"[8] 511",\
"[9] 511",\
"[10] 511",\
"[11] 511",\
"[12] 511",\
"[13] 511",\
"[14] 511",\
"[15] 511",\
"[16] 511",\
"[17] 511",\
"[18] 511",\
"[19] 511",\
"[20] 511",\
"[21] 511",\
"[22] 511",\
"[23] 511",\
"[24] 511",\
"[25] 511",\
"[26] 511",\
"[27] 511",\
"[28] 511",\
"[29] 511",\
"[30] 511",\
"[31] 511",\
"threshold3 = WORD[32] :",\
"[0] 1",\
"[1] 1",\
"[2] 1",\
"[3] 511",\
"[4] 511",\
"[5] 511",\
"[6] 511",\
"[7] 511",\
"[8] 511",\
"[9] 511",\
"[10] 511",\
"[11] 511",\
"[12] 511",\
"[13] 511",\
"[14] 511",\
"[15] 511",\
"[16] 511",\
"[17] 511",\
"[18] 511",\
"[19] 511",\
"[20] 511",\
"[21] 511",\
"[22] 511",\
"[23] 511",\
"[24] 511",\
"[25] 511",\
"[26] 511",\
"[27] 511",\
"[28] 511",\
"[29] 511",\
"[30] 511",\
"[31] 511",\
"threshold4 = WORD[32] :",\
"[0] 10",\
"[1] 10",\
"[2] 10",\
"[3] 511",\
"[4] 511",\
"[5] 511",\
"[6] 511",\
"[7] 511",\
"[8] 511",\
"[9] 511",\
"[10] 511",\
"[11] 511",\
"[12] 511",\
"[13] 511",\
"[14] 511",\
"[15] 511",\
"[16] 511",\
"[17] 511",\
"[18] 511",\
"[19] 511",\
"[20] 511",\
"[21] 511",\
"[22] 511",\
"[23] 511",\
"[24] 511",\
"[25] 511",\
"[26] 511",\
"[27] 511",\
"[28] 511",\
"[29] 511",\
"[30] 511",\
"[31] 511",\
"",\
"[v1190]",\
"LeResolution = INT : 0",\
"WindowOffset = INT : 0",\
"WindowWidth = INT : 0",\
"",\
"[v1729]",\
"post_trigger = DWORD : 0",\
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
"Type = INT : 1",\
"Source = INT : 0",\
"Format = STRING : [8] MIDAS",\
"Enabled = BOOL : y",\
"Read on = INT : 1",\
"Period = INT : 1",\
"Event limit = DOUBLE : 0",\
"Num subevents = DWORD : 0",\
"Log history = INT : 0",\
"Frontend host = STRING : [32] laddvme05.triumf.ca",\
"Frontend name = STRING : [32] FEvf48",\
"Frontend file name = STRING : [256] fevf48.c",\
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
"Type = INT : 1",\
"Source = INT : 0",\
"Format = STRING : [8] MIDAS",\
"Enabled = BOOL : n",\
"Read on = INT : 377",\
"Period = INT : 10000",\
"Event limit = DOUBLE : 0",\
"Num subevents = DWORD : 0",\
"Log history = INT : 0",\
"Frontend host = STRING : [32] laddvme05.triumf.ca",\
"Frontend name = STRING : [32] FEvf48",\
"Frontend file name = STRING : [256] fevf48.c",\
"",\
NULL }

#endif

