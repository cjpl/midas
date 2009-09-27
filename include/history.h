/********************************************************************\

  Name:         history.h
  Created by:   Konstantin Olchanski / TRIUMF

  Contents:     Interface for the MIDAS history system

  $Id$

\********************************************************************/

#ifndef HISTORY_H
#define HISTORY_H

#include <string>
#include <vector>

/// \file history.h
///

class MidasHistoryInterface;
class MidasSqlInterface;

// "factory" pattern

MidasHistoryInterface* MakeMidasHistory();
MidasHistoryInterface* MakeMidasHistoryODBC();

// MIDAS history interface class

class MidasHistoryInterface
{
 private:

 protected:
  virtual ~MidasHistoryInterface() { };

 public:
  virtual int hs_connect(const char* connect_string) = 0; ///< returns HS_SUCCESS
  virtual int hs_disconnect() = 0;                  ///< disconnect from history, returns HS_SUCCESS

  virtual int hs_set_debug(int debug) = 0;          ///< set debug level, returns previous debug level
  virtual int hs_set_alarm(const char* alarm_name) = 0; ///< set alarm name for history failures. Use NULL to disable alarms

  // functions for writing into the history, used by mlogger

  virtual int hs_define_event(const char* event_name, int ntags, const TAG tags[]) = 0; ///< see hs_define_event(), returns HS_SUCCESS or HS_FILE_ERROR

  virtual int hs_write_event(const char*  event_name, time_t timestamp, int data_size, const char* data) = 0; ///< see hs_write_event(), returns HS_SUCCESS or HS_FILE_ERROR

  // functions for reading from the history, used by mhttpd, mhist

  virtual int hs_get_events(std::vector<std::string> **pevents) = 0; ///< get list of all events, returns HS_SUCCESS

  virtual int hs_get_tags(const char* event_name, int *n_tags, TAG **tags) = 0; ///< use event names returned by hs_get_events_odbc(), see hs_get_tags(), returns HS_SUCCESS

  virtual int hs_read(time_t start_time, time_t end_time, time_t interval,
                 const char* event_name, const char* tag_name, int var_index,
                 int *num_entries,
                 time_t** time_buffer, double**data_buffer) = 0; ///< see hs_read(), returns HS_SUCCESS
};

#endif
// end
