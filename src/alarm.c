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
/** @addtogroup alfunctionc
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
BOOL al_evaluate_condition(char *condition, char *value)
{
   HNDLE hDB, hkey;
   int i, j, index, size;
   KEY key;
   double value1, value2;
   char value1_str[256], value2_str[256], str[256], op[3], function[80];
   char data[10000];
   DWORD dtime;

   strcpy(str, condition);
   op[1] = op[2] = 0;
   value1 = value2 = 0;
   index = 0;

   /* find value and operator */
   for (i = strlen(str) - 1; i > 0; i--)
      if (strchr("<>=!", str[i]) != NULL)
         break;
   op[0] = str[i];
   for (j = 1; str[i + j] == ' '; j++);
   strlcpy(value2_str, str + i + j, sizeof(value2_str));
   value2 = atof(value2_str);
   str[i] = 0;

   if (i > 0 && strchr("<>=!", str[i - 1])) {
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
      while (i > 0 && isdigit(str[i]))
         i--;
      index = atoi(str + i + 1);
      str[i] = 0;
   }

   cm_get_experiment_database(&hDB, NULL);
   db_find_key(hDB, 0, str, &hkey);
   if (!hkey) {
      cm_msg(MERROR, "al_evaluate_condition", "Cannot find key %s to evaluate alarm condition", str);
      if (value)
         strcpy(value, "unknown");
      return FALSE;
   }

   if (equal_ustring(function, "access")) {
      /* check key access time */
      db_get_key_time(hDB, hkey, &dtime);
      sprintf(value1_str, "%d", dtime);
      value1 = atof(value1_str);
   } else {
      /* get key data and convert to double */
      db_get_key(hDB, hkey, &key);
      size = sizeof(data);
      db_get_data(hDB, hkey, data, &size, key.type);
      db_sprintf(value1_str, data, size, index, key.type);
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
      return value1 == value2;
   if (strcmp(op, "==") == 0)
      return value1 == value2;
   if (strcmp(op, "!=") == 0)
      return value1 != value2;
   if (strcmp(op, "<") == 0)
      return value1 < value2;
   if (strcmp(op, ">") == 0)
      return value1 > value2;
   if (strcmp(op, "<=") == 0)
      return value1 <= value2;
   if (strcmp(op, ">=") == 0)
      return value1 >= value2;

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
INT al_trigger_alarm(char *alarm_name, char *alarm_message, char *default_class, char *cond_str, INT type)
{
   if (rpc_is_remote())
      return rpc_call(RPC_AL_TRIGGER_ALARM, alarm_name, alarm_message, default_class, cond_str, type);

#ifdef LOCAL_ROUTINES
   {
      int status, size;
      HNDLE hDB, hkeyalarm;
      char str[256];
      ALARM alarm;
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

      size = sizeof(alarm);
      status = db_get_record(hDB, hkeyalarm, &alarm, &size, 0);
      if (status != DB_SUCCESS || alarm.type < 1 || alarm.type > AT_LAST) {
         /* make sure alarm record has right structure */
         db_check_record(hDB, hkeyalarm, "", strcomb(alarm_odb_str), TRUE);

         size = sizeof(alarm);
         status = db_get_record(hDB, hkeyalarm, &alarm, &size, 0);
         if (status != DB_SUCCESS) {
            cm_msg(MERROR, "al_trigger_alarm", "Cannot get alarm record");
            return AL_ERROR_ODB;
         }
      }

      /* if internal alarm, check if active and check interval */
      if (alarm.type != AT_EVALUATED && alarm.type != AT_PERIODIC) {
         /* check global alarm flag */
         flag = TRUE;
         size = sizeof(flag);
         db_get_value(hDB, 0, "/Alarms/Alarm system active", &flag, &size, TID_BOOL, TRUE);
         if (!flag)
            return AL_SUCCESS;

         if (!alarm.active)
            return AL_SUCCESS;

         if ((INT) ss_time() - (INT) alarm.checked_last < alarm.check_interval)
            return AL_SUCCESS;

         /* now the alarm will be triggered, so save time */
         alarm.checked_last = ss_time();
      }

      /* write back alarm message for internal alarms */
      if (alarm.type != AT_EVALUATED && alarm.type != AT_PERIODIC) {
         strncpy(alarm.alarm_message, alarm_message, 79);
         alarm.alarm_message[79] = 0;
      }

      /* now trigger alarm class defined in this alarm */
      if (alarm.alarm_class[0])
         al_trigger_class(alarm.alarm_class, alarm_message, alarm.triggered > 0);

      /* signal alarm being triggered */
      cm_asctime(str, sizeof(str));

      if (!alarm.triggered)
         strcpy(alarm.time_triggered_first, str);

      alarm.triggered++;
      strcpy(alarm.time_triggered_last, str);

      alarm.checked_last = ss_time();

      status = db_set_record(hDB, hkeyalarm, &alarm, sizeof(alarm), 0);
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
INT al_trigger_class(char *alarm_class, char *alarm_message, BOOL first)
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
   if (ac.write_system_message && (INT) ss_time() - (INT) ac.system_message_last > ac.system_message_interval) {
      sprintf(str, "%s: %s", alarm_class, alarm_message);
      cm_msg(MTALK, "al_trigger_class", str);
      ac.system_message_last = ss_time();
   }

   /* write elog message on first trigger if using internal ELOG */
   size = sizeof(url);
   if (ac.write_elog_message && first &&
       db_get_value(hDB, 0, "/Elog/URL", url, &size, TID_STRING, FALSE) != DB_SUCCESS)
      el_submit(0, "Alarm system", "Alarm", "General", alarm_class, str,
                "", "plain", "", "", 0, "", "", 0, "", "", 0, tag, 32);

   /* execute command */
   if (ac.execute_command[0] &&
       ac.execute_interval > 0 && (INT) ss_time() - (INT) ac.execute_last > ac.execute_interval) {
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
      if (state != STATE_STOPPED)
         cm_transition(TR_STOP, 0, NULL, 0, ASYNC, FALSE);
   }

   status = db_set_record(hDB, hkeyclass, &ac, sizeof(ac), 0);
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "al_trigger_class", "Cannot update alarm class record");
      return AL_ERROR_ODB;
   }

   return AL_SUCCESS;
}


