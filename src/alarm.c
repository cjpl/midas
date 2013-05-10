/********************************************************************\

  Name:         ALARM.C
  Created by:   Stefan Ritt

  Contents:     MIDAS alarm functions

  $Id$

\********************************************************************/

#include "midas.h"
#include "msystem.h"
#include "strlcpy.h"
#include <assert.h>

/**dox***************************************************************/
/** @file alarm.c
The Midas Alarm file
*/

/** @defgroup alfunctioncode Midas Alarm Functions (al_xxx)
 */

/**dox***************************************************************/
/** @addtogroup alfunctioncode
 *
 *  @{  */

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/********************************************************************\
*                                                                    *
*                     Alarm functions                                *
*                                                                    *
\********************************************************************/

/********************************************************************/
BOOL al_evaluate_condition(const char *condition, char *value)
{
   HNDLE hDB, hkey;
   int i, j, idx1, idx2, idx, size, state;
   KEY key;
   double value1, value2;
   char value1_str[256], value2_str[256], str[256], op[3], function[80];
   char data[10000];
   DWORD dtime;

   strcpy(str, condition);
   op[1] = op[2] = 0;
   value1 = value2 = 0;
   idx1 = idx2 = 0;

   /* find value and operator */
   for (i = strlen(str) - 1; i > 0; i--)
      if (strchr("<>=!&", str[i]) != NULL)
         break;
   op[0] = str[i];
   for (j = 1; str[i + j] == ' '; j++);
   strlcpy(value2_str, str + i + j, sizeof(value2_str));
   value2 = atof(value2_str);
   str[i] = 0;

   if (i > 0 && strchr("<>=!&", str[i - 1])) {
      op[1] = op[0];
      op[0] = str[--i];
      str[i] = 0;
   }

   i--;
   while (i > 0 && str[i] == ' ')
      i--;
   str[i + 1] = 0;

   /* check if function */
   function[0] = 0;
   if (str[i] == ')') {
      str[i--] = 0;
      if (strchr(str, '(')) {
         *strchr(str, '(') = 0;
         strcpy(function, str);
         for (i = strlen(str) + 1, j = 0; str[i]; i++, j++)
            str[j] = str[i];
         str[j] = 0;
         i = j - 1;
      }
   }

   /* find key */
   if (str[i] == ']') {
      str[i--] = 0;
      if (str[i] == '*') {
         idx1 = -1;
         while (i > 0 && str[i] != '[')
            i--;
         str[i] = 0;
      } else if (strchr(str, '[') &&
                 strchr(strchr(str, '['), '-')) {
         while (i > 0 && isdigit(str[i]))
            i--;
         idx2 = atoi(str + i + 1);
         while (i > 0 && str[i] != '[')
            i--;
         idx1 = atoi(str + i + 1);
         str[i] = 0;
      } else {
         while (i > 0 && isdigit(str[i]))
            i--;
         idx1 = idx2 = atoi(str + i + 1);
         str[i] = 0;
      }
   }

   cm_get_experiment_database(&hDB, NULL);
   db_find_key(hDB, 0, str, &hkey);
   if (!hkey) {
      cm_msg(MERROR, "al_evaluate_condition", "Cannot find key %s to evaluate alarm condition", str);
      if (value)
         strcpy(value, "unknown");
      return FALSE;
   }
   db_get_key(hDB, hkey, &key);

   if (idx1 < 0) {
      idx1 = 0;
      idx2 = key.num_values-1;
   }

   for (idx=idx1; idx<=idx2 ; idx++) {
      
      if (equal_ustring(function, "access")) {
         /* check key access time */
         db_get_key_time(hDB, hkey, &dtime);
         sprintf(value1_str, "%d", dtime);
         value1 = atof(value1_str);
      } else if (equal_ustring(function, "access_running")) {
         /* check key access time if running */
         db_get_key_time(hDB, hkey, &dtime);
         sprintf(value1_str, "%d", dtime);
         size = sizeof(state);
         db_get_value(hDB, 0, "/Runinfo/State", &state, &size, TID_INT, FALSE);
         if (state != STATE_RUNNING)
            strcpy(value1_str, "0");
         value1 = atof(value1_str);
      } else {
         /* get key data and convert to double */
         db_get_key(hDB, hkey, &key);
         size = sizeof(data);
         db_get_data_index(hDB, hkey, data, &size, idx, key.type);
         db_sprintf(value1_str, data, size, 0, key.type);
         value1 = atof(value1_str);
      }

      /* convert boolean values to integers */
      if (key.type == TID_BOOL) {
         value1 = (value1_str[0] == 'Y' || value1_str[0] == 'y' || value1_str[0] == '1');
         value2 = (value2_str[0] == 'Y' || value2_str[0] == 'y' || value2_str[0] == '1');
      }

      /* return value */
      if (value)
         strcpy(value, value1_str);

      /* now do logical operation */
      if (strcmp(op, "=") == 0)
         if (value1 == value2) return TRUE;
      if (strcmp(op, "==") == 0)
         if (value1 == value2) return TRUE;
      if (strcmp(op, "!=") == 0)
         if (value1 != value2) return TRUE;
      if (strcmp(op, "<") == 0)
         if (value1 < value2) return TRUE;
      if (strcmp(op, ">") == 0)
         if (value1 > value2) return TRUE;
      if (strcmp(op, "<=") == 0)
         if (value1 <= value2) return TRUE;
      if (strcmp(op, ">=") == 0)
         if (value1 >= value2) return TRUE;
      if (strcmp(op, "&") == 0)
         if (((unsigned int)value1 & (unsigned int)value2) > 0) return TRUE;
   }

   return FALSE;
}

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/********************************************************************/
/**
Trigger a certain alarm.
\code  ...
  lazy.alarm[0] = 0;
  size = sizeof(lazy.alarm);
  db_get_value(hDB, pLch->hKey, "Settings/Alarm Class", lazy.alarm, &size, TID_STRING, TRUE);

  // trigger alarm if defined
  if (lazy.alarm[0])
    al_trigger_alarm("Tape", "Tape full...load new one!", lazy.alarm, "Tape full", AT_INTERNAL);
  ...
\endcode
@param alarm_name Alarm name, defined in /alarms/alarms
@param alarm_message Optional message which goes with alarm
@param default_class If alarm is not yet defined under
                    /alarms/alarms/\<alarm_name\>, a new one
                    is created and this default class is used.
@param cond_str String displayed in alarm condition
@param type Alarm type, one of AT_xxx
@return AL_SUCCESS, AL_INVALID_NAME
*/
INT al_trigger_alarm(const char *alarm_name, const char *alarm_message, const char *default_class, const char *cond_str, INT type)
{
   if (rpc_is_remote())
      return rpc_call(RPC_AL_TRIGGER_ALARM, alarm_name, alarm_message, default_class, cond_str, type);

#ifdef LOCAL_ROUTINES
   {
      int status, size;
      HNDLE hDB, hkeyalarm, hkey;
      char str[256];
      ALARM a;
      BOOL flag;
      ALARM_ODB_STR(alarm_odb_str);

      cm_get_experiment_database(&hDB, NULL);

      /* check online mode */
      flag = TRUE;
      size = sizeof(flag);
      db_get_value(hDB, 0, "/Runinfo/Online Mode", &flag, &size, TID_INT, TRUE);
      if (!flag)
         return AL_SUCCESS;

      /* find alarm */
      sprintf(str, "/Alarms/Alarms/%s", alarm_name);
      db_find_key(hDB, 0, str, &hkeyalarm);
      if (!hkeyalarm) {
         /* alarm must be an internal analyzer alarm, so create a default alarm */
         status = db_create_record(hDB, 0, str, strcomb(alarm_odb_str));
         db_find_key(hDB, 0, str, &hkeyalarm);
         if (!hkeyalarm) {
            cm_msg(MERROR, "al_trigger_alarm", "Cannot create alarm record");
            return AL_ERROR_ODB;
         }

         if (default_class && default_class[0])
            db_set_value(hDB, hkeyalarm, "Alarm Class", default_class, 32, 1, TID_STRING);
         status = TRUE;
         db_set_value(hDB, hkeyalarm, "Active", &status, sizeof(status), 1, TID_BOOL);
      }

      /* set parameters for internal alarms */
      if (type != AT_EVALUATED && type != AT_PERIODIC) {
         db_set_value(hDB, hkeyalarm, "Type", &type, sizeof(INT), 1, TID_INT);
         strcpy(str, cond_str);
         db_set_value(hDB, hkeyalarm, "Condition", str, 256, 1, TID_STRING);
      }

      size = sizeof(a);
      status = db_get_record(hDB, hkeyalarm, &a, &size, 0);
      if (status != DB_SUCCESS || a.type < 1 || a.type > AT_LAST) {
         /* make sure alarm record has right structure */
         db_check_record(hDB, hkeyalarm, "", strcomb(alarm_odb_str), TRUE);

         size = sizeof(a);
         status = db_get_record(hDB, hkeyalarm, &a, &size, 0);
         if (status != DB_SUCCESS) {
            cm_msg(MERROR, "al_trigger_alarm", "Cannot get alarm record");
            return AL_ERROR_ODB;
         }
      }

      /* if internal alarm, check if active and check interval */
      if (a.type != AT_EVALUATED && a.type != AT_PERIODIC) {
         /* check global alarm flag */
         flag = TRUE;
         size = sizeof(flag);
         db_get_value(hDB, 0, "/Alarms/Alarm system active", &flag, &size, TID_BOOL, TRUE);
         if (!flag)
            return AL_SUCCESS;

         if (!a.active)
            return AL_SUCCESS;

         if ((INT) ss_time() - (INT) a.checked_last < a.check_interval)
            return AL_SUCCESS;

         /* now the alarm will be triggered, so save time */
         a.checked_last = ss_time();
      }

      /* write back alarm message for internal alarms */
      if (a.type != AT_EVALUATED && a.type != AT_PERIODIC) {
         strncpy(a.alarm_message, alarm_message, 79);
         a.alarm_message[79] = 0;
      }

      /* now trigger alarm class defined in this alarm */
      if (a.alarm_class[0])
         al_trigger_class(a.alarm_class, alarm_message, a.triggered > 0);

      /* check for and trigger "All" class */
      if (db_find_key(hDB, 0, "/Alarms/Classes/All", &hkey) == DB_SUCCESS)
         al_trigger_class("All", alarm_message, a.triggered > 0);

      /* signal alarm being triggered */
      cm_asctime(str, sizeof(str));

      if (!a.triggered)
         strcpy(a.time_triggered_first, str);

      a.triggered++;
      strcpy(a.time_triggered_last, str);

      a.checked_last = ss_time();

      status = db_set_record(hDB, hkeyalarm, &a, sizeof(a), 0);
      if (status != DB_SUCCESS) {
         cm_msg(MERROR, "al_trigger_alarm", "Cannot update alarm record");
         return AL_ERROR_ODB;
      }

   }
#endif                          /* LOCAL_ROUTINES */

   return AL_SUCCESS;
}

