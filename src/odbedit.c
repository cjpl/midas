/********************************************************************\

  Name:         odbedit.c
  Created by:   Stefan Ritt

  Contents:     Command-line interface to the MIDAS online data base.

  $Id$

\********************************************************************/

#include "midas.h"
#include "msystem.h"
#include <assert.h>

extern INT cmd_edit(char *prompt, char *cmd, INT(*dir) (char *, INT *), INT(*idle) ());

BOOL need_redraw;
BOOL in_cmd_edit;
char pwd[256];
BOOL cmd_mode;

/* Data record global to save memory for MS-DOS */
#ifdef OS_MSDOS
static char data[5000];
#else
static char data[50000];
#endif

typedef struct {
   int flags;
   char pattern[32];
   int index;
} PRINT_INFO;

#define PI_LONG      (1<<0)
#define PI_RECURSIVE (1<<1)
#define PI_VALUE     (1<<2)
#define PI_HEX       (1<<3)
#define PI_PAUSE     (1<<4)

MUTEX_T *tm;

/*------------------------------------------------------------------*/

INT thread(void *p)
{
   int status;
   do {
      ss_sleep(1000);
      status = ss_mutex_wait_for(tm, 10000);
      printf("%d in critical section, mutex lock status %d\n", ss_gettid(), status);
      ss_sleep(3000);
      printf("%d out of critical section\n", ss_gettid());
      status = ss_mutex_release(tm);
      if (status != SUCCESS)
         printf("%d unlock status %d\n", ss_gettid(), status);
   } while (1);
   
   return 0;
}

/*------------------------------------------------------------------*/

BOOL key_modified;

void key_update(HNDLE hDB, HNDLE hkey, void *info)
{
   key_modified = TRUE;
}

/*------------------------------------------------------------------*/

void print_help(char *command)
{
#ifndef OS_MSDOS                /* save some DGROUP memory under MS-DOS */
   if (!command[0]) {
      printf("Database commands ([] are options, <> are placeholders):\n\n");
      printf("alarm                   - reset all alarms\n");
      printf("cd <dir>                - change current directory\n");
      printf("chat                    - enter chat mode\n");
      printf("chmod <mode> <key>      - change access mode of a key\n");
      printf("                          1=read | 2=write | 4=delete\n");
      printf("cleanup [client] [-f]   - delete hanging clients [force]\n");
      printf("copy <src> <dest>       - copy a subtree to a new location\n");
      printf("create <type> <key>     - create a key of a certain type\n");
      printf("create <type> <key>[n]  - create an array of size [n]\n");
      printf("create string <key>[n][m]  - create an array of [n] strings of [m] characters\n");
      printf("del/rm [-l] [-f] <key>  - delete a key and its subkeys\n");
      printf("  -l                      follow links\n");
      printf("  -f                      force deletion without asking\n");
      printf
          ("exec <key>/<cmd>        - execute shell command (stored in key) on server\n");
      printf("exp <key> <filename>    - import key into ASCII file\n");
      printf("find <pattern>          - find a key with wildcard pattern\n");
      printf("help/? [command]        - print this help [for a specific command]\n");
      printf("hi [analyzer] [id]      - tell analyzer to clear histos\n");
      printf("imp <filename> [key]    - import ASCII file into string key\n");
      printf("ln <source> <linkname>  - create a link to <source> key\n");
      printf
          ("load <file>             - load database from .ODB file at current position\n");
      printf("-- hit return for more --\r");
      getchar();
      printf("ls/dir [-lhvrp] [<pat>] - show database entries which match pattern\n");
      printf("  -l                      detailed info\n");
      printf("  -h                      hex format\n");
      printf("  -v                      only value\n");
      printf("  -r                      show database entries recursively\n");
      printf("  -p                      pause between screens\n");
      printf("make [analyzer name]    - create experim.h\n");
      printf("mem [-v]                - show memeory usage [verbose]\n");
      printf("mkdir <subdir>          - make new <subdir>\n");
      printf("move <key> [top/bottom/[n]] - move key to position in keylist\n");
      printf("msg [type] [user] <msg> - compose user message\n");
      printf("old [n]                 - display old n messages\n");
      printf("passwd                  - change MIDAS password\n");
      printf("pause                   - pause current run\n");
      printf("pwd                     - show current directory\n");
      printf("resume                  - resume current run\n");
      printf("rename <old> <new>      - rename key\n");
      printf("rewind [channel]        - rewind tapes in logger\n");
      printf("-- hit return for more --\r");
      getchar();
      printf("save [-c -s -x -j -cs] <file>  - save database at current position\n");
      printf("                          in ASCII format\n");
      printf("  -c                      as a C structure\n");
      printf("  -s                      as a #define'd string\n");
      printf("  -x                      as a XML file\n");
      printf("  -j                      as a JSON file\n");
      printf("set <key> <value>       - set the value of a key\n");
      printf("set <key>[i] <value>    - set the value of index i\n");
      printf("set <key>[*] <value>    - set the value of all indices of a key\n");
      printf("set <key>[i..j] <value> - set the value of all indices i..j\n");
      printf("scl [-w]                - show all active clients [with watchdog info]\n");
      printf("shutdown <client>/all   - shutdown individual or all clients\n");
      printf("sor                     - show open records in current subtree\n");
      printf("start [number][now][-v] - start a run [with a specific number],\n");
      printf
          ("                          [now] w/o asking parameters, [-v] debug output\n");
      printf("stop [-v]               - stop current run, [-v] debug output\n");
      printf("trunc <key> <index>     - truncate key to [index] values\n");
      printf("ver                     - show MIDAS library version\n");
      printf("webpasswd               - change WWW password for mhttpd\n");
      printf("wait <key>              - wait for key to get modified\n");

      printf("\nquit/exit               - exit\n");
      return;
   }

   if (equal_ustring(command, "cd")) {
      printf("cd <dir> - change current directory. Use \"cd /\" to change to the root\n");
      printf("           of the ODB, \"cd ..\" to change to the parent directory.\n");
   } else if (equal_ustring(command, "chat")) {
      printf("chat - enter chat mode. In this mode, users can \"talk\" to each other.\n");
      printf
          ("       Each user running ODBEdit connected to the same experiment can see\n");
      printf
          ("       the other messages and each user running ODBEdit in chat mode can\n");
      printf
          ("       produce messages. All messages are logged in the MIDAS logging file.\n");
   } else
      printf("No specific help available for command \"%s\".\n", command);

#endif
}

/*------------------------------------------------------------------*/

void process_message(HNDLE hBuf, HNDLE id, EVENT_HEADER * pheader, void *message)
{
   time_t tm;
   char str[80];

   /* prepare time */
   time(&tm);
   strcpy(str, ctime(&tm));
   str[19] = 0;

   /* print message text which comes after event header */
   if (in_cmd_edit)
      printf("\r%s %s\n", str + 11, (char *) message);
   else
      printf("\n%s %s\n", str + 11, (char *) message);

   need_redraw = TRUE;
}

int print_message(const char *msg)
{
   if (in_cmd_edit)
      printf("\r%s\n", msg);
   else
      printf("%s\n", msg);

   need_redraw = TRUE;
   return 0;
}

/*------------------------------------------------------------------*/

BOOL match(char *pat, char *str)
/* process a wildcard match */
{
   if (!*str)
      return *pat == '*' ? match(pat + 1, str) : !*pat;

   switch (*pat) {
   case '\0':
      return 0;
   case '*':
      return match(pat + 1, str) || match(pat, str + 1);
   case '?':
      return match(pat + 1, str + 1);
   default:
      return (toupper(*pat) == toupper(*str)) && match(pat + 1, str + 1);
   }
}

/*------------------------------------------------------------------*/

int ls_line, ls_abort;

BOOL check_abort(int flags, int l)
{
   int c;

   if ((flags & PI_PAUSE) && (l % 24) == 23) {
      printf("Press any key to continue or q to quit ");
      fflush(stdout);
      do {
         c = ss_getchar(0);
         if (c == 'q') {
            printf("\n");
            ls_abort = TRUE;
            return TRUE;
         } else if (c > 0) {
            printf("\r                                        \r");
            return FALSE;
         }

         cm_yield(100);

      } while (!cm_is_ctrlc_pressed());
   }

   if (cm_is_ctrlc_pressed()) {
      ls_abort = TRUE;
      return TRUE;
   }

   return FALSE;
}


