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

int hs_connect_odbc(const char* odbc_dsn); ///< connect to given ODBC DSN, defined in $HOME/.odbc.ini
int hs_disconnect_odbc();                  ///< disconnect from ODBC

int hs_debug_odbc(int debug);              ///< set debug level, returns previous debug level

int hs_define_event_odbc(const char* event_name, const TAG tags[], int tags_size);
int hs_write_event_odbc(const char*  event_name, time_t timestamp, const char* buffer, int buffer_size);

int hs_read_odbc(uint32_t event_id,
                 time_t start_time, time_t end_time, time_t interval,
                 const char* event_name, const char* tag_name, int var_index,
                 uint32_t* time_buffer, uint32_t *tbsize,
                 void *data_buffer, uint32_t *dbsize,
                 uint32_t *type, uint32_t *n);

#ifdef __cplusplus
}
#endif

#endif
// end