/**dox***************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS

/********************************************************************/
INT al_trigger_class(const char *alarm_class, const char *alarm_message, BOOL first)
/********************************************************************\

  Routine: al_trigger_class

  Purpose: Trigger a certain alarm class

  Input:
    char   *alarm_class     Alarm class, must be defined in
                            /alarms/classes
    char   *alarm_message   Optional message which goes with alarm
    BOOL   first            TRUE if alarm is triggered first time
                            (used for elog)

  Output:

  Function value:
    AL_INVALID_NAME         Alarm class not defined
    AL_SUCCESS              Successful completion

\********************************************************************/
{
   int status, size, state;
   HNDLE hDB, hkeyclass;
   char str[256], command[256], tag[32], url[256];
   ALARM_CLASS ac;
   DWORD now = ss_time();

   tag[0] = 0;

   cm_get_experiment_database(&hDB, NULL);

   /* get alarm class */
   sprintf(str, "/Alarms/Classes/%s", alarm_class);
   db_find_key(hDB, 0, str, &hkeyclass);
   if (!hkeyclass) {
      cm_msg(MERROR, "al_trigger_class", "Alarm class %s not found in ODB", alarm_class);
      return AL_INVALID_NAME;
   }

   size = sizeof(ac);
   status = db_get_record(hDB, hkeyclass, &ac, &size, 0);
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "al_trigger_class", "Cannot get alarm class record");
      return AL_ERROR_ODB;
   }

   /* write system message */
   if (ac.write_system_message && (now - ac.system_message_last >= (DWORD)ac.system_message_interval)) {
      if (equal_ustring(alarm_class, "All"))
         sprintf(str, "General alarm: %s", alarm_message);
      else
         sprintf(str, "%s: %s", alarm_class, alarm_message);
      cm_msg(MTALK, "al_trigger_class", str);
      ac.system_message_last = now;
   }

   /* write elog message on first trigger if using internal ELOG */
   size = sizeof(url);
   if (ac.write_elog_message && first &&
       db_get_value(hDB, 0, "/Elog/URL", url, &size, TID_STRING, FALSE) != DB_SUCCESS)
      el_submit(0, "Alarm system", "Alarm", "General", alarm_class, str,
                "", "plain", "", "", 0, "", "", 0, "", "", 0, tag, sizeof(tag));

   /* execute command */
   if (ac.execute_command[0] &&
       ac.execute_interval > 0 && (INT) ss_time() - (INT) ac.execute_last > ac.execute_interval) {
      if (equal_ustring(alarm_class, "All"))
         sprintf(str, "General alarm: %s", alarm_message);
      else
         sprintf(str, "%s: %s", alarm_class, alarm_message);
      sprintf(command, ac.execute_command, str);
      cm_msg(MINFO, "al_trigger_class", "Execute: %s", command);
      ss_system(command);
      ac.execute_last = ss_time();
   }

   /* stop run */
   if (ac.stop_run) {
      state = STATE_STOPPED;
      size = sizeof(state);
      db_get_value(hDB, 0, "/Runinfo/State", &state, &size, TID_INT, TRUE);
      if (state != STATE_STOPPED) {
         cm_msg(MINFO, "al_trigger_class", "Stopping the run from alarm class \'%s\', message \'%s\'", alarm_class, alarm_message);
         cm_transition(TR_STOP, 0, NULL, 0, DETACH, FALSE);
      }
   }

   status = db_set_record(hDB, hkeyclass, &ac, sizeof(ac), 0);
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "al_trigger_class", "Cannot update alarm class record");
      return AL_ERROR_ODB;
   }

   return AL_SUCCESS;
}

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

