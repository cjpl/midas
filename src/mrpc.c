/********************************************************************\

  Name:         MRPC.C
  Created by:   Stefan Ritt

  Contents:     List of MIDAS RPC functions with parameters

  $Log$
  Revision 1.4  1999/01/13 09:40:48  midas
  Added db_set_data_index2 function

  Revision 1.3  1998/10/12 12:19:02  midas
  Added Log tag in header

  Revision 1.2  1998/10/12 11:59:11  midas
  Added Log tag in header

\********************************************************************/

#include "midas.h"
#include "msystem.h"

/*------------------------------------------------------------------*/

/* 
   rpc_list_library contains all MIDAS library functions and gets
   registerd whenever a connection to the MIDAS server is established 
 */

static RPC_LIST rpc_list_library[] = {

  /* common functions */
  { RPC_CM_SET_CLIENT_INFO,
    {{TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_OUT}, 
     {TID_STRING,     RPC_IN}, 
     {TID_STRING,     RPC_IN}, 
     {TID_INT,        RPC_IN}, 
     {TID_STRING,     RPC_IN}, 
     {0} }},

  { RPC_CM_SET_WATCHDOG_PARAMS,
    {{TID_BOOL,       RPC_IN}, 
     {TID_INT,        RPC_IN}, 
     {0} }},

  { RPC_CM_CLEANUP,
    {{0} }},

  { RPC_CM_GET_WATCHDOG_INFO,
    {{TID_INT,        RPC_IN},
     {TID_STRING,     RPC_IN},
     {TID_INT,        RPC_OUT}, 
     {TID_INT,        RPC_OUT}, 
     {0} }},

  { RPC_CM_MSG_LOG,
    {{TID_INT,        RPC_IN},
     {TID_STRING,     RPC_IN},
     {0} }},

  { RPC_CM_EXECUTE,
    {{TID_STRING,     RPC_IN},
     {TID_STRING,     RPC_OUT},
     {TID_INT,        RPC_IN},
     {0} }},

  { RPC_CM_SYNCHRONIZE,
    {{TID_DWORD,      RPC_OUT},
     {0} }},

  { RPC_CM_ASCTIME,
    {{TID_STRING,     RPC_OUT},
     {TID_INT,        RPC_IN},
     {0} }},

  { RPC_CM_TIME,
    {{TID_DWORD,      RPC_OUT},
     {0} }},

  /* buffer manager functions */

  { RPC_BM_OPEN_BUFFER,
    {{TID_STRING,     RPC_IN}, 
     {TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_OUT},
     {0} }},

  { RPC_BM_CLOSE_BUFFER,
    {{TID_INT,        RPC_IN}, 
     {0} }},

  { RPC_BM_CLOSE_ALL_BUFFERS,
    { {0} }},

  { RPC_BM_GET_BUFFER_INFO,
    {{TID_INT,        RPC_IN}, 
     {TID_STRUCT,     RPC_OUT, sizeof(BUFFER_HEADER) }, 
     {0} }},

  { RPC_BM_GET_BUFFER_LEVEL,
    {{TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_OUT}, 
     {0} }},

  { RPC_BM_INIT_BUFFER_COUNTERS,
    {{TID_INT,        RPC_IN}, 
     {0} }},

  { RPC_BM_SET_CACHE_SIZE,
    {{TID_INT,        RPC_IN},
     {TID_INT,        RPC_IN},
     {TID_INT,        RPC_IN},
     {0} }},

  { RPC_BM_ADD_EVENT_REQUEST,
    {{TID_INT,        RPC_IN}, 
     {TID_SHORT,      RPC_IN}, 
     {TID_SHORT,      RPC_IN}, 
     {TID_INT,        RPC_IN},
     {TID_INT,        RPC_IN},
     {TID_INT,        RPC_IN},
     {0} }},

  { RPC_BM_REMOVE_EVENT_REQUEST,
    {{TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN}, 
     {0} }},

  { RPC_BM_SEND_EVENT,
    {{TID_INT,        RPC_IN}, 
     {TID_ARRAY,      RPC_IN | RPC_VARARRAY}, 
     {TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN}, 
     {0} }},

  { RPC_BM_FLUSH_CACHE,
    {{TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN}, 
     {0} }},

  { RPC_BM_RECEIVE_EVENT,
    {{TID_INT,        RPC_IN}, 
     {TID_ARRAY,      RPC_OUT | RPC_VARARRAY}, 
     {TID_INT,        RPC_IN | RPC_OUT}, 
     {TID_INT,        RPC_IN}, 
     {0} }},

  { RPC_BM_MARK_READ_WAITING,
    {{TID_BOOL,       RPC_IN}, 
     {0} }},

  { RPC_BM_EMPTY_BUFFERS,
    {{0} }},

  /* online database functions */

  { RPC_DB_OPEN_DATABASE,
    {{TID_STRING,     RPC_IN}, 
     {TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_OUT},
     {TID_STRING,     RPC_IN},
     {0} }},

  { RPC_DB_CLOSE_DATABASE,
    {{TID_INT,        RPC_IN}, 
     {0} }},

  { RPC_DB_FLUSH_DATABASE,
    {{TID_INT,        RPC_IN}, 
     {0} }},

  { RPC_DB_CLOSE_ALL_DATABASES,
    { {0} }},

  { RPC_DB_CREATE_KEY,
    {{TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN}, 
     {TID_STRING,     RPC_IN}, 
     {TID_DWORD,      RPC_IN},
     {0} }},

  { RPC_DB_CREATE_LINK,
    {{TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN}, 
     {TID_STRING,     RPC_IN}, 
     {TID_STRING,     RPC_IN},
     {0} }},

  { RPC_DB_SET_VALUE,
    {{TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN}, 
     {TID_STRING,     RPC_IN}, 
     {TID_ARRAY,      RPC_IN | RPC_VARARRAY}, 
     {TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN},
     {TID_DWORD,      RPC_IN},
     {0} }},

  { RPC_DB_GET_VALUE,
    {{TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN}, 
     {TID_STRING,     RPC_IN}, 
     {TID_ARRAY,      RPC_IN | RPC_OUT | RPC_VARARRAY}, 
     {TID_INT,        RPC_IN | RPC_OUT}, 
     {TID_DWORD,      RPC_IN},
     {0} }},

  { RPC_DB_FIND_KEY,
    {{TID_INT,        RPC_IN},
     {TID_INT,        RPC_IN},
     {TID_STRING,     RPC_IN}, 
     {TID_INT,        RPC_OUT}, 
     {0} }},

  { RPC_DB_FIND_LINK,
    {{TID_INT,        RPC_IN},
     {TID_INT,        RPC_IN},
     {TID_STRING,     RPC_IN}, 
     {TID_INT,        RPC_OUT}, 
     {0} }},

  { RPC_DB_GET_PATH,
    {{TID_INT,        RPC_IN},
     {TID_INT,        RPC_IN},
     {TID_STRING,     RPC_OUT}, 
     {TID_INT,        RPC_IN},
     {0} }},

  { RPC_DB_DELETE_KEY,
    {{TID_INT,        RPC_IN},
     {TID_INT,        RPC_IN},
     {TID_BOOL,       RPC_IN},
     {0} }},

  { RPC_DB_ENUM_KEY,
    {{TID_INT,        RPC_IN},
     {TID_INT,        RPC_IN},
     {TID_INT,        RPC_IN},
     {TID_INT,        RPC_OUT}, 
     {0} }},

  { RPC_DB_ENUM_LINK,
    {{TID_INT,        RPC_IN},
     {TID_INT,        RPC_IN},
     {TID_INT,        RPC_IN},
     {TID_INT,        RPC_OUT}, 
     {0} }},

  { RPC_DB_GET_KEY,
    {{TID_INT,        RPC_IN},
     {TID_INT,        RPC_IN},
     {TID_STRUCT,     RPC_OUT, sizeof(KEY)}, 
     {0} }},

  { RPC_DB_GET_KEY_TIME,
    {{TID_INT,        RPC_IN},
     {TID_INT,        RPC_IN},
     {TID_DWORD,      RPC_OUT}, 
     {0} }},

  { RPC_DB_RENAME_KEY,
    {{TID_INT,        RPC_IN},
     {TID_INT,        RPC_IN},
     {TID_STRING,     RPC_IN}, 
     {0} }},

  { RPC_DB_REORDER_KEY,
    {{TID_INT,        RPC_IN},
     {TID_INT,        RPC_IN},
     {TID_INT,        RPC_IN}, 
     {0} }},

  { RPC_DB_GET_DATA,
    {{TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN},
     {TID_ARRAY,      RPC_OUT | RPC_VARARRAY}, 
     {TID_INT,        RPC_IN | RPC_OUT},
     {TID_DWORD,      RPC_IN}, 
     {0} }},

  { RPC_DB_GET_DATA_INDEX,
    {{TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN},
     {TID_ARRAY,      RPC_OUT | RPC_VARARRAY}, 
     {TID_INT,        RPC_IN | RPC_OUT},
     {TID_INT,        RPC_IN}, 
     {TID_DWORD,      RPC_IN}, 
     {0} }},

  { RPC_DB_SET_DATA,
    {{TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN},
     {TID_ARRAY,      RPC_IN | RPC_VARARRAY}, 
     {TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN}, 
     {TID_DWORD,      RPC_IN}, 
     {0} }},

  { RPC_DB_SET_DATA_INDEX,
    {{TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN},
     {TID_ARRAY,      RPC_IN | RPC_VARARRAY}, 
     {TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN}, 
     {TID_DWORD,      RPC_IN}, 
     {0} }},

  { RPC_DB_SET_DATA_INDEX2,
    {{TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN},
     {TID_ARRAY,      RPC_IN | RPC_VARARRAY}, 
     {TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN}, 
     {TID_DWORD,      RPC_IN}, 
     {TID_BOOL,       RPC_IN}, 
     {0} }},

  { RPC_DB_SET_MODE,
    {{TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN},
     {TID_WORD,       RPC_IN}, 
     {TID_BOOL,       RPC_IN}, 
     {0} }},

  { RPC_DB_CREATE_RECORD,
    {{TID_INT,        RPC_IN},
     {TID_INT,        RPC_IN},
     {TID_STRING,     RPC_IN},
     {TID_STRING,     RPC_IN},
     {0} }},

  { RPC_DB_GET_RECORD,
    {{TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN},
     {TID_ARRAY,      RPC_OUT | RPC_VARARRAY}, 
     {TID_INT,        RPC_IN | RPC_OUT}, 
     {TID_INT,        RPC_IN}, 
     {0} }},

  { RPC_DB_GET_RECORD_SIZE,
    {{TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN},
     {TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_OUT}, 
     {0} }},

  { RPC_DB_SET_RECORD,
    {{TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN},
     {TID_ARRAY,      RPC_IN | RPC_VARARRAY}, 
     {TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN}, 
     {0} }},

  { RPC_DB_ADD_OPEN_RECORD,
    {{TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN},
     {TID_WORD,       RPC_IN}, 
     {0} }},

  { RPC_DB_REMOVE_OPEN_RECORD,
    {{TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN},
     {0} }},

  { RPC_DB_LOAD,
    {{TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN},
     {TID_STRING,     RPC_IN}, 
     {TID_BOOL  ,     RPC_IN}, 
     {0} }},

  { RPC_DB_SAVE,
    {{TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN},
     {TID_STRING,     RPC_IN}, 
     {TID_BOOL  ,     RPC_IN}, 
     {0} }},

  { RPC_DB_SET_CLIENT_NAME,
    {{TID_INT,        RPC_IN}, 
     {TID_STRING,     RPC_IN}, 
     {0} }},

  { RPC_DB_GET_OPEN_RECORDS,
    {{TID_INT,        RPC_IN},
     {TID_INT,        RPC_IN},
     {TID_STRING,     RPC_OUT}, 
     {TID_INT,        RPC_IN},
     {0} }},

  /* history functions */

  { RPC_HS_SET_PATH,
    {{TID_STRING,     RPC_IN}, 
     {0} }},

  { RPC_HS_DEFINE_EVENT,
    {{TID_DWORD,      RPC_IN}, 
     {TID_STRING,     RPC_IN},
     {TID_ARRAY,      RPC_IN | RPC_VARARRAY}, 
     {TID_INT,        RPC_IN},
     {0} }},

  { RPC_HS_WRITE_EVENT,
    {{TID_DWORD,      RPC_IN}, 
     {TID_ARRAY,      RPC_IN | RPC_VARARRAY}, 
     {TID_INT,        RPC_IN},
     {0} }},

  { RPC_HS_COUNT_EVENTS,
    {{TID_DWORD,      RPC_IN}, 
     {TID_DWORD,      RPC_OUT}, 
     {0} }},

  { RPC_HS_ENUM_EVENTS,
    {{TID_DWORD,      RPC_IN}, 
     {TID_ARRAY,      RPC_OUT | RPC_VARARRAY}, 
     {TID_INT,        RPC_IN | RPC_OUT},
     {0} }},

  { RPC_HS_COUNT_TAGS,
    {{TID_DWORD,      RPC_IN}, 
     {TID_DWORD,      RPC_IN}, 
     {TID_DWORD,      RPC_OUT}, 
     {0} }},

  { RPC_HS_ENUM_TAGS,
    {{TID_DWORD,      RPC_IN}, 
     {TID_DWORD,      RPC_IN}, 
     {TID_ARRAY,      RPC_OUT | RPC_VARARRAY}, 
     {TID_INT,        RPC_IN | RPC_OUT},
     {0} }},

  { RPC_HS_READ,
    {{TID_DWORD,      RPC_IN}, 
     {TID_DWORD,      RPC_IN}, 
     {TID_DWORD,      RPC_IN}, 
     {TID_DWORD,      RPC_IN}, 
     {TID_STRING,     RPC_IN}, 
     {TID_DWORD,      RPC_IN}, 
     {TID_ARRAY,      RPC_OUT | RPC_VARARRAY}, 
     {TID_INT,        RPC_IN | RPC_OUT},
     {TID_ARRAY,      RPC_OUT | RPC_VARARRAY}, 
     {TID_INT,        RPC_IN | RPC_OUT},
     {TID_DWORD,      RPC_OUT}, 
     {TID_DWORD,      RPC_OUT}, 
     {0} }},

  /* run control */

  { RPC_RC_TRANSITION,
    {{TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN}, 
     {TID_STRING,     RPC_OUT}, 
     {TID_INT,        RPC_IN}, 
     {0} }},


  /* analyzer control */

  { RPC_ANA_CLEAR_HISTOS,
    {{TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN}, 
     {0} }},


  /* logger control */

  { RPC_LOG_REWIND,
    {{TID_INT,        RPC_IN}, 
     {0} }},


  /* test functions */

  { RPC_TEST,
    {{TID_BYTE,       RPC_IN}, 
     {TID_WORD,       RPC_IN}, 
     {TID_INT,        RPC_IN}, 
     {TID_FLOAT,      RPC_IN}, 
     {TID_DOUBLE,     RPC_IN}, 
     {TID_BYTE,       RPC_OUT}, 
     {TID_WORD,       RPC_OUT}, 
     {TID_INT,        RPC_OUT}, 
     {TID_FLOAT,      RPC_OUT}, 
     {TID_DOUBLE,     RPC_OUT}, 
     {0} }},

  /* CAMAC server */
  
  { RPC_CNAF16,
    {{TID_DWORD,      RPC_IN},            /* command */
     {TID_DWORD,      RPC_IN},            /* branch */
     {TID_DWORD,      RPC_IN},            /* crate */
     {TID_DWORD,      RPC_IN},            /* station */
     {TID_DWORD,      RPC_IN},            /* subaddress */
     {TID_DWORD,      RPC_IN},            /* function */
     {TID_WORD,       RPC_IN | RPC_OUT | RPC_VARARRAY}, /* data */
     {TID_DWORD,      RPC_IN | RPC_OUT},  /* array size */
     {TID_DWORD,      RPC_OUT},           /* x */
     {TID_DWORD,      RPC_OUT},           /* q */
     {0} }},

  { RPC_CNAF24,
    {{TID_DWORD,      RPC_IN},            /* command */
     {TID_DWORD,      RPC_IN},            /* branch */
     {TID_DWORD,      RPC_IN},            /* crate */
     {TID_DWORD,      RPC_IN},            /* station */
     {TID_DWORD,      RPC_IN},            /* subaddress */
     {TID_DWORD,      RPC_IN},            /* function */
     {TID_DWORD,      RPC_IN | RPC_OUT | RPC_VARARRAY}, /* data */
     {TID_DWORD,      RPC_IN | RPC_OUT},  /* array size */
     {TID_DWORD,      RPC_OUT},           /* x */
     {TID_DWORD,      RPC_OUT},           /* q */
     {0} }},

  { 0 }

};

/* 
   rpc_list_system contains MIDAS system functions and gets
   registerd whenever a RPC server is registered
 */

static RPC_LIST rpc_list_system[] = {

  /* system  functions */
  { RPC_ID_WATCHDOG,
    {{0}}},

  { RPC_ID_SHUTDOWN,
    {{0}}},

  { RPC_ID_EXIT,
    {{0}}},

  { 0 }

};

RPC_LIST* rpc_get_internal_list(INT flag)
{
  if (flag)
    return rpc_list_library;
  else
    return rpc_list_system;
}

/*------------------------------------------------------------------*/