INT print_key(HNDLE hDB, HNDLE hKey, KEY * pkey, INT level, void *info)
{
   INT i, size, status;
   static char data_str[50000], line[256];
   DWORD delta;
   PRINT_INFO *pi;
   KEY key;

   if (ls_abort)
      return 0;

   pi = (PRINT_INFO *) info;
   memcpy(&key, pkey, sizeof(KEY));

   /* if pattern set, check if match */
   if (pi->pattern[0] && !match(pi->pattern, pkey->name))
      return SUCCESS;

   size = sizeof(data);
   if (pi->flags & PI_VALUE) {
      /* only print value */
      if (key.type != TID_KEY) {
         status = db_get_link_data(hDB, hKey, data, &size, key.type);
         data_str[0] = 0;

         /* resolve links */
         if (key.type == TID_LINK) {
            if (strlen(data) > 0 && data[strlen(data)-1] == ']')
               status = DB_SUCCESS;
            else
               status = db_find_key(hDB, 0, data, &hKey);
            if (status == DB_SUCCESS) {
               status = db_get_key(hDB, hKey, &key);
               if (status == DB_SUCCESS) {
                  size = sizeof(data);
                  if (key.type != TID_KEY)
                     status = db_get_data(hDB, hKey, data, &size, key.type);
                  else
                     status = DB_TYPE_MISMATCH;
               }
            }
         }

         if (status == DB_NO_KEY)
            strcat(data_str, "<cannot resolve link>");
         else if (status == DB_NO_ACCESS)
            strcat(data_str, "<no read access>");
         else
            for (i = 0; i < key.num_values; i++) {
               if (pi->flags & PI_HEX)
                  db_sprintfh(data_str, data, key.item_size, i, key.type);
               else
                  db_sprintf(data_str, data, key.item_size, i, key.type);

               if ((pi->index != -1 && i == pi->index) || pi->index == -1)
                  printf("%s\n", data_str);
               if (check_abort(pi->flags, ls_line++))
                  return 0;
            }
      }
   } else {
      /* print key name with value */
      memset(line, ' ', sizeof(line));
      line[sizeof(line)-1] = 0;
      sprintf(line + level * 4, "%s", key.name);
      if (key.type == TID_LINK) {
         db_get_link_data(hDB, hKey, data, &size, key.type);
         sprintf(line + strlen(line), " -> %s", data);
         if (pi->index != -1)
            sprintf(line + strlen(line), "[%d]", pi->index);
         if (strlen(line) >= 32) {
            printf("%s\n", line);
            memset(line, ' ', sizeof(line));
            line[80] = 0;
         } else
            line[strlen(line)] = ' ';
      } else {
         if (pi->index != -1)
            sprintf(line + strlen(line), "[%d]", pi->index);
         line[strlen(line)] = ' ';
      }

      if (key.type == TID_KEY) {
         if (pi->flags & PI_LONG)
            sprintf(line + 32, "DIR");
         else
            line[32] = 0;
         printf("%s\n", line);
         if (check_abort(pi->flags, ls_line++))
            return 0;
      } else {

         status = db_get_link_data(hDB, hKey, data, &size, key.type);
         data_str[0] = 0;

         /* resolve links */
         if (key.type == TID_LINK) {
            if (strlen(data) > 0 && data[strlen(data)-1] == ']')
               status = DB_SUCCESS;
            else
               status = db_find_key(hDB, 0, data, &hKey);
            if (status == DB_SUCCESS) {
               status = db_get_key(hDB, hKey, &key);
               if (status == DB_SUCCESS) {
                  size = sizeof(data);
                  if (key.type != TID_KEY)
                     status = db_get_data(hDB, hKey, data, &size, key.type);
                  else
                     status = DB_TYPE_MISMATCH;
               }
            }
         }

         if (status == DB_TYPE_MISMATCH)
            strcat(data_str, "<subdirectory>");
         else if (status == DB_NO_KEY || status == DB_INVALID_LINK)
            strcat(data_str, "<cannot resolve link>");
         else if (status == DB_NO_ACCESS)
            strcat(data_str, "<no read access>");
         else {
            if (pi->flags & PI_HEX)
               db_sprintfh(data_str+strlen(data_str), data, key.item_size, 0, key.type);
            else
               db_sprintf(data_str+strlen(data_str), data, key.item_size, 0, key.type);
         }

         if (pi->flags & PI_LONG) {
            sprintf(line + 32, "%s", rpc_tid_name(key.type));
            line[strlen(line)] = ' ';
            sprintf(line + 40, "%d", key.num_values);
            line[strlen(line)] = ' ';
            sprintf(line + 46, "%d", key.item_size);
            line[strlen(line)] = ' ';

            db_get_key_time(hDB, hKey, &delta);
            if (delta < 60)
               sprintf(line + 52, "%ds", delta);
            else if (delta < 3600)
               sprintf(line + 52, "%1.0lfm", delta / 60.0);
            else if (delta < 86400)
               sprintf(line + 52, "%1.0lfh", delta / 3600.0);
            else if (delta < 86400 * 99)
               sprintf(line + 52, "%1.0lfh", delta / 86400.0);
            else
               sprintf(line + 52, ">99d");
            line[strlen(line)] = ' ';

            sprintf(line + 57, "%d", key.notify_count);
            line[strlen(line)] = ' ';

            if (key.access_mode & MODE_READ)
               line[61] = 'R';
            if (key.access_mode & MODE_WRITE)
               line[62] = 'W';
            if (key.access_mode & MODE_DELETE)
               line[63] = 'D';
            if (key.access_mode & MODE_EXCLUSIVE)
               line[64] = 'E';

            if (key.type == TID_STRING && strchr(data_str, '\n'))
               strcpy(line + 66, "<multi-line>");
            else if (key.num_values == 1)
               strcpy(line + 66, data_str);
            else
               line[66] = 0;
         } else if (key.num_values == 1)
            if (key.type == TID_STRING && strchr(data_str, '\n'))
               strcpy(line + 32, "<multi-line>");
            else
               strcpy(line + 32, data_str);
         else
            while (line[strlen(line)-1] == ' ')
               line[strlen(line)-1] = 0;

         if (line[0])
            printf("%s\n", line);

         if (key.type == TID_STRING && strchr(data_str, '\n'))
            puts(data_str);

         if (check_abort(pi->flags, ls_line++))
            return 0;

         if (key.num_values > 1) {
            for (i = 0; i < key.num_values; i++) {
               if (pi->flags & PI_HEX)
                  db_sprintfh(data_str, data, key.item_size, i, key.type);
               else
                  db_sprintf(data_str, data, key.item_size, i, key.type);

               memset(line, ' ', 80);
               line[80] = 0;

               if (pi->flags & PI_LONG) {
                  sprintf(line + 40, "[%d]", i);
                  line[strlen(line)] = ' ';

                  strcpy(line + 56, data_str);
               } else
                  strcpy(line + 32, data_str);

               if ((pi->index != -1 && i == pi->index) || pi->index == -1)
                  printf("%s\n", line);

               if (check_abort(pi->flags, ls_line++))
                  return 0;
            }
         }
      }
   }

   return SUCCESS;
}

/*------------------------------------------------------------------*/

void set_key(HNDLE hDB, HNDLE hKey, int index1, int index2, char *value)
{
   KEY key;
   char data[1000];
   int i, size, status = 0;

   db_get_link(hDB, hKey, &key);

   memset(data, 0, sizeof(data));
   db_sscanf(value, data, &size, 0, key.type);

   /* extend data size for single string if necessary */
   if ((key.type == TID_STRING || key.type == TID_LINK)
       && (int) strlen(data) + 1 > key.item_size && key.num_values == 1)
      key.item_size = strlen(data) + 1;

   if (key.item_size == 0)
      key.item_size = rpc_tid_size(key.type);

   if (key.num_values > 1 && index1 == -1) {
      for (i = 0; i < key.num_values; i++)
         status = db_set_link_data_index(hDB, hKey, data, key.item_size, i, key.type);
   } else if (key.num_values > 1 && index2 > index1) {
      for (i = index1; i < key.num_values && i <= index2; i++)
         status = db_set_link_data_index(hDB, hKey, data, key.item_size, i, key.type);
   } else if (key.num_values > 1 || index1 > 0)
      status = db_set_link_data_index(hDB, hKey, data, key.item_size, index1, key.type);
   else
      status = db_set_link_data(hDB, hKey, data, key.item_size, 1, key.type);

   if (status == DB_NO_ACCESS)
      printf("Write access not allowed\n");
}

/*------------------------------------------------------------------*/

void scan_tree(HNDLE hDB, HNDLE hKey, INT * total_size_key, INT * total_size_data,
               INT level, INT flags)
{
   INT i, j, size, status;
   KEY key;
   HNDLE hSubkey;
   static char data_str[256], line[256];
   DWORD delta;

   if (cm_is_ctrlc_pressed()) {
      if (level == 0)
         cm_ack_ctrlc_pressed();
      return;
   }

   db_get_key(hDB, hKey, &key);
   size = sizeof(data);

   *total_size_key += ALIGN8(sizeof(KEY));
   if (key.type == TID_KEY)
      *total_size_key += ALIGN8(sizeof(KEYLIST));
   else
      *total_size_data += ALIGN8(key.total_size);

   if (flags & 0x4) {
      /* only print value */
      if (key.type != TID_KEY) {
         status = db_get_data(hDB, hKey, data, &size, key.type);
         if (status == DB_NO_ACCESS)
            strcpy(data_str, "<no read access>");
         else
            for (j = 0; j < key.num_values; j++) {
               if (flags & 0x8)
                  db_sprintfh(data_str, data, key.item_size, j, key.type);
               else
                  db_sprintf(data_str, data, key.item_size, j, key.type);
               printf("%s\n", data_str);
            }
      }
   } else {
      /* print key name with value */
      memset(line, ' ', 80);
      line[80] = 0;
      sprintf(line + level * 4, "%s", key.name);
      line[strlen(line)] = ' ';

      if (key.type == TID_KEY) {
         line[32] = 0;
         printf("%s\n", line);
      } else {
         status = db_get_data(hDB, hKey, data, &size, key.type);
         if (status == DB_NO_ACCESS)
            strcpy(data_str, "<no read access>");
         else {
            if (flags & 0x8)
               db_sprintfh(data_str, data, key.item_size, 0, key.type);
            else
               db_sprintf(data_str, data, key.item_size, 0, key.type);
         }

         if (flags & 0x1) {
            sprintf(line + 32, "%s", rpc_tid_name(key.type));
            line[strlen(line)] = ' ';
            sprintf(line + 40, "%d", key.num_values);
            line[strlen(line)] = ' ';
            sprintf(line + 46, "%d", key.item_size);
            line[strlen(line)] = ' ';

            db_get_key_time(hDB, hKey, &delta);
            if (delta < 60)
               sprintf(line + 52, "%ds", delta);
            else if (delta < 3600)
               sprintf(line + 52, "%1.0lfm", delta / 60.0);
            else if (delta < 86400)
               sprintf(line + 52, "%1.0lfh", delta / 3600.0);
            else if (delta < 86400 * 99)
               sprintf(line + 52, "%1.0lfh", delta / 86400.0);
            else
               sprintf(line + 52, ">99d");
            line[strlen(line)] = ' ';

            sprintf(line + 57, "%d", key.notify_count);
            line[strlen(line)] = ' ';

            if (key.access_mode & MODE_READ)
               line[61] = 'R';
            if (key.access_mode & MODE_WRITE)
               line[62] = 'W';
            if (key.access_mode & MODE_DELETE)
               line[63] = 'D';
            if (key.access_mode & MODE_EXCLUSIVE)
               line[64] = 'E';

            if (key.num_values == 1)
               strcpy(line + 66, data_str);
            else
               line[66] = 0;
         } else if (key.num_values == 1)
            strcpy(line + 32, data_str);
         else
            line[32] = 0;

         printf("%s\n", line);

         if (key.num_values > 1) {
            for (j = 0; j < key.num_values; j++) {
               if (flags & 0x8)
                  db_sprintfh(data_str, data, key.item_size, j, key.type);
               else
                  db_sprintf(data_str, data, key.item_size, j, key.type);

               memset(line, ' ', 80);
               line[80] = 0;

               if (flags & 0x1) {
                  sprintf(line + 40, "[%d]", j);
                  line[strlen(line)] = ' ';

                  strcpy(line + 56, data_str);
               } else
                  strcpy(line + 32, data_str);

               printf("%s\n", line);
            }
         }
      }
   }

   /* recurse subtree */
   if (key.type == TID_KEY && (flags & 0x2)) {
      for (i = 0;; i++) {
         db_enum_link(hDB, hKey, i, &hSubkey);

         if (!hSubkey)
            break;

         scan_tree(hDB, hSubkey, total_size_key, total_size_data, level + 1, flags);

         if (cm_is_ctrlc_pressed()) {
            if (level == 0)
               cm_ack_ctrlc_pressed();
            return;
         }
      }
   }
}

/*------------------------------------------------------------------*/