/********************************************************************/
/**
Reset (acknoledge) alarm.

@param alarm_name Alarm name, defined in /alarms/alarms
@return AL_SUCCESS, AL_RESETE, AL_INVALID_NAME
*/
INT al_reset_alarm(const char *alarm_name)
{
   int status, size, i;
   HNDLE hDB, hkeyalarm, hkeyclass, hsubkey;
   KEY key;
   char str[256];
   ALARM a;
   ALARM_CLASS ac;

   cm_get_experiment_database(&hDB, NULL);

   if (alarm_name == NULL) {
      /* reset all alarms */
      db_find_key(hDB, 0, "/Alarms/Alarms", &hkeyalarm);
      if (hkeyalarm) {
         for (i = 0;; i++) {
            db_enum_link(hDB, hkeyalarm, i, &hsubkey);

            if (!hsubkey)
               break;

            db_get_key(hDB, hsubkey, &key);
            al_reset_alarm(key.name);
         }
      }
      return AL_SUCCESS;
   }

   /* find alarm and alarm class */
   sprintf(str, "/Alarms/Alarms/%s", alarm_name);
   db_find_key(hDB, 0, str, &hkeyalarm);
   if (!hkeyalarm) {
      /*cm_msg(MERROR, "al_reset_alarm", "Alarm %s not found in ODB", alarm_name);*/
      return AL_INVALID_NAME;
   }

   size = sizeof(a);
   status = db_get_record(hDB, hkeyalarm, &a, &size, 0);
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "al_reset_alarm", "Cannot get alarm record");
      return AL_ERROR_ODB;
   }

   sprintf(str, "/Alarms/Classes/%s", a.alarm_class);
   db_find_key(hDB, 0, str, &hkeyclass);
   if (!hkeyclass) {
      cm_msg(MERROR, "al_reset_alarm", "Alarm class %s not found in ODB", a.alarm_class);
      return AL_INVALID_NAME;
   }

   size = sizeof(ac);
   status = db_get_record(hDB, hkeyclass, &ac, &size, 0);
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "al_reset_alarm", "Cannot get alarm class record");
      return AL_ERROR_ODB;
   }

   if (a.triggered) {
      a.triggered = 0;
      a.time_triggered_first[0] = 0;
      a.time_triggered_last[0] = 0;
      a.checked_last = 0;

      ac.system_message_last = 0;
      ac.execute_last = 0;

      status = db_set_record(hDB, hkeyalarm, &a, sizeof(a), 0);
      if (status != DB_SUCCESS) {
         cm_msg(MERROR, "al_reset_alarm", "Cannot update alarm record");
         return AL_ERROR_ODB;
      }
      status = db_set_record(hDB, hkeyclass, &ac, sizeof(ac), 0);
      if (status != DB_SUCCESS) {
         cm_msg(MERROR, "al_reset_alarm", "Cannot update alarm class record");
         return AL_ERROR_ODB;
      }
      return AL_RESET;
   }

   return AL_SUCCESS;
}


