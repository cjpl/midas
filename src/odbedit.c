/********************************************************************\

  Name:         odbedit.c
  Created by:   Stefan Ritt

  Contents:     Command-line interface to the MIDAS online data base.

  $Log$
  Revision 1.14  1999/04/16 15:12:13  midas
  cm_set_msg_print changed so that info messages are not received twice

  Revision 1.13  1999/02/18 11:16:40  midas
  Added wildcard matching in "ls" and "set" commands

  Revision 1.12  1999/01/13 09:40:49  midas
  Added db_set_data_index2 function

  Revision 1.11  1998/12/10 16:32:14  midas
  Command line completion now also adds values if <tab> is pressed after full key

  Revision 1.10  1998/10/29 14:57:46  midas
  - ODBEDIT prompt can now be modified under /System/Prompt with
  %h host, %e experiment, %t time, %s state (single letter), %S state (string),
  %p directory, %P full path, similar to the UNIX prompt
  - Correct experiment name now shows up in prompt
  - If a stopped run is stopped again, a warning is issued
  - Run parameters are queried with edit capabilities

  Revision 1.9  1998/10/23 14:21:51  midas
  - Modified version scheme from 1.06 to 1.6.0
  - cm_get_version() now returns versino as string

  Revision 1.8  1998/10/12 12:23:51  midas
  Added run state character in ODBEdit prompt

  Revision 1.7  1998/10/12 12:19:03  midas
  Added Log tag in header

\********************************************************************/

#include "midas.h"
#include "msystem.h"

extern INT cmd_edit(char *prompt, char *cmd, INT (*dir)(char*,INT*), INT (*idle)());

BOOL  need_redraw;
BOOL  in_cmd_edit;
char  pwd[256];

/* Data record global to save memory for MS-DOS */
#ifdef OS_MSDOS
static char data[5000];
#else
static char data[50000];
#endif

typedef struct {
  int flags;
  char pattern[32];
} PRINT_INFO;

#define PI_LONG      (1<<0)
#define PI_RECURSIVE (1<<1)
#define PI_VALUE     (1<<2)
#define PI_HEX       (1<<3)

/*------------------------------------------------------------------*/

float test_array[1024];

void update_test(HNDLE hDB, HNDLE hkey, void *info)
{
  printf("Update: %lf\n", test_array[0]);
}

/*------------------------------------------------------------------*/

void print_help(char *command)
{
#ifndef OS_MSDOS /* save some DGROUP memory under MS-DOS */
  if (!command[0])
    {
    printf("Database commands ([] are options, <> are placeholders):\n\n");
    printf("cd <dir>                - change current directory\n");
    printf("chat                    - enter chat mode\n");
    printf("chmod <mode> <key>      - change access mode of a key\n");
    printf("                          1=read | 2=write | 4=delete\n");
    printf("cleanup                 - delete hanging clients\n");
    printf("copy <src> <dest>       - copy a subtree to a new location\n");
    printf("create <type> <key>     - create a key of a certain type\n");
    printf("create <type> <key>[n]  - create an array of size [n]\n");
    printf("del/rm [-l] [-f] <key>  - delete a key and its subkeys\n");
    printf("  -l                      follow links\n");
    printf("  -f                      force deletion without asking\n");
    printf("exec <key>/<cmd>        - execute shell command (stored in key) on server\n");
    printf("find <pattern>          - find a key with wildcard pattern\n");
    printf("help/? [command]        - print this help [for a specific command]\n");
    printf("hi [analyzer] [id]      - tell analyzer to clear histos\n");
    printf("ln <source> <linkname>  - create a link to <source> key\n");
    printf("load <file>             - load database from .ODB file at current position\n");
    printf("-- hit return for more --\r"); getchar();
    printf("ls/dir [-lhvr] [<pat>]  - show database entries which match pattern\n");
    printf("  -l                      detailed info\n");
    printf("  -h                      hex format\n");
    printf("  -v                      only value\n");
    printf("  -r                      show database entries recursively\n");
    printf("make                    - create experim.h\n");
    printf("mem                     - show memeory usage\n");
    printf("mkdir <subdir>          - make new <subdir>\n");
    printf("move <key> [top/bottom/[n]] - move key to position in keylist\n");
    printf("msg [user] <msg>        - compose user message\n");
    printf("passwd                  - change MIDAS password\n");
    printf("pause                   - pause current run\n");
    printf("pwd                     - show current directory\n");
    printf("resume                  - resume current run\n");
    printf("rename <old> <new>      - rename key\n");
    printf("rewind [channel]        - rewind tapes in logger\n");
    printf("-- hit return for more --\r"); getchar();
    printf("save [-c -s] <file>     - save database at current position\n");
    printf("                          in ASCII format\n");
    printf("  -c                      as a C structure\n");
    printf("  -s                      as a #define'd string\n");
    printf("set <key> <value>       - set the value of a key\n");
    printf("set <key>[i] <value>    - set the value of index i\n");
    printf("set <key>[*] <value>    - set the value of all indices of a key\n");
    printf("set <key>[i..j] <value> - set the value of all indices i..j\n");
    printf("scl [-w]                - show all active clients [with watchdog info]\n");
    printf("shutdown <client>/all   - shutdown individual or all clients\n");
    printf("sor                     - show open records in current subtree\n");
    printf("start [number]          - start a run [with a specific number]\n");
    printf("stop                    - stop current run\n");
    printf("trunc <key> <index>     - truncate key to [index] values\n");
    printf("ver                     - show MIDAS library version\n");

    printf("\nquit/exit             - exit\n");
    return;
    }

  if (equal_ustring(command, "cd"))
    {
    printf("cd <dir> - change current directory. Use \"cd /\" to change to the root\n");
    printf("           of the ODB, \"cd ..\" to change to the parent directory.\n");
    }
  else if (equal_ustring(command, "chat"))
    {
    printf("chat - enter chat mode. In this mode, users can \"talk\" to each other.\n");
    printf("       Each user running ODBEdit connected to the same experiment can see\n");
    printf("       the other messages and each user running ODBEdit in chat mode can\n");
    printf("       produce messages. All messages are logged in the MIDAS logging file.\n");
    }
  else
    printf("No specific help available for command \"%s\".\n", command);

#endif
}

/*------------------------------------------------------------------*/

void process_message(HNDLE hBuf, HNDLE id, EVENT_HEADER *pheader, void *message)
{
time_t tm;
char   str[80];

  /* prepare time */
  time(&tm);
  strcpy(str, ctime(&tm));
  str[19] = 0;

  /* print message text which comes after event header */
  if (in_cmd_edit)
    printf("\r%s %s\n", str+11, message);
  else
    printf("\n%s %s\n", str+11, message);

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
    return *pat == '*' ? match (pat+1, str) : !*pat;    

  switch (*pat)    
    {
    case '\0': 
      return 0;
    case '*' : 
      return match(pat+1, str) || match(pat, str+1);
    case '?' : 
      return match(pat+1, str+1);
    default  : 
      return (toupper(*pat) == toupper(*str)) && match(pat+1, str+1);    
    }
}

/*------------------------------------------------------------------*/