/* complete partial key name, gets called from cmd_edit */
INT cmd_dir(char *line, INT * cursor)
{
   KEY key;
   HNDLE hDB, hKey, hSubkey;
   INT i, j, match, size;
   char str[256], *pc, partial[256], last_match[256], head[256], tail[256], key_name[256],
       c;
   char test_key[256];
   BOOL blanks, mismatch;

   cm_get_experiment_database(&hDB, NULL);

   /* remember tail for later */
   strcpy(head, line);
   strcpy(tail, line + *cursor);
   line[*cursor] = 0;

   /* search beginning of key */
   pc = head;
   do {
      while (*pc && *pc != ' ')
         pc++;
      while (*pc && *pc == ' ')
         pc++;
   } while (*pc == '-');        /* skip flags */

   if (*pc) {
      strcpy(key_name, pc);
      *pc = 0;                  /* end of head */
   } else
      key_name[0] = 0;

   /* check if key exists (for "set <key>" completion) */
   if (strncmp(head, "set", 3) == 0 && strlen(key_name) > 0) {
      if (key_name[0] == '"')
         strcpy(str, key_name + 1);
      else
         strcpy(str, key_name);
      if (key_name[0] != '/') {
         strcpy(test_key, pwd);
         if (str[strlen(test_key) - 1] != '/')
            strcat(test_key, "/");
         strcat(test_key, str);
      } else
         strcpy(test_key, str);

      pc = test_key + strlen(test_key) - 1;
      while (pc > test_key && (*pc == ' ' || *pc == '"'))
         *pc-- = 0;
      if (db_find_key(hDB, 0, test_key, &hSubkey) == DB_SUCCESS) {
         if (strlen(key_name) > 0 && key_name[strlen(key_name)-1] != ' ')
            strcat(key_name, " ");

         /* retrieve key data */
         db_get_link(hDB, hSubkey, &key);
         size = sizeof(data);
         db_get_link_data(hDB, hSubkey, data, &size, key.type);
         
         if (key.type == TID_STRING || key.type == TID_LINK)
            sprintf(str, "\"%s\"", data);
         else
            db_sprintf(str, data, size, 0, key.type);

         strcpy(line, head);
         strcat(line, key_name);
         strcat(line, str);
         *cursor = strlen(line);
         strcat(line, tail);
         return TRUE;
      }
   }

   /* combine pwd and key_name */
   pc = key_name;
   if (*pc == '"')
      pc++;

   if (*pc != '/') {
      strcpy(str, pwd);
      if (str[strlen(str) - 1] != '/')
         strcat(str, "/");
      strcat(str, pc);
   } else
      strcpy(str, pc);

   /* split key_name into known and new directory */
   for (pc = str + strlen(str) - 1; pc > str && *pc != '/'; pc--);

   if (*pc == '/') {
      *pc = 0;
      strcpy(partial, pc + 1);
   } else
      strcpy(partial, str);

   db_find_link(hDB, 0, str, &hKey);
   for (i = 0, match = 0;; i++) {
      db_enum_link(hDB, hKey, i, &hSubkey);

      if (!hSubkey)
         break;

      db_get_link(hDB, hSubkey, &key);
      strcpy(str, key.name);

      str[strlen(partial)] = 0;

      if (equal_ustring(str, partial))
         match++;
   }

   if (match != 1)
      printf("\r\n");

   for (i = 0;; i++) {
      db_enum_link(hDB, hKey, i, &hSubkey);

      if (!hSubkey)
         break;

      db_get_link(hDB, hSubkey, &key);
      strcpy(str, key.name);

      str[strlen(partial)] = 0;

      if (equal_ustring(str, partial)) {
         if (match == 1) {
            /* search split point */
            pc = key_name;
            if (strlen(key_name) > 0)
               for (pc = key_name + strlen(key_name) - 1; pc > key_name && *pc != '/';
                    pc--);
            if (*pc == '/')
               pc++;

            strcpy(pc, key.name);
            if (key.type == TID_KEY)
               strcat(pc, "/");

            /* insert '"' if blanks in name */
            if (strchr(key.name, ' ')) {
               if (key_name[0] != '"') {
                  for (i = strlen(key_name); i >= 0; i--)
                     key_name[i + 1] = key_name[i];

                  key_name[0] = '"';
               }
               if (key.type != TID_KEY)
                  strcat(key_name, "\"");
            }

            if (key.type != TID_KEY) {
               if (key_name[0] == '"' && key_name[strlen(key_name) - 1] != '"')
                  strcat(pc, "\" ");
               else
                  strcat(pc, " ");
            }

            strcpy(line, head);
            strcat(line, key_name);
            *cursor = strlen(line);
            strcat(line, tail);
            return TRUE;
         }
      }
      if (match == 0 || (match > 1 && equal_ustring(str, partial)))
         printf("%s\r\n", key.name);
   }

   if (match > 1 && key_name[0]) {
      blanks = FALSE;

      for (j = strlen(partial);; j++) {
         for (i = 0, c = 1, mismatch = FALSE;; i++) {
            db_enum_link(hDB, hKey, i, &hSubkey);

            if (!hSubkey)
               break;

            db_get_link(hDB, hSubkey, &key);
            strcpy(str, key.name);

            str[strlen(partial)] = 0;

            if (strchr(key.name, ' '))
               blanks = TRUE;

            if (equal_ustring(str, partial)) {
               strcpy(last_match, key.name);
               if (c == 1)
                  c = toupper(key.name[j]);
               else if (c != toupper(key.name[j])) {
                  mismatch = TRUE;
                  break;
               }
            }
         }

         if (mismatch || last_match[j] == 0)
            break;
      }

      /* search split point */
      for (pc = key_name + strlen(key_name) - 1; pc > key_name && *pc != '/'; pc--);
      if (*pc == '/')
         pc++;

      for (i = 0; i < j; i++)
         pc[i] = last_match[i];
      pc[i] = 0;

      /* insert '"' if blanks in name */
      if (blanks) {
         if (key_name[0] != '"') {
            for (i = strlen(key_name); i >= 0; i--)
               key_name[i + 1] = key_name[i];

            key_name[0] = '"';
         }
      }

      strcpy(line, head);
      strcat(line, key_name);
      *cursor = strlen(line);
      strcat(line, tail);
      return TRUE;
   }

   /* beep if not found */
   printf("\007");

   strcpy(line, head);
   strcat(line, key_name);
   *cursor = strlen(line);
   strcat(line, tail);
   return FALSE;
}

/*------------------------------------------------------------------*/

INT search_key(HNDLE hDB, HNDLE hKey, KEY * key, INT level, void *info)
{
   INT i, size, status;
   char *pattern;
   static char data_str[MAX_ODB_PATH], line[MAX_ODB_PATH];
   char path[MAX_ODB_PATH];

   pattern = (char *) info;

   if (match(pattern, key->name)) {
      db_get_path(hDB, hKey, path, MAX_ODB_PATH);
      strcpy(line, path);
      strcat(line, " : ");

      if (key->type != TID_KEY) {
         size = sizeof(data);
         status = db_get_data(hDB, hKey, data, &size, key->type);

         if (status == DB_NO_ACCESS)
            strcpy(data_str, "<no read access>");
         else
            db_sprintf(data_str, data, key->item_size, 0, key->type);

         if (key->num_values == 1)
            strcat(line, data_str);

         printf("%s\n", line);

         if (key->num_values > 1)
            for (i = 0; i < key->num_values; i++) {
               db_sprintf(data_str, data, key->item_size, i, key->type);

               printf("  [%d] : %s\n", i, data_str);
            }
      } else
         printf("%s\n", line);
   }

   return SUCCESS;
}

/*------------------------------------------------------------------*/

void del_tree(HNDLE hDB, HNDLE hKey, INT level)
{
   INT i;
   KEY key;
   HNDLE hSubkey;

   for (i = 0;; i++) {
      db_enum_link(hDB, hKey, i, &hSubkey);

      if (!hSubkey)
         break;

      db_get_key(hDB, hSubkey, &key);
      if (key.type == TID_KEY)
         del_tree(hDB, hSubkey, level + 1);

      if (rand() < RAND_MAX / 10)
         db_delete_key(hDB, hSubkey, 0);
   }
}

/*------------------------------------------------------------------*/

void create_experim_h(HNDLE hDB, char *analyzer_name)
{
   INT i, index, subindex, hfile, status, size;
   HNDLE hKey, hKeyRoot, hKeyEq, hDefKey, hKeyBank, hKeyPar;
   char str[80], eq_name[80], subeq_name[80];
   KEY key;
   time_t now;

   char experim_h_comment1[] =
       "/********************************************************************\\\n\
\n\
  Name:         experim.h\n\
  Created by:   ODBedit program\n\
\n\
  Contents:     This file contains C structures for the \"Experiment\"\n\
                tree in the ODB and the \"/Analyzer/Parameters\" tree.\n\
\n\
                Additionally, it contains the \"Settings\" subtree for\n\
                all items listed under \"/Equipment\" as well as their\n\
                event definition.\n\
\n\
                It can be used by the frontend and analyzer to work\n\
                with these information.\n\
\n\
                All C structures are accompanied with a string represen-\n\
                tation which can be used in the db_create_record function\n\
                to setup an ODB structure which matches the C structure.\n\
\n";
   char experim_h_comment2[] =
       "\\********************************************************************/\n\n";

   char *file_name = "experim.h";

   /* create file */
   hfile = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
   if (hfile == -1)
      cm_msg(MERROR, "create_experim_h", "cannot open experim.h file.");

   /* write comment to file */
   write(hfile, experim_h_comment1, strlen(experim_h_comment1));
   time(&now);
   sprintf(str, "  Created on:   %s\n", ctime(&now));
   write(hfile, str, strlen(str));
   write(hfile, experim_h_comment2, strlen(experim_h_comment2));

   /* write /experiment/run parameters */
   db_find_key(hDB, 0, "/Experiment/Run Parameters", &hKey);
   if (hKey) {
      sprintf(str, "#define EXP_PARAM_DEFINED\n\n");
      write(hfile, str, strlen(str));
      db_save_struct(hDB, hKey, file_name, "EXP_PARAM", TRUE);
      db_save_string(hDB, hKey, file_name, "EXP_PARAM_STR", TRUE);
      lseek(hfile, 0, SEEK_END);
   }

   /* write /experiment/edit on start */
   db_find_key(hDB, 0, "/Experiment/Edit on start", &hKey);
   if (hKey) {
      sprintf(str, "#define EXP_EDIT_DEFINED\n\n");
      write(hfile, str, strlen(str));
      db_save_struct(hDB, hKey, file_name, "EXP_EDIT", TRUE);
      db_save_string(hDB, hKey, file_name, "EXP_EDIT_STR", TRUE);
      lseek(hfile, 0, SEEK_END);
   }

   /* write /<Analyzer>/parameters tree */
   sprintf(str, "/%s/Parameters", analyzer_name);
   status = db_find_key(hDB, 0, str, &hKeyRoot);
   if (status != DB_SUCCESS) {
      printf("Analyzer \"%s\" not found in ODB, skipping analyzer parameters.\n",
             analyzer_name);
   } else {
      for (index = 0;; index++) {
         status = db_enum_key(hDB, hKeyRoot, index, &hKeyPar);
         if (status == DB_NO_MORE_SUBKEYS)
            break;

         db_get_key(hDB, hKeyPar, &key);
         strcpy(eq_name, key.name);
         name2c(eq_name);
         for (i = 0; i < (int) strlen(eq_name); i++)
            eq_name[i] = toupper(eq_name[i]);

         lseek(hfile, 0, SEEK_END);
         sprintf(str, "#ifndef EXCL_%s\n\n", eq_name);
         write(hfile, str, strlen(str));

         sprintf(str, "#define %s_PARAM_DEFINED\n\n", eq_name);
         write(hfile, str, strlen(str));
         sprintf(str, "%s_PARAM", eq_name);
         db_save_struct(hDB, hKeyPar, file_name, str, TRUE);
         sprintf(str, "%s_PARAM_STR", eq_name);
         db_save_string(hDB, hKeyPar, file_name, str, TRUE);

         lseek(hfile, 0, SEEK_END);
         sprintf(str, "#endif\n\n");
         write(hfile, str, strlen(str));
      }
   }

   /* loop through equipment list */
   status = db_find_key(hDB, 0, "/Equipment", &hKeyRoot);
   if (status == DB_SUCCESS)
      for (index = 0;; index++) {
         status = db_enum_key(hDB, hKeyRoot, index, &hKeyEq);
         if (status == DB_NO_MORE_SUBKEYS)
            break;

         db_get_key(hDB, hKeyEq, &key);
         strcpy(eq_name, key.name);
         name2c(eq_name);
         for (i = 0; i < (int) strlen(eq_name); i++)
            eq_name[i] = toupper(eq_name[i]);

         lseek(hfile, 0, SEEK_END);
         sprintf(str, "#ifndef EXCL_%s\n\n", eq_name);
         write(hfile, str, strlen(str));

         size = sizeof(str);
         str[0] = 0;
         db_get_value(hDB, hKeyEq, "Common/Format", str, &size, TID_STRING, TRUE);

         /* if event is in fixed format, extract header file */
         if (equal_ustring(str, "Fixed")) {
            db_find_key(hDB, hKeyEq, "Variables", &hDefKey);
            if (hDefKey) {
               lseek(hfile, 0, SEEK_END);
               sprintf(str, "#define %s_EVENT_DEFINED\n\n", eq_name);
               write(hfile, str, strlen(str));

               sprintf(str, "%s_EVENT", eq_name);
               db_save_struct(hDB, hDefKey, file_name, str, TRUE);
               sprintf(str, "%s_EVENT_STR", eq_name);
               db_save_string(hDB, hDefKey, file_name, str, TRUE);
            }
         }

         /* if event is in MIDAS format, extract bank definition */
         else if (equal_ustring(str, "MIDAS")) {
            db_find_key(hDB, hKeyEq, "Variables", &hDefKey);
            if (hDefKey) {
               for (i = 0;; i++) {
                  status = db_enum_key(hDB, hDefKey, i, &hKeyBank);
                  if (status == DB_NO_MORE_SUBKEYS)
                     break;

                  db_get_key(hDB, hKeyBank, &key);

                  if (key.type == TID_KEY) {
                     lseek(hfile, 0, SEEK_END);
                     sprintf(str, "#define %s_BANK_DEFINED\n\n", key.name);
                     write(hfile, str, strlen(str));

                     sprintf(str, "%s_BANK", key.name);
                     db_save_struct(hDB, hKeyBank, file_name, str, TRUE);
                     sprintf(str, "%s_BANK_STR", key.name);
                     db_save_string(hDB, hKeyBank, file_name, str, TRUE);
                  }
               }
            }
         }

         /* Scan sub tree for that equipment */
         for (subindex = 0;; subindex++) {
            status = db_enum_key(hDB, hKeyEq, subindex, &hDefKey);
            if (status == DB_NO_MORE_SUBKEYS)
               break;

            db_get_key(hDB, hDefKey, &key);
            strcpy(subeq_name, key.name);
            name2c(subeq_name);

            for (i = 0; i < (int) strlen(subeq_name); i++)
               subeq_name[i] = toupper(subeq_name[i]);

            /* Skip only the statistics */
            if (!equal_ustring(subeq_name, "statistics") &&
                !equal_ustring(subeq_name, "variables")) {
               lseek(hfile, 0, SEEK_END);
               sprintf(str, "#define %s_%s_DEFINED\n\n", eq_name, subeq_name);
               write(hfile, str, strlen(str));

               sprintf(str, "%s_%s", eq_name, subeq_name);
               db_save_struct(hDB, hDefKey, file_name, str, TRUE);
               sprintf(str, "%s_%s_STR", eq_name, subeq_name);
               db_save_string(hDB, hDefKey, file_name, str, TRUE);
            }
         }

         lseek(hfile, 0, SEEK_END);
         sprintf(str, "#endif\n\n");
         write(hfile, str, strlen(str));
      }

   close(hfile);

   getcwd(str, sizeof(str));
   printf("\"experim.h\" has been written to %s\n", str);
}

