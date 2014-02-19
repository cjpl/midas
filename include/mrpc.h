
/********************************************************************\

  Name:         MRPC.H
  Created by:   Stefan Ritt

  Purpose:      MIDAS RPC function header file
  Contents:     Symbolic constants for internal RPC functions

  $Id$

\********************************************************************/

/**dox***************************************************************/
/** @file mrpc.h
The mrpc include file
*/

/** @defgroup mrpcincludecode The mrpc.h & mrpc.c
 */
/** @defgroup mrpcdefineh RPC Define 
 */

/**dox***************************************************************/
/** @addtogroup mrpcincludecode
 *  
 *  @{  */

/**dox***************************************************************/
/** @addtogroup mrpcdefineh
 *  
 *  @{  */

/********************************************************************/
/**
routine IDs for RPC calls
*/
#define RPC_CM_SET_CLIENT_INFO          11000 /**< - */
#define RPC_CM_SET_WATCHDOG_PARAMS      11001 /**< - */
#define RPC_CM_CLEANUP                  11002 /**< - */
#define RPC_CM_GET_WATCHDOG_INFO        11003 /**< - */
#define RPC_CM_MSG_LOG                  11004 /**< - */
#define RPC_CM_EXECUTE                  11005 /**< - */
#define RPC_CM_SYNCHRONIZE              11006 /**< - */
#define RPC_CM_ASCTIME                  11007 /**< - */
#define RPC_CM_TIME                     11008 /**< - */
#define RPC_CM_MSG                      11009 /**< - */
#define RPC_CM_EXIST                    11011 /**< - */
#define RPC_CM_MSG_RETRIEVE             11012 /**< - */
#define RPC_CM_MSG_LOG1                 11013 /**< - */
#define RPC_CM_CHECK_CLIENT             11014 /**< - */

#define RPC_BM_OPEN_BUFFER              11100 /**< - */
#define RPC_BM_CLOSE_BUFFER             11101 /**< - */
#define RPC_BM_CLOSE_ALL_BUFFERS        11102 /**< - */
#define RPC_BM_GET_BUFFER_INFO          11103 /**< - */
#define RPC_BM_GET_BUFFER_LEVEL         11104 /**< - */
#define RPC_BM_INIT_BUFFER_COUNTERS     11105 /**< - */
#define RPC_BM_SET_CACHE_SIZE           11106 /**< - */
#define RPC_BM_ADD_EVENT_REQUEST        11107 /**< - */
#define RPC_BM_REMOVE_EVENT_REQUEST     11108 /**< - */
#define RPC_BM_SEND_EVENT               11109 /**< - */
#define RPC_BM_FLUSH_CACHE              11110 /**< - */
#define RPC_BM_RECEIVE_EVENT            11111 /**< - */
#define RPC_BM_MARK_READ_WAITING        11112 /**< - */
#define RPC_BM_EMPTY_BUFFERS            11113 /**< - */
#define RPC_BM_SKIP_EVENT               11114 /**< - */