void print_key(HNDLE hDB, HNDLE hKey, KEY *key, INT level, void *info)
{
INT         i, size, status;
static char data_str[256], line[256];
DWORD       delta;
PRINT_INFO  *pi;

  pi = (PRINT_INFO *) info;

  /* if pattern set, check if match */
  if (pi->pattern[0] &&
      !match(pi->pattern, key->name))
    return;
  
  size = sizeof(data);
  if (pi->flags & PI_VALUE)
    {
    /* only print value */
    if (key->type != TID_KEY)
      {
      status = db_get_data(hDB, hKey, data, &size, key->type);
      if (status == DB_NO_ACCESS)
        strcpy(data_str, "<no read access>");
      else
        for (i=0 ; i<key->num_values ; i++)
          {
          if (pi->flags & PI_HEX)
            db_sprintfh(data_str, data, key->item_size, i, key->type);
          else
            db_sprintf(data_str, data, key->item_size, i, key->type);
          printf("%s\n", data_str);
          }
      }
    }
  else
    {
    /* print key name with value */
    memset(line, ' ', 80);
    line[80] = 0;
    sprintf(line + level*4, "%s", key->name);
    line[strlen(line)] = ' ';

    if (key->type == TID_KEY)
      {
      if (pi->flags & PI_LONG)
        sprintf(line+32, "DIR"); 
      else
        line[32] = 0;
      printf("%s\n", line);
      }
    else
      {
      status = db_get_data(hDB, hKey, data, &size, key->type);
      if (status == DB_NO_ACCESS)
        strcpy(data_str, "<no read access>");
      else
        {
        if (pi->flags & PI_HEX)
          db_sprintfh(data_str, data, key->item_size, 0, key->type);
        else
          db_sprintf(data_str, data, key->item_size, 0, key->type);
        }

      if (pi->flags & PI_LONG)
        {
        sprintf(line+32, "%s", rpc_tid_name(key->type));
        line[strlen(line)] = ' ';
        sprintf(line+40, "%ld", key->num_values);
        line[strlen(line)] = ' ';
        sprintf(line+46, "%ld", key->item_size);
        line[strlen(line)] = ' ';

        db_get_key_time(hDB, hKey, &delta);
        if (delta < 60)
          sprintf(line+52, "%ds", delta);
        else if (delta < 3600)
          sprintf(line+52, "%1.0lfm", delta/60.0);
        else if (delta < 86400)
          sprintf(line+52, "%1.0lfh", delta/3600.0);
        else if (delta < 86400*99)
          sprintf(line+52, "%1.0lfh", delta/86400.0);
        else
          sprintf(line+52, ">99d");
        line[strlen(line)] = ' ';

        sprintf(line+57, "%d", key->notify_count);
        line[strlen(line)] = ' ';

        if (key->access_mode & MODE_READ)
          line[61] = 'R';
        if (key->access_mode & MODE_WRITE)
          line[62] = 'W';
        if (key->access_mode & MODE_DELETE)
          line[63] = 'D';
        if (key->access_mode & MODE_EXCLUSIVE)
          line[64] = 'E';

        if (key->num_values == 1)
          strcpy(line+66, data_str);
        else
          line[66] = 0;
        }
      else
        if (key->num_values == 1)
          strcpy(line+32, data_str);
        else
          line[32] = 0;

      printf("%s\n", line);
    
      if (key->num_values > 1)
        {
        for (i=0 ; i<key->num_values ; i++)
          {
          if (pi->flags & PI_HEX)
            db_sprintfh(data_str, data, key->item_size, i, key->type);
          else
            db_sprintf(data_str, data, key->item_size, i, key->type);

          memset(line, ' ', 80);
          line[80] = 0;

          if (pi->flags & PI_LONG)
            {
            sprintf(line+40, "[%ld]", i);
            line[strlen(line)] = ' ';

            strcpy(line+56, data_str);
            }
          else
            strcpy(line+32, data_str);

          printf("%s\n", line);
          }
        }
      }
    }
}

/*------------------------------------------------------------------*/

void set_key(HNDLE hDB, HNDLE hKey, int index1, int index2, char *value)
{
KEY  key;
char data[1000];
int  i, size, status;

  db_get_key(hDB, hKey, &key);

  memset(data, 0, sizeof(data));
  db_sscanf(value, data, &size, 0, key.type);

  /* extend data size for single string if necessary */
  if ((key.type == TID_STRING || key.type == TID_LINK) 
        && (int) strlen(data)+1 > key.item_size &&
      key.num_values == 1)
    key.item_size = strlen(data)+1;

  if (key.item_size == 0)
    key.item_size = rpc_tid_size(key.type);

  if (key.num_values > 1 && index1 == -1)
    {
    for (i=0 ; i<key.num_values ; i++)
      status = db_set_data_index(hDB, hKey, data, key.item_size, i, key.type);
    }
  else if (key.num_values > 1 && index2>index1)
    {
    for (i=index1 ; i<key.num_values && i<=index2; i++)
      status = db_set_data_index(hDB, hKey, data, key.item_size, i, key.type);
    }
  else if (key.num_values > 1 || index1 > 0)
    status = db_set_data_index(hDB, hKey, data, key.item_size, index1, key.type);
  else
    status = db_set_data(hDB, hKey, data, key.item_size, 1, key.type);

  if (status == DB_NO_ACCESS)
    printf("Write access not allowed\n");
}

/*------------------------------------------------------------------*/

void scan_tree(HNDLE hDB, HNDLE hKey, INT *total_size_key, INT *total_size_data, INT level, INT flags)
{
INT         i, j, size, status;
KEY         key;
HNDLE       hSubkey;
static char data_str[256], line[256];
DWORD       delta;

  db_get_key(hDB, hKey, &key);
  size = sizeof(data);

  *total_size_key += ALIGN(sizeof(KEY));
  if (key.type == TID_KEY)
    *total_size_key += ALIGN(sizeof(KEYLIST));
  else
    *total_size_data += ALIGN(key.total_size);

  if (flags & 0x4)
    {
    /* only print value */
    if (key.type != TID_KEY)
      {
      status = db_get_data(hDB, hKey, data, &size, key.type);
      if (status == DB_NO_ACCESS)
        strcpy(data_str, "<no read access>");
      else
        for (j=0 ; j<key.num_values ; j++)
          {
          if (flags & 0x8)
            db_sprintfh(data_str, data, key.item_size, j, key.type);
          else
            db_sprintf(data_str, data, key.item_size, j, key.type);
          printf("%s\n", data_str);
          }
      }
    }
  else
    {
    /* print key name with value */
    memset(line, ' ', 80);
    line[80] = 0;
    sprintf(line + level*4, "%s", key.name);
    line[strlen(line)] = ' ';

    if (key.type == TID_KEY)
      {
      line[32] = 0;
      printf("%s\n", line);
      }
    else
      {
      status = db_get_data(hDB, hKey, data, &size, key.type);
      if (status == DB_NO_ACCESS)
        strcpy(data_str, "<no read access>");
      else
        {
        if (flags & 0x8)
          db_sprintfh(data_str, data, key.item_size, 0, key.type);
        else
          db_sprintf(data_str, data, key.item_size, 0, key.type);
        }

      if (flags & 0x1)
        {
        sprintf(line+32, "%s", rpc_tid_name(key.type));
        line[strlen(line)] = ' ';
        sprintf(line+40, "%ld", key.num_values);
        line[strlen(line)] = ' ';
        sprintf(line+46, "%ld", key.item_size);
        line[strlen(line)] = ' ';

        db_get_key_time(hDB, hKey, &delta);
        if (delta < 60)
          sprintf(line+52, "%ds", delta);
        else if (delta < 3600)
          sprintf(line+52, "%1.0lfm", delta/60.0);
        else if (delta < 86400)
          sprintf(line+52, "%1.0lfh", delta/3600.0);
        else if (delta < 86400*99)
          sprintf(line+52, "%1.0lfh", delta/86400.0);
        else
          sprintf(line+52, ">99d");
        line[strlen(line)] = ' ';

        sprintf(line+57, "%d", key.notify_count);
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
          strcpy(line+66, data_str);
        else
          line[66] = 0;
        }
      else
        if (key.num_values == 1)
          strcpy(line+32, data_str);
        else
          line[32] = 0;

      printf("%s\n", line);
    
      if (key.num_values > 1)
        {
        for (j=0 ; j<key.num_values ; j++)
          {
          if (flags & 0x8)
            db_sprintfh(data_str, data, key.item_size, j, key.type);
          else
            db_sprintf(data_str, data, key.item_size, j, key.type);

          memset(line, ' ', 80);
          line[80] = 0;

          if (flags & 0x1)
            {
            sprintf(line+40, "[%ld]", j);
            line[strlen(line)] = ' ';

            strcpy(line+56, data_str);
            }
          else
            strcpy(line+32, data_str);

          printf("%s\n", line);
          }
        }
      }
    }

  /* recurse subtree */
  if (key.type == TID_KEY && (flags & 0x2))
    {
    for (i=0 ; ; i++)
      {
      db_enum_link(hDB, hKey, i, &hSubkey);

      if (!hSubkey)
        break;

      scan_tree(hDB, hSubkey, total_size_key, total_size_data, level+1, flags);
      }
    }
}

/*------------------------------------------------------------------*/