/*------------------------------------------------------------------*/

INT cmd_idle()
{
   INT status;

   need_redraw = FALSE;

   status = cm_yield(100);

   /* abort if server connection is broken */
   if (status == SS_ABORT || status == RPC_SHUTDOWN) {
      if (status == SS_ABORT)
         printf("Server connection broken.\n");

      cm_disconnect_experiment();
      exit(0);
   }

   /* check for broken client connections */
   rpc_client_check();

   return need_redraw;
}

/*------------------------------------------------------------------*/

void compose_name(char *pwd, char *name, char *full_name)
{
   if (name[0] != '/') {
      strcpy(full_name, pwd);
      if (full_name[strlen(full_name) - 1] != '/')
         strcat(full_name, "/");
      strcat(full_name, name);
   } else
      strcpy(full_name, name);
}

/*------------------------------------------------------------------*/

void assemble_prompt(char *prompt, int psize, char *host_name, char *exp_name, char *pwd)
{
   HNDLE hDB;
   char mask[256], str[32];
   int state, size;
   char *pp, *pm, *pc;
   time_t now;

   char *state_char[] = { "U", "S", "P", "R" };
   char *state_str[] = { "Unknown", "Stopped", "Paused", "Running" };

   cm_get_experiment_database(&hDB, NULL);

   size = sizeof(mask);
   strcpy(mask, "[%h:%e:%s]%p>");
   db_get_value(hDB, 0, "/System/Prompt", mask, &size, TID_STRING, TRUE);

   state = STATE_STOPPED;
   size = sizeof(state);
   db_get_value(hDB, 0, "/Runinfo/State", &state, &size, TID_INT, TRUE);
   if (state > STATE_RUNNING)
      state = 0;

   pm = mask;
   pp = prompt;
   memset(prompt, 0, psize);
   do {
      if (*pm != '%') {
         *pp++ = *pm++;
      } else {
         switch (*++pm) {
         case 't':
            time(&now);
            strcpy(str, ctime(&now));
            str[19] = 0;
            strcpy(pp, str + 11);
            break;
         case 'h':
            if (host_name[0])
               strcat(pp, host_name);
            else
               strcat(pp, "local");
            break;
         case 'e':
            if (exp_name[0])
               strcat(pp, exp_name);
            else
               strcat(pp, "Default");
            break;
         case 's':
            strcat(pp, state_char[state]);
            break;
         case 'S':
            strcat(pp, state_str[state]);
            break;
         case 'P':
            strcat(pp, pwd);
            break;
         case 'p':
            pc = pwd + strlen(pwd) - 1;
            while (*pc != '/' && pc != pwd)
               pc--;
            if (pc == pwd)
               strcat(pp, pwd);
            else
               strcat(pp, pc + 1);
            break;
         }
         pm++;
         pp += strlen(pp);
      }
   } while (*pm);
}

/*------------------------------------------------------------------*/