/********************************************************************/
/**
Scan ODB for alarms.
@return AL_SUCCESS
*/
INT al_check()
{
   if (rpc_is_remote())
      return rpc_call(RPC_AL_CHECK);

#ifdef LOCAL_ROUTINES
   {
      INT i, status, size, semaphore;
      HNDLE hDB, hkeyroot, hkey;
      KEY key;
      ALARM a;
      char str[256], value[256];
      time_t now;
      PROGRAM_INFO program_info;
      BOOL flag;

      ALARM_CLASS_STR(alarm_class_str);
      ALARM_ODB_STR(alarm_odb_str);
      ALARM_PERIODIC_STR(alarm_periodic_str);

      cm_get_experiment_database(&hDB, NULL);

      if (hDB == 0)
         return AL_SUCCESS;     /* called from server not yet connected */

      /* check online mode */
      flag = TRUE;
      size = sizeof(flag);
      db_get_value(hDB, 0, "/Runinfo/Online Mode", &flag, &size, TID_INT, TRUE);
      if (!flag)
         return AL_SUCCESS;

      /* check global alarm flag */
      flag = TRUE;
      size = sizeof(flag);
      db_get_value(hDB, 0, "/Alarms/Alarm system active", &flag, &size, TID_BOOL, TRUE);
      if (!flag)
         return AL_SUCCESS;

      /* request semaphore */
      cm_get_experiment_semaphore(&semaphore, NULL, NULL, NULL);
      status = ss_semaphore_wait_for(semaphore, 100);
      if (status != SS_SUCCESS)
         return SUCCESS;        /* someone else is doing alarm business */

      /* check ODB alarms */
      db_find_key(hDB, 0, "/Alarms/Alarms", &hkeyroot);
      if (!hkeyroot) {
         /* create default ODB alarm */
         status = db_create_record(hDB, 0, "/Alarms/Alarms/Demo ODB", strcomb(alarm_odb_str));
         db_find_key(hDB, 0, "/Alarms/Alarms", &hkeyroot);
         if (!hkeyroot) {
            ss_semaphore_release(semaphore);
            return SUCCESS;
         }

         status = db_create_record(hDB, 0, "/Alarms/Alarms/Demo periodic", strcomb(alarm_periodic_str));
         db_find_key(hDB, 0, "/Alarms/Alarms", &hkeyroot);
         if (!hkeyroot) {
            ss_semaphore_release(semaphore);
            return SUCCESS;
         }

         /* create default alarm classes */
         status = db_create_record(hDB, 0, "/Alarms/Classes/Alarm", strcomb(alarm_class_str));
         status = db_create_record(hDB, 0, "/Alarms/Classes/Warning", strcomb(alarm_class_str));
         if (status != DB_SUCCESS) {
            ss_semaphore_release(semaphore);
            return SUCCESS;
         }
      }

      for (i = 0;; i++) {
         status = db_enum_key(hDB, hkeyroot, i, &hkey);
         if (status == DB_NO_MORE_SUBKEYS)
            break;

         db_get_key(hDB, hkey, &key);

         size = sizeof(a);
         status = db_get_record(hDB, hkey, &a, &size, 0);
         if (status != DB_SUCCESS || a.type < 1 || a.type > AT_LAST) {
            /* make sure alarm record has right structure */
            db_check_record(hDB, hkey, "", strcomb(alarm_odb_str), TRUE);
            size = sizeof(a);
            status = db_get_record(hDB, hkey, &a, &size, 0);
            if (status != DB_SUCCESS || a.type < 1 || a.type > AT_LAST) {
               cm_msg(MERROR, "al_check", "Cannot get alarm record");
               continue;
            }
         }

         /* check periodic alarm only when active */
         if (a.active &&
             a.type == AT_PERIODIC &&
             a.check_interval > 0 && (INT) ss_time() - (INT) a.checked_last > a.check_interval) {
            /* if checked_last has not been set, set it to current time */
            if (a.checked_last == 0) {
               a.checked_last = ss_time();
               db_set_record(hDB, hkey, &a, size, 0);
            } else
               al_trigger_alarm(key.name, a.alarm_message, a.alarm_class, "", AT_PERIODIC);
         }

         /* check alarm only when active and not internal */
         if (a.active &&
             a.type == AT_EVALUATED &&
             a.check_interval > 0 && (INT) ss_time() - (INT) a.checked_last > a.check_interval) {
            /* if condition is true, trigger alarm */
            if (al_evaluate_condition(a.condition, value)) {
               sprintf(str, a.alarm_message, value);
               al_trigger_alarm(key.name, str, a.alarm_class, "", AT_EVALUATED);
            } else {
               a.checked_last = ss_time();
               status = db_set_record(hDB, hkey, &a, sizeof(a), 0);
               if (status != DB_SUCCESS) {
                  cm_msg(MERROR, "al_check", "Cannot write back alarm record");
                  continue;
               }
            }
         }
      }

      /* check /programs alarms */
      db_find_key(hDB, 0, "/Programs", &hkeyroot);
      if (hkeyroot) {
         for (i = 0;; i++) {
            status = db_enum_key(hDB, hkeyroot, i, &hkey);
            if (status == DB_NO_MORE_SUBKEYS)
               break;

            db_get_key(hDB, hkey, &key);

            /* don't check "execute on xxx" */
            if (key.type != TID_KEY)
               continue;

            size = sizeof(program_info);
            status = db_get_record(hDB, hkey, &program_info, &size, 0);
            if (status != DB_SUCCESS) {
               cm_msg(MERROR, "al_check", "Cannot get program info record");
               continue;
            }

            now = ss_time();

            rpc_get_name(str);
            str[strlen(key.name)] = 0;
            if (!equal_ustring(str, key.name) && cm_exist(key.name, FALSE) == CM_NO_CLIENT) {
               if (program_info.first_failed == 0)
                  program_info.first_failed = (DWORD) now;

               /* fire alarm when not running for more than what specified in check interval */
               if (now - program_info.first_failed >= program_info.check_interval / 1000) {
                  /* if not running and alarm calss defined, trigger alarm */
                  if (program_info.alarm_class[0]) {
                     sprintf(str, "Program %s is not running", key.name);
                     al_trigger_alarm(key.name, str, program_info.alarm_class,
                                      "Program not running", AT_PROGRAM);
                  }

                  /* auto restart program */
                  if (program_info.auto_restart && program_info.start_command[0]) {
                     ss_system(program_info.start_command);
                     program_info.first_failed = 0;
                     cm_msg(MTALK, "al_check", "Program %s restarted", key.name);
                  }
               }
            } else
               program_info.first_failed = 0;

            db_set_record(hDB, hkey, &program_info, sizeof(program_info), 0);
         }
      }

      ss_semaphore_release(semaphore);
   }
#endif                          /* LOCAL_COUTINES */

   return SUCCESS;
}

