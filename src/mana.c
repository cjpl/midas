/********************************************************************\

  Name:         mana.c
  Created by:   Stefan Ritt

  Contents:     The system part of the MIDAS analyzer. Has to be
                linked with analyze.c to form a complete analyzer

  $Log$
  Revision 1.13  1999/02/22 11:55:20  midas
  Fixed bug with rebooking of N-tuples

  Revision 1.12  1999/02/03 16:36:50  midas
  Added -f flag to filter events

  Revision 1.11  1999/02/01 15:47:47  midas
  - removed ASCII read function (only historical for pibeta)
  - analyze_file puts current offline run number into ODB

  Revision 1.10  1999/01/15 12:50:04  midas
  - Set /Analyzer/Bank switches/... to FALSE by default to avoid N-tuple
    overflow when an analyzer is started the first time
  - Assume MIDAS data format on /dev/... input

  Revision 1.9  1999/01/08 15:00:53  midas
  Analyzer does not stop if a file in a range of files is missing

  Revision 1.8  1998/12/16 11:07:41  midas
  - verbose output (via -v flag) for N-Tuple booking
  - error message if histo dump is on when running online

  Revision 1.7  1998/12/10 12:50:47  midas
  Program abort with "!" now works without a return under UNIX

  Revision 1.6  1998/12/10 05:28:18  midas
  Removed convert-only flag, added -v (verbose) flag

  Revision 1.5  1998/10/29 14:37:58  midas
  Fixed problem with '!' for stopping analyzer

  Revision 1.4  1998/10/22 07:11:15  midas
  *** empty log message ***

  Revision 1.3  1998/10/19 16:40:44  midas
  ASCII output of multi-module LRS1877 now possible

  Revision 1.2  1998/10/12 12:19:01  midas
  Added Log tag in header


\********************************************************************/

#include <stdio.h>
#include "midas.h"
#include "msystem.h"
#include "hardware.h"
#include "zlib.h"

/* cernlib includes */
#ifdef OS_WINNT
#define VISUAL_CPLUSPLUS
#endif

#ifdef OS_LINUX
#define f2cFortran
#endif

#include <cfortran.h>
#include <hbook.h>

/* missing in hbook.h */
#ifndef HFNOV
#define HFNOV(A1,A2)  CCALLSFSUB2(HFNOV,hfnov,INT,FLOATV,A1,A2) 
#endif

/*------------------------------------------------------------------*/

/* items defined in analyzer.c */

extern char *analyzer_name;
extern INT  analyzer_loop_period;
extern INT  analyzer_init(void);
extern INT  analyzer_exit(void);
extern INT  analyzer_loop(void);
extern INT  ana_begin_of_run(INT run_number, char *error);
extern INT  ana_end_of_run(INT run_number, char *error);
extern INT  ana_pause_run(INT run_number, char *error);
extern INT  ana_resume_run(INT run_number, char *error);
extern INT  odb_size;

/*---- globals -----------------------------------------------------*/

/* ODB handle */
HNDLE hDB;

/* run number */
DWORD current_run_number;

/* analyze_request defined in analyze.c or anasys.c */
extern ANALYZE_REQUEST analyze_request[];

/* N-tupel booking and filling flag */
BOOL ntuple_flag;

#ifdef extname
int quest_[100];
#else
extern int QUEST[100];
#endif

extern INT pawc_size;

/* HBOOK record size */
#define HBOOK_LREC 8190

/* command line parameters */
struct {
  INT   online;
  char  host_name[100];
  char  exp_name[NAME_LENGTH];
  char  input_file_name[10][256];
  char  output_file_name[256];
  INT   run_number[2];
  DWORD n[4];
  INT   filter[10];
  char  config_file_name[10][256];
  char  param[10][256];
  BOOL  rwnt;
  BOOL  debug;
  BOOL  verbose;
  BOOL  quiet;
} clp;

struct {
  char flag_char;
  char description[1000];
  void *data;
  INT  type;
  INT  n;
  INT  index;
} clp_descrip[] = {
  {'h', 
   "<hostname>    MIDAS host to connect to when running the analyzer online", 
   clp.host_name, TID_STRING, 1 },

  {'e', 
   "<experiment>  MIDAS experiment to connect to", 
   clp.exp_name, TID_STRING, 1 },
  
  {'i', 
   "<filename1>   Input file name. May contain a '%05d' to be replaced by\n\
     <filename2>   the run number. Up to ten input files can be specified\n\
         ...       in one \"-i\" statement", 
   clp.input_file_name, TID_STRING, 10 },

  {'o', 
   "<filename>    Output file name. Extension may be .mid (MIDAS binary),\n\
                   .asc (ASCII) or .rz (HBOOK). If the name contains a '%05d',\n\
                   one output file is generated for each run.", 
   clp.output_file_name, TID_STRING, 1 },

  {'r', 
   "<range>       Range of run numbers to analyzer like \"-r 120 125\"\n\
                   to analyze runs 120 to 125 (inclusive). The \"-r\"\n\
                   flag must be used with a '%05d' in the input file name.", 
   clp.run_number, TID_INT, 2 },

  {'n', 
   "<count>       Analyze only \"count\" events.\n\
     <first> <last>\n\
                   Analyze only events from \"first\" to \"last\".\n\
     <first> <last> <n>\n\
                   Analyze every n-th event from \"first\" to \"last\".",
   clp.n, TID_INT, 4 },

  {'f', 
   "<event ID1>     Filter events. Only write events to the output file\n\
     <event ID2>     if their event ID matches.\n\
         ...         ", 
   clp.filter, TID_INT, 10 },

  {'c', 
   "<filename1>   Configuration file name(s). May contain a '%05d' to be\n\
     <filename2>   replaced by the run number. Up to ten files can be\n\
         ...       specified in one \"-c\" statement", 
   clp.config_file_name, TID_STRING, 10 },

  {'p', 
   "<param=value> Set individual parameters to a specific value.\n\
                   Overrides any setting in configuration files",
   clp.param, TID_STRING, 10 },

  {'w', 
   "              Produce row-wise N-tuples in outpur .rz file. By\n\
                   default, column-wise N-tuples are used.", 
   &clp.rwnt, TID_BOOL, 0 },

  {'v', 
   "              Verbose output.",
   &clp.verbose, TID_BOOL, 0 },

  {'d', 
   "              Debug flag when started the analyzer fron a debugger.\n\
                   Prevents the system to kill the analyzer when the\n\
                   debugger stops at a breakpoint", 
   &clp.debug, TID_BOOL, 0 },

  {'q', 
   "              Quiet flag. If set, don't display run progress in offline mode.",
   &clp.quiet, TID_BOOL, 0 },

  { 0 }
};

/* output file information, maps to /<analyzer>/Output */
struct {
  char      filename[256];
  BOOL      rwnt;
  BOOL      histo_dump;
  char      histo_dump_filename[256];
  BOOL      clear_histos;
} out_info;

FILE *out_file;
BOOL  out_gzip;
INT   out_format;
BOOL  out_append;

/*---- ODB records -------------------------------------------------*/

#define ANALYZER_REQUEST_STR "\
Event ID = INT : 0\n\
Trigger mask = INT : -1\n\
Sampling type = INT : 1\n\
Buffer = STRING : [32] SYSTEM\n\
Enabled = BOOL : 1\n\
Client name = STRING : [32] \n\
Host = STRING : [32] \n\
"

#define ANALYZER_STATS_STR "\
Events received = DWORD : 0\n\
Events per sec. = DWORD : 0\n\
Events written = DWORD : 0\n\
"

#define OUT_INFO_STR "\
Filename = STRING : [256] run%05d.asc\n\
RWNT = BOOL : 0\n\
Histo Dump = BOOL : 0\n\
Histo Dump Filename = STRING : [256] his%05d.rz\n\
Clear histos = BOOL : 1\n\
"

/*-- interprete command line parameters ----------------------------*/

INT getparam(int argc, char **argv)
{
INT index, i, j, size;

  /* parse command line parameters */
  for (index=1 ; index<argc ; )
    {
    /* search flag in parameter description */
    if (argv[index][0] == '-')
      {
      for (j=0 ; clp_descrip[j].flag_char ; j++)
        if (argv[index][1] == clp_descrip[j].flag_char)
          break;

      if (!clp_descrip[j].flag_char)
        goto usage;

      if (clp_descrip[j].n > 0 && index >= argc-1)
        goto usage;
      index++;

      if (clp_descrip[j].type == TID_BOOL)
        {
        *((BOOL *) clp_descrip[j].data) = TRUE;
        continue;
        }

      do
        {
        if (clp_descrip[j].type == TID_STRING)
          strcpy((char *) clp_descrip[j].data + clp_descrip[j].index*256, argv[index]);
        else
          db_sscanf(argv[index], clp_descrip[j].data, 
                    &size, clp_descrip[j].index, clp_descrip[j].type);

        if (clp_descrip[j].n > 1)
          clp_descrip[j].index++;

        if (clp_descrip[j].index > clp_descrip[j].n)
          {
          printf("Note more than %d options possible for flag -%c\n", 
                 clp_descrip[j].n, clp_descrip[j].flag_char);
          return 0;
          }

        index++;

        } while(index < argc && argv[index][0] != '-');

      }
    else
      goto usage;
    }

  return SUCCESS;

usage:
  
  printf("usage: analyzer [options]\n\n");
  printf("valid options are:\n");
  for (i=0 ; clp_descrip[i].flag_char ; i++)
    printf("  -%c %s\n", clp_descrip[i].flag_char, clp_descrip[i].description);

  return 0;
}

/*-- db_get_event_definition ---------------------------------------*/

typedef struct {
  short int event_id;
  WORD      format;
  HNDLE     hDefKey;
  } EVENT_DEF;

EVENT_DEF *db_get_event_definition(short int event_id)
{
INT   i, index, status, size;
char  str[80];
HNDLE hKey, hKeyRoot;
WORD  id;

#define EVENT_DEF_CACHE_SIZE 30
static EVENT_DEF *event_def=NULL;

  /* allocate memory for cache */
  if (event_def == NULL)
    event_def = calloc(EVENT_DEF_CACHE_SIZE, sizeof(EVENT_DEF));

  /* lookup if event definition in cache */
  for (i=0 ; event_def[i].event_id ; i++)
    if (event_def[i].event_id == event_id)
      return &event_def[i];

  /* search free cache entry */
  for (index=0 ; index<EVENT_DEF_CACHE_SIZE ; index++)
    if (event_def[index].event_id == 0)
      break;

  if (index == EVENT_DEF_CACHE_SIZE)
    {
    cm_msg(MERROR, "db_get_event_definition", "too many event definitions");
    return NULL;
    }

  /* check for system events */
  if (event_id < 0)
    {
    event_def[index].event_id = event_id;
    event_def[index].format   = FORMAT_ASCII;
    event_def[index].hDefKey  = 0;
    return &event_def[index];
    }

  status = db_find_key(hDB, 0, "/equipment", &hKeyRoot);
  if (status != DB_SUCCESS)
    {
    cm_msg(MERROR, "db_get_event_definition", "cannot find /equipment entry in ODB");
    return NULL;
    }

  for (i=0 ; ; i++)
    {
    /* search for equipment with specific name */
    status = db_enum_key(hDB, hKeyRoot, i, &hKey);
    if (status == DB_NO_MORE_SUBKEYS)
      {
      sprintf(str, "Cannot find event id %d under /equipment", event_id);
      cm_msg(MERROR, "db_get_event_definition", str);
      return NULL;
      }

    size = sizeof(id);
    status = db_get_value(hDB, hKey, "Common/Event ID", &id, &size, TID_WORD);
    if (status != DB_SUCCESS)
      continue;

    if (id == event_id)
      {
      /* set cache entry */
      event_def[index].event_id = id;

      size = sizeof(str);
      str[0] = 0;
      db_get_value(hDB, hKey, "Common/Format", str, &size, TID_STRING);

      if (equal_ustring(str, "Fixed"))
        event_def[index].format = FORMAT_FIXED;
      else if (equal_ustring(str, "ASCII"))
        event_def[index].format = FORMAT_ASCII;
      else if (equal_ustring(str, "MIDAS"))
        event_def[index].format = FORMAT_MIDAS;
      else if (equal_ustring(str, "YBOS"))
        event_def[index].format = FORMAT_YBOS;
      else if (equal_ustring(str, "DUMP"))
        event_def[index].format = FORMAT_DUMP;
      else
        {
        cm_msg(MERROR, "db_get_event_definition", "unknown data format");
        event_def[index].event_id = 0;
        return NULL;
        }

      db_find_key(hDB, hKey, "Variables", &event_def[index].hDefKey);
      return &event_def[index];
      }
    }
}