int command_loop(char *host_name, char *exp_name, char *cmd, char *start_dir)
{
   INT status = 0, i, j, k, state, size, old_run_number, new_run_number, channel;
   char line[2000], prompt[256];
   char param[10][2000];
   char str[2000], str2[80], old_dir[256], cwd[256], name[256], *pc, data_str[256];
   char old_password[32], new_password[32];
   INT nparam, flags, index1, index2, debug_flag;
   WORD mode;
   HNDLE hDB, hKey, hKeyClient, hSubkey, hRootKey;
   KEY key;
   char user_name[80] = "";
   FILE *cmd_file = NULL;
   DWORD last_msg_time = 0;
   char message[2000], client_name[256], *p;
   INT n1, n2, msg_type;
   PRINT_INFO print_info;

   cm_get_experiment_database(&hDB, &hKeyClient);

   /* command loop */
   if (start_dir[0])
      strcpy(pwd, start_dir);
   else
      strcpy(pwd, "/");

   /* check if dir exists */
   if (db_find_key(hDB, 0, pwd, &hKey) != DB_SUCCESS) {
      printf("Directory \"%s\" not found.\n", pwd);
      return -1;
   }

   /* open command file */
   if (cmd[0] == '@') {
      cmd_file = fopen(cmd + 1, "r");
      if (cmd_file == NULL) {
         printf("Command file %s not found.\n", cmd + 1);
         return -1;
      }
   }

   do {
      /* print prompt */
      if (!cmd_mode) {
         assemble_prompt(prompt, sizeof(prompt), host_name, exp_name, pwd);

         in_cmd_edit = TRUE;
         line[0] = 0;
         cmd_edit(prompt, line, cmd_dir, cmd_idle);
         in_cmd_edit = FALSE;
      } else if (cmd[0] != '@')
         strlcpy(line, cmd, sizeof(line));
      else {
         memset(line, 0, sizeof(line));
         fgets(line, sizeof(line), cmd_file);

         if (line[0] == 0)
            break;

         /* cut off CR */
         while (strlen(line) > 0 && line[strlen(line) - 1] == '\n')
            line[strlen(line) - 1] = 0;

         if (line[0] == 0)
            continue;
      }

      /* analyze line */
      nparam = 0;
      pc = line;
      while (*pc == ' ')
         pc++;

      memset(param, 0, sizeof(param));
      do {
         if (*pc == '"') {
            pc++;
            for (i = 0; *pc && *pc != '"' && i<sizeof(param[0])-1; i++)
               param[nparam][i] = *pc++;
            if (*pc)
               pc++;
         } else if (*pc == '\'') {
            pc++;
            for (i = 0; *pc && *pc != '\'' && i<sizeof(param[0])-1; i++)
               param[nparam][i] = *pc++;
            if (*pc)
               pc++;
         } else if (*pc == '`') {
            pc++;
            for (i = 0; *pc && *pc != '`' && i<sizeof(param[0])-1; i++)
               param[nparam][i] = *pc++;
            if (*pc)
               pc++;
         } else
            for (i = 0; *pc && *pc != ' ' && i<sizeof(param[0])-1; i++)
               param[nparam][i] = *pc++;
         param[nparam][i] = 0;
         while (*pc == ' ')
            pc++;
         nparam++;
      } while (*pc);

      /* help */
      if ((param[0][0] == 'h' && param[0][1] == 'e') || param[0][0] == '?')
         print_help(param[1]);

      /* ls */
      else if ((param[0][0] == 'l' && param[0][1] == 's') ||
               (param[0][0] == 'd' && param[0][1] == 'i')) {
         db_find_key(hDB, 0, pwd, &hKey);
         print_info.flags = 0;
         print_info.pattern[0] = 0;
         ls_line = 0;
         ls_abort = FALSE;

         /* parse options */
         for (i = 1; i < 4; i++)
            if (param[i][0] == '-') {
               for (j = 1; param[i][j] != ' ' && param[i][j]; j++) {
                  if (param[i][j] == 'l')
                     print_info.flags |= PI_LONG;
                  if (param[i][j] == 'r')
                     print_info.flags |= PI_RECURSIVE;
                  if (param[i][j] == 'v')
                     print_info.flags |= PI_VALUE;
                  if (param[i][j] == 'h')
                     print_info.flags |= PI_HEX;
                  if (param[i][j] == 'p')
                     print_info.flags |= PI_PAUSE;
               }
            }

         for (i = 1; param[i][0] == '-'; i++);

         /* check if parameter contains array index */
         print_info.index = -1;
         if (strchr(param[i], '[') && strchr(param[i], ']')) {
            for (p = strchr(param[i], '[') + 1; *p && *p != ']'; p++)
               if (!isdigit(*p))
                  break;

            if (*p && *p == ']') {
               print_info.index = atoi(strchr(param[i], '[') + 1);
               *strchr(param[i], '[') = 0;
            }
         }

         if (param[i][0]) {
            if (strpbrk(param[i], "*?") != NULL) {
               /* if parameter contains wildcards, set pattern */
               strcpy(print_info.pattern, param[i]);
            } else {
               if (param[i][0] == '/')
                  status = db_find_link(hDB, 0, param[i], &hKey);
               else
                  status = db_find_link(hDB, hKey, param[i], &hKey);

               if (status == DB_NO_KEY)
                  printf("key %s not found\n", param[i]);

               if (status == DB_INVALID_LINK)
                  printf("link %s points to invalid location\n", param[i]);
            }
         }

         if (hKey) {
            if ((print_info.flags & PI_LONG) && (print_info.flags & PI_VALUE) == 0) {
               printf
                   ("Key name                        Type    #Val  Size  Last Opn Mode Value\n");
               printf
                   ("---------------------------------------------------------------------------\n");
            }

            if (print_info.flags & PI_RECURSIVE) {
               db_scan_tree(hDB, hKey, 0, print_key, &print_info);
               if (cm_is_ctrlc_pressed())
                  cm_ack_ctrlc_pressed();
            } else {
               db_get_link(hDB, hKey, &key);
               if (key.type != TID_KEY)
                  print_key(hDB, hKey, &key, 0, &print_info);
               else
                  for (i = 0;; i++) {
                     if (cm_is_ctrlc_pressed()) {
                        cm_ack_ctrlc_pressed();
                        break;
                     }

                     db_enum_link(hDB, hKey, i, &hSubkey);

                     if (!hSubkey)
                        break;

                     db_get_link(hDB, hSubkey, &key);
                     status = print_key(hDB, hSubkey, &key, 0, &print_info);
                     if (status == 0)
                        break;
                  }
            }
         }
      }

      /* cd */
      else if (param[0][0] == 'c' && param[0][1] == 'd') {
         compose_name(pwd, param[1], str);

         status = db_find_key(hDB, 0, str, &hKey);

         if (strcmp(str, "/") == 0)
            strcpy(pwd, str);
         else if (status == DB_SUCCESS) {
            db_get_key(hDB, hKey, &key);
            if (key.type != TID_KEY)
               printf("key has no subkeys\n");
            else
               db_get_path(hDB, hKey, pwd, 256);
         } else
            printf("key not found\n");
      }

      /* pwd */
      else if (param[0][0] == 'p' && param[0][1] == 'w') {
         printf("%s\n", pwd);
      }

      /* create */
      else if (param[0][0] == 'c' && param[0][1] == 'r') {
         compose_name(pwd, param[2], str);

         /* check if array */

         k = -1;
         if (str[strlen(str) - 1] == ']') {
            if (strchr(str, '[')) {
               j = atoi(strchr(str, '[') + 1);
               strlcpy(str2, strchr(str, '[') + 1, sizeof(str2));
               *strchr(str, '[') = 0;
               if (strchr(str2, '['))
                  k = atoi(strchr(str2, '[')+1);
            }
         } else
            j = 1;

         /* get TID */
         for (i = 0; i < TID_LAST; i++) {
            if (equal_ustring(rpc_tid_name(i), param[1]))
               break;
         }

         if (i == TID_LAST) {
            printf("Unknown type. Must be one of:\n");
            printf("{ BYTE,CHAR,WORD,SHORT,DWORD,INT,BOOL,FLOAT,DOUBLE,STRING }\n");
         } else {
            db_create_key(hDB, 0, str, i);
            db_find_key(hDB, 0, str, &hKey);
            db_get_key(hDB, hKey, &key);

            if (i == TID_STRING) {
               if (!cmd_mode && k == -1) {
                  memset(data, 0, sizeof(data));

                  printf("String length [%d]: ", NAME_LENGTH);
                  ss_gets(str, 256);
                  if (str[0])
                     key.item_size = atoi(str);
                  else
                     key.item_size = NAME_LENGTH;
               } else if (k == -1)
                  key.item_size = NAME_LENGTH;
               else 
                  key.item_size = k;

               db_set_link_data(hDB, hKey, data, key.item_size, 1, key.type);
            }

            if (j > 1) {
               memset(data, 0, sizeof(data));

               if (key.type == TID_LINK)
                  key.item_size = NAME_LENGTH;
               db_set_link_data_index(hDB, hKey, data, key.item_size, j - 1, key.type);
            }
         }
      }

      /* mkdir */
      else if (param[0][0] == 'm' && param[0][1] == 'k') {
         compose_name(pwd, param[1], str);
         db_create_key(hDB, 0, str, TID_KEY);
      }

      /* link */
      else if (param[0][0] == 'l' && param[0][1] == 'n') {
         compose_name(pwd, param[2], str);
         db_create_link(hDB, 0, str, param[1]);
      }

      /* copy */
      else if (param[0][0] == 'c' && (param[0][1] == 'o' || param[0][1] == 'p')) {
         /* test if destination exists */
         compose_name(pwd, param[2], str);
         status = db_find_link(hDB, 0, str, &hKey);
         if (status == DB_SUCCESS) {
            if (cmd_mode)
               str[0] = 'y';
            else {
               printf("Overwrite existing key\n\"%s\"\n(y/[n]) ", str);
               ss_gets(str, 256);
            }
            if (str[0] == 'y')
               db_delete_key(hDB, hKey, FALSE);
         }

         if (status == DB_NO_KEY || str[0] == 'y') {
            compose_name(pwd, param[1], str);

            status = db_find_link(hDB, 0, str, &hKey);
            if (status == DB_SUCCESS) {
               compose_name(pwd, param[2], str);

               db_get_key(hDB, hKey, &key);
               db_create_key(hDB, 0, str, key.type);

               if (key.type != TID_KEY) {
                  size = sizeof(data);
                  db_get_data(hDB, hKey, data, &size, key.type);
                  db_find_key(hDB, 0, str, &hKey);
                  db_set_link_data(hDB, hKey, data, size, key.num_values, key.type);
               } else {
                  size = sizeof(data);
                  db_copy(hDB, hKey, data, &size, "");
                  db_find_key(hDB, 0, str, &hKey);
                  db_paste(hDB, hKey, data);
               }
            } else
               printf("key not found\n");
         }
      }

      /* delete */
      else if ((param[0][0] == 'd' && param[0][1] == 'e') ||
               (param[0][0] == 'r' && param[0][1] == 'm')) {
         flags = 0;
         if ((param[1][0] == '-' && param[1][1] == 'f') ||
             (param[2][0] == '-' && param[2][1] == 'f'))
            flags |= (1 << 0);
         if ((param[1][0] == '-' && param[1][1] == 'l') ||
             (param[2][0] == '-' && param[2][1] == 'l'))
            flags |= (1 << 1);

         for (i = 1; param[i][0] == '-'; i++);

         compose_name(pwd, param[i], str);

         status = db_find_link(hDB, 0, str, &hKey);
         db_get_key(hDB, hKey, &key);

         if (status == DB_SUCCESS) {
            if (flags & (1 << 0) || cmd_mode)
               str[0] = 'y';
            else {
               if (key.type == TID_KEY)
                  printf
                      ("Are you sure to delete the key\n\"%s\"\nand all its subkeys? (y/[n]) ",
                       str);
               else
                  printf("Are you sure to delete the key\n\"%s\"\n(y/[n]) ", str);

               ss_gets(str, 256);
            }

            if (str[0] == 'y') {
               status = db_delete_key(hDB, hKey, (flags & (1 << 1)) > 0);
               if (status == DB_NO_ACCESS)
                  printf("deletion of key not allowed\n");
               if (status == DB_OPEN_RECORD)
                  printf("key is open by other client\n");
            }
         } else
            printf("key not found\n");
      }

      /* set */
      else if (param[0][0] == 's' && param[0][1] == 'e') {
         /* check if index is supplied */
         index1 = index2 = 0;
         strcpy(str, param[1]);
         if (str[strlen(str) - 1] == ']') {
            if (strchr(str, '[')) {
               if (*(strchr(str, '[') + 1) == '*')
                  index1 = -1;
               else if (strchr((strchr(str, '[') + 1), '.') || 
                        strchr((strchr(str, '[') + 1), '-')) {
                  index1 = atoi(strchr(str, '[') + 1);
                  pc = strchr(str, '[') + 1;
                  while (*pc != '.' && *pc != '-')
                     pc++;
                  while (*pc == '.' || *pc == '-')
                     pc++;
                  index2 = atoi(pc);
               } else
                  index1 = atoi(strchr(str, '[') + 1);
            }
            *strchr(str, '[') = 0;
         }

         if (strpbrk(str, "*?") != NULL) {
            db_find_key(hDB, 0, pwd, &hKey);
            for (i = 0;; i++) {
               db_enum_link(hDB, hKey, i, &hSubkey);

               if (!hSubkey)
                  break;

               db_get_key(hDB, hSubkey, &key);
               if (match(str, key.name))
                  set_key(hDB, hSubkey, index1, index2, param[2]);
            }
         } else {
            compose_name(pwd, str, name);

            status = db_find_key(hDB, 0, name, &hKey);
            if (status == DB_SUCCESS)
               set_key(hDB, hKey, index1, index2, param[2]);
            else {
               printf("Error: Key \"%s\" not found\n", name);
               if (cmd_mode)
                  return -1;
            }
         }
      }

      /* set mode */
      else if (param[0][0] == 'c' && param[0][1] == 'h' && param[0][2] == 'm') {
         compose_name(pwd, param[2], str);

         mode = atoi(param[1]);

         if (strcmp(str, "/") != 0)
            status = db_find_key(hDB, 0, str, &hKey);
         else
            hKey = 0;

         if (status == DB_SUCCESS || !hKey) {
            if (cmd_mode)
               str[0] = 'y';
            else {
               printf
                   ("Are you sure to change the mode of key\n  %s\nand all its subkeys\n",
                    str);
               printf("to mode [%c%c%c%c]? (y/[n]) ", mode & MODE_READ ? 'R' : 0,
                      mode & MODE_WRITE ? 'W' : 0, mode & MODE_DELETE ? 'D' : 0,
                      mode & MODE_EXCLUSIVE ? 'E' : 0);
               ss_gets(str, 256);
            }
            if (str[0] == 'y')
               db_set_mode(hDB, hKey, mode, TRUE);
         } else {
            printf("Error: Key \"%s\" not found\n", str);
            if (cmd_mode)
               return -1;
         }
      }

      /* truncate */
      else if (param[0][0] == 't' && param[0][1] == 'r') {
         compose_name(pwd, param[1], str);

         status = db_find_key(hDB, 0, str, &hKey);

         i = atoi(param[2]);
         if (i == 0)
            i = 1;

         if (status == DB_SUCCESS)
            db_set_num_values(hDB, hKey, i);
         else {
            printf("Error: Key \"%s\" not found\n", str);
            if (cmd_mode)
               return -1;
         }
      }

      /* rename */
      else if (param[0][0] == 'r' && param[0][1] == 'e' && param[0][2] == 'n') {
         compose_name(pwd, param[1], str);

         if (strcmp(str, "/") != 0)
            status = db_find_link(hDB, 0, str, &hKey);
         else
            hKey = 0;

         if (status == DB_SUCCESS || !hKey)
            db_rename_key(hDB, hKey, param[2]);
         else {
            printf("Error: Key \"%s\" not found\n", str);
            if (cmd_mode)
               return -1;
         }
      }

      /* move */
      else if (param[0][0] == 'm' && param[0][1] == 'o') {
         compose_name(pwd, param[1], str);

         if (strcmp(str, "/") != 0)
            status = db_find_link(hDB, 0, str, &hKey);
         else
            hKey = 0;

         if (status == DB_SUCCESS || !hKey) {
            if (param[2][0] == 't')
               i = 0;
            else if (param[2][0] == 'b')
               i = -1;
            else
               i = atoi(param[2]);

            status = db_reorder_key(hDB, hKey, i);
            if (status == DB_NO_ACCESS)
               printf("no write access to key\n");
            if (status == DB_OPEN_RECORD)
               printf("key is open by other client\n");
         } else {
            printf("Error: Key \"%s\" not found\n", str);
            if (cmd_mode)
               return -1;
         }
      }

      /* find key */
      else if (param[0][0] == 'f' && param[0][1] == 'i') {
         status = db_find_key(hDB, 0, pwd, &hKey);

         if (status == DB_SUCCESS)
            db_scan_tree(hDB, hKey, 0, search_key, (void *) param[1]);
         else
            printf("current key is invalid / no read access\n");
      }

      /* load */
      else if (param[0][0] == 'l' && param[0][1] == 'o') {
         db_find_key(hDB, 0, pwd, &hKey);

         /* keep /Logger/Data dir if new one doesn't exist */
         cm_get_path(old_dir);
         size = sizeof(old_dir);
         db_get_value(hDB, 0, "/Logger/Data dir", old_dir, &size, TID_STRING, TRUE);

         db_load(hDB, hKey, param[1], FALSE);

         str[0] = 0;
         size = sizeof(str);
         db_get_value(hDB, 0, "/Logger/Data dir", str, &size, TID_STRING, FALSE);

         /* check if new one exists */
         getcwd(cwd, sizeof(cwd));
         status = chdir(str);
         if (status == -1 && strcmp(old_dir, str) != 0) {
            printf
                ("\"/Logger/Data dir = %s\" in file \"%s\" does not exist locally,\nset anyhow? (y/[n]): ",
                 str, param[1]);
            ss_gets(line, 256);

            /* restor old one */
            if (line[0] != 'y' && line[0] != 'Y')
               db_set_value(hDB, 0, "/Logger/Data dir", old_dir, sizeof(old_dir), 1,
                            TID_STRING);
         }

         chdir(cwd);
      }

      /* save */
      else if (param[0][0] == 's' && param[0][1] == 'a') {
         db_find_key(hDB, 0, pwd, &hKey);

         if (strstr(param[1], ".xml") || strstr(param[1], ".XML"))
            db_save_xml(hDB, hKey, param[1]);
         else if (strstr(param[1], ".json") || strstr(param[1], ".js"))
            db_save_json(hDB, hKey, param[1]);
         else if (param[1][0] == '-') {
            if (param[1][1] == 'c' && param[1][2] == 's') {
               db_save_struct(hDB, hKey, param[2], NULL, FALSE);
               db_save_string(hDB, hKey, param[2], NULL, TRUE);
            }
            else if (param[1][1] == 'c')
               db_save_struct(hDB, hKey, param[2], NULL, FALSE);
            else if (param[1][1] == 's')
               db_save_string(hDB, hKey, param[2], NULL, FALSE);
            else if (param[1][1] == 'x')
               db_save_xml(hDB, hKey, param[2]);
            else if (param[1][1] == 'j')
               db_save_json(hDB, hKey, param[2]);
         } else
            db_save(hDB, hKey, param[1], FALSE);
      }

      /* make */
      else if (param[0][0] == 'm' && param[0][1] == 'a') {
         if (param[1][0])
            create_experim_h(hDB, param[1]);
         else
            create_experim_h(hDB, "Analyzer");
      }

      /* passwd */
      else if (param[0][0] == 'p' && param[0][1] == 'a' && param[0][2] == 's') {
         /*
            strcpy(str, ss_crypt("foob", "ar"));
            if(strcmp(str, "arlEKn0OzVJn.") != 0)
            printf("Warning: ss_crypt() works incorrect");
          */

         if (db_find_key(hDB, 0, "/Experiment/Security/Password", &hKey) == DB_SUCCESS) {
            size = sizeof(old_password);
            db_get_data(hDB, hKey, old_password, &size, TID_STRING);

            strcpy(str, ss_getpass("Old password: "));
            strcpy(str, ss_crypt(str, "mi"));

            if (strcmp(str, old_password) == 0 || strcmp(str, "mid7qBxsNMHux") == 0) {
               strcpy(str, ss_getpass("New password: "));
               strcpy(new_password, ss_crypt(str, "mi"));

               strcpy(str, ss_getpass("Retype new password: "));
               if (strcmp(new_password, ss_crypt(str, "mi")) != 0)
                  printf("Mismatch - password unchanged\n");
               else
                  db_set_data(hDB, hKey, new_password, 32, 1, TID_STRING);
            } else
               printf("Wrong password\n");
         } else {
            strcpy(str, ss_getpass("Password: "));
            strcpy(new_password, ss_crypt(str, "mi"));

            strcpy(str, ss_getpass("Retype password: "));
            if (strcmp(new_password, ss_crypt(str, "mi")) != 0)
               printf("Mismatch - password not set\n");
            else {
               /* set password */
               db_set_value(hDB, 0, "/Experiment/Security/Password", new_password, 32, 1,
                            TID_STRING);

               /* create empty allowed hosts and allowd programs entries */
               db_create_key(hDB, 0,
                             "/Experiment/Security/Allowed hosts/host.sample.domain",
                             TID_INT);
               db_create_key(hDB, 0, "/Experiment/Security/Allowed programs/mstat",
                             TID_INT);
            }
         }

      }

      /* webpasswd */
      else if (param[0][0] == 'w' && param[0][1] == 'e' && param[0][2] == 'b') {
         if (db_find_key(hDB, 0, "/Experiment/Security/Web Password", &hKey) ==
             DB_SUCCESS) {
            size = sizeof(old_password);
            db_get_data(hDB, hKey, old_password, &size, TID_STRING);

            strcpy(str, ss_getpass("Old password: "));
            strcpy(str, ss_crypt(str, "mi"));

            if (strcmp(str, old_password) == 0 || strcmp(str, "mid7qBxsNMHux") == 0) {
               strcpy(str, ss_getpass("New password: "));
               strcpy(new_password, ss_crypt(str, "mi"));

               strcpy(str, ss_getpass("Retype new password: "));
               if (strcmp(new_password, ss_crypt(str, "mi")) != 0)
                  printf("Mismatch - password unchanged\n");
               else
                  db_set_data(hDB, hKey, new_password, 32, 1, TID_STRING);
            } else
               printf("Wrong password\n");
         } else {
            strcpy(str, ss_getpass("Password: "));
            strcpy(new_password, ss_crypt(str, "mi"));

            strcpy(str, ss_getpass("Retype password: "));
            if (strcmp(new_password, ss_crypt(str, "mi")) != 0)
               printf("Mismatch - password not set\n");
            else
               /* set password */
               db_set_value(hDB, 0, "/Experiment/Security/Web Password", new_password, 32,
                            1, TID_STRING);
         }
      }

      /* hi */
      else if (param[0][0] == 'h' && param[0][1] == 'i') {
         HNDLE hConn;

         client_name[0] = 0;

         if (!isalpha(param[1][0])) {
            /* find client which exports RPC_ANA_CLEAR_HISTOS */
            status = db_find_key(hDB, 0, "System/Clients", &hRootKey);
            if (status == DB_SUCCESS) {
               for (i = 0;; i++) {
                  status = db_enum_key(hDB, hRootKey, i, &hSubkey);
                  if (status == DB_NO_MORE_SUBKEYS) {
                     printf
                         ("No client currently exports the CLEAR HISTO functionality.\n");
                     break;
                  }

                  sprintf(str, "RPC/%d", RPC_ANA_CLEAR_HISTOS);
                  status = db_find_key(hDB, hSubkey, str, &hKey);
                  if (status == DB_SUCCESS) {
                     size = sizeof(client_name);
                     db_get_value(hDB, hSubkey, "Name", client_name, &size, TID_STRING,
                                  TRUE);
                     break;
                  }
               }
            }

            if (isdigit(param[1][0]))
               n1 = atoi(param[1]);
            else
               n1 = 0;          /* all histos by default */

            if (isdigit(param[2][0]))
               n2 = atoi(param[2]);
            else
               n2 = n1;         /* single histo by default */
         } else {
            strcpy(client_name, param[1]);

            if (isdigit(param[2][0]))
               n1 = atoi(param[2]);
            else
               n1 = 0;          /* all histos by default */

            if (isdigit(param[3][0]))
               n2 = atoi(param[3]);
            else
               n2 = n1;         /* single histo by default */
         }

         if (client_name[0]) {
            if (cm_connect_client(client_name, &hConn) == CM_SUCCESS) {
               rpc_client_call(hConn, RPC_ANA_CLEAR_HISTOS, n1, n2);
               cm_disconnect_client(hConn, FALSE);
            } else
               printf("Cannot connect to client %s\n", client_name);
         }
      }

      /* import */
      else if (param[0][0] == 'i' && param[0][1] == 'm') {
         int fh;

         fh = open(param[1], O_RDONLY | O_TEXT);
         if (fh < 0)
            printf("File %s not found\n", param[1]);
         else {
            size = lseek(fh, 0, SEEK_END);
            lseek(fh, 0, SEEK_SET);
            size = read(fh, data, size);
            data[size++] = 0;
            close(fh);

            if (param[2][0] == 0) {
               printf("Key name: ");
               ss_gets(name, 256);
            } else
               strcpy(name, param[2]);

            compose_name(pwd, name, str);

            db_create_key(hDB, 0, str, TID_STRING);
            db_find_key(hDB, 0, str, &hKey);
            db_set_data(hDB, hKey, data, size, 1, TID_STRING);
         }

      }

      /* export */
      else if (param[0][0] == 'e' && param[0][1] == 'x' && param[0][2] == 'p') {
         FILE *f;

         if (param[1][0] == 0)
            printf("please specify key\n");
         else {
            compose_name(pwd, param[1], str);

            db_find_key(hDB, 0, str, &hKey);
            if (hKey == 0)
               printf("Error: Key \"%s\" not found\n", param[1]);
            else {
               if (param[2][0] == 0) {
                  printf("File name: ");
                  ss_gets(name, 256);
               } else
                  strcpy(name, param[2]);

               f = fopen(name, "w");
               if (f == NULL)
                  printf("Cannot open file \"%s\"\n", name);
               else {
                  db_get_key(hDB, hKey, &key);
                  if (key.type != TID_STRING)
                     printf("Only export of STRING key possible\n");
                  else {
                     size = sizeof(data);
                     memset(data, 0, size);
                     db_get_data(hDB, hKey, data, &size, key.type);

                     fprintf(f, "%s", data);
                     fclose(f);
                  }
               }
            }
         }
      }

      /* rewind */
      else if (param[0][0] == 'r' && param[0][1] == 'e' && param[0][2] == 'w') {
         HNDLE hConn;

         /* check if run is stopped and no transition in progress */
         size = sizeof(i);
         db_get_value(hDB, 0, "/Runinfo/State", &i, &size, TID_INT, TRUE);
         db_get_value(hDB, 0, "/Runinfo/Transition in progress", &j, &size, TID_INT,
                      TRUE);
         if (i != STATE_STOPPED || j == 1) {
            printf
                ("Cannot rewind tapes when run is not stopped or transition in progress\n");
         } else {
            if (param[1][0] == 0)
               channel = -1;
            else
               channel = atoi(param[1]);

            /* find client which exports RPC_LOG_REWIND */
            status = db_find_key(hDB, 0, "System/Clients", &hRootKey);
            if (status == DB_SUCCESS) {
               for (i = 0;; i++) {
                  status = db_enum_key(hDB, hRootKey, i, &hSubkey);
                  if (status == DB_NO_MORE_SUBKEYS) {
                     printf("No active logger found.\n");
                     break;
                  }

                  sprintf(str, "RPC/%d", RPC_LOG_REWIND);
                  status = db_find_key(hDB, hSubkey, str, &hKey);
                  if (status == DB_SUCCESS) {
                     size = sizeof(client_name);
                     db_get_value(hDB, hSubkey, "Name", client_name, &size, TID_STRING,
                                  TRUE);
                     break;
                  }
               }
            }

            if (status != DB_NO_MORE_SUBKEYS) {
               if (cm_connect_client(client_name, &hConn) == CM_SUCCESS) {
                  /* increase timeout */
                  rpc_set_option(hConn, RPC_OTIMEOUT, 600000);

                  if (channel == -1)
                     printf("Rewinding all channels...\n");
                  else
                     printf("Rewinding channel %d...\n", channel);

                  status = rpc_client_call(hConn, RPC_LOG_REWIND, channel);
                  cm_disconnect_client(hConn, FALSE);

                  if (status == CM_SUCCESS)
                     printf("Rewind successful\n");
                  else
                     printf("Rewind error, code = %d\n", status);
               } else
                  printf("Cannot connect to logger\n");
            }
         }
      }

      /* alarm reset */
      else if (param[0][0] == 'a' && param[0][1] == 'l') {
         /* go through all alarms */
         db_find_key(hDB, 0, "/Alarms/Alarms", &hKey);
         if (hKey) {
            for (i = 0;; i++) {
               db_enum_link(hDB, hKey, i, &hSubkey);

               if (!hSubkey)
                  break;

               db_get_key(hDB, hSubkey, &key);
               status = al_reset_alarm(key.name);
               if (status == AL_RESET)
                  printf("Alarm of class \"%s\" reset sucessfully\n", key.name);
            }
         }
      }

      /* mem */
      else if (param[0][0] == 'm' && param[0][1] == 'e') {
         if (rpc_is_remote())
            printf("This function works only locally\n");
         else {
            db_show_mem(hDB, data, sizeof(data), param[1][0]);
            puts(data);
         }
      }

      /* sor (show open records) */
      else if (param[0][0] == 's' && param[0][1] == 'o') {
         db_find_key(hDB, 0, pwd, &hKey);
         db_get_open_records(hDB, hKey, data, sizeof(data), FALSE);
         printf("%s", data);
      }

      /* scl (show clients ) */
      else if (param[0][0] == 's' && param[0][1] == 'c') {
         status = db_find_key(hDB, 0, "System/Clients", &hKey);
         if (status != DB_SUCCESS)
            cm_msg(MERROR, "command_loop",
                   "cannot find System/Clients entry in database");
         else {
            if (param[1][1] == 'w')
               printf("Name                Host                Timeout    Last called\n");
            else
               printf("Name                Host\n");

            /* search database for clients with transition mask set */
            for (i = 0, status = 0;; i++) {
               status = db_enum_key(hDB, hKey, i, &hSubkey);
               if (status == DB_NO_MORE_SUBKEYS)
                  break;

               if (status == DB_SUCCESS) {
                  size = sizeof(name);
                  db_get_value(hDB, hSubkey, "Name", name, &size, TID_STRING, TRUE);
                  printf("%s", name);
                  for (j = 0; j < 20 - (int) strlen(name); j++)
                     printf(" ");

                  size = sizeof(str);
                  db_get_value(hDB, hSubkey, "Host", str, &size, TID_STRING, TRUE);
                  printf("%s", str);
                  for (j = 0; j < 20 - (int) strlen(str); j++)
                     printf(" ");

                  /* display optional watchdog info */
                  if (param[1][1] == 'w') {
                     DWORD timeout, last;

                     status = cm_get_watchdog_info(hDB, name, &timeout, &last);
                     printf("%-10d %-10d", timeout, last);
                  }

                  printf("\n");
               }
            }
         }
      }

      /* start */
      else if (param[0][0] == 's' && param[0][1] == 't' && param[0][2] == 'a') {
         debug_flag = ((param[1][0] == '-' && param[1][1] == 'v') ||
                       (param[2][0] == '-' && param[2][1] == 'v'));

         /* check if run is already started */
         size = sizeof(i);
         i = STATE_STOPPED;
         db_get_value(hDB, 0, "/Runinfo/State", &i, &size, TID_INT, TRUE);
         if (i == STATE_RUNNING) {
            printf("Run is already started\n");
         } else if (i == STATE_PAUSED) {
            printf("Run is paused, please use \"resume\"\n");
         } else {
            /* get present run number */
            old_run_number = 0;
            status =
                db_get_value(hDB, 0, "/Runinfo/Run number", &old_run_number, &size,
                             TID_INT, TRUE);
            assert(status == SUCCESS);
            assert(old_run_number >= 0);

            /* edit run parameter if command is not "start now" */
            if ((param[1][0] == 'n' && param[1][1] == 'o' && param[1][2] == 'w') ||
                cmd_mode) {
               new_run_number = old_run_number + 1;
               line[0] = 'y';
            } else {
               db_find_key(hDB, 0, "/Experiment/Edit on start", &hKey);
               do {
                  if (hKey) {
                     for (i = 0;; i++) {
                        db_enum_link(hDB, hKey, i, &hSubkey);

                        if (!hSubkey)
                           break;

                        db_get_key(hDB, hSubkey, &key);
                        strcpy(str, key.name);

                        if (equal_ustring(str, "Edit run number"))
                           continue;

                        db_enum_key(hDB, hKey, i, &hSubkey);
                        db_get_key(hDB, hSubkey, &key);

                        size = sizeof(data);
                        status = db_get_data(hDB, hSubkey, data, &size, key.type);
                        if (status != DB_SUCCESS)
                           continue;

                        for (j = 0; j < key.num_values; j++) {
                           db_sprintf(data_str, data, key.item_size, j, key.type);
                           sprintf(prompt, "%s : ", str);

                           strcpy(line, data_str);
                           in_cmd_edit = TRUE;
                           cmd_edit(prompt, line, NULL, cmd_idle);
                           in_cmd_edit = FALSE;

                           if (line[0]) {
                              db_sscanf(line, data, &size, j, key.type);
                              db_set_data_index(hDB, hSubkey, data, key.item_size, j,
                                                key.type);
                           }
                        }
                     }
                  }

                  /* increment run number */
                  new_run_number = old_run_number + 1;
                  printf("Run number [%d]: ", new_run_number);
                  ss_gets(line, 256);
                  if (line[0] && atoi(line) > 0)
                     new_run_number = atoi(line);

                  printf("Are the above parameters correct? ([y]/n/q): ");
                  ss_gets(line, 256);

               } while (line[0] == 'n' || line[0] == 'N');
            }

            if (line[0] != 'q' && line[0] != 'Q') {
               /* start run */
               printf("Starting run #%d\n", new_run_number);

               assert(new_run_number > 0);

               status =
                   cm_transition(TR_START, new_run_number, str, sizeof(str), SYNC,
                                 debug_flag);
               if (status != CM_SUCCESS) {
                  /* in case of error, reset run number */
                  status =
                      db_set_value(hDB, 0, "/Runinfo/Run number", &old_run_number,
                                   sizeof(old_run_number), 1, TID_INT);
                  assert(status == SUCCESS);

                  printf("Error: %s\n", str);
               }
            }
         }
      }

      /* stop */
      else if (param[0][0] == 's' && param[0][1] == 't' && param[0][2] == 'o') {
         debug_flag = ((param[1][0] == '-' && param[1][1] == 'v') ||
                       (param[2][0] == '-' && param[2][1] == 'v'));

         /* check if run is stopped */
         state = STATE_STOPPED;
         size = sizeof(i);
         db_get_value(hDB, 0, "/Runinfo/State", &state, &size, TID_INT, TRUE);
         str[0] = 0;
         if (state == STATE_STOPPED && !cmd_mode) {
            printf("Run is already stopped. Stop again? (y/[n]) ");
            ss_gets(str, 256);
         }
         if (str[0] == 'y' || state != STATE_STOPPED || cmd_mode) {
            if (param[1][0] == 'n')
               status =
                   cm_transition(TR_STOP | TR_DEFERRED, 0, str, sizeof(str), SYNC,
                                 debug_flag);
            else
               status = cm_transition(TR_STOP, 0, str, sizeof(str), SYNC, debug_flag);

            if (status == CM_DEFERRED_TRANSITION)
               printf("%s\n", str);
            else if (status == CM_TRANSITION_IN_PROGRESS)
               printf
                   ("Deferred stop already in progress, enter \"stop now\" to force stop\n");
            else if (status != CM_SUCCESS)
               printf("Error: %s\n", str);
         }
      }

      /* pause */
      else if (param[0][0] == 'p' && param[0][1] == 'a' && param[0][2] == 'u') {
         debug_flag = ((param[1][0] == '-' && param[1][1] == 'v') ||
                       (param[2][0] == '-' && param[2][1] == 'v'));

         /* check if run is started */
         i = STATE_STOPPED;
         size = sizeof(i);
         db_get_value(hDB, 0, "/Runinfo/State", &i, &size, TID_INT, TRUE);
         if (i != STATE_RUNNING) {
            printf("Run is not started\n");
         } else {
            if (param[1][0] == 'n')
               status =
                   cm_transition(TR_PAUSE | TR_DEFERRED, 0, str, sizeof(str), SYNC,
                                 debug_flag);
            else
               status = cm_transition(TR_PAUSE, 0, str, sizeof(str), SYNC, debug_flag);

            if (status == CM_DEFERRED_TRANSITION)
               printf("%s\n", str);
            else if (status == CM_TRANSITION_IN_PROGRESS)
               printf
                   ("Deferred pause already in progress, enter \"pause now\" to force pause\n");
            else if (status != CM_SUCCESS)
               printf("Error: %s\n", str);
         }
      }

      /* resume */
      else if (param[0][0] == 'r' && param[0][1] == 'e' && param[0][2] == 's') {
         debug_flag = ((param[1][0] == '-' && param[1][1] == 'v') ||
                       (param[2][0] == '-' && param[2][1] == 'v'));

         /* check if run is paused */
         i = STATE_STOPPED;
         size = sizeof(i);
         db_get_value(hDB, 0, "/Runinfo/State", &i, &size, TID_INT, TRUE);
         if (i != STATE_PAUSED) {
            printf("Run is not paused\n");
         } else {
            status = cm_transition(TR_RESUME, 0, str, sizeof(str), SYNC, debug_flag);
            if (status != CM_SUCCESS)
               printf("Error: %s\n", str);
         }
      }

      /* msg */
      else if (param[0][0] == 'm' && param[0][1] == 's') {
         /*
            if (cm_exist("Speaker", FALSE) != CM_SUCCESS)
            printf("Note: No speaker application present.\n");
          */

         /* user message type by default */
         msg_type = MT_USER;
         message[0] = 0;
         if (cmd_mode)
            strcpy(user_name, "script");

         if (param[3][0]) {
            msg_type = atoi(param[1]);
            last_msg_time = ss_time();
            strcpy(user_name, param[2]);
            strcpy(message, param[3]);
         } else if (param[2][0]) {
            last_msg_time = ss_time();
            strcpy(user_name, param[1]);
            strcpy(message, param[2]);
         } else if (param[1][0])
            strlcpy(message, param[1], sizeof(message));

         if (!cmd_mode) {
            if (ss_time() - last_msg_time > 300) {
               printf("Your name> ");
               ss_gets(user_name, 80);
            }

            if (param[1][0] == 0) {
               printf("Msg> ");
               ss_gets(message, 256);
            }
         }

         if (message[0])
            cm_msg(msg_type, __FILE__, __LINE__, user_name, message);

         last_msg_time = ss_time();
      }

      /* chat */
      else if (param[0][0] == 'c' && param[0][1] == 'h' && param[0][2] == 'a') {
         message[0] = 0;

         if (ss_time() - last_msg_time > 300) {
            printf("Your name> ");
            ss_gets(user_name, 80);
         }

         printf("Exit chat mode with empty line.\n");
         do {
            in_cmd_edit = TRUE;
            message[0] = 0;
            cmd_edit("> ", message, NULL, cmd_idle);
            in_cmd_edit = FALSE;

            if (message[0])
               cm_msg(MUSER, user_name, message);

         } while (message[0]);

         last_msg_time = ss_time();
      }

      /* old */
      else if (param[0][0] == 'o' && param[0][1] == 'l') {
         i = 20;
         if (param[1][0])
            i = atoi(param[1]);

         cm_msg_retrieve(i, data, sizeof(data));
         printf("%s\n\n", data);
      }

      /* cleanup */
      else if (param[0][0] == 'c' && param[0][1] == 'l') {
         HNDLE hBuf;
         BOOL force;

         force = FALSE;
         if (param[1][0] == '-' && param[1][1] == 'f')
            force = TRUE;
         if (param[2][0] == '-' && param[2][1] == 'f')
            force = TRUE;

         bm_open_buffer(EVENT_BUFFER_NAME, 2*MAX_EVENT_SIZE, &hBuf);

         if (param[1][0] && param[1][0] != '-')
            cm_cleanup(param[1], force);
         else
            cm_cleanup("", force);
         bm_close_buffer(hBuf);
      }

      /* shutdown */
      else if (param[0][0] == 's' && param[0][1] == 'h') {
         if (param[1][0] == 0)
            printf("Please enter client name or \"all\" to shutdown all clients.\n");
         else {
            status = cm_shutdown(param[1], TRUE);
            if (status != CM_SUCCESS) {
               if (strcmp(param[1], "all") == 0)
                  printf("No clients found\n");
               else
                  printf("Client %s not active\n", param[1]);
            }
         }
      }

      /* ver */
      else if (param[0][0] == 'v' && param[0][1] == 'e') {
         printf("MIDAS version %s\n", cm_get_version());
         printf("SVN revision %d\n", cm_get_revision());
         printf("ODB version %d\n", DATABASE_VERSION);
         printf("Maximum event size %1.1lf MB\n", MAX_EVENT_SIZE/1024.0/1024.0);
      }

      /* exec */
      else if (param[0][0] == 'e' && param[0][1] == 'x' && param[0][2] == 'e') {
         compose_name(pwd, param[1], str);

         status = db_find_key(hDB, 0, str, &hKey);
         if (status == DB_SUCCESS) {
            db_get_key(hDB, hKey, &key);
            if (key.type != TID_STRING) {
               printf("Key contains no command\n");
               continue;
            }
            size = sizeof(str);
            db_get_data(hDB, hKey, str, &size, key.type);

            cm_execute(str, data, sizeof(data));
         } else {
            cm_execute(param[1], data, sizeof(data));
         }
         puts(data);
      }

      /* wait */
      else if (param[0][0] == 'w' && param[0][1] == 'a') {
         compose_name(pwd, param[1], str);

         if (strcmp(str, "/") != 0)
            status = db_find_link(hDB, 0, str, &hKey);
         else
            hKey = 0;

         if (status == DB_SUCCESS || !hKey) {
            db_get_key(hDB, hKey, &key);
            printf("Waiting for key \"%s\" to be modified, abort with \"!\"\n", key.name);
            db_get_record_size(hDB, hKey, 0, &size);
            db_open_record(hDB, hKey, data, size, MODE_READ, key_update, NULL);
            key_modified = FALSE;

            i = 0;
            do {
               cm_yield(1000);

               while (ss_kbhit()) {
                  i = ss_getchar(0);
                  if (i == -1)
                     i = getchar();

                  if ((char) i == '!')
                     break;
               }

            } while (!key_modified && i != '!');

            db_close_record(hDB, hKey);
            if (i == '!')
               printf("Wait aborted.\n");
            else
               printf("Key has been modified.\n");
         } else
            printf("key not found\n");
      }

      /* test 1  */
      else if (param[0][0] == 't' && param[0][1] == '1') {
         DWORD start_time;
         INT i, size, rn;
         HNDLE hKey;

         start_time = ss_millitime();
         status = db_find_key(hDB, 0, "/runinfo/run number", &hKey);
         assert(status == SUCCESS);
         size = sizeof(rn);

         i = 0;
         do {
            db_get_data(hDB, hKey, &rn, &size, TID_INT);
            db_get_data(hDB, hKey, &rn, &size, TID_INT);
            db_get_data(hDB, hKey, &rn, &size, TID_INT);
            db_get_data(hDB, hKey, &rn, &size, TID_INT);
            db_get_data(hDB, hKey, &rn, &size, TID_INT);
            db_get_data(hDB, hKey, &rn, &size, TID_INT);
            db_get_data(hDB, hKey, &rn, &size, TID_INT);
            db_get_data(hDB, hKey, &rn, &size, TID_INT);
            db_get_data(hDB, hKey, &rn, &size, TID_INT);
            db_get_data(hDB, hKey, &rn, &size, TID_INT);
            i += 10;
         } while (ss_millitime() - start_time < 5000);

         printf("%d accesses per second\n", i / 5);
      }

      /* test 2 */
      else if (param[0][0] == 't' && param[0][1] == '2') {
         do {
            do {
               i = ss_getchar(0);
               printf("%d\n", i);
            } while (i > 0 && i != 4);

            ss_sleep(1000);
         } while (i != 4);

         ss_getchar(1);
      }

      /* test 3 */
      else if (param[0][0] == 't' && param[0][1] == '3') {
         ss_mutex_create(&tm);
         ss_thread_create(thread, NULL);
         ss_thread_create(thread, NULL);
         ss_thread_create(thread, NULL);
      }

      /* exit/quit */
      else if ((param[0][0] == 'e' && param[0][1] == 'x' && param[0][2] == 'i') ||
               (param[0][0] == 'q'))
         break;

      else if (param[0][0] == 0);

      else
         printf("Unknown command %s %s %s\n", param[0], param[1], param[2]);

      /* exit after single command */
      if (cmd_mode && cmd[0] != '@')
         break;

      /* check if client connections are broken */
      status = cm_yield(0);
      if (status == SS_ABORT)
         break;

   } while (TRUE);

   /* check if client connections are broken */
   for (i = 0; i < MAX_RPC_CONNECTION; i++)
      cm_yield(0);

   return 1; /* indicate success */
}