/********************************************************************/
INT al_reset_alarm(char *alarm_name)
/********************************************************************\

  Routine: al_reset_alarm

  Purpose: Reset (acknowledge) alarm

  Input:
    char   *alarm_name      Alarm name, must be defined in /Alarms/Alarms
                            If NULL reset all alarms

  Output:
    <none>

  Function value:
    AL_INVALID_NAME         Alarm name not defined
    AL_RESET                Alarm was triggered and reset
    AL_SUCCESS              Successful completion

\********************************************************************/
{
   int status, size, i;
   HNDLE hDB, hkeyalarm, hkeyclass, hsubkey;
   KEY key;
   char str[256];
   ALARM alarm;
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
      cm_msg(MERROR, "al_reset_alarm", "Alarm %s not found in ODB", alarm_name);
      return AL_INVALID_NAME;
   }

   size = sizeof(alarm);
   status = db_get_record(hDB, hkeyalarm, &alarm, &size, 0);
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "al_reset_alarm", "Cannot get alarm record");
      return AL_ERROR_ODB;
   }

   sprintf(str, "/Alarms/Classes/%s", alarm.alarm_class);
   db_find_key(hDB, 0, str, &hkeyclass);
   if (!hkeyclass) {
      cm_msg(MERROR, "al_reset_alarm", "Alarm class %s not found in ODB", alarm.alarm_class);
      return AL_INVALID_NAME;
   }

   size = sizeof(ac);
   status = db_get_record(hDB, hkeyclass, &ac, &size, 0);
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "al_reset_alarm", "Cannot get alarm class record");
      return AL_ERROR_ODB;
   }

   if (alarm.triggered) {
      alarm.triggered = 0;
      alarm.time_triggered_first[0] = 0;
      alarm.time_triggered_last[0] = 0;
      alarm.checked_last = 0;

      ac.system_message_last = 0;
      ac.execute_last = 0;

      status = db_set_record(hDB, hkeyalarm, &alarm, sizeof(alarm), 0);
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
INT al_check()
/********************************************************************\

  Routine: al_scan

  Purpose: Scan ODB alarams and programs

  Input:

  Output:

  Function value:
    AL_SUCCESS              Successful completion

\********************************************************************/
{
   if (rpc_is_remote())
      return rpc_call(RPC_AL_CHECK);

#ifdef LOCAL_ROUTINES
   {
      INT i, status, size, mutex;
      HNDLE hDB, hkeyroot, hkey;
      KEY key;
      ALARM alarm;
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
      cm_get_experiment_mutex(&mutex, NULL);
      status = ss_mutex_wait_for(mutex, 100);
      if (status != SS_SUCCESS)
         return SUCCESS;        /* someone else is doing alarm business */

      /* check ODB alarms */
      db_find_key(hDB, 0, "/Alarms/Alarms", &hkeyroot);
      if (!hkeyroot) {
         /* create default ODB alarm */
         status = db_create_record(hDB, 0, "/Alarms/Alarms/Demo ODB", strcomb(alarm_odb_str));
         db_find_key(hDB, 0, "/Alarms/Alarms", &hkeyroot);
         if (!hkeyroot) {
            ss_mutex_release(mutex);
            return SUCCESS;
         }

         status = db_create_record(hDB, 0, "/Alarms/Alarms/Demo periodic", strcomb(alarm_periodic_str));
         db_find_key(hDB, 0, "/Alarms/Alarms", &hkeyroot);
         if (!hkeyroot) {
            ss_mutex_release(mutex);
            return SUCCESS;
         }

         /* create default alarm classes */
         status = db_create_record(hDB, 0, "/Alarms/Classes/Alarm", strcomb(alarm_class_str));
         status = db_create_record(hDB, 0, "/Alarms/Classes/Warning", strcomb(alarm_class_str));
         if (status != DB_SUCCESS) {
            ss_mutex_release(mutex);
            return SUCCESS;
         }
      }

      for (i = 0;; i++) {
         status = db_enum_key(hDB, hkeyroot, i, &hkey);
         if (status == DB_NO_MORE_SUBKEYS)
            break;

         db_get_key(hDB, hkey, &key);

         size = sizeof(alarm);
         status = db_get_record(hDB, hkey, &alarm, &size, 0);
         if (status != DB_SUCCESS || alarm.type < 1 || alarm.type > AT_LAST) {
            /* make sure alarm record has right structure */
            db_check_record(hDB, hkey, "", strcomb(alarm_odb_str), TRUE);
            size = sizeof(alarm);
            status = db_get_record(hDB, hkey, &alarm, &size, 0);
            if (status != DB_SUCCESS || alarm.type < 1 || alarm.type > AT_LAST) {
               cm_msg(MERROR, "al_check", "Cannot get alarm record");
               continue;
            }
         }

         /* check periodic alarm only when active */
         if (alarm.active &&
             alarm.type == AT_PERIODIC &&
             alarm.check_interval > 0 && (INT) ss_time() - (INT) alarm.checked_last > alarm.check_interval) {
            /* if checked_last has not been set, set it to current time */
            if (alarm.checked_last == 0) {
               alarm.checked_last = ss_time();
               db_set_record(hDB, hkey, &alarm, size, 0);
            } else
               al_trigger_alarm(key.name, alarm.alarm_message, alarm.alarm_class, "", AT_PERIODIC);
         }

         /* check alarm only when active and not internal */
         if (alarm.active &&
             alarm.type == AT_EVALUATED &&
             alarm.check_interval > 0 && (INT) ss_time() - (INT) alarm.checked_last > alarm.check_interval) {
            /* if condition is true, trigger alarm */
            if (al_evaluate_condition(alarm.condition, value)) {
               sprintf(str, alarm.alarm_message, value);
               al_trigger_alarm(key.name, str, alarm.alarm_class, "", AT_EVALUATED);
            } else {
               alarm.checked_last = ss_time();
               status = db_set_record(hDB, hkey, &alarm, sizeof(alarm), 0);
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

      ss_mutex_release(mutex);
   }
#endif                          /* LOCAL_COUTINES */

   return SUCCESS;
}

/**dox***************************************************************/
#endif                          /* DOXYGEN_SHOULD_SKIP_THIS */

                                                                                                             /** @} *//* end of alfunctionc */