/********************************************************************/
/**
 Scan ODB for alarms.
 @return AL_SUCCESS
 */
INT al_get_alarms(char *result, int result_size)
{
   HNDLE hDB, hkey, hsubkey;
   int i, j, n, flag, size;
   char alarm_class[32], msg[256], value_str[256], str[256];
   
   cm_get_experiment_database(&hDB, NULL);
   result[0] = 0;
   n = 0;
   db_find_key(hDB, 0, "/Alarms/Alarms", &hkey);
   if (hkey) {
      /* check global alarm flag */
      flag = TRUE;
      size = sizeof(flag);
      db_get_value(hDB, 0, "/Alarms/Alarm System active", &flag, &size, TID_BOOL, TRUE);
      if (flag) {
         for (i = 0;; i++) {
            db_enum_link(hDB, hkey, i, &hsubkey);
            
            if (!hsubkey)
               break;
            
            size = sizeof(flag);
            db_get_value(hDB, hsubkey, "Triggered", &flag, &size, TID_INT, TRUE);
            if (flag) {
               n++;
               
               size = sizeof(alarm_class);
               db_get_value(hDB, hsubkey, "Alarm Class", alarm_class, &size, TID_STRING,
                            TRUE);
               
                              
               size = sizeof(msg);
               db_get_value(hDB, hsubkey, "Alarm Message", msg, &size, TID_STRING, TRUE);
               
               size = sizeof(j);
               db_get_value(hDB, hsubkey, "Type", &j, &size, TID_INT, TRUE);
               
               if (j == AT_EVALUATED) {
                  size = sizeof(str);
                  db_get_value(hDB, hsubkey, "Condition", str, &size, TID_STRING, TRUE);
                  
                  /* retrieve value */
                  al_evaluate_condition(str, value_str);
                  sprintf(str, msg, value_str);
               } else
                  strlcpy(str, msg, sizeof(str));

               strlcat(result, alarm_class, result_size);
               strlcat(result, ": ", result_size);
               strlcat(result, str, result_size);
               strlcat(result, "\n", result_size);
            }
         }
      }
   }
   
   return n;
}

/**dox***************************************************************/
/** @} *//* end of alfunctioncode */