/*-- load parameters specified on command line ---------------------*/

INT load_parameters(INT run_number)
{
INT   i, size, index, status;
HNDLE hkey;
char  file_name[256], str[80], value_string[80], param_string[80];
char  data[32];
KEY   key;

  /* loop over configutation file names */
  for (i=0 ; clp.config_file_name[i][0] && i<10 ; i++)
    {
    if (strchr(clp.config_file_name[i], '%') != NULL)
      sprintf(file_name, clp.config_file_name[i], run_number);
    else
      strcpy(file_name, clp.config_file_name[i]);

    /* load file under "/" */
    if (db_load(hDB, 0, file_name, FALSE) == DB_SUCCESS)
      printf("Configuration file \"%s\" loaded\n", file_name);
    }

  /* loop over parameters */
  for (i=0 ; clp.param[i][0] && i<10 ; i++)
    {
    if (strchr(clp.param[i], '=') == NULL)
      {
      printf("Error: parameter %s contains no value\n", clp.param[i]);
      }
    else
      {
      strcpy(value_string, strchr(clp.param[i], '=')+1);
      strcpy(param_string, clp.param[i]);
      *strchr(param_string, '=') = 0;

      index = 0;
      if (strchr(param_string, '[') != NULL)
        {
        index = atoi(strchr(param_string, '[')+1);
        *strchr(param_string, '[') = 0;
        }

      if (param_string[0] == '/')
        strcpy(str, param_string);
      else
        sprintf(str, "/%s/Parameters/%s", analyzer_name, param_string);
      db_find_key(hDB, 0, str, &hkey);
      if (hkey == 0)
        {
        printf("Error: cannot find parameter %s in ODB\n", str);
        }
      else
        {
        db_get_key(hDB, hkey, &key);
        db_sscanf(value_string, data, &size, 0, key.type);

        status = db_set_data_index(hDB, hkey, data, size, index, key.type);
        if (status == DB_SUCCESS)
          printf("Parameter %s changed to %s\n", str, value_string);
        else
          printf("Cannot change parameter %s\n", str);
        }
      }
    }

  /* let parameter changes propagate to modules */
  cm_yield(0);

  return SUCCESS;
}

/*-- book N-tuples from ODB bank structures ------------------------*/

char hbook_types[][8] = {
  "",
  ":U:8",  /* TID_BYTE      */
  ":I:8",  /* TID_SBYTE     */          
  ":I:8",  /* TID_CHAR      */          
  ":U:16", /* TID_WORD      */          
  ":I:16", /* TID_SHORT     */          
  ":U*4",  /* TID_DWORD     */          
  ":I*4",  /* TID_INT       */          
  ":I*4",  /* TID_BOOL      */          
  ":R*4",  /* TID_FLOAT     */          
  ":R*8",  /* TID_DOUBLE    */          
  ":U:8",  /* TID_BITFIELD  */          
  ":C:32", /* TID_STRING    */          
  "",      /* TID_ARRAY     */     
  "",      /* TID_STRUCT    */     
  "",      /* TID_KEY       */     
  "",      /* TID_LINK      */     
  "",      /* TID_LAST      */     

};

INT book_ntuples(void);

void banks_changed(INT hDB, INT hKey, void *info)
{
char  str[80];
HNDLE hkey;

  /* close previously opened hot link */
  sprintf(str, "/%s/Bank switches", analyzer_name);
  db_find_key(hDB, 0, str, &hkey);
  db_close_record(hDB, hkey);

  book_ntuples();
  printf("N-tuples rebooked\n");
}

INT book_ntuples(void)
{
INT        index, i, j, status, n_tag, size, id;
HNDLE      hkey;
KEY        key;
char       ch_tags[2000];
char       rw_tag[512][8];
char       str[80], key_name[NAME_LENGTH], block_name[NAME_LENGTH];
BANK_LIST  *bank_list;
EVENT_DEF  *event_def;

  /* check global N-tuple flag */
  ntuple_flag = 1;
  size = sizeof(ntuple_flag);
  sprintf(str, "/%s/Book N-tuples", analyzer_name);
  db_get_value(hDB, 0, str, &ntuple_flag, &size, TID_BOOL);

  if (!ntuple_flag)
    return SUCCESS;

  /* copy output flag from ODB to bank_list */
  for (i=0 ; analyze_request[i].event_name[0] ; i++)
    {
    bank_list = analyze_request[i].bank_list;

    if (bank_list != NULL)
      for (; bank_list->name[0] ; bank_list++)
        {
        sprintf(str, "/%s/Bank switches/%s", analyzer_name, bank_list->name);
        bank_list->output_flag = FALSE;
        size = sizeof(DWORD);
        db_get_value(hDB, 0, str, &bank_list->output_flag, &size, TID_DWORD); 
        }
    }

  /* hot link bank switches to N-tuple re-booking */
  sprintf(str, "/%s/Bank switches", analyzer_name);
  db_find_key(hDB, 0, str, &hkey);
  db_open_record(hDB, hkey, NULL, 0, MODE_READ, banks_changed, NULL);

  if (!clp.rwnt)
    {
    /* book CW N-tuples */

    /* go through all analyzer requests (events) */
    for (index=0 ; analyze_request[index].event_name[0] ; index++)
      {
      /* book N-tuple with evend ID */
      HBNT(analyze_request[index].ar_info.event_id, analyze_request[index].event_name, " ");

      /* book run number/event number/time */
      HBNAME(analyze_request[index].ar_info.event_id, "Number", 
             (int *)&analyze_request[index].number, "Run:U*4,Number:U*4,Time:U*4");

      bank_list = analyze_request[index].bank_list;
      if (bank_list == NULL)
        {
        /* book fixed event */
        event_def = db_get_event_definition((short int) analyze_request[index].ar_info.event_id);
        if (event_def == NULL)
          {
          cm_msg(MERROR, "book_ntuples", "Cannot find definition of event %s in ODB", 
                 analyze_request[index].event_name);
          return 0;
          }

        ch_tags[0] = 0;
        for (i=0 ; ; i++)
          {
          status = db_enum_key(hDB, event_def->hDefKey, i, &hkey);
          if (status == DB_NO_MORE_SUBKEYS)
            break;

          db_get_key(hDB, hkey, &key);

          /* convert blanks etc to '_' */
          strcpy(str, key.name);
          for (j=0 ; str[j] ; j++)
            {
            if (!(str[j] >= 'a' && str[j] <= 'z') &&
                !(str[j] >= 'A' && str[j] <= 'Z') &&
                !(str[j] >= '0' && str[j] <= '9'))
              str[j] = '_';
            }
          strcat(ch_tags, str);
          str[0] = 0;

          if (key.num_values > 1)
            sprintf(str, "(%d)", key.num_values);

          if (hbook_types[key.type] != NULL)
            strcat(str, hbook_types[key.type]);
          else
            {
            cm_msg(MERROR, "book_ntuples", "Key %s in event %s is of type %s with no HBOOK correspondence", 
                            key.name, analyze_request[index].event_name, rpc_tid_name(key.type));
            return 0;
            }
          strcat(ch_tags, str);
          strcat(ch_tags, ",");
          }

        ch_tags[strlen(ch_tags)-1] = 0;
        db_get_record_size(hDB, event_def->hDefKey, 0, &size);
        analyze_request[index].addr = calloc(1, size);
  
        strcpy(block_name, analyze_request[index].event_name);
        block_name[8] = 0;

        HBNAME(analyze_request[index].ar_info.event_id, block_name, 
               analyze_request[index].addr, ch_tags);
        }
      else
        {
        /* go thorough all banks in bank_list */
        for (; bank_list->name[0] ; bank_list++)
          {
          if (bank_list->output_flag == 0)
            continue;

          if (bank_list->type != TID_STRUCT)
            {
            sprintf(str, "N%s[0,%d]", bank_list->name, bank_list->size);
            HBNAME(analyze_request[index].ar_info.event_id, 
                   bank_list->name, (INT *) &bank_list->n_data, str);

            sprintf(str, "%s(N%s)", bank_list->name, bank_list->name);

            /* define variable length array */
            if (hbook_types[bank_list->type] != NULL)
              strcat(str, hbook_types[bank_list->type]);
            else
              {
              cm_msg(MERROR, "book_ntuples", "Bank %s is of type %s with no HBOOK correspondence", 
                              bank_list->name, rpc_tid_name(bank_list->type));
              return 0;
              }
                            
            if (rpc_tid_size(bank_list->type) == 0)
              {
              cm_msg(MERROR, "book_ntuples", "Bank %s is of type with unknown size", bank_list->name);
              return 0;
              }

            bank_list->addr = calloc(bank_list->size, max(4, rpc_tid_size(bank_list->type)));
      
            HBNAME(analyze_request[index].ar_info.event_id, 
                   bank_list->name, bank_list->addr, str);
            }
          else
            {
            /* define structured bank */
            ch_tags[0] = 0;
            for (i=0 ; ; i++)
              {
              status = db_enum_key(hDB, bank_list->def_key, i, &hkey);
              if (status == DB_NO_MORE_SUBKEYS)
                break;

              db_get_key(hDB, hkey, &key);

              /* convert blanks etc to '_' */
              strcpy(str, key.name);
              for (j=0 ; str[j] ; j++)
                {
                if (!(str[j] >= 'a' && str[j] <= 'z') &&
                    !(str[j] >= 'A' && str[j] <= 'Z') &&
                    !(str[j] >= '0' && str[j] <= '9'))
                  str[j] = '_';
                }
              strcat(ch_tags, str);
              str[0] = 0;

              if (key.num_values > 1)
                sprintf(str, "(%d)", key.num_values);

              if (hbook_types[key.type] != NULL)
                strcat(str, hbook_types[key.type]);
              else
                {
                cm_msg(MERROR, "book_ntuples", "Key %s in bank %s is of type %s with no HBOOK correspondence", 
                                key.name, bank_list->name, rpc_tid_name(key.type));
                return 0;
                }
              strcat(ch_tags, str);
              strcat(ch_tags, ",");
              }

            ch_tags[strlen(ch_tags)-1] = 0;
            bank_list->addr = calloc(1, bank_list->size);
      
            HBNAME(analyze_request[index].ar_info.event_id, 
                   bank_list->name, bank_list->addr, ch_tags);
            }
          }
        }

      /* HPRNT(analyze_request[index].ar_info.event_id); */
      }
    }
  else
    {
    /* book RW N-tuples */

    /* go through all analyzer requests (events) */
    for (index=0 ; analyze_request[index].event_name[0] ; index++)
      {
      /* don't book NT if not requested */
      if (analyze_request[index].rwnt_buffer_size == 0)
        continue;

      n_tag = 0;
    
      strcpy(rw_tag[n_tag++], "Run");
      strcpy(rw_tag[n_tag++], "Number");
      strcpy(rw_tag[n_tag++], "Time");

      if (clp.verbose)
        {
        printf("NT #%d-1: Run\n", analyze_request[index].ar_info.event_id);
        printf("NT #%d-2: Number\n", analyze_request[index].ar_info.event_id);
        printf("NT #%d-3: Time\n", analyze_request[index].ar_info.event_id);
        }

      bank_list = analyze_request[index].bank_list;
      if (bank_list == NULL)
        {
        /* book fixed event */
        event_def = db_get_event_definition((short int) analyze_request[index].ar_info.event_id);

        for (i=0 ; ; i++)
          {
          status = db_enum_key(hDB, event_def->hDefKey, i, &hkey);
          if (status == DB_NO_MORE_SUBKEYS)
            break;

          db_get_key(hDB, hkey, &key);

          /* convert blanks etc to '_' */
          strcpy(key_name, key.name);
          for (j=0 ; key_name[j] ; j++)
            {
            if (!(key_name[j] >= 'a' && key_name[j] <= 'z') &&
                !(key_name[j] >= 'A' && key_name[j] <= 'Z') &&
                !(key_name[j] >= '0' && key_name[j] <= '9'))
              key_name[j] = '_';
            }

          if (key.num_values > 1)
            for (j=0 ; j<key.num_values ; j++)
              {
              sprintf(str, "%s%d", key_name, j);
              strncpy(rw_tag[n_tag++], str, 8);

              if (clp.verbose)
                printf("NT #%d-%d: %s\n", analyze_request[index].ar_info.event_id, 
                                          n_tag+1, str);

              if (n_tag >= 512)
                {
                cm_msg(MERROR, "book_ntuples", "Too much tags for RW N-tupeles (512 maximum)");
                return 0;
                }
              }
          else
            {
            strncpy(rw_tag[n_tag++], key_name, 8);
          
            if (clp.verbose)
              printf("NT #%d-%d: %s\n", analyze_request[index].ar_info.event_id, 
                                        n_tag, key_name);
            }
          
          if (n_tag >= 512)
            {
            cm_msg(MERROR, "book_ntuples", "Too much tags for RW N-tupeles (512 maximum)");
            return 0;
            }
          }
        }
      else
        {
        /* go through all banks in bank_list */
        for (; bank_list->name[0] ; bank_list++)
          {
          /* remember tag offset in n_data variable */
          bank_list->n_data = n_tag;

          if (bank_list->output_flag == 0)
            continue;

          if (bank_list->type != TID_STRUCT)
            {
            for (i=0 ; i<(INT)bank_list->size ; i++)
              {
              sprintf(str, "%s%d", bank_list->name, i);
              strncpy(rw_tag[n_tag++], str, 8);

              if (clp.verbose)
                printf("NT #%d-%d: %s\n", analyze_request[index].ar_info.event_id, 
                                          n_tag, str);
              if (n_tag >= 512)
                {
                cm_msg(MERROR, "book_ntuples", "Too much tags for RW N-tupeles (512 maximum)");
                return 0;
                }
              }
            }
          else
            {
            /* define structured bank */
            for (i=0 ; ; i++)
              {
              status = db_enum_key(hDB, bank_list->def_key, i, &hkey);
              if (status == DB_NO_MORE_SUBKEYS)
                break;

              db_get_key(hDB, hkey, &key);

              /* convert blanks etc to '_' */
              strcpy(key_name, key.name);
              for (j=0 ; key_name[j] ; j++)
                {
                if (!(key_name[j] >= 'a' && key_name[j] <= 'z') &&
                    !(key_name[j] >= 'A' && key_name[j] <= 'Z') &&
                    !(key_name[j] >= '0' && key_name[j] <= '9'))
                  key_name[j] = '_';
                }

              if (key.num_values > 1)
                for (j=0 ; j<key.num_values ; j++)
                  {
                  sprintf(str, "%s%d", key_name, j);
                  strncpy(rw_tag[n_tag++], str, 8);

                  if (clp.verbose)
                    printf("NT #%d-%d: %s\n", analyze_request[index].ar_info.event_id, 
                                              n_tag, str);
                  
                  if (n_tag >= 512)
                    {
                    cm_msg(MERROR, "book_ntuples", "Too much tags for RW N-tupeles (512 maximum)");
                    return 0;
                    }
                  }
              else
                {
                strncpy(rw_tag[n_tag++], key_name, 8);
                if (clp.verbose)
                  printf("NT #%d-%d: %s\n", analyze_request[index].ar_info.event_id, 
                                            n_tag, key_name);
                }
              
              if (n_tag >= 512)
                {
                cm_msg(MERROR, "book_ntuples", "Too much tags for RW N-tupeles (512 maximum)");
                return 0;
                }
              }
            }
          }
        }

      /* book N-tuple with evend ID */
      strcpy(block_name, analyze_request[index].event_name);
      block_name[8] = 0;

      id = analyze_request[index].ar_info.event_id;
      if (HEXIST(id))
        HDELET(id);

      if (clp.online)
        HBOOKN(id, block_name,n_tag, " ", 
               n_tag*analyze_request[index].rwnt_buffer_size, rw_tag);
      else
        HBOOKN(id, block_name, n_tag, "//OFFLINE", 5120, rw_tag);

      if (!HEXIST(id))
        {
        printf("\n");
        cm_msg(MINFO, "book_ntuples", "Cannot book N-tuple #%d. Increase PAWC size via the -s flag or switch off banks", id);
        }
      }
    }

  return SUCCESS;
}