/*------------------------------------------------------------------*/

void ctrlc_odbedit(INT i)
{
   /* reset terminal */
   ss_getchar(1);

   /* no shutdown message */
   cm_set_msg_print(MT_ERROR, 0, NULL);

   cm_disconnect_experiment();

   exit(EXIT_SUCCESS);
}

/*------------------------------------------------------------------*/

#ifdef OS_VXWORKS
int odbedit(char *ahost_name, char *aexp_name)
#else
int main(int argc, char *argv[])
#endif
{
   INT status, i, odb_size, size;
   char host_name[HOST_NAME_LENGTH], exp_name[NAME_LENGTH];
   char cmd[2000], dir[256], str[2000];
   BOOL debug;
   BOOL corrupted;
   BOOL reload_from_file = FALSE;
   HNDLE hDB;

   cmd[0] = dir[0] = 0;
   odb_size = DEFAULT_ODB_SIZE;
   debug = corrupted = cmd_mode = FALSE;

#ifdef OS_VXWORKS
   strcpy(host_name, ahost_name);
   strcpy(exp_name, aexp_name);
#else

   /* get default from environment */
   cm_get_environment(host_name, sizeof(host_name), exp_name, sizeof(exp_name));

   /* check for MIDASSYS */
   if (!getenv("MIDASSYS")) {
      puts("Please define environment variable 'MIDASSYS'");
      puts("pointing to the midas source installation directory.");
   }

   /* parse command line parameters */
   for (i = 1; i < argc; i++) {
      if (argv[i][0] == '-' && argv[i][1] == 'g')
         debug = TRUE;
      else if (argv[i][0] == '-' && argv[i][1] == 'R')
         reload_from_file = TRUE;
      else if (argv[i][0] == '-' && argv[i][1] == 'C')
         corrupted = TRUE;
      else if (argv[i][0] == '-') {
         if (i + 1 >= argc || argv[i + 1][0] == '-')
            goto usage;
         if (argv[i][1] == 'e')
            strcpy(exp_name, argv[++i]);
         else if (argv[i][1] == 'h')
            strcpy(host_name, argv[++i]);
         else if (argv[i][1] == 'c') {
            if (strlen(argv[i+1]) >= sizeof(cmd)) {
               printf("error: command line too long (>%d bytes)\n", (int)sizeof(cmd));
               return 0;
            }
            strlcpy(cmd, argv[++i], sizeof(cmd));
            cmd_mode = TRUE;
         } else if (argv[i][1] == 'd')
            strlcpy(dir, argv[++i], sizeof(dir));
         else if (argv[i][1] == 's')
            odb_size = atoi(argv[++i]);
         else {
          usage:
            printf("usage: odbedit [-h Hostname] [-e Experiment] [-d ODB Subtree]\n");
            printf("               [-c Command] [-c @CommandFile] [-s size]\n");
            printf("               [-g (debug)] [-C (connect to corrupted ODB)] [-R (reload ODB from .ODB.SHM)]\n\n");
            printf("For a list of valid commands start odbedit interactively\n");
            printf("and type \"help\".\n");
            return 0;
         }
      } else
         strlcpy(host_name, argv[i], sizeof(host_name));
   }
#endif

   /* no startup message if called with command */
   if (cmd[0])
      cm_set_msg_print(MT_ERROR, 0, NULL);

   status = cm_connect_experiment1(host_name, exp_name, "ODBEdit", NULL,
                                   odb_size, DEFAULT_WATCHDOG_TIMEOUT);

   cm_msg_flush_buffer();

   if (reload_from_file) {
      status = ss_shm_delete("ODB");
      printf("ss_shm_delete(ODB) status %d\n", status);
      printf("Please run odbedit again without \'-R\' and ODB will be reloaded from .ODB.SHM\n");
      return 1;
   }

   if (status == CM_WRONG_PASSWORD)
      return 1;
   else if ((status == DB_INVALID_HANDLE) && corrupted) {
      cm_get_error(status, str);
      puts(str);
      printf("ODB is corrupted, connecting anyway...\n");
   } else if (status != CM_SUCCESS) {
      cm_get_error(status, str);
      puts(str);
      return 1;
   }

   /* place a request for system messages */
   cm_msg_register(process_message);

   /* route local messages through print_message */
   cm_set_msg_print(MT_ALL, MT_ALL, print_message);

   /* turn off watchdog if in debug mode */
   if (debug)
      cm_set_watchdog_params(FALSE, 0);

   /* get experiment name */
   if (!exp_name[0]) {
      cm_get_experiment_database(&hDB, NULL);
      size = NAME_LENGTH;
      db_get_value(hDB, 0, "/Experiment/Name", exp_name, &size, TID_STRING, TRUE);
   }

   /* register Ctrl-C handler */
   ss_ctrlc_handler(ctrlc_odbedit);

   /* command loop */
   status = command_loop(host_name, exp_name, cmd, dir);

   /* no shutdown message if called with command */
   if (cmd_mode)
      cm_set_msg_print(MT_ERROR, 0, NULL);

   cm_disconnect_experiment();

   if (status != 1)
      return EXIT_FAILURE;

   return EXIT_SUCCESS;
}