/* complete partial key name, gets called from cmd_edit */
INT cmd_dir(char *line, INT *cursor)
{
KEY   key;
HNDLE hDB, hKey, hSubkey;
INT   i, j, match, size;
char  str[256], *pc, partial[256], last_match[256], head[256], tail[256], key_name[256], c;
char  test_key[256];
BOOL  blanks, mismatch;

  cm_get_experiment_database(&hDB, NULL);

  /* remember tail for later */
  strcpy(head, line);
  strcpy(tail, line+*cursor);
  line[*cursor] = 0; 

  /* search beginning of key */
  pc = head;
  while (*pc && *pc != ' ')
    pc++;
  while (*pc && *pc == ' ')
    pc++;

  if (*pc)
    {
    strcpy(key_name, pc);
    *pc = 0; /* end of head */
    }
  else
    key_name[0] = 0;
  
  /* check if key exists (for "set <key>" completion */
  if (strncmp(head, "set", 3) == 0 && strlen(key_name)>0)
    {
    if (key_name[0] == '"')
      strcpy(str, key_name+1);
    else
      strcpy(str, key_name);
    if (key_name[0] != '/')
      {
      strcpy(test_key, pwd);
      if (str[strlen(test_key)-1] != '/')
        strcat(test_key, "/");
      strcat(test_key, str);
      }
    else
      strcpy(test_key, str);

    pc = test_key + strlen(test_key) - 1;
    while (pc > test_key && (*pc == ' ' || *pc == '"'))
      *pc-- = 0;
    if (db_find_key(hDB, 0, test_key, &hSubkey) == DB_SUCCESS)
      {
      /* retrieve key data */
      db_get_key(hDB, hSubkey, &key);
      size = sizeof(data);
      db_get_data(hDB, hSubkey, data, &size, key.type);
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

  if (*pc != '/')
    {
    strcpy(str, pwd);
    if (str[strlen(str)-1] != '/')
      strcat(str, "/");
    strcat(str, pc);
    }
  else
    strcpy(str, pc);

  /* split key_name into known and new directory */
  for (pc = str+strlen(str)-1 ; pc>str && *pc != '/' ; pc--);

  if (*pc == '/')
    {
    *pc = 0;
    strcpy(partial, pc+1);
    }
  else
    strcpy(partial, str);

  db_find_key(hDB, 0, str, &hKey);
  for (i=0,match=0 ; ; i++)
    {
    db_enum_link(hDB, hKey, i, &hSubkey);

    if (!hSubkey)
      break;

    db_get_key(hDB, hSubkey, &key);
    strcpy(str, key.name);

    str[strlen(partial)] = 0;

    if (equal_ustring(str, partial))
      match++;
    }

  if (match != 1)
    printf("\r\n");

  for (i=0 ; ; i++)
    {
    db_enum_link(hDB, hKey, i, &hSubkey);

    if (!hSubkey)
      break;

    db_get_key(hDB, hSubkey, &key);
    strcpy(str, key.name);

    str[strlen(partial)] = 0;

    if (equal_ustring(str, partial))
      {
      if (match == 1)
        {
        /* search split point */
        for (pc = key_name+strlen(key_name)-1 ; pc>key_name && *pc != '/' ; pc--);
        if (*pc == '/')
          pc++;

        strcpy(pc, key.name);
        if (key.type == TID_KEY)
          strcat(pc, "/");

        /* insert '"' if blanks in name */
        if (strchr(key.name, ' '))
          {
          if (key_name[0] != '"')
            {
            for (i=strlen(key_name) ; i >= 0 ; i--)
              key_name[i+1] = key_name[i];

            key_name[0] = '"';
            }
          if (key.type != TID_KEY)
            strcat(key_name, "\"");
          }

        if (key.type != TID_KEY)
          {
          if (key_name[0] == '"' && key_name[strlen(key_name)-1] != '"')
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

  if (match > 1 && key_name[0])
    {
    blanks = FALSE;

    for (j=strlen(partial) ; ; j++)
      {
      for (i=0,c=1,mismatch=FALSE ; ; i++)
        {
        db_enum_link(hDB, hKey, i, &hSubkey);

        if (!hSubkey)
          break;

        db_get_key(hDB, hSubkey, &key);
        strcpy(str, key.name);

        str[strlen(partial)] = 0;

        if (strchr(key.name, ' '))
          blanks = TRUE;

        if (equal_ustring(str, partial))
          {
          strcpy(last_match, key.name);
          if (c == 1)
            c = toupper(key.name[j]);
          else
            if (c != toupper(key.name[j]))
              {
              mismatch = TRUE;
              break;
              }
          }
        }

      if (mismatch || last_match[j] == 0)
        break;
      }

    /* search split point */
    for (pc = key_name+strlen(key_name)-1 ; pc>key_name && *pc != '/' ; pc--);
    if (*pc == '/')
      pc++;

    for (i=0 ; i<j ; i++)
      pc[i] = last_match[i];
    pc[i] = 0;

    /* insert '"' if blanks in name */
    if (blanks)
      {
      if (key_name[0] != '"')
        {
        for (i=strlen(key_name) ; i >= 0 ; i--)
          key_name[i+1] = key_name[i];

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

void search_key(HNDLE hDB, HNDLE hKey, KEY *key, INT level, void *info)
{
INT         i, size, status;
char        *pattern;
static char data_str[MAX_ODB_PATH], line[MAX_ODB_PATH];
char        path[MAX_ODB_PATH];

  pattern = (char *) info;

  if (match(pattern, key->name))
    {
    db_get_path(hDB, hKey, path, MAX_ODB_PATH);
    strcpy(line, path);
    strcat(line, " : ");

    if (key->type != TID_KEY)
      {
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
        for (i=0 ; i<key->num_values ; i++)
          {
          db_sprintf(data_str, data, key->item_size, i, key->type);

          printf("  [%ld] : %s\n", i, data_str);
          }
      }
    else
      printf("%s\n", line);
    }

}

/*------------------------------------------------------------------*/

void del_tree(HNDLE hDB, HNDLE hKey, INT level)
{
INT        i;
KEY        key;
HNDLE      hSubkey;

  for (i=0 ; ; i++)
    {
    db_enum_link(hDB, hKey, i, &hSubkey);

    if (!hSubkey)
      break;

    db_get_key(hDB, hSubkey, &key);
    if (key.type == TID_KEY)
      del_tree(hDB, hSubkey, level+1);

    if (rand() < RAND_MAX / 10)
      db_delete_key(hDB, hSubkey, 0);
    }
}

/*------------------------------------------------------------------*/

void create_experim_h(HNDLE hDB)
{
INT    i, index, hfile, status, size;
HNDLE  hKey, hKeyRoot, hKeyEq, hDefKey, hKeyBank, hKeyPar;
char   str[80], eq_name[80];
KEY    key;
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

  /* create fiel */
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
  if (hKey)
    {
    sprintf(str, "#define EXP_PARAM_DEFINED\n\n"); 
    write(hfile, str, strlen(str));
    db_save_struct(hDB, hKey, file_name, "EXP_PARAM", TRUE);
    db_save_string(hDB, hKey, file_name, "EXP_PARAM_STR", TRUE);
    lseek(hfile, 0, SEEK_END);
    }

  /* write /experiment/edit on start */
  db_find_key(hDB, 0, "/Experiment/Edit on start", &hKey);
  if (hKey)
    {
    sprintf(str, "#define EXP_EDIT_DEFINED\n\n"); 
    write(hfile, str, strlen(str));
    db_save_struct(hDB, hKey, file_name, "EXP_EDIT", TRUE);
    db_save_string(hDB, hKey, file_name, "EXP_EDIT_STR", TRUE);
    lseek(hfile, 0, SEEK_END);
    }

  /* write /analyzer/parameters tree */
  status = db_find_key(hDB, 0, "/Analyzer/Parameters", &hKeyRoot);
  if (status == DB_SUCCESS)
    for (index=0 ; ; index++)
      {
      status = db_enum_key(hDB, hKeyRoot, index, &hKeyPar);
      if (status == DB_NO_MORE_SUBKEYS)
        break;

      db_get_key(hDB, hKeyPar, &key);
      strcpy(eq_name, key.name);
      name2c(eq_name);
      for (i=0 ; i<(int) strlen(eq_name) ; i++)
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

  /* loop through equipment list */
  status = db_find_key(hDB, 0, "/Equipment", &hKeyRoot);
  if (status == DB_SUCCESS)
    for (index=0 ; ; index++)
      {
      status = db_enum_key(hDB, hKeyRoot, index, &hKeyEq);
      if (status == DB_NO_MORE_SUBKEYS)
        break;

      db_get_key(hDB, hKeyEq, &key);
      strcpy(eq_name, key.name);
      name2c(eq_name);
      for (i=0 ; i<(int) strlen(eq_name) ; i++)
        eq_name[i] = toupper(eq_name[i]);

      lseek(hfile, 0, SEEK_END);
      sprintf(str, "#ifndef EXCL_%s\n\n", eq_name);
      write(hfile, str, strlen(str));

      size = sizeof(str);
      str[0] = 0;
      db_get_value(hDB, hKeyEq, "Common/Format", str, &size, TID_STRING);
      
      /* if event is in fixed format, extract header file */
      if (equal_ustring(str, "Fixed"))
        {
        db_find_key(hDB, hKeyEq, "Variables", &hDefKey);
        if (hDefKey)
          {
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
      else if (equal_ustring(str, "MIDAS"))
        {
        db_find_key(hDB, hKeyEq, "Variables", &hDefKey);
        if (hDefKey)
          {
          for (i=0 ; ; i++)
            {
            status = db_enum_key(hDB, hDefKey, i, &hKeyBank);
            if (status == DB_NO_MORE_SUBKEYS)
              break;

            db_get_key(hDB, hKeyBank, &key);

            if (key.type == TID_KEY)
              {
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

      /* wite common tree */
      db_find_key(hDB, hKeyEq, "Common", &hDefKey);
      if (hDefKey)
        {
        lseek(hfile, 0, SEEK_END);
        sprintf(str, "#define %s_COMMON_DEFINED\n\n", eq_name); 
        write(hfile, str, strlen(str));

        sprintf(str, "%s_COMMON_STR", eq_name);
        db_save_string(hDB, hDefKey, file_name, str, TRUE);
        }

      /* if settings present, write them */
      db_find_key(hDB, hKeyEq, "Settings", &hDefKey);
      if (hDefKey)
        {
        lseek(hfile, 0, SEEK_END);
        sprintf(str, "#define %s_SETTINGS_DEFINED\n\n", eq_name); 
        write(hfile, str, strlen(str));

        sprintf(str, "%s_SETTINGS", eq_name);
        db_save_struct(hDB, hDefKey, file_name, str, TRUE);
        sprintf(str, "%s_SETTINGS_STR", eq_name);
        db_save_string(hDB, hDefKey, file_name, str, TRUE);
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
INT i, status;

  need_redraw = FALSE;

  status = cm_yield(100);

  /* check if client connections are broken */
  if (status == SS_ABORT || status == RPC_SHUTDOWN)
    {
    /* check if client connections are broken */
    for (i=0 ; i < MAX_RPC_CONNECTION ; i++)
      cm_yield(0);

    cm_disconnect_experiment();
    exit(0);
    }

  return need_redraw;
}

/*------------------------------------------------------------------*/

void compose_name(char *pwd, char *name, char *full_name)
{
  if (name[0] != '/')
    {
    strcpy(full_name, pwd);
    if (full_name[strlen(full_name)-1] != '/')
      strcat(full_name, "/");
    strcat(full_name, name);
    }
  else
    strcpy(full_name, name);
}

/*------------------------------------------------------------------*/

void assemble_prompt(char *prompt, char *host_name, char *exp_name, char *pwd)
{
HNDLE  hDB;
char   mask[256], str[32];
int    state, size;
char   *pp, *pm, *pc;
time_t now;

char  *state_char[] = { "U", "S", "P", "R" };
char  *state_str[]  = { "Unknown", "Stopped", "Paused", "Running" };

  cm_get_experiment_database(&hDB, NULL);

  size = sizeof(mask);
  strcpy(mask, "[%h:%e:%s]%p>");
  db_get_value(hDB, 0, "/System/Prompt", mask, &size, TID_STRING);

  state = STATE_STOPPED;
  size = sizeof(state);
  db_get_value(hDB, 0, "/Runinfo/State", &state, &size, TID_INT);
  if (state > STATE_RUNNING)
    state = 0;

  pm = mask;
  pp = prompt;
  do
    {
    if (*pm != '%')
      {
      *pp++ = *pm++;
      *pp = 0;
      }
    else
      {
      switch (*++pm)
        {
        case 't':
          time(&now);
          strcpy(str, ctime(&now));
          str[19] = 0;
          strcpy(pp, str+11);
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
          pc = pwd+strlen(pwd)-1;
          while (*pc != '/' && pc != pwd)
            pc--;
          if (pc == pwd)
            strcat(pp, pwd);
          else
            strcat(pp, pc+1);
          break;
        }
      pm++;
      pp += strlen(pp);
      }
    } while (*pm);
}

/*------------------------------------------------------------------*/

void command_loop(char *host_name, char *exp_name, char *cmd, char *start_dir)
{
INT             status, i, j, state, size, old_run_number, new_run_number, channel;
char            line[256], prompt[256];
char            param[10][100];
char            str[256], name[256], *pc, data_str[256];
char            old_password[32], new_password[32];
INT             nparam, flags, index1, index2;
WORD            mode;
HNDLE           hDB, hKey, hKeyClient, hSubkey, hRootKey;
KEY             key;
char            user_name[80] = "";
FILE            *cmd_file = NULL;
DWORD           last_msg_time=0;
char            message[256], client_name[256];
INT             n1, n2;
PRINT_INFO      print_info;

  cm_get_experiment_database(&hDB, &hKeyClient);
  
  /* command loop */
  if (start_dir[0])
    strcpy(pwd, start_dir);
  else
    strcpy(pwd, "/");

  /* check if dir exists */
  if (db_find_key(hDB, 0, pwd, &hKey) != DB_SUCCESS)
    {
    printf("Directory \"%s\" not found.\n", pwd);
    return;
    }

  /* open command file */
  if (cmd[0] == '@')
    {
    cmd_file = fopen(cmd+1, "r");
    if (cmd_file == NULL)
      {
      printf("Command file %s not found.\n", cmd+1);
      return;
      }
    }

  do
    {
    /* print prompt */
    if (cmd[0] == 0)
      {
      assemble_prompt(prompt, host_name, exp_name, pwd);

      in_cmd_edit = TRUE;
      line[0] = 0;
      cmd_edit(prompt, line, cmd_dir, cmd_idle);
      in_cmd_edit = FALSE;
      }
    else if (cmd[0] != '@')
      strcpy(line, cmd);
    else
      {
      memset(line, 0, sizeof(line));
      fgets(line, sizeof(line), cmd_file);

      if (line[0] == 0)
        break;

      /* cut off CR */
      while (strlen(line)>0 && line[strlen(line)-1] == '\n')
        line[strlen(line)-1] = 0;

      if (line[0] == 0)
        continue;
      }

    /* analyze line */
    nparam = 0;
    pc = line;
    while (*pc == ' ')
      pc++;

    memset(param, 0, sizeof(param));
    do
      {
      if (*pc == '"')
        {
        pc++;
        for (i=0 ; *pc && *pc != '"' ; i++)
          param[nparam][i] = *pc++;
        while(strlen(param[nparam]) > 0 && param[nparam][strlen(param[nparam])-1] == ' ')
          param[nparam][strlen(param[nparam])-1] = 0;
        if (*pc)
          pc++;
        }
      else if (*pc == '\'')
        {
        pc++;
        for (i=0 ; *pc && *pc != '\'' ; i++)
          param[nparam][i] = *pc++;
        if (*pc)
          pc++;
        }
      else if (*pc == '`')
        {
        pc++;
        for (i=0 ; *pc && *pc != '`' ; i++)
          param[nparam][i] = *pc++;
        if (*pc)
          pc++;
        }
      else
        for (i=0 ; *pc && *pc != ' ' ; i++)
          param[nparam][i] = *pc++;
      param[nparam][i] = 0;
      while (*pc == ' ')
        pc++;
      nparam++;
      } while (*pc);

    /* help */
    if ((param[0][0] == 'h' && param[0][1] == 'e') ||
         param[0][0] == '?')
      print_help(param[1]);

    /* ls */
    else if ((param[0][0] == 'l' && param[0][1] == 's') ||
             (param[0][0] == 'd' && param[0][1] == 'i'))
      {
      db_find_key(hDB, 0, pwd, &hKey);
      print_info.flags = 0;
      print_info.pattern[0] = 0;

      /* parse options */
      for (i=1 ; i<4 ; i++)
        if (param[i][0] == '-')
          {
          for (j=1 ; param[i][j] != ' ' && param[i][j] ; j++)
            {
            if (param[i][j] == 'l')
              print_info.flags |= PI_LONG;
            if (param[i][j] == 'r')
              print_info.flags |= PI_RECURSIVE;
            if (param[i][j] == 'v')
              print_info.flags |= PI_VALUE;
            if (param[i][j] == 'h')
              print_info.flags |= PI_HEX;
            }
          }

      for (i=1 ; param[i][0] == '-' ; i++);

      if (param[i][0])
        {
        if (strpbrk(param[i], "*?") != NULL)
          {
          /* if parameter contains wildcards, set pattern */
          strcpy(print_info.pattern, param[i]);
          }
        else
          {
          if (param[i][0] == '/')
            status = db_find_key(hDB, 0, param[i], &hKey);
          else
            status = db_find_key(hDB, hKey, param[i], &hKey);

          if (status != DB_SUCCESS)
            printf("key %s not found\n", param[i]);
          }
        }

      if (hKey)
        {
        if ((print_info.flags & PI_LONG) && (print_info.flags & PI_VALUE)==0)
          {
          printf("Key name                        Type    #Val  Size  Last Opn Mode Value\n");
          printf("---------------------------------------------------------------------------\n");
          }

        if (print_info.flags & PI_RECURSIVE)
          db_scan_tree(hDB, hKey, 0, print_key, &print_info);
        else
          {
          db_get_key(hDB, hKey, &key);
          if (key.type != TID_KEY)
            print_key(hDB, hKey, &key, 0, &print_info);
          else
            for (i=0 ; ; i++)
              {
              db_enum_link(hDB, hKey, i, &hSubkey);

              if (!hSubkey)
                break;

              db_get_key(hDB, hSubkey, &key);
              print_key(hDB, hSubkey, &key, 0, &print_info);
              }
          }
        }
      }

    /* cd */
    else if (param[0][0] == 'c' && param[0][1] == 'd')
      {
      compose_name(pwd, param[1], str);

      status = db_find_key(hDB, 0, str, &hKey);

      if (strcmp(str, "/") == 0)
        strcpy(pwd, str);
      else if (status == DB_SUCCESS)
        {
        db_get_key(hDB, hKey, &key);
        if (key.type != TID_KEY)
          printf("key has no subkeys\n");
        else 
          db_get_path(hDB, hKey, pwd, 256);
        }
      else
        printf("key not found\n");
      }

    /* pwd */
    else if (param[0][0] == 'p' && param[0][1] == 'w')
      {
      printf("%s\n", pwd);
      }

    /* create */
    else if (param[0][0] == 'c' && param[0][1] == 'r')
      {
      compose_name(pwd, param[2], str);

      /* check if array */
      if (str[strlen(str)-1] == ']')
        {
        if (strchr(str, '['))
          j = atoi(strchr(str, '[')+1);
        *strchr(str, '[') = 0;
        }
      else
        j = 1;

      /* get TID */
      for (i=0 ; i<TID_LAST ; i++)
        {
        if (equal_ustring(rpc_tid_name(i), param[1]))
          break;
        }

      if (i == TID_LAST)
        {
        printf("Unknown type. Must be one of:\n");
        printf("{ BYTE,CHAR,WORD,SHORT,DWORD,INT,BOOL,FLOAT,DOUBLE,STRING }\n");
        }
      else
        {
        db_create_key(hDB, 0, str, i);
        db_find_key(hDB, 0, str, &hKey);
        db_get_key(hDB, hKey, &key);

        if (i == TID_STRING)
          {
          memset(data, 0, sizeof(data));

          printf("String length [%d]: ", NAME_LENGTH);
          ss_gets(str, 256);
          if (str[0])
            key.item_size = atoi(str);
          else
            key.item_size = NAME_LENGTH;

          db_set_data(hDB, hKey, data, key.item_size, 1, key.type);
          }

        if (j > 1)
          {
          memset(data, 0, sizeof(data));
          
          if (key.type == TID_LINK)
            key.item_size = NAME_LENGTH;
          db_set_data_index(hDB, hKey, data, key.item_size, j-1, key.type);
          }
        }
      }
    
    /* mkdir */
    else if (param[0][0] == 'm' && param[0][1] == 'k')
      {
      compose_name(pwd, param[1], str);
      db_create_key(hDB, 0, str, TID_KEY);
      }

    /* link */
    else if (param[0][0] == 'l' && param[0][1] == 'n')
      {
      compose_name(pwd, param[2], str);
      db_create_link(hDB, 0, str, param[1]);
      }

    /* copy */
    else if (param[0][0] == 'c' && param[0][1] == 'o')
      {
      /* test if destination exists */
      compose_name(pwd, param[2], str);
      status = db_find_link(hDB, 0, str, &hKey);
      if (status == DB_SUCCESS)
        {
        printf("Overwrite existing key\n\"%s\"\n(y/[n]) ", str);
        ss_gets(str, 256);
        if (str[0] == 'y')
          db_delete_key(hDB, hKey, FALSE);
        }

      if (status == DB_NO_KEY || str[0] == 'y')
        {
        compose_name(pwd, param[1], str);

        status = db_find_link(hDB, 0, str, &hKey);
        if (status == DB_SUCCESS)
          {
          compose_name(pwd, param[2], str);

          db_get_key(hDB, hKey, &key);
          db_create_key(hDB, 0, str, key.type);
          
          if (key.type != TID_KEY)
            {
            size = sizeof(data);
            db_get_data(hDB, hKey, data, &size, key.type); 
            db_find_key(hDB, 0, str, &hKey);
            db_set_data(hDB, hKey, data, size, key.num_values, key.type);
            }
          else
            {
            size = sizeof(data);
            db_copy(hDB, hKey, data, &size, "");
            db_find_key(hDB, 0, str, &hKey);
            db_paste(hDB, hKey, data);
            }
          }
        else
          printf("key not found\n");
        }
      }

    /* delete */
    else if ((param[0][0] == 'd' && param[0][1] == 'e') ||
             (param[0][0] == 'r' && param[0][1] == 'm'))
      {
      flags = 0;
      if (param[1][1] == 'f' || param[2][1] == 'f')
        flags |= (1<<0);
      if (param[1][1] == 'l' || param[2][1] == 'l')
        flags |= (1<<1);

      for (i=1 ; param[i][0] == '-' ; i++);

      compose_name(pwd, param[i], str);

      status = db_find_link(hDB, 0, str, &hKey);
      db_get_key(hDB, hKey, &key);

      if (status == DB_SUCCESS)
        {
        if (flags & (1<<0))
          str[0] = 'y';
        else
          {
          if (key.type == TID_KEY)
            printf("Are you sure to delete the key\n\"%s\"\nand all its subkeys? (y/[n]) ", str);
          else
            printf("Are you sure to delete the key\n\"%s\"\n(y/[n]) ", str);

          ss_gets(str, 256);
          }

        if (str[0] == 'y')
          {
          status = db_delete_key(hDB, hKey, (flags & (1<<1))>0);
          if (status == DB_NO_ACCESS)
            printf("deletion of key not allowed\n");
          if (status == DB_OPEN_RECORD)
            printf("key is open by other client\n");
          }
        }
      else
        printf("key not found\n");
      }

    /* set */
    else if (param[0][0] == 's' && param[0][1] == 'e')
      {
      /* check if index is supplied */
      index1 = index2 = 0;
      strcpy(str, param[1]);
      if (str[strlen(str)-1] == ']')
        {
        if (strchr(str, '['))
          {
          if (*(strchr(str, '[')+1) == '*')
            index1 = -1;
          else if (strchr((strchr(str, '[')+1), '.'))
            {
            index1 = atoi(strchr(str, '[')+1);
            pc = strchr(str, '[')+1;
            while (*pc != '.')
              pc++;
            while (*pc == '.')
              pc++;
            index2 = atoi(pc);
            }
          else
            index1 = atoi(strchr(str, '[')+1);
          }
        *strchr(str, '[') = 0;
        }

      if (strpbrk(str, "*?") != NULL)
        {
        db_find_key(hDB, 0, pwd, &hKey);
        for (i=0 ; ; i++)
          {
          db_enum_link(hDB, hKey, i, &hSubkey);

          if (!hSubkey)
            break;

          db_get_key(hDB, hSubkey, &key);
          if (match(str, key.name))
            set_key(hDB, hSubkey, index1, index2, param[2]);
          }
        }
      else
        {
        compose_name(pwd, str, name);

        status = db_find_key(hDB, 0, name, &hKey);
        if (status == DB_SUCCESS)
          set_key(hDB, hKey, index1, index2, param[2]);
        else
          {
          printf("key not found\n");
          }
        }
      }

    /* set mode */
    else if (param[0][0] == 'c' && param[0][1] == 'h' && param[0][2] == 'm')
      {
      compose_name(pwd, param[2], str);

      mode = atoi(param[1]);

      if (strcmp(str, "/") != 0)
        status = db_find_key(hDB, 0, str, &hKey);
      else
        hKey = 0;

      if (status == DB_SUCCESS || !hKey)
        {
        printf("Are you sure to change the mode of key\n  %s\nand all its subkeys\n", str);
        printf("to mode [%c%c%c%c]? (y/[n]) ", mode & MODE_READ      ? 'R' : 0,
                                               mode & MODE_WRITE     ? 'W' : 0,
                                               mode & MODE_DELETE    ? 'D' : 0,
                                               mode & MODE_EXCLUSIVE ? 'E' : 0);
        ss_gets(str, 256);
        if (str[0] == 'y')
          db_set_mode(hDB, hKey, mode, TRUE);
        }
      else
        printf("key not found\n");
      }

    /* truncate */
    else if (param[0][0] == 't' && param[0][1] == 'r')
      {
      compose_name(pwd, param[1], str);

      if (strcmp(str, "/") != 0)
        status = db_find_key(hDB, 0, str, &hKey);
      else
        hKey = 0;

      i = atoi(param[2]);
      if (i==0)
        i = 1;

      if (status == DB_SUCCESS || !hKey)
        {
        db_get_key(hDB, hKey, &key);
        if (i > key.num_values)
          printf("Key has already less than %d values\n");
        else
          {
          size = sizeof(data);
          db_get_data(hDB, hKey, data, &size, key.type);
          db_set_data(hDB, hKey, data, i*key.item_size, i, key.type);
          }
        }
      else
        printf("key not found\n");
      }

    /* rename */
    else if (param[0][0] == 'r' && param[0][1] == 'e' && param[0][2] == 'n')
      {
      compose_name(pwd, param[1], str);

      if (strcmp(str, "/") != 0)
        status = db_find_link(hDB, 0, str, &hKey);
      else
        hKey = 0;

      if (status == DB_SUCCESS || !hKey)
        db_rename_key(hDB, hKey, param[2]);
      else
        printf("key not found\n");
      }

    /* move */
    else if (param[0][0] == 'm' && param[0][1] == 'o')
      {
      compose_name(pwd, param[1], str);

      if (strcmp(str, "/") != 0)
        status = db_find_link(hDB, 0, str, &hKey);
      else
        hKey = 0;

      if (status == DB_SUCCESS || !hKey)
        {
        if (param[2][0] == 't')
          i = 0;
        else if (param[2][0] == 'b')
          i = -1;
        else i = atoi(param[2]);

        status = db_reorder_key(hDB, hKey, i);
        if (status == DB_NO_ACCESS)
          printf("no write access to key\n");
        if (status == DB_OPEN_RECORD)
          printf("key is open by other client\n");
        }
      else
        printf("key not found\n");
      }

    /* find key */
    else if (param[0][0] == 'f' && param[0][1] == 'i')
      {
      status = db_find_key(hDB, 0, pwd, &hKey);

      if (status == DB_SUCCESS)
        db_scan_tree(hDB, hKey, 0, search_key, (void *) param[1]);
      else
        printf("current key is invalid / no read access\n");
      }

    /* load */
    else if (param[0][0] == 'l' && param[0][1] == 'o')
      {
      db_find_key(hDB, 0, pwd, &hKey);
      db_load(hDB, hKey, param[1], FALSE);
      }

    /* save */
    else if (param[0][0] == 's' && param[0][1] == 'a')
      {
      db_find_key(hDB, 0, pwd, &hKey);
      
      if (param[1][0] == '-')
        {
        if (param[1][1] == 'c')
          db_save_struct(hDB, hKey, param[2], NULL, FALSE);
        if (param[1][1] == 's')
          db_save_string(hDB, hKey, param[2], NULL, FALSE);
        }
      else
        db_save(hDB, hKey, param[1], FALSE);
      }

    /* make */
    else if (param[0][0] == 'm' && param[0][1] == 'a')
      {
      create_experim_h(hDB);
      }

    /* passwd */
    else if (param[0][0] == 'p' && param[0][1] == 'a' && param[0][2] == 's')
      {
      /*
      strcpy(str, ss_crypt("foob", "ar"));
      if(strcmp(str, "arlEKn0OzVJn.") != 0)
        printf("Warning: ss_crypt() works incorrect");
      */

      if (db_find_key(hDB, 0, "/Experiment/Security/Password", &hKey) == DB_SUCCESS)
        {
        size = sizeof(old_password);
        db_get_data(hDB, hKey, old_password, &size, TID_STRING);

        strcpy(str, ss_getpass("Old password: "));
        strcpy(str, ss_crypt(str, "mi"));

        if (strcmp(str, old_password) == 0 || strcmp(str, "mid7qBxsNMHux") == 0)
          {
          strcpy(str, ss_getpass("New password: "));
          strcpy(new_password, ss_crypt(str, "mi"));

          strcpy(str, ss_getpass("Retype new password: "));
          if (strcmp(new_password, ss_crypt(str, "mi")) != 0)
            printf("Mismatch - password unchanged\n");
          else
            db_set_data(hDB, hKey, new_password, 32, 1, TID_STRING);
          }
        else
          printf("Wrong password\n");
        }
      else
        {
        strcpy(str, ss_getpass("Password: "));
        strcpy(new_password, ss_crypt(str, "mi"));

        strcpy(str, ss_getpass("Retype password: "));
        if (strcmp(new_password, ss_crypt(str, "mi")) != 0)
          printf("Mismatch - password not set\n");
        else
          {
          /* set password */
          db_set_value(hDB, 0, "/Experiment/Security/Password", new_password, 32, 1, TID_STRING);

          /* create empty rhost entry */
          db_create_key(hDB, 0, "/Experiment/Security/rhosts/host.sample.domain", TID_INT);
          }
        }
        
      }
    

    /* hi */
    else if (param[0][0] == 'h' && param[0][1] == 'i')
      {
      HNDLE hConn;

      client_name[0] = 0;

      if (!isalpha(param[1][0]))
        {
        /* find client which exports RPC_ANA_CLEAR_HISTOS */
        status = db_find_key(hDB, 0, "System/Clients", &hRootKey);
        if (status == DB_SUCCESS)
          {
          for (i=0; ; i++)
            {
            status = db_enum_key(hDB, hRootKey, i, &hSubkey);
            if (status == DB_NO_MORE_SUBKEYS)
              {
              printf("No client currently exports the CLEAR HISTO functionality.\n");
              break;
              }

            sprintf(str, "RPC/%d", RPC_ANA_CLEAR_HISTOS);
            status = db_find_key(hDB, hSubkey, str, &hKey); 
            if (status == DB_SUCCESS)
              {
              size = sizeof(client_name);
              db_get_value(hDB, hSubkey, "Name", client_name, &size, TID_STRING);
              break;
              }
            }
          }

        if (isdigit(param[1][0]))
          n1 = atoi(param[1]);
        else
          n1 = 0; /* all histos by default */

        if (isdigit(param[2][0]))
          n2 = atoi(param[2]);
        else
          n2 = n1; /* single histo by default */
        }
      else
        {
        strcpy(client_name, param[1]);

        if (isdigit(param[2][0]))
          n1 = atoi(param[2]);
        else
          n1 = 0; /* all histos by default */

        if (isdigit(param[3][0]))
          n2 = atoi(param[3]);
        else
          n2 = n1; /* single histo by default */
        }

      if (client_name[0])
        {
        if (cm_connect_client(client_name, &hConn) == CM_SUCCESS)
          {
          rpc_client_call(hConn, RPC_ANA_CLEAR_HISTOS, n1, n2);
          cm_disconnect_client(hConn, FALSE);
          }
        else
          printf("Cannot connect to client %s\n", client_name);
        }
      }
    
    /* rewind */
    else if (param[0][0] == 'r' && param[0][1] == 'e' && param[0][2] == 'w')
      {
      HNDLE hConn;

      /* check if run is stopped and no transition in progress */
      size = sizeof(i);
      db_get_value(hDB, 0, "/Runinfo/State", &i, &size, TID_INT);
      db_get_value(hDB, 0, "/Runinfo/Transition in progress", &j, &size, TID_INT);
      if (i != STATE_STOPPED || j == 1)
        {
        printf("Cannot rewind tapes when run is not stopped or transition in progress\n");
        }
      else
        {
        if (param[1][0] == 0)
          channel = -1;
        else
          channel = atoi(param[1]);

        /* find client which exports RPC_LOG_REWIND */
        status = db_find_key(hDB, 0, "System/Clients", &hRootKey);
        if (status == DB_SUCCESS)
          {
          for (i=0 ; ; i++)
            {
            status = db_enum_key(hDB, hRootKey, i, &hSubkey);
            if (status == DB_NO_MORE_SUBKEYS)
              {
              printf("No active logger found.\n");
              break;
              }

            sprintf(str, "RPC/%d", RPC_LOG_REWIND);
            status = db_find_key(hDB, hSubkey, str, &hKey); 
            if (status == DB_SUCCESS)
              {
              size = sizeof(client_name);
              db_get_value(hDB, hSubkey, "Name", client_name, &size, TID_STRING);
              break;
              }
            }
          }

        if (status != DB_NO_MORE_SUBKEYS)
          {
          if (cm_connect_client(client_name, &hConn) == CM_SUCCESS)
            {
            /* increase timeout */
            rpc_set_option(hConn, RPC_OTIMEOUT, 600000);

            if (channel == -1)
              printf("Rewinding all channels...\n");
            else
              printf("Rewinding channel %d...\n");

            status = rpc_client_call(hConn, RPC_LOG_REWIND, channel);
            cm_disconnect_client(hConn, FALSE);

            if (status == CM_SUCCESS)
              printf("Rewind successful\n");
            else
              printf("Rewind error, code = %d\n", status);
            }
          else
            printf("Cannot connect to logger\n");
          }
        }
      }

    /* mem */
    else if (param[0][0] == 'm' && param[0][1] == 'e')
      {
      if (rpc_is_remote())
        printf("This function works only locally\n");
      else
        {
        db_show_mem(hDB, data, sizeof(data));
        printf(data);
        }
      }

    /* sor (show open records) */
    else if (param[0][0] == 's' && param[0][1] == 'o')
      {
      db_find_key(hDB, 0, pwd, &hKey);
      db_get_open_records(hDB, hKey, data, sizeof(data));
      printf(data);
      }

    /* scl (show clients ) */
    else if (param[0][0] == 's' && param[0][1] == 'c')
      {
      status = db_find_key(hDB, 0, "System/Clients", &hKey);
      if (status != DB_SUCCESS)
        cm_msg(MERROR, "command_loop", "cannot find System/Clients entry in database");
      else
        {
        if (param[1][1] == 'w')
          printf("Name                Host                Timeout    Last called\n");
        else
          printf("Name                Host\n");

        /* search database for clients with transition mask set */
        for (i=0,status=0 ; ; i++)
          {
          status = db_enum_key(hDB, hKey, i, &hSubkey);
          if (status == DB_NO_MORE_SUBKEYS)
            break;

          if (status == DB_SUCCESS)
            {
            size = sizeof(name);
            db_get_value(hDB, hSubkey, "Name", name, &size, TID_STRING);
            printf(name);
            for (j=0 ; j<20-(int)strlen(name) ; j++)
              printf(" ");

            size = sizeof(str);
            db_get_value(hDB, hSubkey, "Host", str, &size, TID_STRING);
            printf(str);
            for (j=0 ; j<20-(int)strlen(str) ; j++)
              printf(" ");

            /* display optional watchdog info */
            if (param[1][1] == 'w')
              {
              INT timeout, last;

              status = cm_get_watchdog_info(hDB, name, &timeout, &last);
              printf("%-10d %-10d", timeout, last);
              }

            printf("\n");
            }
          }
        }
      }
 
    /* start */
    else if (param[0][0] == 's' && param[0][1] == 't' && param[0][2] == 'a')
      {
      /* check if run is already started */
      size = sizeof(i);
      i = STATE_STOPPED;
      db_get_value(hDB, 0, "/Runinfo/State", &i, &size, TID_INT);
      if (i == STATE_RUNNING)
        {
        printf("Run is already started\n");
        }
      else if (i == STATE_PAUSED)
        {
        printf("Run is paused, please use \"resume\"\n");
        }
      else
        {
        /* check if transition in progress */
        i = 0;
        db_get_value(hDB, 0, "/Runinfo/Transition in progress", &i, &size, TID_INT);
        if (i == 1)
          {
          printf("Start/Stop already in progress, please try again later\n");
          printf("or set \"/Runinfo/Transition in progress\" manually to zero.\n");
          }
        else
          {
          /* get present run number */
          old_run_number = 0;
          db_get_value(hDB, 0, "/Runinfo/Run number", &old_run_number, &size, TID_INT);

          /* edit run parameter */
          db_find_key(hDB, 0, "/Experiment/Edit on start", &hKey);
          do
            {
            if (hKey)
              {
              for (i=0 ; ; i++)
                {
                db_enum_link(hDB, hKey, i, &hSubkey);

                if (!hSubkey)
                  break;

                db_get_key(hDB, hSubkey, &key);
                strcpy(str, key.name);

                db_enum_key(hDB, hKey, i, &hSubkey);
                db_get_key(hDB, hSubkey, &key);

                size = sizeof(data);
                status = db_get_data(hDB, hSubkey, data, &size, key.type);
                if (status != DB_SUCCESS)
                  continue;

                for (j=0 ; j<key.num_values ; j++)
                  {
                  db_sprintf(data_str, data, key.item_size, j, key.type);
                  sprintf(prompt, "%s : ", str);

                  strcpy(line, data_str);
                  in_cmd_edit = TRUE;
                  cmd_edit(prompt, line, NULL, cmd_idle);
                  in_cmd_edit = FALSE;

                  if (line[0])
                    {
                    db_sscanf(line, data, &size, j, key.type);
                    db_set_data_index(hDB, hSubkey, data, size+1, j, key.type); 
                    }
                  }
                }
              }

            /* increment run number */
            new_run_number = old_run_number+1;
            printf("Run number [%d]: ", new_run_number);
            ss_gets(line, 256);
            if (line[0])
              new_run_number = atoi(line);

            printf("Are the above parameters correct? ([y]/n/q): ");
            ss_gets(line, 256);

            } while (line[0] == 'n' || line[0] == 'N');


          if (line[0] != 'q' && line[0] != 'Q')
            {
            /* start run */
            printf("Starting run #%d\n", new_run_number);

            i = 1;
            db_set_value(hDB, 0, "/Runinfo/Transition in progress", &i, sizeof(INT), 1, TID_INT);
         
            status = cm_transition(TR_START, new_run_number, str, sizeof(str), SYNC);
            if (status != CM_SUCCESS)
              {
              /* in case of error, reset run number */
              db_set_value(hDB, 0, "/Runinfo/Run number", &old_run_number, size, 1, TID_INT);

              printf("Error: %s\n", str);
              }

            i = 0;
            db_set_value(hDB, 0, "/Runinfo/Transition in progress", &i, sizeof(INT), 1, TID_INT);
            }
          }
        }
      }

    /* stop */
    else if (param[0][0] == 's' && param[0][1] == 't' && param[0][2] == 'o')
      {
      /* check if transition in progress */
      i = 0;
      size = sizeof(INT);
      db_get_value(hDB, 0, "/Runinfo/Transition in progress", &i, &size, TID_INT);
      if (i == 1)
        {
        printf("Start/Stop already in progress, please try again later\n");
        printf("or set \"/Runinfo/Transition in progress\" manually to zero.\n");
        }
      else
        {
        /* check if run is stopped */
        state = STATE_STOPPED;
        size = sizeof(i);
        db_get_value(hDB, 0, "/Runinfo/State", &state, &size, TID_INT);
        if (state == STATE_STOPPED)
          {
          printf("Run is already stopped. Stop again? (y/[n]) ");
          i = getchar();
          }
        if (i == 'y' || state != STATE_STOPPED)
          {
          i = 1;
          db_set_value(hDB, 0, "/Runinfo/Transition in progress", &i, sizeof(INT), 1, TID_INT);
          status = cm_transition(TR_STOP, 0, str, sizeof(str), SYNC);
          if (status != CM_SUCCESS)
            printf("Error: %s\n", str);
          i = 0;
          db_set_value(hDB, 0, "/Runinfo/Transition in progress", &i, sizeof(INT), 1, TID_INT);
          }
        }
      }
    
    /* pause */
    else if (param[0][0] == 'p' && param[0][1] == 'a' && param[0][2] == 'u')
      {
      /* check if run is started */
      i = STATE_STOPPED;
      size = sizeof(i);
      db_get_value(hDB, 0, "/Runinfo/State", &i, &size, TID_INT);
      if (i != STATE_RUNNING)
        {
        printf("Run is not started\n");
        }
      else
        {
        status = cm_transition(TR_PAUSE, 0, str, sizeof(str), SYNC);
        if (status != CM_SUCCESS)
          printf("Error: %s\n", str);
        }
      }
    
    /* resume */
    else if (param[0][0] == 'r' && param[0][1] == 'e' && param[0][2] == 's')
      {
      /* check if run is paused */
      i = STATE_STOPPED;
      size = sizeof(i);
      db_get_value(hDB, 0, "/Runinfo/State", &i, &size, TID_INT);
      if (i != STATE_PAUSED)
        {
        printf("Run is not paused\n");
        }
      else
        {
        status = cm_transition(TR_RESUME, 0, str, sizeof(str), SYNC);
        if (status != CM_SUCCESS)
          printf("Error: %s\n", str);
        }
      }

    /* msg */
    else if (param[0][0] == 'm' && param[0][1] == 's')
      {
      if (cm_exist("Speaker", FALSE) != CM_SUCCESS)
        printf("Warning: No speaker application present.\n");

      message[0] = 0;
      if (param[2][0])
        {
        last_msg_time = ss_time();
        strcpy(user_name, param[1]);
        strcpy(message, param[2]);
        }
      else if (param[1][0])
        strcpy(message, param[1]);

      if (ss_time() - last_msg_time > 300 )
        {
        printf("Your name> ");
        ss_gets(user_name, 80);
        }

      if (param[1][0] == 0)
        {
        printf("Msg> ");
        ss_gets(message, 256);
        }

      if (message[0])
        cm_msg(MUSER, user_name, message);

      last_msg_time = ss_time();
      }
    
    /* chat */
    else if (param[0][0] == 'c' && param[0][1] == 'h' && param[0][2] == 'a')
      {
      message[0] = 0;

      if (ss_time() - last_msg_time > 300 )
        {
        printf("Your name> ");
        ss_gets(user_name, 80);
        }

      printf("Exit chat mode with empty line.\n");
      do
        {
        in_cmd_edit = TRUE;
        message[0] = 0;
        cmd_edit("> ", message, NULL, cmd_idle);
        in_cmd_edit = FALSE;

        if (message[0])
          cm_msg(MUSER, user_name, message);

        } while(message[0]);

      last_msg_time = ss_time();
      }

    /* cleanup */
    else if (param[0][0] == 'c' && param[0][1] == 'l')
      {
      HNDLE hBuf;
      bm_open_buffer(EVENT_BUFFER_NAME, EVENT_BUFFER_SIZE, &hBuf);
      cm_cleanup();
      bm_close_buffer(hBuf);
      }

    /* shutdown */
    else if (param[0][0] == 's' && param[0][1] == 'h')
      {
      if (param[1][0] == 0)
        printf("Please enter client name or \"all\" to shutdown all clients.\n");
      else
        {
        status = cm_shutdown(param[1]);
        if (status != CM_SUCCESS)
          {
          if (strcmp(param[1], "all") == 0)
            printf("No clients found\n");
          else
            printf("Client %s not active\n", param[1]);
          }
        }
      }

    /* ver */
    else if (param[0][0] == 'v' && param[0][1] == 'e')
      {
      printf("MIDAS library version %s\n", cm_get_version());
      }

    /* exec */
    else if (param[0][0] == 'e' && param[0][1] == 'x' && param[0][2] == 'e')
      {
      compose_name(pwd, param[1], str);

      status = db_find_key(hDB, 0, str, &hKey);
      if (status == DB_SUCCESS)
        {
        db_get_key(hDB, hKey, &key);
        if (key.type != TID_STRING)
          {
          printf("Key contains no command\n");
          continue;
          }
        size = sizeof(str);
        db_get_data(hDB, hKey, str, &size, key.type);

        cm_execute(str, data, sizeof(data));
        }
      else
        {
        cm_execute(param[1], data, sizeof(data));
        }
      puts(data);
      }

    /* test 1  */
    else if (param[0][0] == 't' && param[0][1] == '1')
      {
      DWORD start_time;
      INT   i, size, rn;
      HNDLE hKey;

      start_time = ss_millitime();
      db_find_key(hDB, 0, "/runinfo/run number", &hKey);
      size = sizeof(rn);

      i = 0;
      do
        {
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
        i+= 10;
        } while (ss_millitime()-start_time < 5000);

      printf("%d accesses per second\n", i/5);
      }

    /* test 2 */
    else if (param[0][0] == 't' && param[0][1] == '2')
      {
      do
        {
        do
          {
          i = ss_getchar(0);
          printf("%d\n", i);
          } while (i>0 && i != 4);

        ss_sleep(1000);
        } while (i != 4);

      ss_getchar(1);
      }

    /* test 3 */
    else if (param[0][0] == 't' && param[0][1] == '3')
      {
      db_find_key(hDB, 0, "/Equipment/HV/Variables/Demand", &hKey);
      if (hKey)
        {
        db_get_key(hDB, hKey, &key);
        db_open_record(hDB, hKey, test_array, key.total_size, MODE_READ, update_test, NULL);
        }
      else
        printf("No /Equipment/HV/Variables/Demand found\n");
      }

    /* test 4 */
    else if (param[0][0] == 't' && param[0][1] == '4')
      {
      db_find_key(hDB, 0, "/Equipment/HV/Variables/Demand", &hKey);
      if (hKey)
        {
        for (i=0 ; i<10 ; i++)
          db_set_data_index2(hDB, hKey, &test_array[i], sizeof(float), i, TID_FLOAT, i==9);
        }
      }

    /* exit/quit */
    else if ((param[0][0] == 'e' && param[0][1] == 'x' && param[0][2] == 'i') ||
        (param[0][0] == 'q'))
      break;

    else if (param[0][0] == 0);

    else
      printf("Unknown command %s %s %s\n", param[0], param[1], param[2]);

    /* exit after single command */
    if (cmd[0] && cmd[0] != '@')
      break;

    /* check if client connections are broken */
    status = cm_yield(0);
    if (status == SS_ABORT)
      break;

    } while (TRUE);

  /* check if client connections are broken */
  for (i=0 ; i < MAX_RPC_CONNECTION ; i++)
    cm_yield(0);
  
  return;
}

/*------------------------------------------------------------------*/

#ifdef OS_VXWORKS
odbedit(char *ahost_name, char *aexp_name)
#else
main(int argc, char *argv[])
#endif
{
INT           status, i, odb_size, size;
char          host_name[100], exp_name[NAME_LENGTH];
char          cmd[100], dir[100], str[256];
BOOL          debug;
HNDLE         hDB;

  cmd[0] = dir[0] = 0;
  odb_size = DEFAULT_ODB_SIZE;
  debug = FALSE;

#ifdef OS_VXWORKS
  strcpy(host_name, ahost_name);
  strcpy(exp_name, aexp_name);
#else

  /* get default from environment */
  cm_get_environment(host_name, exp_name);

  /* parse command line parameters */
  for (i=1 ; i<argc ; i++)
    {
    if (argv[i][0] == '-' && argv[i][1] == 'g')
      debug = TRUE;
    else if (argv[i][0] == '-')
      {
      if (i+1 >= argc || argv[i+1][0] == '-')
        goto usage;
      if (argv[i][1] == 'e')
        strcpy(exp_name, argv[++i]);
      else if (argv[i][1] == 'h')
        strcpy(host_name, argv[++i]);
      else if (argv[i][1] == 'c')
        strcpy(cmd, argv[++i]);
      else if (argv[i][1] == 'd')
        strcpy(dir, argv[++i]);
      else if (argv[i][1] == 's')
        odb_size = atoi(argv[++i]);
      else
        {
usage:
        printf("usage: odbedit [-h Hostname] [-e Experiment] [-d ODB Subtree]\n");
        printf("               [-c Command] [-c @CommandFile] [-s size] [-g (debug)]\n\n");
        printf("For a list of valid commands start odbedit interactively\n");
        printf("and type \"help\".\n");
        return 0;
        }
      }
    else
      strcpy(host_name, argv[i]);
    }
#endif

  /* no startup message if called with command */
  if (cmd[0])
    cm_set_msg_print(MT_ERROR, 0, NULL);

  status = cm_connect_experiment1(host_name, exp_name, "ODBEdit", NULL, odb_size);
  if (status != CM_SUCCESS)
    {
    cm_get_error(status, str);
    puts(str);
    return 1;
    }

  /* place a request for system messages */
  cm_msg_register(process_message);

  /* route local messages through print_message */
  cm_set_msg_print(MT_ALL, MT_ERROR, print_message);

  /* turn off watchdog if in debug mode */
  if (debug)
    cm_set_watchdog_params(TRUE, 0);

  /* get experiment name */
  if (!exp_name[0])
    {
    cm_get_experiment_database(&hDB, NULL);
    size = NAME_LENGTH;
    db_get_value(hDB, 0, "/Experiment/Name", exp_name, &size, TID_STRING);
    }

  /* command loop */
  command_loop(host_name, exp_name, cmd, dir);

  /* no shutdown message if called with command */
  if (cmd[0])
    cm_set_msg_print(MT_ERROR, 0, NULL);

  cm_disconnect_experiment();

  return 0;
}