/*-- analyzer init routine -----------------------------------------*/

INT mana_init()
{
ANA_MODULE **module;
INT        i, j, status, size;
HNDLE      hkey;
char       str[256], block_name[32];
BANK_LIST  *bank_list;
double     dummy;

  /* create ODB structure for output */
  sprintf(str, "/%s/Output", analyzer_name);
  db_create_record(hDB, 0, str, OUT_INFO_STR);
  db_find_key(hDB, 0, str, &hkey);
  size = sizeof(out_info);
  db_get_record(hDB, hkey, &out_info, &size, 0);
  
  if (clp.online)
    {
    status = db_open_record(hDB, hkey, &out_info, sizeof(out_info), MODE_READ, NULL, NULL);
    if (status != DB_SUCCESS)
      {
      cm_msg(MERROR, "bor", "Cannot read output info record");
      return 0;
      }
    }
  
  /* create ODB structures for banks */
  for (i=0 ; analyze_request[i].event_name[0] ; i++)
    {
    bank_list = analyze_request[i].bank_list;

    if (bank_list == NULL)
      continue;

    for (; bank_list->name[0] ; bank_list++)
      {
      strncpy(block_name, bank_list->name, 4);
      block_name[4] = 0;

      if (bank_list->type == TID_STRUCT)
        {
        sprintf(str, "/Equipment/%s/Variables/%s", analyze_request[i].event_name, block_name);
        db_create_record(hDB, 0, str, strcomb(bank_list->init_str));
        db_find_key(hDB, 0, str, &hkey);
        bank_list->def_key = hkey;
        }
      else
        {
        sprintf(str, "/Equipment/%s/Variables/%s", analyze_request[i].event_name, block_name);
        dummy = 0;
        db_set_value(hDB, 0, str, &dummy, rpc_tid_size(bank_list->type), 1, bank_list->type);
        db_find_key(hDB, 0, str, &hkey);
        bank_list->def_key = hkey;
        }
      }
    }

  /* create ODB structures for fixed events */
  for (i=0 ; analyze_request[i].event_name[0] ; i++)
    {
    if (analyze_request[i].init_string)
      {
      sprintf(str, "/Equipment/%s/Variables", analyze_request[i].event_name);
      db_create_record(hDB, 0, str, strcomb(analyze_request[i].init_string));
      }
    }

  /* create global section */
  if (clp.online)
    {
    HLIMAP(pawc_size/4, "ONLN");
    printf("\n");

    /* book online N-tuples only once when online */
    status = book_ntuples();
    if (status != SUCCESS)
      return status;
    }
  else
    {
    HLIMIT(pawc_size/4);
    }

  /* call main analyzer init routine */
  status = analyzer_init();
  if (status != SUCCESS)
    return status;

  /* initialize modules */
  for (i=0 ; analyze_request[i].event_name[0] ; i++)
    {
    module = analyze_request[i].ana_module;
    for (j=0 ; module != NULL && module[j] != NULL ; j++)
      {
      /* copy module enabled flag to ana_module */
      sprintf(str, "/%s/Module switches/%s", analyzer_name, module[j]->name);
      module[j]->enabled = TRUE;
      size = sizeof(BOOL);
      db_get_value(hDB, 0, str, &module[j]->enabled, &size, TID_BOOL);
  
      if (module[j]->init != NULL && module[j]->enabled)
        module[j]->init();
      }
    }

  return SUCCESS;
}

/*-- exit routine --------------------------------------------------*/

INT mana_exit()
{
ANA_MODULE **module;
INT        i, j;

  /* call exit routines from modules */
  for (i=0 ; analyze_request[i].event_name[0] ; i++)
    {
    module = analyze_request[i].ana_module;
    for (j=0 ; module != NULL && module[j] != NULL ; j++)
      if (module[j]->exit != NULL && module[j]->enabled)
        module[j]->exit();
    }

  /* call main analyzer exit routine */
  return analyzer_exit();
}

/*-- BOR routine ---------------------------------------------------*/

