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
/// \note
///
/// Decoding event_names and var_names returned by hs_enum_events_odbc(), hs_enum_vars_odbc() and hs_get_events_odbc():
///
/// \code
///      int num_events = 0;
///      char* event_names = NULL;
///
///      int status = hs_enum_events_odbc(int *num_events, char**event_names);
///
///      printf("enum events: num %d\n", num_events);
///      for (int i=0, p=0; i<num_events; i++) {
///         if (event_names[p]==0)
///            break;
///         printf("event %d [%s]\n", i, event_names+p);
///         p += strlen(event_names+p) + 1;
///      }
///
///      free(event_names);
/// \endcode


int hs_connect_odbc(const char* odbc_dsn); ///< connect to given ODBC DSN, defined in $HOME/.odbc.ini, returns HS_SUCCESS
int hs_disconnect_odbc();                  ///< disconnect from ODBC, returns HS_SUCCESS

int hs_debug_odbc(int debug);              ///< set debug level, returns previous debug level

int hs_define_event_odbc(const char* event_name, const TAG tags[], int tags_size); ///< see hs_define_event(), returns HS_SUCCESS
int hs_write_event_odbc(const char*  event_name, time_t timestamp, const char* buffer, int buffer_size); ///< see hs_write_event(), returns HS_SUCCESS

int hs_enum_events_odbc(int *num_events, char**event_names); ///< enumerate all history events, returns HS_SUCCESS
int hs_enum_vars_odbc(const char* event_name, int *num_vars, char** var_names); ///< enumerate all variables for events listed by hs_enum_events_odbc(), returns HS_SUCCESS

int hs_get_events_odbc(int *n_events, char **events); ///< see hs_get_xxx(), returns HS_SUCCESS
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