#define RPC_DB_OPEN_DATABASE            11200 /**< - */
#define RPC_DB_CLOSE_DATABASE           11201 /**< - */
#define RPC_DB_CLOSE_ALL_DATABASES      11202 /**< - */
#define RPC_DB_CREATE_KEY               11203 /**< - */
#define RPC_DB_CREATE_LINK              11204 /**< - */
#define RPC_DB_SET_VALUE                11205 /**< - */
#define RPC_DB_GET_VALUE                11206 /**< - */
#define RPC_DB_FIND_KEY                 11207 /**< - */
#define RPC_DB_FIND_LINK                11208 /**< - */
#define RPC_DB_GET_PATH                 11209 /**< - */
#define RPC_DB_DELETE_KEY               11210 /**< - */
#define RPC_DB_ENUM_KEY                 11211 /**< - */
#define RPC_DB_GET_KEY                  11212 /**< - */
#define RPC_DB_GET_DATA                 11213 /**< - */
#define RPC_DB_SET_DATA                 11214 /**< - */
#define RPC_DB_SET_DATA_INDEX           11215 /**< - */
#define RPC_DB_SET_MODE                 11216 /**< - */
#define RPC_DB_GET_RECORD_SIZE          11219 /**< - */
#define RPC_DB_GET_RECORD               11220 /**< - */
#define RPC_DB_SET_RECORD               11221 /**< - */
#define RPC_DB_ADD_OPEN_RECORD          11222 /**< - */
#define RPC_DB_REMOVE_OPEN_RECORD       11223 /**< - */
#define RPC_DB_SAVE                     11224 /**< - */
#define RPC_DB_LOAD                     11225 /**< - */
#define RPC_DB_SET_CLIENT_NAME          11226 /**< - */
#define RPC_DB_RENAME_KEY               11227 /**< - */
#define RPC_DB_ENUM_LINK                11228 /**< - */
#define RPC_DB_REORDER_KEY              11229 /**< - */
#define RPC_DB_CREATE_RECORD            11230 /**< - */
#define RPC_DB_GET_DATA_INDEX           11231 /**< - */
#define RPC_DB_GET_KEY_TIME             11232 /**< - */
#define RPC_DB_GET_OPEN_RECORDS         11233 /**< - */
#define RPC_DB_FLUSH_DATABASE           11235 /**< - */
#define RPC_DB_SET_DATA_INDEX2          11236 /**< - */
#define RPC_DB_GET_KEY_INFO             11237 /**< - */
#define RPC_DB_GET_DATA1                11238 /**< - */
#define RPC_DB_SET_NUM_VALUES           11239 /**< - */
#define RPC_DB_CHECK_RECORD             11240 /**< - */
#define RPC_DB_GET_NEXT_LINK            11241 /**< - */
#define RPC_DB_GET_LINK                 11242 /**< - */
#define RPC_DB_GET_LINK_DATA            11243 /**< - */
#define RPC_DB_SET_LINK_DATA            11244 /**< - */
#define RPC_DB_SET_LINK_DATA_INDEX      11245 /**< - */

#define RPC_HS_SET_PATH                 11300 /**< - */
#define RPC_HS_DEFINE_EVENT             11301 /**< - */
#define RPC_HS_WRITE_EVENT              11302 /**< - */
#define RPC_HS_COUNT_EVENTS             11303 /**< - */
#define RPC_HS_ENUM_EVENTS              11304 /**< - */
#define RPC_HS_COUNT_VARS               11305 /**< - */
#define RPC_HS_ENUM_VARS                11306 /**< - */
#define RPC_HS_READ                     11307 /**< - */
#define RPC_HS_GET_VAR                  11308 /**< - */
#define RPC_HS_GET_EVENT_ID             11309 /**< - */

#define RPC_EL_SUBMIT                   11400 /**< - */

#define RPC_AL_CHECK                    11500 /**< - */
#define RPC_AL_TRIGGER_ALARM            11501 /**< - */

#define RPC_RC_TRANSITION               12000 /**< - */

#define RPC_ANA_CLEAR_HISTOS            13000 /**< - */

#define RPC_LOG_REWIND                  14000 /**< - */

#define RPC_TEST                        15000 /**< - */

#define RPC_CNAF16                      16000 /**< - */
#define RPC_CNAF24                      16001 /**< - */

#define RPC_MANUAL_TRIG                 17000 /**< - */

#define RPC_JRPC                        18000 /**< - */

#define RPC_ID_WATCHDOG                 99997 /**< - */
#define RPC_ID_SHUTDOWN                 99998 /**< - */
#define RPC_ID_EXIT                     99999 /**< - */

/*------------------------------------------------------------------*/

/**dox***************************************************************/
/** @} *//* end of rpcdefineh */

/**dox***************************************************************/
/** @} *//* end of rpcincludecode */