INT bor(INT run_number, char *error)
{
ANA_MODULE **module;
INT        i, j, status, size;
char       str[256], file_name[256];
char       *ext_str;
BANK_LIST  *bank_list;
INT        lrec;

  /* load parameters */
  load_parameters(run_number);
  
  for (i=0 ; analyze_request[i].event_name[0] ; i++)
    {
    /* copy output flag from ODB to bank_list */
    bank_list = analyze_request[i].bank_list;

    if (bank_list != NULL)
      for (; bank_list->name[0] ; bank_list++)
        {
        sprintf(str, "/%s/Bank switches/%s", analyzer_name, bank_list->name);
        bank_list->output_flag = FALSE;
        size = sizeof(DWORD);
        db_get_value(hDB, 0, str, &bank_list->output_flag, &size, TID_DWORD);
        }

    /* copy module enabled flag to ana_module */
    module = analyze_request[i].ana_module;
    for (j=0 ; module != NULL && module[j] != NULL ; j++)
      {
      sprintf(str, "/%s/Module switches/%s", analyzer_name, module[j]->name);
      module[j]->enabled = TRUE;
      size = sizeof(BOOL);
      db_get_value(hDB, 0, str, &module[j]->enabled, &size, TID_BOOL);
      }

    }

  /* clear histos and N-tuples */
  if (clp.online && out_info.clear_histos)
    {
    for (i=0 ; analyze_request[i].event_name[0] ; i++)
      if (analyze_request[i].bank_list != NULL)
        if (HEXIST(analyze_request[i].ar_info.event_id))
          HRESET(analyze_request[i].ar_info.event_id, " ");
    HRESET(0, " ");
    }

  /* open output file if not already open (append mode) and in offline mode */
  if (!clp.online && out_file == NULL)
    {
    if (out_info.filename[0])
      {
      strcpy(str, out_info.filename);
      if (strchr(str, '%') != NULL)
        sprintf(file_name, str, run_number);
      else
        strcpy(file_name, str);
    
      /* check output file extension */
      out_gzip = FALSE;
      if (strchr(file_name, '.'))
        {
        ext_str = file_name + strlen(file_name)-1;
        while (*ext_str != '.')
          ext_str--;
        }
      if (strncmp(ext_str, ".gz", 3) == 0)
        {
        out_gzip = TRUE;
        ext_str--;
        while (*ext_str != '.' && ext_str > file_name)
          ext_str--;
        }

      if (strncmp(ext_str, ".asc", 4) == 0)
        out_format = FORMAT_ASCII;
      else if (strncmp(ext_str, ".mid", 4) == 0)
        out_format = FORMAT_MIDAS;
      else if (strncmp(ext_str, ".rz", 3) == 0)
        out_format = FORMAT_HBOOK;
      else
        {
        strcpy(error, "Unknown output data format. Please use file extension .asc, .mid or .rz.\n");
        cm_msg(MERROR, "bor", error);
        return 0;
        }

      /* open output file */
      if (out_format == FORMAT_HBOOK)
        {
        lrec = HBOOK_LREC;
#ifdef extname
        quest_[9] = 65000;
#else
        QUEST[9] = 65000;
#endif
        
        HBSET("BSIZE", HBOOK_LREC, status);
        HROPEN(1, "OFFLINE", file_name, "NQ", lrec, status);
        if (status != 0)
          {
          sprintf(error, "Cannot open output file %s", out_info.filename);
          cm_msg(MERROR, "bor", error);
          out_file = NULL;
          return 0;
          }
        else
          out_file = (FILE *) 1;
        }
      else
        {
        if (out_gzip)
          out_file = (FILE *) gzopen(out_info.filename, "wb");
        else if (out_format == FORMAT_ASCII)
          out_file = fopen(out_info.filename, "wt");
        else
          out_file = fopen(out_info.filename, "wb");
        if (out_file == NULL)
          {
          sprintf(error, "Cannot open output file %s", out_info.filename);
          cm_msg(MERROR, "bor", error);
          return 0;
          }
        }
      }
    else
      out_file = NULL;

    /* book N-tuples */
    if (out_format == FORMAT_HBOOK)
      {
      status = book_ntuples();
      if (status != SUCCESS)
        return status;
      }

    } /* if (out_file == NULL) */

  /* call bor for modules */
  for (i=0 ; analyze_request[i].event_name[0] ; i++)
    {
    module = analyze_request[i].ana_module;
    for (j=0 ; module != NULL && module[j] != NULL ; j++)
      if (module[j]->bor != NULL && module[j]->enabled)
        module[j]->bor(run_number);
    }

  /* save run number */
  current_run_number = run_number;
  
  /* call main analyzer BOR routine */
  return ana_begin_of_run(run_number, error);
}

/*-- EOR routine ---------------------------------------------------*/

INT eor(INT run_number, char *error)
{
ANA_MODULE **module;
BANK_LIST  *bank_list;
INT        i, j, size, status;
char       str[256], file_name[256];

  /* call EOR routines modules */
  for (i=0 ; analyze_request[i].event_name[0] ; i++)
    {
    module = analyze_request[i].ana_module;
    for (j=0 ; module != NULL && module[j] != NULL ; j++)
      if (module[j]->eor != NULL && module[j]->enabled)
        module[j]->eor(run_number);
    }

  /* call main analyzer BOR routine */
  status = ana_end_of_run(run_number, error);

  /* save histos if requested */
  if (out_info.histo_dump)
    {
    if (out_file)
      {
      printf("\nCannot dump histos together with output RZ file.\n");
      printf("Please switch off \"/Analyzer/Output/Dump Histos\" flag.\n");
      }
    else
      {
      if (clp.online)
        {
        size = sizeof(str);
        str[0] = 0;
        db_get_value(hDB, 0, "/Logger/Data Dir", str, &size, TID_STRING);
        if (str[0] != 0)
          if (str[strlen(str)-1] != DIR_SEPARATOR)
            strcat(str, DIR_SEPARATOR_STR);
        }
      else
        str[0] = 0;

      strcat(str, out_info.histo_dump_filename);
      if (strchr(str, '%') != NULL)
        sprintf(file_name, str, run_number);
      else
        strcpy(file_name, str);

      HRPUT(0, file_name, "NT");
      }
    }

  /* close output file */
  if (out_file && !out_append)
    {
    if (out_format == FORMAT_HBOOK)
      {
      HROUT(0, i, " ");
      HREND("OFFLINE");
      }
    else
      {
      if (out_gzip)
        gzclose(out_file);
      else
        fclose(out_file);
      }

    out_file = NULL;

    /* free CWNT buffer */
    for (i=0 ; analyze_request[i].event_name[0] ; i++)
      {
      bank_list = analyze_request[i].bank_list;

      if (bank_list == NULL)
        {
        if (analyze_request[i].addr)
          {
          free(analyze_request[i].addr);
          analyze_request[i].addr = NULL;
          }
        }
      else
        {
        for (; bank_list->name[0] ; bank_list++)
          if (bank_list->addr)
            {
            free(bank_list->addr);
            bank_list->addr = NULL;
            }
        }
      }
    }

  return status;
}

/*-- transition callbacks ------------------------------------------*/

/*-- start ---------------------------------------------------------*/

INT tr_prestart(INT rn, char *error)
{
INT status, i;

  /* reset counters */
  for (i=0 ; analyze_request[i].event_name[0] ; i++)
    {
    analyze_request[i].ar_stats.events_received = 0;
    analyze_request[i].ar_stats.events_per_sec = 0;
    analyze_request[i].ar_stats.events_written = 0;
    analyze_request[i].events_received = 0;
    analyze_request[i].events_written = 0;
    }

  status = bor(rn, error);
  if (status != SUCCESS)
    return status;

  return SUCCESS;
}

/*-- stop ----------------------------------------------------------*/

INT tr_stop(INT rn, char *error)
{
INT i, status, n_bytes;

  /* wait until all events in buffers are analyzed */

  if (rpc_is_remote())
    while (bm_poll_event(TRUE));
  else 
    for (i=0 ; analyze_request[i].event_name[0] ; i++)
      {
      do
        {
        bm_get_buffer_level(analyze_request[i].buffer_handle, &n_bytes);
        if (n_bytes > 0)
          cm_yield(100);
        } while (n_bytes > 0);
      } 

  status = eor(rn, error);
  if (status != SUCCESS)
    return status;

  return CM_SUCCESS;
}

/*-- pause ---------------------------------------------------------*/

INT tr_pause(INT rn, char *error)
{
INT status;

  status = ana_pause_run(rn, error);
  if (status != CM_SUCCESS)
    return status;

  return CM_SUCCESS;
}

/*-- resume --------------------------------------------------------*/

INT tr_resume(INT rn, char *error)
{
INT status;

  status = ana_resume_run(rn, error);
  if (status != CM_SUCCESS)
    return status;

  return CM_SUCCESS;
}

/*---- ASCII output ------------------------------------------------*/

#define STR_INC(p,base) { p+=strlen(p); \
                          if (p > base+sizeof(base)) \
                            cm_msg(MERROR, "STR_INC", "ASCII buffer too small"); }


INT write_event_ascii(FILE *file, EVENT_HEADER *pevent, ANALYZE_REQUEST *par)
{
INT            status, size, i, j, count;
BOOL           exclude;
BANK_HEADER    *pbh;
BANK_LIST      *pbl;
EVENT_DEF      *event_def;  
BANK           *pbk;
void           *pdata;
char           *pbuf, name[5], type_name[10];
LRS1882_DATA   *lrs1882;
LRS1877_DATA   *lrs1877;
LRS1877_HEADER *lrs1877_header;
HNDLE          hKey;
KEY            key;
char           buffer[100000];

  event_def = db_get_event_definition(pevent->event_id);
  if (event_def == NULL)
    return SS_SUCCESS;

  /* write event header */
  pbuf = buffer;
  name[4] = 0;

  if (pevent->event_id == EVENTID_BOR)
    sprintf(pbuf, "%%ID BOR NR %d\n", pevent->serial_number);
  else if (pevent->event_id == EVENTID_EOR)
    sprintf(pbuf, "%%ID EOR NR %d\n", pevent->serial_number);
  else
    sprintf(pbuf, "%%ID %d TR %d NR %d\n", pevent->event_id, pevent->trigger_mask, 
                                           pevent->serial_number);
  STR_INC(pbuf,buffer);

  /*---- MIDAS format ----------------------------------------------*/
  if (event_def->format == FORMAT_MIDAS)
    {
    pbh = (BANK_HEADER *) (pevent+1);
    pbk = NULL;
    do
      {
      /* scan all banks */
      size = bk_iterate(pbh, &pbk, &pdata);
      if (pbk == NULL)
        break;

      /* look if bank is in exclude list */
      exclude = FALSE;
      pbl = NULL;
      if (par->bank_list != NULL)
        for (i=0 ; par->bank_list[i].name[0] ; i++)
          if ( *((DWORD *) par->bank_list[i].name) == *((DWORD *) pbk->name))
            {
            pbl = &par->bank_list[i];
            exclude = (pbl->output_flag == 0);
            break;
            }

      if (!exclude)
        {
        if (rpc_tid_size(pbk->type & 0xFF))
          size /= rpc_tid_size(pbk->type & 0xFF);
    
        lrs1882        = (LRS1882_DATA *)   pdata;
        lrs1877        = (LRS1877_DATA *)   pdata;

        /* write bank header */
        *((DWORD *) name) = *((DWORD *) (pbk)->name);

        if ((pbk->type & 0xFF00) == 0)
          strcpy(type_name, rpc_tid_name(pbk->type & 0xFF));
        else if ((pbk->type & 0xFF00) == TID_LRS1882)
          strcpy(type_name, "LRS1882");
        else if ((pbk->type & 0xFF00) == TID_LRS1877)
          strcpy(type_name, "LRS1877");
        else if ((pbk->type & 0xFF00) == TID_PCOS3)
          strcpy(type_name, "PCOS3");
        else
          strcpy(type_name, "unknown");

        sprintf(pbuf, "BK %s TP %s SZ %d\n", name, type_name, size);
        STR_INC(pbuf, buffer);

        if (pbk->type == TID_STRUCT)
          {
          if (pbl == NULL)
            cm_msg(MERROR, "write_event_ascii", "received unknown bank %s", name);
          else
            /* write structured bank */
            for (i=0 ;; i++)
              {
              status = db_enum_key(hDB, pbl->def_key, i, &hKey);
              if (status == DB_NO_MORE_SUBKEYS)
                break;

              db_get_key(hDB, hKey, &key);
              sprintf(pbuf, "%s:\n", key.name);
              STR_INC(pbuf, buffer);

              /* adjust for alignment */
              pdata = (void *) VALIGN(pdata, min(ss_get_struct_align(),key.item_size));

              for (j=0 ; j<key.num_values ; j++)
                {
                db_sprintf(pbuf, pdata, key.item_size, j, key.type);
                strcat(pbuf, "\n");
                STR_INC(pbuf, buffer);
                }

              /* shift data pointer to next item */
              (char *) pdata += key.item_size*key.num_values;
              }
          }
        else
          {
          /* write variable length bank  */
          if ((pbk->type & 0xFF00) == TID_LRS1877)
            {
            for (i=0 ; i<size ;)
              {
              lrs1877_header = (LRS1877_HEADER *) &lrs1877[i];

              /* print header */
              sprintf(pbuf, "GA %d BF %d CN %d", 
                  lrs1877_header->geo_addr, lrs1877_header->buffer, lrs1877_header->count);
              strcat(pbuf, "\n");
              STR_INC(pbuf,buffer);

              count = lrs1877_header->count;
              if (count == 0)
                break;
              for (j=1 ; j<count ; j++)
                {
                /* print data */
                sprintf(pbuf, "GA %d CH %02d ED %d DA %1.1lf", 
                  lrs1877[i].geo_addr, lrs1877[i+j].channel, lrs1877[i+j].edge, lrs1877[i+j].data*0.5);
                strcat(pbuf, "\n");
                STR_INC(pbuf,buffer);
                }

              i += count;
              }
            }
          else for (i=0 ; i<size ; i++)
            {
            if ((pbk->type & 0xFF00) == 0)
              db_sprintf(pbuf, pdata, size, i, pbk->type & 0xFF);

            else if ((pbk->type & 0xFF00) == TID_LRS1882)
              sprintf(pbuf, "GA %d CH %02d DA %d", 
                lrs1882[i].geo_addr, lrs1882[i].channel, lrs1882[i].data);
      
            else if ((pbk->type & 0xFF00) == TID_PCOS3)
              sprintf(pbuf, ""); /* TBD */
            else
              db_sprintf(pbuf, pdata, size, i, pbk->type & 0xFF);

            strcat(pbuf, "\n");
            STR_INC(pbuf,buffer);
            }
          }
        }

      } while (1);
    }

  /*---- FIXED format ----------------------------------------------*/
  if (event_def->format == FORMAT_FIXED)
    {
    if (event_def->hDefKey == 0)
      cm_msg(MERROR, "write_event_ascii", "cannot find event definition");
    else
      {
      pdata = (char *) (pevent+1);
      for (i=0 ;; i++)
        {
        status = db_enum_key(hDB, event_def->hDefKey, i, &hKey);
        if (status == DB_NO_MORE_SUBKEYS)
          break;

        db_get_key(hDB, hKey, &key);
        sprintf(pbuf, "%s\n", key.name);
        STR_INC(pbuf, buffer);

        /* adjust for alignment */
        pdata = (void *) VALIGN(pdata, min(ss_get_struct_align(),key.item_size));

        for (j=0 ; j<key.num_values ; j++)
          {
          db_sprintf(pbuf, pdata, key.item_size, j, key.type);
          strcat(pbuf, "\n");
          STR_INC(pbuf, buffer);
          }

        /* shift data pointer to next item */
        (char *) pdata += key.item_size*key.num_values;
        }
      }
    }

  /* insert empty line after each event */
  strcat(pbuf, "\n");
  size = strlen(buffer);

  /* write record to device */
  if (out_gzip)
    status = gzwrite(file, buffer, size) == size ? SS_SUCCESS : SS_FILE_ERROR;
  else
    status = fwrite(buffer, 1, size, file) == (size_t) size ? SS_SUCCESS : SS_FILE_ERROR;

  return status;
}

