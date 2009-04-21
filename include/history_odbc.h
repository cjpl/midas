/********************************************************************\

  Name:         history_odbc.h
  Created by:   Konstantin Olchanski / TRIUMF

  Contents:     MIDAS history stored in ODBC SQL database


  $Id$

\********************************************************************/

#ifndef HISTORY_ODBC_H
#define HISTORY_ODBC_H

#ifdef __cplusplus
extern "C" {
#endif

/// \file history_odbc.h
///

int hs_connect_odbc(const char* odbc_dsn); ///< connect to given ODBC DSN, defined in $HOME/.odbc.ini, returns HS_SUCCESS
int hs_disconnect_odbc();                  ///< disconnect from ODBC, returns HS_SUCCESS

int hs_debug_odbc(int debug);              ///< set debug level, returns previous debug level
int hs_set_alarm_odbc(const char* alarm_name); ///< set alarm name for history failures. Use NULL to disable alarms

int hs_define_event_odbc(const char* event_name, const TAG tags[], int tags_size); ///< see hs_define_event(), returns HS_SUCCESS or HS_FILE_ERROR
int hs_write_event_odbc(const char*  event_name, time_t timestamp, const char* buffer, int buffer_size); ///< see hs_write_event(), returns HS_SUCCESS or HS_FILE_ERROR

int hs_get_tags_odbc(const char* event_name, int *n_tags, TAG **tags); ///< use event names returned by hs_get_events_odbc(), see hs_get_tags(), returns HS_SUCCESS

int hs_read_odbc(time_t start_time, time_t end_time, time_t interval,
                 const char* event_name, const char* tag_name, int var_index,
                 int *num_entries,
                 time_t** time_buffer, double**data_buffer); ///< see hs_read(), returns HS_SUCCESS

#ifdef __cplusplus
}
#endif

#endif
// end