/*---- MIDAS output ------------------------------------------------*/

INT write_event_midas(FILE *file, EVENT_HEADER *pevent, ANALYZE_REQUEST *par)
{
INT            status, size, i;
BOOL           exclude;
BANK_HEADER    *pbh;
BANK_LIST      *pbl;
EVENT_DEF      *event_def;  
BANK           *pbk;
char           *pdata, *pdata_copy;
char           buffer[NET_TCP_SIZE], *pbuf;
EVENT_HEADER   *pevent_copy;

  event_def = db_get_event_definition(pevent->event_id);
  if (event_def == NULL)
    return SUCCESS;
  pevent_copy = (EVENT_HEADER *) ALIGN((PTYPE)buffer);

  /*---- MIDAS format ----------------------------------------------*/
  
  if (event_def->format == FORMAT_MIDAS)
    {
    /* copy event header */
    pbuf = (char *) pevent_copy;
    memcpy(pbuf, pevent, sizeof(EVENT_HEADER));
    pbuf += sizeof(EVENT_HEADER);
    bk_init(pbuf);

    pbh = (BANK_HEADER *) (pevent+1);
    pbk = NULL;
    pdata_copy = pbuf;
    do
      {
      /* scan all banks */
      size = bk_iterate(pbh, &pbk, &pdata);
      if (pbk == NULL)
        break;

      /* look if bank is in exclude list */
      exclude = FALSE;
      pbl = NULL;
      if (par->bank_list != NULL)
        for (i=0 ; par->bank_list[i].name[0] ; i++)
          if ( *((DWORD *) par->bank_list[i].name) == *((DWORD *) pbk->name))
            {
            pbl = &par->bank_list[i];
            exclude = (pbl->output_flag == 0);
            break;
            }

      if (!exclude)
        {
        /* copy bank */
        bk_create(pbuf, pbk->name, pbk->type, &pdata_copy);
        memcpy(pdata_copy, pdata, pbk->data_size);
        pdata_copy += pbk->data_size;
        bk_close(pbuf, pdata_copy);
        }

      } while (1);

    /* set event size in header */
    size = (PTYPE) pdata_copy  - (PTYPE) pbuf;
    pevent_copy->data_size = size;
    size += sizeof(EVENT_HEADER);
    }

  /*---- FIXED format ----------------------------------------------*/
  if (event_def->format == FORMAT_FIXED)
    {
    size = pevent->data_size + sizeof(EVENT_HEADER);
    memcpy(pevent_copy, pevent, size);
    }

  if (pevent_copy->data_size == 0)
    return SUCCESS;

  /* write record to device */
  if (out_gzip)
    status = gzwrite(file, pevent_copy, size) == size ? SUCCESS : SS_FILE_ERROR;
  else
    status = fwrite(pevent_copy, 1, size, file) == (size_t) size ? SUCCESS : SS_FILE_ERROR;
  
  return status;
}

/*---- HBOOK output ------------------------------------------------*/

INT write_event_hbook(FILE *file, EVENT_HEADER *pevent, ANALYZE_REQUEST *par)
{
INT        i, j, k, n, size, item_size, status;
BANK       *pbk;                      
BANK_LIST  *pbl;                      
void       *pdata;                    
BOOL       exclude, exclude_all;
char       block_name[5];             
float      rwnt[512];                 
EVENT_DEF  *event_def;                
HNDLE      hkey;
KEY        key;

  /* retunr if N-tuples are disabled */
  if (!ntuple_flag)
    return SS_SUCCESS;

  event_def = db_get_event_definition(pevent->event_id);
  if (event_def == NULL)
    return SS_SUCCESS;

  /* fill number info */
  if (clp.rwnt)
    {
    memset(rwnt, 0, sizeof(rwnt));
    rwnt[0] = (float) current_run_number;
    rwnt[1] = (float) pevent->serial_number;
    rwnt[2] = (float) pevent->time_stamp;
    }
  else
    {
    par->number.run = current_run_number;
    par->number.serial = pevent->serial_number;
    par->number.time = pevent->time_stamp;
    }

  /*---- MIDAS format ----------------------------------------------*/
  
  if (event_def->format == FORMAT_MIDAS)
    {
    /* first fill number block */
    if (!clp.rwnt)
      HFNTB(pevent->event_id, "Number");

    pbk = NULL;
    exclude_all = TRUE;
    do
      {
      /* scan all banks */
      size = bk_iterate((BANK_HEADER *) (pevent+1), &pbk, &pdata);
      if (pbk == NULL)
        break;

      /* look if bank is in exclude list */
      *((DWORD *) block_name) = *((DWORD *) pbk->name);
      block_name[4] = 0;

      exclude = FALSE;
      pbl = NULL;
      if (par->bank_list != NULL)
        {
        for (i=0 ; par->bank_list[i].name[0] ; i++)
          if ( *((DWORD *) par->bank_list[i].name) == *((DWORD *) pbk->name))
            {
            pbl = &par->bank_list[i];
            exclude = (pbl->output_flag == 0);
            break;
            }
        if (par->bank_list[i].name[0] == 0)
          cm_msg(MERROR, "write_event_hbook", "Received unknown bank %s", block_name);
        }

      /* fill CW N-tuple */
      if (!exclude && pbl != NULL && !clp.rwnt)
        {
        /* set array size in bank list */
        if ((pbk->type & 0xFF) != TID_STRUCT)
          {
          item_size = rpc_tid_size(pbk->type & 0xFF);
          if (item_size == 0)
            {
            cm_msg(MERROR, "write_event_hbook", "Received bank %s with unknown item size", block_name);
            continue;
            }

          pbl->n_data = size / item_size;

          /* check bank size */
          if (pbl->n_data > pbl->size)
            {
            cm_msg(MERROR, "write_event_hbook", 
              "Bank %s has more (%d) entries than maximum value (%d)", block_name, pbl->n_data,
              pbl->size);
            continue;
            }

          /* copy bank to buffer in bank list, DWORD aligned */
          if (item_size >= 4)
            {
            size = min((INT)pbl->size*item_size, size);
            memcpy(pbl->addr, pdata, size);
            }
          else if (item_size == 2)
            for (i=0 ; i<(INT)pbl->n_data ; i++)
              *((DWORD *) pbl->addr+i) = (DWORD) *((WORD *) pdata+i);
          else if (item_size == 1)
            for (i=0 ; i<(INT)pbl->n_data ; i++)
              *((DWORD *) pbl->addr+i) = (DWORD) *((BYTE *) pdata+i);
          }
        else
          {
          /* hope that structure members are aligned as HBOOK thinks ... */
          memcpy(pbl->addr, pdata, size);
          }

        /* fill N-tuple */
        HFNTB(pevent->event_id, block_name);
        }

      /* fill RW N-tuple */
      if (!exclude && pbl != NULL && clp.rwnt)
        {
        exclude_all = FALSE;

        item_size = rpc_tid_size(pbk->type & 0xFF);
        /* set array size in bank list */
        if ((pbk->type & 0xFF) != TID_STRUCT)
          {
          n = size / item_size;

          /* check bank size */
          if (n > (INT)pbl->size)
            {
            cm_msg(MERROR, "write_event_hbook", 
              "Bank %s has more (%d) entries than maximum value (%d)", block_name, n,
              pbl->size);
            continue;
            }

          /* convert bank to float values */
          for (i=0 ; i<n ; i++)
            {
            switch (pbk->type & 0xFF)
              {
              case TID_BYTE : 
                rwnt[pbl->n_data + i] = (float) (*((BYTE *) pdata+i));
                break;
              case TID_WORD : 
                rwnt[pbl->n_data + i] = (float) (*((WORD *) pdata+i));
                break;
              case TID_DWORD : 
                rwnt[pbl->n_data + i] = (float) (*((DWORD *) pdata+i));
                break;
              case TID_FLOAT : 
                rwnt[pbl->n_data + i] = (float) (*((float *) pdata+i));
                break;
              case TID_DOUBLE : 
                rwnt[pbl->n_data + i] = (float) (*((double *) pdata+i));
                break;
              }
            }

          /* zero padding */
          for ( ; i<(INT)pbl->size ; i++)
            rwnt[pbl->n_data + i] = 0.f;
          }
        else
          {
          /* fill N-tuple from structured bank */
          k = pbl->n_data;

          for (i=0 ; ; i++)
            {
            status = db_enum_key(hDB, pbl->def_key, i, &hkey);
            if (status == DB_NO_MORE_SUBKEYS)
              break;

            db_get_key(hDB, hkey, &key);

            /* align data pointer */
            pdata = (void *) VALIGN(pdata, min(ss_get_struct_align(),key.item_size));

            for (j=0 ; j<key.num_values ; j++)
              {
              switch (key.type & 0xFF)
                {
                case TID_BYTE : 
                  rwnt[k++] = (float) (*((BYTE *) pdata+j));
                  break;
                case TID_WORD : 
                  rwnt[k++] = (float) (*((WORD *) pdata+j));
                  break;
                case TID_SHORT : 
                  rwnt[k++] = (float) (*((short int *) pdata+j));
                  break;
                case TID_INT : 
                  rwnt[k++] = (float) (*((INT *) pdata+j));
                  break;
                case TID_DWORD : 
                  rwnt[k++] = (float) (*((DWORD *) pdata+j));
                  break;
                case TID_BOOL : 
                  rwnt[k++] = (float) (*((BOOL *) pdata+j));
                  break;
                case TID_FLOAT : 
                  rwnt[k++] = (float) (*((float *) pdata+j));
                  break;
                case TID_DOUBLE : 
                  rwnt[k++] = (float) (*((double *) pdata+j));
                  break;
                }
              }

            /* shift data pointer to next item */
            (char *) pdata += key.item_size*key.num_values;
            }
          }
        }

      } while(TRUE);

    /* fill RW N-tuple */
    if (clp.rwnt && file != NULL && !exclude_all)
      HFN(pevent->event_id, rwnt);

    /* fill shared memory */
    if (file == NULL)
      HFNOV(pevent->event_id, rwnt);
    
    } /* if (event_def->format == FORMAT_MIDAS) */

  /*---- FIXED format ----------------------------------------------*/
  
  if (event_def->format == FORMAT_FIXED)
    {
    if (clp.rwnt)
      {
      /* fill N-tuple from structured bank */
      pdata = pevent+1;
      k = 3; /* index 0..2 for run/serial/time */

      for (i=0 ; ; i++)
        {
        status = db_enum_key(hDB, event_def->hDefKey, i, &hkey);
        if (status == DB_NO_MORE_SUBKEYS)
          break;

        db_get_key(hDB, hkey, &key);

        /* align data pointer */
        pdata = (void *) VALIGN(pdata, min(ss_get_struct_align(),key.item_size));

        for (j=0 ; j<key.num_values ; j++)
          {
          switch (key.type & 0xFF)
            {
            case TID_BYTE : 
              rwnt[k++] = (float) (*((BYTE *) pdata+j));
              break;
            case TID_WORD : 
              rwnt[k++] = (float) (*((WORD *) pdata+j));
              break;
            case TID_SHORT : 
              rwnt[k++] = (float) (*((short int *) pdata+j));
              break;
            case TID_INT : 
              rwnt[k++] = (float) (*((INT *) pdata+j));
              break;
            case TID_DWORD : 
              rwnt[k++] = (float) (*((DWORD *) pdata+j));
              break;
            case TID_BOOL : 
              rwnt[k++] = (float) (*((BOOL *) pdata+j));
              break;
            case TID_FLOAT : 
              rwnt[k++] = (float) (*((float *) pdata+j));
              break;
            case TID_DOUBLE : 
              rwnt[k++] = (float) (*((double *) pdata+j));
              break;
            }
          }

        /* shift data pointer to next item */
        (char *) pdata += key.item_size*key.num_values;
        }

      /* fill RW N-tuple */
      if (file != NULL)
        HFN(pevent->event_id, rwnt);

      /* fill shared memory */
      if (file == NULL)
        HFNOV(pevent->event_id, rwnt);
      }
    else
      {
      memcpy(par->addr, pevent+1, pevent->data_size);

      /* fill N-tuple */
      HFNT(pevent->event_id);
      }

    }

  return SUCCESS;
}

/*---- ODB output --------------------------------------------------*/

INT write_event_odb(EVENT_HEADER *pevent, ANALYZE_REQUEST *par)
{
INT            status, size, n_data, i;
BANK_HEADER    *pbh;
EVENT_DEF      *event_def;  
BANK           *pbk;
void           *pdata;
char           name[5];
HNDLE          hKeyRoot, hKey;
KEY            key;

  event_def = db_get_event_definition(pevent->event_id);
  if (event_def == NULL)
    return SS_SUCCESS;

  /*---- MIDAS format ----------------------------------------------*/
  if (event_def->format == FORMAT_MIDAS)
    {
    pbh = (BANK_HEADER *) (pevent+1);
    pbk = NULL;
    do
      {
      /* scan all banks */
      size = bk_iterate(pbh, &pbk, &pdata);
      if (pbk == NULL)
        break;

      n_data = size;
      if (rpc_tid_size(pbk->type & 0xFF))
        n_data /= rpc_tid_size(pbk->type & 0xFF);
  
      /* get bank key */
      *((DWORD *) name) = *((DWORD *) (pbk)->name);
      name[4] = 0;

      status = db_find_key(hDB, event_def->hDefKey, name, &hKeyRoot);
      if (status != DB_SUCCESS)
        {
        cm_msg(MERROR, "write_event_odb", "received unknown bank %s", name);
        continue;
        }

      if (pbk->type == TID_STRUCT)
        {
        /* write structured bank */
        for (i=0 ;; i++)
          {
          status = db_enum_key(hDB, hKeyRoot, i, &hKey);
          if (status == DB_NO_MORE_SUBKEYS)
            break;

          db_get_key(hDB, hKey, &key);

          /* adjust for alignment */
          pdata = (void *) VALIGN(pdata, min(ss_get_struct_align(),key.item_size));

          status = db_set_data(hDB, hKey, pdata, key.item_size*key.num_values, 
                               key.num_values, key.type);
          if (status != DB_SUCCESS)
            {
            cm_msg(MERROR, "write_event_odb", "cannot write %s to ODB", name);
            continue;
            }

          /* shift data pointer to next item */
          (char *) pdata += key.item_size*key.num_values;
          }
        }
      else
        {
        db_get_key(hDB, hKeyRoot, &key);

        /* write variable length bank  */
        if (n_data > 0)
          {
          status = db_set_data(hDB, hKeyRoot, pdata, size, n_data, key.type);
          if (status != DB_SUCCESS)
            {
            cm_msg(MERROR, "write_event_odb", "cannot write %s to ODB", name);
            continue;
            }
          }
        }
      } while (1);
    }

  /*---- FIXED format ----------------------------------------------*/
  if (event_def->format == FORMAT_FIXED && !clp.online)
    {
    if (db_set_record(hDB, event_def->hDefKey, (char *) (pevent+1),
                      pevent->data_size, 0) != DB_SUCCESS)
      cm_msg(MERROR, "write_event_odb", "event #%d size mismatch", pevent->event_id);
    }

  return SUCCESS;
}

/*------------------------------------------------------------------*/

struct {
  short int event_id;
  DWORD     last_time;
} last_time_event[50];

INT process_event(ANALYZE_REQUEST *par, EVENT_HEADER *pevent)
{
INT          i, status, ch;
ANA_MODULE   **module;
DWORD        actual_time;
static DWORD last_time_kb = 0;
EVENT_DEF    *event_def;

  /* verbose output */
  if (clp.verbose)
    printf("event %d, number %d\n", pevent->event_id, pevent->serial_number);

  /* increment event counter */
  par->events_received++;

  /* swap event if necessary */
  event_def = db_get_event_definition(pevent->event_id);
  if (event_def == NULL)
    return 0;

  if (event_def->format == FORMAT_MIDAS)
    bk_swap((BANK_HEADER *) (pevent+1), FALSE);

  /*---- analyze event ----*/

  /* call non-modular analyzer if defined */
  status = CM_SUCCESS;
  if (par->analyzer)
    status = par->analyzer(pevent, (void *) (pevent+1));

  /* don't continue if event was rejected */
  if (status == 0)
    return 0;

  /* loop over analyzer modules */
  module = par->ana_module;
  for (i=0 ; module != NULL && module[i] != NULL ; i++)
    {
    if (module[i]->enabled)
      {
      status = module[i]->analyzer(pevent, (void *) (pevent+1));

      /* don't continue if event was rejected */
      if (status == 0)
        return 0;
      }
    }

  /* write resulting event */
  if (out_file)
    {
    /* check if events matches filter */
    for (i=0 ; i<10 && clp.filter[i] ; i++)
      if (clp.filter[i] == pevent->event_id)
        break;

    if (clp.filter[0] == 0 || clp.filter[i] == pevent->event_id)
      {
      if (out_format == FORMAT_HBOOK)
        status = write_event_hbook(out_file, pevent, par);
      else if (out_format == FORMAT_ASCII)
        status = write_event_ascii(out_file, pevent, par);
      else if (out_format == FORMAT_MIDAS)
        status = write_event_midas(out_file, pevent, par);

      if (status != SUCCESS)
        {
        cm_msg(MERROR, "process_event", "Error writing to file (Disk full?)");
        return -1;
        }

      par->events_written++;
      }
    else
      /* signal rejection of event */
      return 0;
    }

  /* fill shared memory */
  if (clp.online)
    write_event_hbook(NULL, pevent, par);

  /* put event in ODB once every second */
  actual_time = ss_millitime();
  for (i=0 ; i<50 ; i++)
    {
    if (last_time_event[i].event_id == pevent->event_id)
      {
      if (actual_time - last_time_event[i].last_time > 1000)
        {
        last_time_event[i].last_time = actual_time;
        write_event_odb(pevent, par);
        }
      break;
      }
    if (last_time_event[i].event_id == 0)
      {
      last_time_event[i].event_id = pevent->event_id;
      last_time_event[i].last_time = actual_time;
      write_event_odb(pevent, par);
      break;
      }
    }

  /* check keyboard once every second */
  if (!clp.online && actual_time - last_time_kb > 1000)
    {
    last_time_kb = actual_time;

    while (ss_kbhit())
      {
      ch = ss_getchar(0);
      if (ch == -1)
        ch = getchar();

      if ((char) ch == '!')
        return RPC_SHUTDOWN;
      }
    }

  return SUCCESS;
}

/*------------------------------------------------------------------*/

void receive_event(HNDLE buffer_handle, HNDLE request_id, EVENT_HEADER *pheader, void *pevent)
/* receive online event */
{
INT i;
ANALYZE_REQUEST *par;
char buffer[60000], *pb;

  /* align buffer */
  pb = (char *) ALIGN((int) buffer);

  /* copy event to local buffer */
  memcpy(pb, pheader, pheader->data_size + sizeof(EVENT_HEADER));

  par = analyze_request;

  for (i=0 ; par->event_name[0] ; par++)
    if (par->buffer_handle == buffer_handle &&
        par->request_id == request_id)
    {
    process_event(par, (EVENT_HEADER *) pb);
    }
}

/*------------------------------------------------------------------*/

void update_request(HNDLE hDB, HNDLE hKey, void *info)
{
AR_INFO *ar_info;
INT i;

  if (!clp.online)
    return;

  /* check which request's key has changed */
  for (i=0 ; analyze_request[i].event_name[0] ; i++)
    if (analyze_request[i].hkey_common == hKey)
      {
      ar_info = &analyze_request[i].ar_info;

      /* remove previous request */
      if (analyze_request[i].request_id != -1)
        bm_delete_request(analyze_request[i].request_id);

      /* if enabled, add new request */
      if (ar_info->enabled)
        bm_request_event(analyze_request[i].buffer_handle, (short) ar_info->event_id,
                       (short) ar_info->trigger_mask, ar_info->sampling_type,
                       &analyze_request[i].request_id,
                       receive_event);
      else
        analyze_request[i].request_id = -1;
      } 
  
}

/*------------------------------------------------------------------*/

void register_requests(void)
{
INT      index, status;
char     str[256];
AR_INFO  *ar_info;
AR_STATS *ar_stats;
HNDLE    hKey;

  /* scan ANALYZE_REQUEST table from ANALYZE.C */
  for (index=0 ; analyze_request[index].event_name[0] ; index++)
    {
    ar_info = &analyze_request[index].ar_info;
    ar_stats = &analyze_request[index].ar_stats;

    /* create common subtree from analyze_request table in analyze.c */
    sprintf(str, "/%s/%s/Common", analyzer_name, analyze_request[index].event_name);
    db_create_record(hDB, 0, str, ANALYZER_REQUEST_STR);
    db_find_key(hDB, 0, str, &hKey);
    analyze_request[index].hkey_common = hKey;

    strcpy(ar_info->client_name, analyzer_name);
    gethostname(ar_info->host, sizeof(ar_info->host));
    db_set_record(hDB, hKey, ar_info, sizeof(AR_INFO), 0);

    /* open hot link to analyzer request info */
    db_open_record(hDB, hKey, ar_info, sizeof(AR_INFO), MODE_READ, update_request, NULL);

    /* create statistics tree */
    sprintf(str, "/%s/%s/Statistics", analyzer_name, analyze_request[index].event_name);
    db_create_record(hDB, 0, str, ANALYZER_STATS_STR);
    db_find_key(hDB, 0, str, &hKey);

    ar_stats->events_received = 0;
    ar_stats->events_per_sec = 0;
    ar_stats->events_written = 0;

    /* open hot link to statistics tree */
    status = db_open_record(hDB, hKey, ar_stats, sizeof(AR_STATS), MODE_WRITE, NULL, NULL);
    if (status != DB_SUCCESS)
      printf("Cannot open statistics record, probably other analyzer is using it\n");

    if (clp.online)
      {
      /*---- open event buffer ---------------------------------------*/
      bm_open_buffer(ar_info->buffer, EVENT_BUFFER_SIZE, &analyze_request[index].buffer_handle);

      /* set the default buffer cache size */
      bm_set_cache_size(analyze_request[index].buffer_handle, 100000, 0);

      /*---- request event -------------------------------------------*/
      if (ar_info->enabled)
        bm_request_event(analyze_request[index].buffer_handle, (short) ar_info->event_id,
                         (short) ar_info->trigger_mask, ar_info->sampling_type,
                         &analyze_request[index].request_id,
                         receive_event);
      else
        analyze_request[index].request_id = -1;
      }
    }
}

/*------------------------------------------------------------------*/
      
void update_stats()
{
int i;
AR_STATS *ar_stats;
static   DWORD last_time=0;
DWORD    actual_time;

  actual_time = ss_millitime();

  if (last_time == 0)
    last_time = actual_time;

  if (actual_time - last_time == 0)
    return; 

  for (i=0 ; analyze_request[i].event_name[0] ; i++)
    {
    ar_stats = &analyze_request[i].ar_stats;
    ar_stats->events_received += analyze_request[i].events_received;
    ar_stats->events_written += analyze_request[i].events_written;
    ar_stats->events_per_sec =
      (DWORD) (analyze_request[i].events_received / ((actual_time-last_time)/1000.0));
    analyze_request[i].events_received = 0;
    analyze_request[i].events_written = 0;
    }

  /* propagate new statistics to ODB */
  db_send_changed_records();

  last_time = actual_time;
}

/*-- Clear histos --------------------------------------------------*/

INT clear_histos(INT id1, INT id2)
{
INT i;

  if (id1 != id2)
    {
    printf("Clear ID %d to ID %d\n", id1, id2);
    for (i=id1 ; i<=id2 ; i++)
      if (HEXIST(i))
        HRESET(i, " ");
    }
  else
    {
    printf("Clear ID %d\n", id1);
    HRESET(id1, " ");
    }

  return SUCCESS;
}

/*------------------------------------------------------------------*/

INT ana_callback(INT index, void *prpc_param[])
{
  if (index == RPC_ANA_CLEAR_HISTOS)
    clear_histos(CINT(0),CINT(1));

  return RPC_SUCCESS;
}

/*------------------------------------------------------------------*/

INT loop_online()
{
INT      status;
DWORD    last_time_loop, last_time_update, actual_time;
int      ch;

  printf("Running analyzer online. Stop with \"!\"\n");

  /* main loop */
  last_time_update = 0;
  last_time_loop   = 0;

  do
    {
    /* calculate events per second */
    actual_time = ss_millitime();

    if (actual_time - last_time_update > 1000)
      {
      /* update statistics */
      update_stats();
      last_time_update = actual_time;

      /* check keyboard */
      if (ss_kbhit())
        {
#if defined(OS_MSDOS) || defined(OS_WINNT)
        ch = getch();
#else
        ch = getchar();
#endif

        if (ch == '!')
          break;
        }
      }

    if (analyzer_loop_period == 0)
      status = cm_yield(1000);
    else
      {
      if (actual_time - last_time_loop > (DWORD) analyzer_loop_period)
        {
        last_time_loop = actual_time;
        analyzer_loop();
        }

      status = cm_yield(analyzer_loop_period);
      }

    } while (status != RPC_SHUTDOWN && status != SS_ABORT);

  /* update statistics */
  update_stats();

  return status;
}
    
/*---- Offline Code ------------------------------------------------*/

INT iogets(gzFile file, char *str, int maxlen)
{
static char *buffer=NULL;
static int  n=0, rp=0;
INT    i, j;

  if (!buffer)
    buffer = malloc(10000);

  /* if buffer empty, read it in */
  if (rp >= n)
    {
    n = gzread(file, buffer, 10000);

    if (n < 0)
      printf("Error reading input file\n");

    if (n <= 0)
      {
      str[0] = 0;
      return -1;
      }
    rp = 0;
    }

  /* copy from buffer to str until end of line */
  for (i=0 ; rp<n ; i++,rp++)
    {
    if (i >= maxlen)
      return -1;

    str[i] = buffer[rp];
    if (buffer[rp] == '\n')
      {
      i++;
      rp++;
      break;
      }
    }
  str[i] = 0;

  if (rp >= n && str[i-1] != '\n')
    {
    /* line not fully in buffer, read next buffer */
    j = iogets(file, str+i, maxlen-i);
    if (j < 0)
      {
      str[0] = 0;
      return -1;
      }
    }

  return strlen(str);
}

/*------------------------------------------------------------------*/

INT init_module_parameters(BOOL bclose)
{
INT        i, j;
ANA_MODULE **module;
char       str[80];
HNDLE      hkey;

  for (i=0 ; analyze_request[i].event_name[0] ; i++)
    {
    module = analyze_request[i].ana_module;
    for (j=0 ; module != NULL && module[j] != NULL ; j++)
      {
      if (module[j]->parameters != NULL)
        {
        sprintf(str, "/%s/Parameters/%s", analyzer_name, module[j]->name);

        if (bclose)
          {
          db_find_key(hDB, 0, str, &hkey);
          db_close_record(hDB, hkey);
          }
        else
          {
          if (module[j]->init_str)
            db_create_record(hDB, 0, str, strcomb(module[j]->init_str));

          db_find_key(hDB, 0, str, &hkey);
          if (db_open_record(hDB, hkey, module[j]->parameters, module[j]->param_size, 
                             MODE_READ, NULL, NULL) != DB_SUCCESS)
            {
            cm_msg(MERROR, "init_module_parameters", "Cannot open \"%s\" parameters in ODB", str);
            return 0;
            }
          }
        }
      }
    }
 
  return SUCCESS;  
}

/*------------------------------------------------------------------*/

INT analyze_file(INT run_number, char *input_file_name, char *output_file_name)
{
EVENT_HEADER    *pevent, *pevent_unaligned;
ANALYZE_REQUEST *par;
INT             i, n, flag, size;
DWORD           num_events_in, num_events_out;
char            error[256], str[256];
char            *ext_str;
INT             format, status;
gzFile          file;
BOOL            skip;
HNDLE           hKey, hKeyEq, hKeyRoot;

  /* set output file name and flags in ODB */
  sprintf(str, "/%s/Output/Filename", analyzer_name);
  db_set_value(hDB, 0, str, output_file_name, 256, 1, TID_STRING);
  sprintf(str, "/%s/Output/RWNT", analyzer_name);
  db_set_value(hDB, 0, str, &clp.rwnt, sizeof(BOOL), 1, TID_BOOL);

  /* set run number in ODB */
  db_set_value(hDB, 0, "/Runinfo/Run number", &run_number, sizeof(run_number), 1, TID_INT);

  /* set file name in out_info */
  strcpy(out_info.filename, output_file_name);

  /* let changes propagate to modules */
  cm_yield(0);

  /* check input file extension */
  if (strchr(input_file_name, '.'))
    {
    ext_str = input_file_name + strlen(input_file_name)-1;
    while (*ext_str != '.')
      ext_str--;
    }
  else
    ext_str = "";

  if (strncmp(ext_str, ".gz", 3) == 0)
    {
    ext_str--;
    while (*ext_str != '.' && ext_str > input_file_name)
      ext_str--;
    }

  if (strncmp(input_file_name, "/dev/", 4) == 0) /* assume MIDAS tape */
    format = FORMAT_MIDAS;
  else if (strncmp(ext_str, ".mid", 4) == 0)
    format = FORMAT_MIDAS;
  else
    {
    printf("Unknown input data format \"%s\". Please use file extension .mid or mid.gz.\n", ext_str);
    return -1;
    }

  file = gzopen(input_file_name, "rb");

  if (file == NULL)
    {
    printf("File %s not found\n", input_file_name);
    return -1;
    }

  pevent_unaligned = malloc(MAX_EVENT_SIZE+sizeof(EVENT_HEADER));
  if (pevent_unaligned == NULL)
    {
    printf("Not enough memeory\n");
    return -1;
    }
  pevent = (EVENT_HEADER *) ALIGN((PTYPE)pevent_unaligned);

  /* call analyzer bor routines */
  bor(run_number, error);

  num_events_in = num_events_out = 0;

  /* event loop */
  do
    {
    if (format == FORMAT_MIDAS)
      {
      /* read event header */
      n = gzread(file, pevent, sizeof(EVENT_HEADER));
      if (n < sizeof(EVENT_HEADER))
        {
        if (n > 0)
          printf("Unexpected end of file %s, last event skipped\n", input_file_name);

        break;
        }

      /* read event */
      if (pevent->data_size > 0)
        {
        n = gzread(file, pevent+1, pevent->data_size);
        if (n != (INT) pevent->data_size)
          {
          printf("Unexpected end of file %s, last event skipped\n", input_file_name);
          break;
          }
        }
      }

    num_events_in++;

    /* load ODB with BOR event */
    if (pevent->event_id == EVENTID_BOR)
      {
      run_number = pevent->serial_number;

      flag = TRUE;
      size = sizeof(flag);
      sprintf(str, "/%s/ODB Load", analyzer_name);
      db_get_value(hDB, 0, str, &flag, &size, TID_BOOL);

      if (flag)
        {
        printf("Load ODB from run %d...", run_number);
        
        if (flag == 1)
          {
          /* lock all ODB values except run parameters */
          db_set_mode(hDB, 0, MODE_READ, TRUE);

          db_find_key(hDB, 0, "/Experiment/Run Parameters", &hKey);
          if (hKey)
            db_set_mode(hDB, hKey, MODE_READ | MODE_WRITE | MODE_DELETE, TRUE);

          /* and analyzer parameters */
          sprintf(str, "/%s/Parameters", analyzer_name);
          db_find_key(hDB, 0, str, &hKey);
          if (hKey)
            db_set_mode(hDB, hKey, MODE_READ | MODE_WRITE | MODE_DELETE, TRUE);

          /* and equipment (except /variables) */
          db_find_key(hDB, 0, "/Equipment", &hKeyRoot);
          if (hKeyRoot)
            {
            db_set_mode(hDB, hKeyRoot, MODE_READ | MODE_WRITE | MODE_DELETE, FALSE);

            for (i=0 ; ; i++)
              {
              status = db_enum_key(hDB, hKeyRoot, i, &hKeyEq);
              if (status == DB_NO_MORE_SUBKEYS)
                break;
              
              db_set_mode(hDB, hKeyEq, MODE_READ | MODE_WRITE | MODE_DELETE, TRUE);

              db_find_key(hDB, hKeyEq, "Variables", &hKey);
              if (hKey)
                db_set_mode(hDB, hKey, MODE_READ, TRUE);
              }
            }
          }

        /* close open records to parameters */
        init_module_parameters(TRUE);

        db_paste(hDB, 0, (char *) (pevent+1));

        if (flag == 1)
          db_set_mode(hDB, 0, MODE_READ | MODE_WRITE | MODE_DELETE, TRUE);

        /* reload parameter files after BOR event */
        printf("OK\n");
        load_parameters(run_number);

        /* open module parameters again */
        init_module_parameters(FALSE);
        }
      }

    /* copy system events (BOR, EOR, MESSAGE) to output file */
    if (pevent->event_id < 0)
      {
      if (out_file && out_format == FORMAT_MIDAS)
        {
        size = pevent->data_size + sizeof(EVENT_HEADER);
        if (out_gzip)
          status = gzwrite(out_file, pevent, size) == size ? SUCCESS : SS_FILE_ERROR;
        else
          status = fwrite(pevent, 1, size, out_file) == (size_t) size ? SUCCESS : SS_FILE_ERROR;

        if (status != SUCCESS)
          {
          cm_msg(MERROR, "analyze_file", "Error writing to file (Disk full?)");
          return -1;
          }

        num_events_out++;
        }
      }

    /* check if event is in event limit */
    skip = FALSE;

    if (clp.n[0] > 0 || clp.n[1] > 0)
      {
      if (clp.n[1] == 0)
        {
        /* treat n[0] as upper limit */
        if (num_events_in > clp.n[0])
          {
          num_events_in--;
          status = SUCCESS;
          break;
          }
        }
      else
        {
        if (num_events_in > clp.n[1])
          {
          status = SUCCESS;
          break;
          }
        if (num_events_in < clp.n[0])
          skip = TRUE;
        else if (clp.n[2] > 0 &&
                 num_events_in % clp.n[2] != 0)
            skip = TRUE;
        }
      }

    if (!skip)
      {
      /* find request belonging to this event */
      par = analyze_request;
      status = SUCCESS;
      for (i=0 ; par->event_name[0] ; par++)
        if ((par->ar_info.event_id     == EVENTID_ALL ||
             par->ar_info.event_id     == pevent->event_id) &&
            (par->ar_info.trigger_mask == TRIGGER_ALL ||
            (par->ar_info.trigger_mask & pevent->trigger_mask)) &&
             par->ar_info.enabled)
        {
        /* analyze this event */
        status = process_event(par, pevent);
        if (status == SUCCESS)
          num_events_out++;
        if (status < 0 || status == RPC_SHUTDOWN) /* disk full/stop analyzer */
          break;
        }
      if (status < 0 || status == RPC_SHUTDOWN)
        break;
      }

    /* update ODB statistics once every 100 events */
    if (num_events_in % 100 == 0)
      {
      update_stats();
      if (!clp.quiet)
        {
        if (out_file)
          printf("%s:%d  %s:%d  events\r", input_file_name, num_events_in, 
                                           out_info.filename, num_events_out);
        else
          printf("%s:%d  events\r", input_file_name, num_events_in); 

#ifndef OS_WINNT
        fflush(stdout);
#endif  
        }
      }
    } while(1);

  update_stats();
  if (out_file)
    printf("%s:%d  %s:%d  events\n", input_file_name, num_events_in, 
                                     out_info.filename, num_events_out);
  else
    printf("%s:%d  events\n", input_file_name, num_events_in); 

  /* call analyzer eor routines */
  eor(run_number, error);

  gzclose(file);

  free(pevent_unaligned);

  return status;
}

/*------------------------------------------------------------------*/

INT loop_runs_offline()
{
INT  i, status, run_number;
char input_file_name[256], output_file_name[256];
BANK_LIST *bank_list;

  printf("Running analyzer offline. Stop with \"!\"\n");

  run_number = 0;
  out_append = (strchr(clp.input_file_name[0], '%') != NULL) &&
               (strchr(clp.output_file_name, '%') == NULL);

  /* loop over range of files */
  if (clp.run_number[0] > 0)
    {
    if (strchr(clp.input_file_name[0], '%') == NULL)
      {
      printf("Input file name must contain a wildcard like \"%%05d\" when using a range.\n");
      return 0;
      }

    if (clp.run_number[0] == 0)
      {
      printf("End of range not specified.\n");
      return 0;
      }

    for (run_number=clp.run_number[0] ; run_number<=clp.run_number[1] ; run_number++)
      {
      sprintf(input_file_name, clp.input_file_name[0], run_number);
      if (strchr(clp.output_file_name, '%') != NULL)
        sprintf(output_file_name, clp.output_file_name, run_number);
      else
        strcpy(output_file_name, clp.output_file_name);

      status = analyze_file(run_number, input_file_name, output_file_name);
      }
    }
  else
    {
    /* loop over input file names */
    for (i=0 ; clp.input_file_name[i][0] && i<10 ; i++)
      {
      strcpy(input_file_name, clp.input_file_name[i]);

      /* get run number from input file */
      if (strpbrk(clp.input_file_name[i], "0123456789"))
        run_number = atoi(strpbrk(clp.input_file_name[i], "0123456789"));

      if (strchr(clp.output_file_name, '%') != NULL)
        {
        if (run_number == 0)
          {
          printf("Cannot extract run number from input file name.\n");
          return 0;
          }
        sprintf(output_file_name, clp.output_file_name, run_number);
        }
      else
        strcpy(output_file_name, clp.output_file_name);

      status = analyze_file(run_number, input_file_name, output_file_name);
      if (status != CM_SUCCESS)
        break;
      }
    }

  /* close output file in append mode */
  if (out_file && out_append)
    {
    if (out_format == FORMAT_HBOOK)
      {
      HROUT(0, i, " ");
      HREND("OFFLINE");
      }
    else
      {
      if (out_gzip)
        gzclose(out_file);
      else
        fclose(out_file);
      }

    /* free bank buffer */
    for (i=0 ; analyze_request[i].event_name[0] ; i++)
      {
      bank_list = analyze_request[i].bank_list;

      if (bank_list == NULL)
        continue;

      for (; bank_list->name[0] ; bank_list++)
        if (bank_list->addr)
          {
          free(bank_list->addr);
          bank_list->addr = NULL;
          }
      }
    }

  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

main(int argc, char *argv[])
{
INT status;

  /* get default from environment */
  cm_get_environment(clp.host_name, clp.exp_name);

  /* read in command line parameters into clp structure */
  status = getparam(argc, argv);
  if (status != CM_SUCCESS)
    return 1;

  /* set online mode if no input filename is given */
  clp.online = (clp.input_file_name[0][0] == 0);

  /* set Ntuple format to RWNT if online */
  if (clp.online)
    clp.rwnt = TRUE;

  /* now connect to server */
  if (clp.online)
    {
    if (clp.host_name[0])
      printf("Connect to experiment %s on host %s...", clp.exp_name, clp.host_name);
    else
      printf("Connect to experiment %s...", clp.exp_name);
    }

  status = cm_connect_experiment1(clp.host_name, clp.exp_name, analyzer_name, NULL, odb_size);
  if (status != CM_SUCCESS)
    return 1;

  if (clp.online)
    printf("OK\n");

  /* set online/offline mode */
  cm_get_experiment_database(&hDB, NULL);
  db_set_value(hDB, 0, "/Runinfo/Online Mode", &clp.online, sizeof(clp.online), 1, TID_INT);

  if (clp.online)
    {
    /* check for duplicate name */
    status = cm_exist(analyzer_name, FALSE);
    if (status == CM_SUCCESS)
      {
      cm_disconnect_experiment();
      printf("An analyzer named \"%s\" is already running in this experiment.\n", analyzer_name);
      printf("Please select another analyzer name in analyzer.c or stop other analyzer.\n");
      return 1;
      }
  
    /* register transitions if started online */
    if (cm_register_transition(TR_PRESTART, tr_prestart) != CM_SUCCESS ||
        cm_register_transition(TR_STOP, tr_stop) != CM_SUCCESS ||
        cm_register_transition(TR_PAUSE, tr_pause) != CM_SUCCESS ||
        cm_register_transition(TR_RESUME, tr_resume) != CM_SUCCESS)
      {
      printf("Failed to start local RPC server");
      return 1;
      }
    }

  /* register callback for clearing histos */
  cm_register_function(RPC_ANA_CLEAR_HISTOS, ana_callback);

  /* turn on keepalive messages */
  cm_set_watchdog_params(TRUE, DEFAULT_RPC_TIMEOUT);

  /* decrease watchdog timeout in offline mode */
  if (!clp.online)
    cm_set_watchdog_params(TRUE, 2000);

  /* turn off watchdog if in debug mode */
  if (clp.debug)
    cm_set_watchdog_params(0, 0);

  /* initialize module parameters */
  if (init_module_parameters(FALSE) != CM_SUCCESS)
    {
    cm_disconnect_experiment();
    return 1;
    }

  /* analyzer init function */
  if (mana_init() != CM_SUCCESS)
    {
    cm_disconnect_experiment();
    return 1;
    }

  /* reqister event requests */
  register_requests();

  /* initialize ss_getchar */
  ss_getchar(0);

  /* start main loop */
  if (clp.online)
    loop_online();
  else
    loop_runs_offline();

  /* reset terminal */
  ss_getchar(TRUE);

  /* call exit function */
  mana_exit();

  /* close network connection to server */
  cm_disconnect_experiment();

  return 0;
}
