/********************************************************************\

  Name:         mana.c
  Created by:   Stefan Ritt

  Contents:     The system part of the MIDAS analyzer. Has to be
                linked with analyze.c to form a complete analyzer

  $Id$

\********************************************************************/

#include <assert.h>
#include "midas.h"
#include "msystem.h"
#include "hardware.h"

#include "mdsupport.h"

#ifdef HAVE_ZLIB
#include "zlib.h"
#endif

/*------------------------------------------------------------------*/

/* cernlib includes */
#ifdef OS_WINNT
#define VISUAL_CPLUSPLUS
#endif

#ifdef OS_LINUX
#define f2cFortran
#endif

#ifdef HAVE_HBOOK

#include <cfortran.h>
#include <hbook.h>

/* missing in hbook.h */
#ifndef HFNOV
#define HFNOV(A1,A2)  CCALLSFSUB2(HFNOV,hfnov,INT,FLOATV,A1,A2)
#endif

#ifndef HMERGE
#define HMERGE(A1,A2,A3)  CCALLSFSUB3(HMERGE,hmerge,INT,STRINGV,STRING,A1,A2,A3)
#endif

#endif                          /* HAVE_HBOOK */

ANA_OUTPUT_INFO out_info;

/*------------------------------------------------------------------*/

#ifdef USE_ROOT

#undef abs

#include <assert.h>
#include <TApplication.h>
#include <TKey.h>
#include <TROOT.h>
#include <TH1.h>
#include <TH2.h>
#include <TFile.h>
#include <TTree.h>
#include <TLeaf.h>
#include <TSocket.h>
#include <TServerSocket.h>
#include <TMessage.h>
#include <TObjString.h>
#include <TSystem.h>
#include <TFolder.h>
#include <TRint.h>
#include <TCutG.h>

#ifdef OS_LINUX
#include <TThread.h>
#endif

/* Our own ROOT global objects */

TApplication *manaApp;
TFolder *gManaHistosFolder = NULL;      // Container for all histograms
TFile *gManaOutputFile = NULL;  // MIDAS output file
TObjArray *gHistoFolderStack = NULL;    //

/* MIDAS output tree structure holing one tree per event type */

typedef struct {
   int event_id;
   TTree *tree;
   int n_branch;
   char *branch_name;
   int *branch_filled;
   int *branch_len;
   TBranch **branch;
} EVENT_TREE;

typedef struct {
   TFile *f;
   int n_tree;
   EVENT_TREE *event_tree;
} TREE_STRUCT;

TREE_STRUCT tree_struct;

#endif                          /* USE_ROOT */

/*------------------------------------------------------------------*/

/* PVM include */

#ifdef HAVE_PVM

INT pvm_start_time = 0;

extern BOOL disable_shm_write;

void pvm_debug(char *format, ...)
{
   va_list argptr;
   char msg[256], str[256];

   if (pvm_start_time == 0)
      pvm_start_time = ss_millitime();

   va_start(argptr, format);
   vsprintf(msg, (char *) format, argptr);
   va_end(argptr);
   sprintf(str, "%1.3lf:  %s", (ss_millitime() - pvm_start_time) / 1000.0, msg);
   puts(str);
#ifdef OS_LINUX
   {
      char cmd[256];

      sprintf(cmd, "echo > /dev/console \"%s\"", str);
      system(cmd);
   }
#endif
}

#undef STRICT
#include <pvm3.h>
#endif

/*----- PVM TAGS and data ------------------------------------------*/

/* buffer size must be larger than largest event (ODB dump!) */
#define PVM_BUFFER_SIZE (1024*1024)

#ifdef HAVE_PVM
#define TAG_DATA  1
#define TAG_BOR   2
#define TAG_EOR   3
#define TAG_EXIT  4
#define TAG_INIT  5

int pvm_n_task;
int pvm_myparent;
int pvm_client_index;

typedef struct {
   int tid;
   char host[80];
   char *buffer;
   int wp;
   DWORD n_events;
   DWORD time;
   BOOL eor_sent;
} PVM_CLIENT;

PVM_CLIENT *pvmc;

int pvm_eor(int);
int pvm_merge(void);
void pvm_debug(char *format, ...);
int pvm_distribute(ANALYZE_REQUEST * par, EVENT_HEADER * pevent);

//#define PVM_DEBUG pvm_debug
#define PVM_DEBUG

#endif                          /* PVM */

BOOL pvm_master = FALSE, pvm_slave = FALSE;

char *bstr = " ";

/*------------------------------------------------------------------*/

/* items defined in analyzer.c */
extern char *analyzer_name;
extern INT analyzer_loop_period;
extern INT analyzer_init(void);
extern INT analyzer_exit(void);
extern INT analyzer_loop(void);
extern INT ana_begin_of_run(INT run_number, char *error);
extern INT ana_end_of_run(INT run_number, char *error);
extern INT ana_pause_run(INT run_number, char *error);
extern INT ana_resume_run(INT run_number, char *error);
extern INT odb_size;

/*---- globals -----------------------------------------------------*/

/* ODB handle */
HNDLE hDB;

/* run number */
DWORD current_run_number;

/* analyze_request defined in analyze.c or anasys.c */
extern ANALYZE_REQUEST analyze_request[];

/* N-tupel booking and filling flag */
BOOL ntuple_flag;

/* list of locked histos (won't get cleared) */
INT lock_list[10000];

#ifdef extname
int quest_[100];
#else
extern int QUEST[100];
#endif

extern INT pawc_size;

/* default HBOOK record size */
#define HBOOK_LREC 8190

/* maximum extended event size */
#ifndef EXT_EVENT_SIZE
#define EXT_EVENT_SIZE (2*(MAX_EVENT_SIZE+sizeof(EVENT_HEADER)))
#endif

/* command line parameters */
static struct {
   INT online;
   char host_name[HOST_NAME_LENGTH];
   char exp_name[NAME_LENGTH];
   char input_file_name[10][256];
   char output_file_name[256];
   INT run_number[2];
   DWORD n[4];
   BOOL filter;
   char config_file_name[10][256];
   char param[10][256];
   char protect[10][256];
   BOOL rwnt;
   INT lrec;
   BOOL debug;
   BOOL verbose;
   BOOL quiet;
   BOOL no_load;
   BOOL daemon;
   INT n_task;
   INT pvm_buf_size;
   INT root_port;
   BOOL start_rint;
} clp;

static struct {
   char flag_char;
   char description[1000];
   void *data;
   INT type;
   INT n;
   INT index;
} clp_descrip[] = {

   {
   'c', "<filename1>   Configuration file name(s). May contain a '%05d' to be\n\
     <filename2>   replaced by the run number. Up to ten files can be\n\
         ...       specified in one \"-c\" statement", clp.config_file_name, TID_STRING, 10}, {
   'd', "              Debug flag when started the analyzer fron a debugger.\n\
                   Prevents the system to kill the analyzer when the\n\
                   debugger stops at a breakpoint", &clp.debug, TID_BOOL, 0}, {
   'D', "              Start analyzer as a daemon in the background (UNIX only).",
          &clp.daemon, TID_BOOL, 0}, {
   'e', "<experiment>  MIDAS experiment to connect to", clp.exp_name, TID_STRING, 1}, {
   'f', "              Filter mode. Write original events to output file\n\
                   only if analyzer accepts them (doesn't return ANA_SKIP).\n", &clp.filter, TID_BOOL, 0}, {
   'h', "<hostname>    MIDAS host to connect to when running the analyzer online",
          clp.host_name, TID_STRING, 1}, {
   'i', "<filename1>   Input file name. May contain a '%05d' to be replaced by\n\
     <filename2>   the run number. Up to ten input files can be specified\n\
         ...       in one \"-i\" statement", clp.input_file_name, TID_STRING, 10}, {
   'l', "              If set, don't load histos from last histo file when\n\
                   running online.", &clp.no_load, TID_BOOL, 0}, {
   'L', "              HBOOK LREC size. Default is 8190.", &clp.lrec, TID_INT, 0}, {
   'n', "<count>       Analyze only \"count\" events.\n\
     <first> <last>\n\
                   Analyze only events from \"first\" to \"last\".\n\
     <first> <last> <n>\n\
                   Analyze every n-th event from \"first\" to \"last\".", clp.n, TID_INT, 4}, {
   'o', "<filename>    Output file name. Extension may be .mid (MIDAS binary),\n\
                   .asc (ASCII) or .rz (HBOOK). If the name contains a '%05d',\n\
                   one output file is generated for each run. Use \"OFLN\" as\n\
                   output file name to creaate a HBOOK shared memory instead\n\
                   of a file.", clp.output_file_name, TID_STRING, 1}, {
   'p', "<param=value> Set individual parameters to a specific value.\n\
                   Overrides any setting in configuration files", clp.param, TID_STRING, 10}, {
   'P', "<ODB tree>    Protect an ODB subtree from being overwritten\n\
                   with the online data when ODB gets loaded from .mid file", clp.protect, TID_STRING, 10}, {
   'q', "              Quiet flag. If set, don't display run progress in\n\
                   offline mode.", &clp.quiet, TID_BOOL, 0}, {
   'r', "<range>       Range of run numbers to analyzer like \"-r 120 125\"\n\
                   to analyze runs 120 to 125 (inclusive). The \"-r\"\n\
                   flag must be used with a '%05d' in the input file name.", clp.run_number, TID_INT, 2},
#ifdef HAVE_PVM
   {
   't', "<n>           Parallelize analyzer using <n> tasks with PVM.", &clp.n_task,
          TID_INT, 1}, {
   'b', "<n>           Buffer size for parallelization in kB.", &clp.pvm_buf_size,
          TID_INT, 1},
#endif
#ifdef USE_ROOT
   {
   's', "<port>        Start ROOT histo server under <port>. If port==0, don't start server.", &clp.root_port, TID_INT, 1}, {
   'R', "              Start ROOT interpreter after analysis has finished.",
          &clp.start_rint, TID_BOOL, 0},
#endif
   {
   'v', "              Verbose output.", &clp.verbose, TID_BOOL, 0}, {
   'w', "              Produce row-wise N-tuples in outpur .rz file. By\n\
                   default, column-wise N-tuples are used.", &clp.rwnt, TID_BOOL, 0}, {
   0}
};

FILE *out_file;
#ifdef HAVE_ZLIB
BOOL out_gzip;
#endif
INT out_format;
BOOL out_append;

void update_stats();
void odb_load(EVENT_HEADER * pevent);

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
Events received = DOUBLE : 0\n\
Events per sec. = DOUBLE : 0\n\
Events written = DOUBLE : 0\n\
"

/*-- interprete command line parameters ----------------------------*/

INT getparam(int argc, char **argv)
{
   INT index, i, j, size;

   /* parse command line parameters */
   for (index = 1; index < argc;) {
      /* search flag in parameter description */
      if (argv[index][0] == '-') {
         for (j = 0; clp_descrip[j].flag_char; j++)
            if (argv[index][1] == clp_descrip[j].flag_char)
               break;

         if (!clp_descrip[j].flag_char)
            goto usage;

         if (clp_descrip[j].n > 0 && index >= argc - 1)
            goto usage;
         index++;

         if (clp_descrip[j].type == TID_BOOL) {
            *((BOOL *) clp_descrip[j].data) = TRUE;
            continue;
         }

         do {
            if (clp_descrip[j].type == TID_STRING)
               strcpy((char *) clp_descrip[j].data + clp_descrip[j].index * 256,
                      argv[index]);
            else
               db_sscanf(argv[index], clp_descrip[j].data, &size, clp_descrip[j].index,
                         clp_descrip[j].type);

            if (clp_descrip[j].n > 1)
               clp_descrip[j].index++;

            if (clp_descrip[j].index > clp_descrip[j].n) {
               printf("Note more than %d options possible for flag -%c\n",
                      clp_descrip[j].n, clp_descrip[j].flag_char);
               return 0;
            }

            index++;

         } while (index < argc && argv[index][0] != '-');

      } else
         goto usage;
   }

   return SUCCESS;

 usage:

   printf("usage: analyzer [options]\n\n");
   printf("valid options are:\n");
   for (i = 0; clp_descrip[i].flag_char; i++)
      printf("  -%c %s\n", clp_descrip[i].flag_char, clp_descrip[i].description);

   return 0;
}

/*-- add </logger/data dir> before filename ------------------------*/

void add_data_dir(char *result, char *file)
{
   HNDLE hDB, hkey;
   char str[256];
   int size;

   cm_get_experiment_database(&hDB, NULL);
   db_find_key(hDB, 0, "/Logger/Data dir", &hkey);

   if (hkey) {
      size = sizeof(str);
      db_get_data(hDB, hkey, str, &size, TID_STRING);
      if (str[strlen(str) - 1] != DIR_SEPARATOR)
         strcat(str, DIR_SEPARATOR_STR);
      strcat(str, file);
      strcpy(result, str);
   } else
      strcpy(result, file);
}

/*-- db_get_event_definition ---------------------------------------*/

typedef struct {
   short int event_id;
   int type;
   WORD format;
   HNDLE hDefKey;
   BOOL disabled;
} EVENT_DEF;

EVENT_DEF *db_get_event_definition(short int event_id)
{
   INT i, index, status, size, type;
   char str[80];
   HNDLE hKey, hKeyRoot;
   WORD id;
   static EVENT_DEF *event_def = NULL;
   static int n_cache = 0;

   /* search free cache entry */
   for (index = 0; index < n_cache; index++)
      if (event_def[index].event_id == event_id)
         return &event_def[index];

   /* If we get here, we have an undefined ID;
      allocate memory for it, zero it, then cache the ODB data */
   n_cache = index + 1;

   event_def = (EVENT_DEF *) realloc(event_def, (n_cache) * sizeof(EVENT_DEF));
   assert(event_def);

   memset(&event_def[index], 0, sizeof(EVENT_DEF));

   /* check for system events */
   if (event_id < 0) {
      event_def[index].event_id = event_id;
      event_def[index].format = FORMAT_ASCII;
      event_def[index].hDefKey = 0;
      event_def[index].disabled = FALSE;
      return &event_def[index];
   }

   status = db_find_key(hDB, 0, "/equipment", &hKeyRoot);
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "db_get_event_definition", "cannot find /equipment entry in ODB");
      return NULL;
   }

   for (i = 0;; i++) {
      /* search for equipment with specific name */
      status = db_enum_key(hDB, hKeyRoot, i, &hKey);
      if (status == DB_NO_MORE_SUBKEYS) {
         sprintf(str, "Cannot find event id %d under /equipment", event_id);
         cm_msg(MERROR, "db_get_event_definition", str);
         return NULL;
      }

      size = sizeof(id);
      status = db_get_value(hDB, hKey, "Common/Event ID", &id, &size, TID_WORD, TRUE);
      if (status != DB_SUCCESS)
         continue;

      size = sizeof(type);
      status = db_get_value(hDB, hKey, "Common/Type", &type, &size, TID_INT, TRUE);
      if (status != DB_SUCCESS)
         continue;

      if (id == event_id) {
         /* set cache entry */
         event_def[index].event_id = id;
         event_def[index].type = type;

         size = sizeof(str);
         str[0] = 0;
         db_get_value(hDB, hKey, "Common/Format", str, &size, TID_STRING, TRUE);

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
         else {
            cm_msg(MERROR, "db_get_event_definition", "unknown data format");
            event_def[index].event_id = 0;
            return NULL;
         }

         db_find_key(hDB, hKey, "Variables", &event_def[index].hDefKey);
         return &event_def[index];
      }
   }
}

/*-- functions for internal tests ----------------------------------*/

ANA_TEST **tl;
int n_test = 0;

void test_register(ANA_TEST * t)
{
   int i;

   /* check if test already registered */
   for (i = 0; i < n_test; i++)
      if (tl[i] == t)
         break;
   if (i < n_test) {
      t->registered = TRUE;
      return;
   }

   /* allocate space for pointer to test */
   if (n_test == 0) {
      tl = (ANA_TEST **) malloc(2 * sizeof(void *));

      /* define "always true" test */
      tl[0] = (ANA_TEST *) malloc(sizeof(ANA_TEST));
      strcpy(tl[0]->name, "Always true");
      tl[0]->count = 0;
      tl[0]->previous_count = 0;
      tl[0]->value = TRUE;
      tl[0]->registered = TRUE;
      n_test++;
   } else
      tl = (ANA_TEST **) realloc(tl, (n_test + 1) * sizeof(void *));

   tl[n_test] = t;
   t->count = 0;
   t->value = FALSE;
   t->registered = TRUE;

   n_test++;
}

void test_clear()
{
   int i;

   /* clear all tests in interal list */
   for (i = 0; i < n_test; i++) {
      tl[i]->count = 0;
      tl[i]->value = FALSE;
   }

   /* set "always true" test */
   if (n_test > 0)
      tl[0]->value = TRUE;
}

void test_increment()
{
   int i;

   /* increment test counters based on their value and reset them */
   for (i = 0; i < n_test; i++) {
      if (tl[i]->value)
         tl[i]->count++;
      if (i > 0)
         tl[i]->value = FALSE;
   }
}

void test_write(int delta_time)
{
   int i;
   char str[256];
   float rate;

   /* write all test counts to /analyzer/tests/<name> */
   for (i = 0; i < n_test; i++) {
      sprintf(str, "/%s/Tests/%s/Count", analyzer_name, tl[i]->name);
      db_set_value(hDB, 0, str, &tl[i]->count, sizeof(DWORD), 1, TID_DWORD);

      /* calcluate rate */
      if (delta_time > 0) {
         rate = (float) ((tl[i]->count - tl[i]->previous_count) / (delta_time / 1000.0));
         tl[i]->previous_count = tl[i]->count;
         sprintf(str, "/%s/Tests/%s/Rate [Hz]", analyzer_name, tl[i]->name);
         db_set_value(hDB, 0, str, &rate, sizeof(float), 1, TID_FLOAT);
      }
   }
}

/*-- load parameters specified on command line ---------------------*/

INT load_parameters(INT run_number)
{
   INT i, size, index, status;
   HNDLE hkey;
   char file_name[256], str[80], value_string[80], param_string[80];
   char data[32];
   KEY key;

   /* loop over configutation file names */
   for (i = 0; clp.config_file_name[i][0] && i < 10; i++) {
      if (strchr(clp.config_file_name[i], '%') != NULL)
         sprintf(file_name, clp.config_file_name[i], run_number);
      else
         strcpy(file_name, clp.config_file_name[i]);

      /* load file under "/" */
      if (db_load(hDB, 0, file_name, FALSE) == DB_SUCCESS)
         printf("Configuration file \"%s\" loaded\n", file_name);
   }

   /* loop over parameters */
   for (i = 0; clp.param[i][0] && i < 10; i++) {
      if (strchr(clp.param[i], '=') == NULL) {
         printf("Error: parameter %s contains no value\n", clp.param[i]);
      } else {
         strcpy(value_string, strchr(clp.param[i], '=') + 1);
         strcpy(param_string, clp.param[i]);
         *strchr(param_string, '=') = 0;

         index = 0;
         if (strchr(param_string, '[') != NULL) {
            index = atoi(strchr(param_string, '[') + 1);
            *strchr(param_string, '[') = 0;
         }

         if (param_string[0] == '/')
            strcpy(str, param_string);
         else
            sprintf(str, "/%s/Parameters/%s", analyzer_name, param_string);
         db_find_key(hDB, 0, str, &hkey);
         if (hkey == 0) {
            printf("Error: cannot find parameter %s in ODB\n", str);
         } else {
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
   ":U:8",                      /* TID_BYTE      */
   ":I:8",                      /* TID_SBYTE     */
   ":I:8",                      /* TID_CHAR      */
   ":U:16",                     /* TID_WORD      */
   ":I:16",                     /* TID_SHORT     */
   ":U*4",                      /* TID_DWORD     */
   ":I*4",                      /* TID_INT       */
   ":I*4",                      /* TID_BOOL      */
   ":R*4",                      /* TID_FLOAT     */
   ":R*8",                      /* TID_DOUBLE    */
   ":U:8",                      /* TID_BITFIELD  */
   ":C:32",                     /* TID_STRING    */
   "",                          /* TID_ARRAY     */
   "",                          /* TID_STRUCT    */
   "",                          /* TID_KEY       */
   "",                          /* TID_LINK      */
   "",                          /* TID_LAST      */

};

INT book_ntuples(void);
INT book_ttree(void);

void banks_changed(INT hDB, INT hKey, void *info)
{
   char str[80];
   HNDLE hkey;

   /* close previously opened hot link */
   sprintf(str, "/%s/Bank switches", analyzer_name);
   db_find_key(hDB, 0, str, &hkey);
   db_close_record(hDB, hkey);

#ifdef HAVE_HBOOK
   book_ntuples();
   printf("N-tuples rebooked\n");
#endif
#ifdef USE_ROOT
   book_ttree();
   printf("ROOT TTree rebooked\n");
#endif
}

#ifdef HAVE_HBOOK
INT book_ntuples(void)
{
   INT index, i, j, status, n_tag, size, id;
   HNDLE hkey;
   KEY key;
   int *t;
   char ch_tags[2000];
   char rw_tag[512][8];
   char str[80], str2[80], key_name[NAME_LENGTH], block_name[NAME_LENGTH];
   BANK_LIST *bank_list;
   EVENT_DEF *event_def;

   /* check global N-tuple flag */
   ntuple_flag = 1;
   size = sizeof(ntuple_flag);
   sprintf(str, "/%s/Book N-tuples", analyzer_name);
   db_get_value(hDB, 0, str, &ntuple_flag, &size, TID_BOOL, TRUE);

   if (!ntuple_flag)
      return SUCCESS;

   /* copy output flag from ODB to bank_list */
   for (i = 0; analyze_request[i].event_name[0]; i++) {
      bank_list = analyze_request[i].bank_list;

      if (bank_list != NULL)
         for (; bank_list->name[0]; bank_list++) {
            sprintf(str, "/%s/Bank switches/%s", analyzer_name, bank_list->name);
            bank_list->output_flag = FALSE;
            size = sizeof(DWORD);
            db_get_value(hDB, 0, str, &bank_list->output_flag, &size, TID_DWORD, TRUE);
         }
   }

   /* hot link bank switches to N-tuple re-booking */
   sprintf(str, "/%s/Bank switches", analyzer_name);
   status = db_find_key(hDB, 0, str, &hkey);
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "book_ntuples", "Cannot find key \'%s\', status %d", str, status);
      return SUCCESS;
   }

   db_open_record(hDB, hkey, NULL, 0, MODE_READ, banks_changed, NULL);

   if (!clp.rwnt) {
      /* book CW N-tuples */

      /* go through all analyzer requests (events) */
      for (index = 0; analyze_request[index].event_name[0]; index++) {
         /* book N-tuple with evend ID */
         HBNT(analyze_request[index].ar_info.event_id, analyze_request[index].event_name,
              bstr);

         /* book run number/event number/time */
         strcpy(str, "Number");
         strcpy(str2, "Run:U*4,Number:U*4,Time:U*4");
         t = (int *) (&analyze_request[index].number.run);
         HBNAME(analyze_request[index].ar_info.event_id, str, t, str2);

         bank_list = analyze_request[index].bank_list;
         if (bank_list == NULL) {
            /* book fixed event */
            event_def =
                db_get_event_definition((short int) analyze_request[index].ar_info.
                                        event_id);
            if (event_def == NULL) {
               cm_msg(MERROR, "book_ntuples", "Cannot find definition of event %s in ODB",
                      analyze_request[index].event_name);
               return 0;
            }

            ch_tags[0] = 0;
            for (i = 0;; i++) {
               status = db_enum_key(hDB, event_def->hDefKey, i, &hkey);
               if (status == DB_NO_MORE_SUBKEYS)
                  break;

               db_get_key(hDB, hkey, &key);

               /* convert blanks etc to '_' */
               strcpy(str, key.name);
               for (j = 0; str[j]; j++) {
                  if (!(str[j] >= 'a' && str[j] <= 'z') &&
                      !(str[j] >= 'A' && str[j] <= 'Z') && !(str[j] >= '0'
                                                             && str[j] <= '9'))
                     str[j] = '_';
               }
               strcat(ch_tags, str);
               str[0] = 0;

               if (key.num_values > 1)
                  sprintf(str, "(%d)", key.num_values);

               if (hbook_types[key.type] != NULL)
                  strcat(str, hbook_types[key.type]);
               else {
                  cm_msg(MERROR, "book_ntuples",
                         "Key %s in event %s is of type %s with no HBOOK correspondence",
                         key.name, analyze_request[index].event_name,
                         rpc_tid_name(key.type));
                  return 0;
               }
               strcat(ch_tags, str);
               strcat(ch_tags, ",");
            }

            ch_tags[strlen(ch_tags) - 1] = 0;
            db_get_record_size(hDB, event_def->hDefKey, 0, &size);
            analyze_request[index].addr = calloc(1, size);

            strcpy(block_name, analyze_request[index].event_name);
            block_name[8] = 0;

            HBNAME(analyze_request[index].ar_info.event_id, block_name,
                   analyze_request[index].addr, ch_tags);
         } else {
            /* go thorough all banks in bank_list */
            for (; bank_list->name[0]; bank_list++) {
               if (bank_list->output_flag == 0)
                  continue;

               if (bank_list->type != TID_STRUCT) {
                  sprintf(str, "N%s[0,%d]", bank_list->name, bank_list->size);
                  INT *t = (INT *) & bank_list->n_data;
                  HBNAME(analyze_request[index].ar_info.event_id,
                         bank_list->name, t, str);

                  sprintf(str, "%s(N%s)", bank_list->name, bank_list->name);

                  /* define variable length array */
                  if (hbook_types[bank_list->type] != NULL)
                     strcat(str, hbook_types[bank_list->type]);
                  else {
                     cm_msg(MERROR, "book_ntuples",
                            "Bank %s is of type %s with no HBOOK correspondence",
                            bank_list->name, rpc_tid_name(bank_list->type));
                     return 0;
                  }

                  if (rpc_tid_size(bank_list->type) == 0) {
                     cm_msg(MERROR, "book_ntuples",
                            "Bank %s is of type with unknown size", bank_list->name);
                     return 0;
                  }

                  bank_list->addr =
                      calloc(bank_list->size, MAX(4, rpc_tid_size(bank_list->type)));

                  HBNAME(analyze_request[index].ar_info.event_id, bank_list->name,
                         bank_list->addr, str);
               } else {
                  /* define structured bank */
                  ch_tags[0] = 0;
                  for (i = 0;; i++) {
                     status = db_enum_key(hDB, bank_list->def_key, i, &hkey);
                     if (status == DB_NO_MORE_SUBKEYS)
                        break;

                     db_get_key(hDB, hkey, &key);

                     /* convert blanks etc to '_' */
                     strcpy(str, key.name);
                     for (j = 0; str[j]; j++) {
                        if (!(str[j] >= 'a' && str[j] <= 'z') &&
                            !(str[j] >= 'A' && str[j] <= 'Z') && !(str[j] >= '0'
                                                                   && str[j] <= '9'))
                           str[j] = '_';
                     }
                     strcat(ch_tags, str);
                     str[0] = 0;

                     if (key.num_values > 1)
                        sprintf(str, "(%d)", key.num_values);

                     if (hbook_types[key.type] != NULL)
                        strcat(str, hbook_types[key.type]);
                     else {
                        cm_msg(MERROR, "book_ntuples",
                               "Key %s in bank %s is of type %s with no HBOOK correspondence",
                               key.name, bank_list->name, rpc_tid_name(key.type));
                        return 0;
                     }
                     strcat(ch_tags, str);
                     strcat(ch_tags, ",");
                  }

                  ch_tags[strlen(ch_tags) - 1] = 0;
                  bank_list->addr = calloc(1, bank_list->size);

                  HBNAME(analyze_request[index].ar_info.event_id, bank_list->name,
                         bank_list->addr, ch_tags);
               }
            }
         }

         /* HPRNT(analyze_request[index].ar_info.event_id); */
      }
   } else {
      /* book RW N-tuples */

      /* go through all analyzer requests (events) */
      for (index = 0; analyze_request[index].event_name[0]; index++) {
         /* get pointer to event definition */
         event_def =
             db_get_event_definition((short int) analyze_request[index].ar_info.event_id);

         /* skip if not found */
         if (!event_def)
            continue;

         /* don't book NT if not requested */
         if (analyze_request[index].rwnt_buffer_size == 0) {
            event_def->disabled = TRUE;
            continue;
         }

         n_tag = 0;

         strcpy(rw_tag[n_tag++], "Run");
         strcpy(rw_tag[n_tag++], "Number");
         strcpy(rw_tag[n_tag++], "Time");

         if (clp.verbose) {
            printf("NT #%d-1: Run\n", analyze_request[index].ar_info.event_id);
            printf("NT #%d-2: Number\n", analyze_request[index].ar_info.event_id);
            printf("NT #%d-3: Time\n", analyze_request[index].ar_info.event_id);
         }

         bank_list = analyze_request[index].bank_list;
         if (bank_list == NULL) {
            /* book fixed event */

            for (i = 0;; i++) {
               status = db_enum_key(hDB, event_def->hDefKey, i, &hkey);
               if (status == DB_NO_MORE_SUBKEYS)
                  break;

               db_get_key(hDB, hkey, &key);

               /* convert blanks etc to '_' */
               strcpy(key_name, key.name);
               for (j = 0; key_name[j]; j++) {
                  if (!(key_name[j] >= 'a' && key_name[j] <= 'z') &&
                      !(key_name[j] >= 'A' && key_name[j] <= 'Z') &&
                      !(key_name[j] >= '0' && key_name[j] <= '9'))
                     key_name[j] = '_';
               }

               if (key.num_values > 1)
                  for (j = 0; j < key.num_values; j++) {
                     sprintf(str, "%s%d", key_name, j);
                     strncpy(rw_tag[n_tag++], str, 8);

                     if (clp.verbose)
                        printf("NT #%d-%d: %s\n", analyze_request[index].ar_info.event_id,
                               n_tag + 1, str);

                     if (n_tag >= 512) {
                        cm_msg(MERROR, "book_ntuples",
                               "Too much tags for RW N-tupeles (512 maximum)");
                        return 0;
                     }
               } else {
                  strncpy(rw_tag[n_tag++], key_name, 8);

                  if (clp.verbose)
                     printf("NT #%d-%d: %s\n", analyze_request[index].ar_info.event_id,
                            n_tag, key_name);
               }

               if (n_tag >= 512) {
                  cm_msg(MERROR, "book_ntuples",
                         "Too much tags for RW N-tupeles (512 maximum)");
                  return 0;
               }
            }
         } else {
            /* go through all banks in bank_list */
            for (; bank_list->name[0]; bank_list++) {
               /* remember tag offset in n_data variable */
               bank_list->n_data = n_tag;

               if (bank_list->output_flag == 0)
                  continue;

               if (bank_list->type != TID_STRUCT) {
                  for (i = 0; i < (INT) bank_list->size; i++) {
                     sprintf(str, "%s%d", bank_list->name, i);
                     strncpy(rw_tag[n_tag++], str, 8);

                     if (clp.verbose)
                        printf("NT #%d-%d: %s\n", analyze_request[index].ar_info.event_id,
                               n_tag, str);
                     if (n_tag >= 512) {
                        cm_msg(MERROR, "book_ntuples",
                               "Too much tags for RW N-tupeles (512 maximum)");
                        return 0;
                     }
                  }
               } else {
                  /* define structured bank */
                  for (i = 0;; i++) {
                     status = db_enum_key(hDB, bank_list->def_key, i, &hkey);
                     if (status == DB_NO_MORE_SUBKEYS)
                        break;

                     db_get_key(hDB, hkey, &key);

                     /* convert blanks etc to '_' */
                     strcpy(key_name, key.name);
                     for (j = 0; key_name[j]; j++) {
                        if (!(key_name[j] >= 'a' && key_name[j] <= 'z') &&
                            !(key_name[j] >= 'A' && key_name[j] <= 'Z') &&
                            !(key_name[j] >= '0' && key_name[j] <= '9'))
                           key_name[j] = '_';
                     }

                     if (key.num_values > 1)
                        for (j = 0; j < key.num_values; j++) {
                           sprintf(str, "%s%d", key_name, j);
                           strncpy(rw_tag[n_tag++], str, 8);

                           if (clp.verbose)
                              printf("NT #%d-%d: %s\n",
                                     analyze_request[index].ar_info.event_id, n_tag, str);

                           if (n_tag >= 512) {
                              cm_msg(MERROR, "book_ntuples",
                                     "Too much tags for RW N-tupeles (512 maximum)");
                              return 0;
                           }
                     } else {
                        strncpy(rw_tag[n_tag++], key_name, 8);
                        if (clp.verbose)
                           printf("NT #%d-%d: %s\n",
                                  analyze_request[index].ar_info.event_id, n_tag,
                                  key_name);
                     }

                     if (n_tag >= 512) {
                        cm_msg(MERROR, "book_ntuples",
                               "Too much tags for RW N-tupeles (512 maximum)");
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

         if (clp.online || equal_ustring(clp.output_file_name, "OFLN"))
            HBOOKN(id, block_name, n_tag, bstr,
                   n_tag * analyze_request[index].rwnt_buffer_size, rw_tag);
         else {
            strcpy(str, "//OFFLINE");
            HBOOKN(id, block_name, n_tag, str, 5120, rw_tag);
         }

         if (!HEXIST(id)) {
            printf("\n");
            cm_msg(MINFO, "book_ntuples",
                   "Cannot book N-tuple #%d. Increase PAWC size via the -s flag or switch off banks",
                   id);
         }
      }
   }

   return SUCCESS;
}
#endif                          /* HAVE_HBOOK */

/*-- book TTree from ODB bank structures ---------------------------*/

#ifdef USE_ROOT

char ttree_types[][8] = {
   "",
   "b",                         /* TID_BYTE      */
   "B",                         /* TID_SBYTE     */
   "b",                         /* TID_CHAR      */
   "s",                         /* TID_WORD      */
   "S",                         /* TID_SHORT     */
   "i",                         /* TID_DWORD     */
   "I",                         /* TID_INT       */
   "I",                         /* TID_BOOL      */
   "F",                         /* TID_FLOAT     */
   "D",                         /* TID_DOUBLE    */
   "b",                         /* TID_BITFIELD  */
   "C",                         /* TID_STRING    */
   "",                          /* TID_ARRAY     */
   "",                          /* TID_STRUCT    */
   "",                          /* TID_KEY       */
   "",                          /* TID_LINK      */
   "",                          /* TID_LAST      */

};

INT book_ttree()
{
   INT index, i, status, size;
   HNDLE hkey;
   KEY key;
   char leaf_tags[2000];
   char str[80];
   BANK_LIST *bank_list;
   EVENT_DEF *event_def;
   EVENT_TREE *et;

   /* check global N-tuple flag */
   ntuple_flag = 1;
   size = sizeof(ntuple_flag);
   sprintf(str, "/%s/Book TTree", analyzer_name);
   db_get_value(hDB, 0, str, &ntuple_flag, &size, TID_BOOL, TRUE);

   if (!ntuple_flag)
      return SUCCESS;

   /* copy output flag from ODB to bank_list */
   for (i = 0; analyze_request[i].event_name[0]; i++) {
      bank_list = analyze_request[i].bank_list;

      if (bank_list != NULL)
         for (; bank_list->name[0]; bank_list++) {
            sprintf(str, "/%s/Bank switches/%s", analyzer_name, bank_list->name);
            bank_list->output_flag = FALSE;
            size = sizeof(DWORD);
            db_get_value(hDB, 0, str, &bank_list->output_flag, &size, TID_DWORD, TRUE);
         }
   }

   /* hot link bank switches to N-tuple re-booking */
   sprintf(str, "/%s/Bank switches", analyzer_name);
   status = db_find_key(hDB, 0, str, &hkey);
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "book_ttree", "Cannot find key \'%s\', status %d", str, status);
      return SUCCESS;
   }

   db_open_record(hDB, hkey, NULL, 0, MODE_READ, banks_changed, NULL);

   /* go through all analyzer requests (events) */
   for (index = 0; analyze_request[index].event_name[0]; index++) {
      /* create tree */
      tree_struct.n_tree++;
      if (tree_struct.n_tree == 1)
         tree_struct.event_tree = (EVENT_TREE *) malloc(sizeof(EVENT_TREE));
      else
         tree_struct.event_tree =
             (EVENT_TREE *) realloc(tree_struct.event_tree,
                                    sizeof(EVENT_TREE) * tree_struct.n_tree);

      et = tree_struct.event_tree + (tree_struct.n_tree - 1);

      et->event_id = analyze_request[index].ar_info.event_id;
      et->n_branch = 0;

      /* create tree */
      sprintf(str, "Event \"%s\", ID %d", analyze_request[index].event_name,
              et->event_id);
      et->tree = new TTree(analyze_request[index].event_name, str);
#if (ROOT_VERSION_CODE >= 262401)
      et->tree->SetCircular(analyze_request[index].rwnt_buffer_size);
#endif

      /* book run number/event number/time */
      et->branch = (TBranch **) malloc(sizeof(TBranch *));
      et->branch_name = (char *) malloc(NAME_LENGTH);
      et->branch_filled = (int *) malloc(sizeof(int));
      et->branch_len = (int *) malloc(sizeof(int));

      et->branch[et->n_branch] =
          et->tree->Branch("Number", &analyze_request[index].number,
                           "Run/I:Number/I:Time/i");
      strcpy(et->branch_name, "Number");
      et->n_branch++;

      bank_list = analyze_request[index].bank_list;
      if (bank_list == NULL) {
         /* book fixed event */
         event_def =
             db_get_event_definition((short int) analyze_request[index].ar_info.event_id);
         if (event_def == NULL) {
            cm_msg(MERROR, "book_ttree", "Cannot find definition of event %s in ODB",
                   analyze_request[index].event_name);
            return 0;
         }

         leaf_tags[0] = 0;
         for (i = 0;; i++) {
            status = db_enum_key(hDB, event_def->hDefKey, i, &hkey);
            if (status == DB_NO_MORE_SUBKEYS)
               break;

            db_get_key(hDB, hkey, &key);

            strcat(leaf_tags, key.name);

            if (key.num_values > 1)
               sprintf(leaf_tags + strlen(leaf_tags), "[%d]", key.num_values);

            strcat(leaf_tags, "/");

            if (ttree_types[key.type] != NULL)
               strcat(leaf_tags, ttree_types[key.type]);
            else {
               cm_msg(MERROR, "book_ttree",
                      "Key %s in event %s is of type %s with no TTREE correspondence",
                      key.name, analyze_request[index].event_name,
                      rpc_tid_name(key.type));
               return 0;
            }
            strcat(leaf_tags, ":");
         }

         leaf_tags[strlen(leaf_tags) - 1] = 0;  /* delete last ':' */

         et->branch =
             (TBranch **) realloc(et->branch, sizeof(TBranch *) * (et->n_branch + 1));
         et->branch_name =
             (char *) realloc(et->branch_name, NAME_LENGTH * (et->n_branch + 1));
         et->branch_filled =
             (int *) realloc(et->branch_filled, sizeof(int) * (et->n_branch + 1));
         et->branch_len =
             (int *) realloc(et->branch_len, sizeof(int) * (et->n_branch + 1));

         et->branch[et->n_branch] =
             et->tree->Branch((const char*)analyze_request[index].event_name, (const char*)NULL, leaf_tags);
         strcpy(&et->branch_name[et->n_branch * NAME_LENGTH],
                analyze_request[index].event_name);
         et->n_branch++;
      } else {
         /* go thorough all banks in bank_list */
         for (; bank_list->name[0]; bank_list++) {
            if (bank_list->output_flag == 0)
               continue;

            if (bank_list->type != TID_STRUCT) {
               sprintf(leaf_tags, "n%s/I:%s[n%s]/", bank_list->name, bank_list->name,
                       bank_list->name);

               /* define variable length array */
               if (ttree_types[bank_list->type] != NULL)
                  strcat(leaf_tags, ttree_types[bank_list->type]);
               else {
                  cm_msg(MERROR, "book_ttree",
                         "Bank %s is of type %s with no TTREE correspondence",
                         bank_list->name, rpc_tid_name(bank_list->type));
                  return 0;
               }

               if (rpc_tid_size(bank_list->type) == 0) {
                  cm_msg(MERROR, "book_ttree", "Bank %s is of type with unknown size",
                         bank_list->name);
                  return 0;
               }

               et->branch =
                   (TBranch **) realloc(et->branch,
                                        sizeof(TBranch *) * (et->n_branch + 1));
               et->branch_name =
                   (char *) realloc(et->branch_name, NAME_LENGTH * (et->n_branch + 1));
               et->branch_filled =
                   (int *) realloc(et->branch_filled, sizeof(int) * (et->n_branch + 1));
               et->branch_len =
                   (int *) realloc(et->branch_len, sizeof(int) * (et->n_branch + 1));

               et->branch[et->n_branch] =
                   et->tree->Branch((const char*)bank_list->name, (const char*)NULL, leaf_tags);
               strcpy(&et->branch_name[et->n_branch * NAME_LENGTH], bank_list->name);
               et->n_branch++;
            } else {
               /* define structured bank */
               leaf_tags[0] = 0;
               for (i = 0;; i++) {
                  status = db_enum_key(hDB, bank_list->def_key, i, &hkey);
                  if (status == DB_NO_MORE_SUBKEYS)
                     break;

                  db_get_key(hDB, hkey, &key);

                  strcat(leaf_tags, key.name);

                  if (key.num_values > 1)
                     sprintf(leaf_tags + strlen(leaf_tags), "[%d]", key.num_values);

                  strcat(leaf_tags, "/");

                  if (ttree_types[key.type] != NULL)
                     strcat(leaf_tags, ttree_types[key.type]);
                  else {
                     cm_msg(MERROR, "book_ttree",
                            "Key %s in bank %s is of type %s with no HBOOK correspondence",
                            key.name, bank_list->name, rpc_tid_name(key.type));
                     return 0;
                  }
                  strcat(leaf_tags, ":");
               }

               leaf_tags[strlen(leaf_tags) - 1] = 0;

               et->branch =
                   (TBranch **) realloc(et->branch,
                                        sizeof(TBranch *) * (et->n_branch + 1));
               et->branch_name =
                   (char *) realloc(et->branch_name, NAME_LENGTH * (et->n_branch + 1));
               et->branch_filled =
                   (int *) realloc(et->branch_filled, sizeof(int) * (et->n_branch + 1));
               et->branch_len =
                   (int *) realloc(et->branch_len, sizeof(int) * (et->n_branch + 1));

               et->branch[et->n_branch] =
                   et->tree->Branch((const char*)bank_list->name, (const char*)NULL, leaf_tags);
               strcpy(&et->branch_name[et->n_branch * NAME_LENGTH], bank_list->name);
               et->n_branch++;
            }
         }
      }
   }

   return SUCCESS;
}

/*-- root histogram routines ---------------------------------------*/

// Save all objects from given directory into given file
INT SaveRootHistograms(TFolder * folder, const char *filename)
{
   TDirectory *savedir = gDirectory;
   TFile *outf = new TFile(filename, "RECREATE", "Midas Analyzer Histograms");
   if (outf == 0) {
      cm_msg(MERROR, "SaveRootHistograms", "Cannot create output file %s", filename);
      return 0;
   }

   outf->cd();
   folder->Write();
   outf->Close();
   delete outf;
   // restore current directory
   savedir->cd();
   return SUCCESS;
}

/*------------------------------------------------------------------*/

// copy object from a last folder, to an online folder,
// and call itself to handle subfolders
void copy_from_last(TFolder * lastFolder, TFolder * onlineFolder)
{

   TIter next(lastFolder->GetListOfFolders());
   while (TObject * obj = next()) {
      const char *name = obj->GetName();

      if (obj->InheritsFrom("TFolder")) {

         TFolder *onlineSubfolder = (TFolder *) onlineFolder->FindObject(name);
         if (onlineSubfolder)
            copy_from_last((TFolder *) obj, onlineSubfolder);

      } else if (obj->InheritsFrom("TH1")) {

         // still don't know how to do TH1s

      } else if (obj->InheritsFrom("TCutG")) {

         TCutG *onlineObj = (TCutG *) onlineFolder->FindObject(name);
         if (onlineObj) {
            TCutG *lastObj = (TCutG *) obj;

            lastObj->TAttMarker::Copy(*onlineObj);
            lastObj->TAttFill::Copy(*onlineObj);
            lastObj->TAttLine::Copy(*onlineObj);
            lastObj->TNamed::Copy(*onlineObj);
            onlineObj->Set(lastObj->GetN());
            for (int i = 0; i < lastObj->GetN(); ++i) {
               onlineObj->SetPoint(i, lastObj->GetX()[i], lastObj->GetY()[i]);
            }
         }
      }
   }
   return;
}

/*------------------------------------------------------------------*/

// Load all objects from given file into given directory
INT LoadRootHistograms(TFolder * folder, const char *filename)
{
   TFile *inf = TFile::Open(filename, "READ");
   if (inf == NULL)
      printf("Error: File \"%s\" not found\n", filename);
   else {

      TFolder *lastHistos = (TFolder *) inf->Get("histos");
      if (lastHistos) {
         // copy histos to online folder
         copy_from_last(lastHistos, folder);
         inf->Close();
      }
   }
   return SUCCESS;
}

/*------------------------------------------------------------------*/

// Clear all TH1 objects in the given directory,
// and it's subdirectories
INT ClearRootHistograms(TFolder * folder)
{
   TIter next(folder->GetListOfFolders());
   while (TObject * obj = next())
      if (obj->InheritsFrom("TH1"))
         ((TH1 *) obj)->Reset();
      else if (obj->InheritsFrom("TFolder"))
         ClearRootHistograms((TFolder *) obj);
   return SUCCESS;
}

/*------------------------------------------------------------------*/

INT CloseRootOutputFile()
{
   int i;

   // ensure that we do have an open file
   assert(gManaOutputFile != NULL);

   // save the histograms
   gManaOutputFile->cd();
   gManaHistosFolder->Write();

   // close the output file
   gManaOutputFile->Write();
   gManaOutputFile->Close();
   delete gManaOutputFile;
   gManaOutputFile = NULL;

   // delete the output trees
   for (i = 0; i < tree_struct.n_tree; i++)
      if (tree_struct.event_tree[i].branch) {
         free(tree_struct.event_tree[i].branch);
         free(tree_struct.event_tree[i].branch_name);
         free(tree_struct.event_tree[i].branch_filled);
         free(tree_struct.event_tree[i].branch_len);
         tree_struct.event_tree[i].branch = NULL;
      }

   /* delete event tree */
   free(tree_struct.event_tree);
   tree_struct.event_tree = NULL;
   tree_struct.n_tree = 0;

   // go to ROOT root directory
   gROOT->cd();

   return SUCCESS;
}

#endif                          /* USE_ROOT */

/*-- analyzer init routine -----------------------------------------*/

INT mana_init()
{
   ANA_MODULE **module;
   INT i, j, status, size;
   HNDLE hkey;
   char str[256], block_name[32];
   BANK_LIST *bank_list;
   double dummy;

   sprintf(str, "/%s/Output", analyzer_name);
   db_find_key(hDB, 0, str, &hkey);

   if (clp.online) {
      status =
          db_open_record(hDB, hkey, &out_info, sizeof(out_info), MODE_READ, NULL, NULL);
      if (status != DB_SUCCESS) {
         cm_msg(MERROR, "bor", "Cannot read output info record");
         return 0;
      }
   }

   /* create ODB structures for banks */
   for (i = 0; analyze_request[i].event_name[0]; i++) {
      bank_list = analyze_request[i].bank_list;

      if (bank_list == NULL)
         continue;

      for (; bank_list->name[0]; bank_list++) {
         strncpy(block_name, bank_list->name, 4);
         block_name[4] = 0;

         if (bank_list->type == TID_STRUCT) {
            sprintf(str, "/Equipment/%s/Variables/%s", analyze_request[i].event_name,
                    block_name);
            db_check_record(hDB, 0, str, strcomb((const char **)bank_list->init_str), TRUE);
            db_find_key(hDB, 0, str, &hkey);
            bank_list->def_key = hkey;
         } else {
            sprintf(str, "/Equipment/%s/Variables/%s", analyze_request[i].event_name,
                    block_name);
            status = db_find_key(hDB, 0, str, &hkey);
            if (status != DB_SUCCESS) {
               dummy = 0;
               db_set_value(hDB, 0, str, &dummy, rpc_tid_size(bank_list->type), 1,
                            bank_list->type);
            }
            bank_list->def_key = hkey;
         }
      }
   }

   /* create ODB structures for fixed events */
   for (i = 0; analyze_request[i].event_name[0]; i++) {
      if (analyze_request[i].init_string) {
         sprintf(str, "/Equipment/%s/Variables", analyze_request[i].event_name);
         db_check_record(hDB, 0, str, strcomb((const char **)analyze_request[i].init_string), TRUE);
      }
   }

   /* delete tests in ODB */
   sprintf(str, "/%s/Tests", analyzer_name);
   db_find_key(hDB, 0, str, &hkey);
   if (hkey)
      db_delete_key(hDB, hkey, FALSE);

#ifdef HAVE_HBOOK
   /* create global memory */
   if (clp.online) {
      /* book online N-tuples only once when online */
      status = book_ntuples();
      if (status != SUCCESS)
         return status;
   } else {
      if (equal_ustring(clp.output_file_name, "OFLN")) {
         /* book online N-tuples only once when online */
         status = book_ntuples();
         if (status != SUCCESS)
            return status;
      }
   }
#endif                          /* HAVE_HBOOK */

#ifdef USE_ROOT
   if (clp.online) {
      /* book online N-tuples only once when online */
      status = book_ttree();
      if (status != SUCCESS)
         return status;
   }
#endif

   /* call main analyzer init routine */
   status = analyzer_init();
   if (status != SUCCESS)
      return status;

   /* initialize modules */
   for (i = 0; analyze_request[i].event_name[0]; i++) {
      module = analyze_request[i].ana_module;
      for (j = 0; module != NULL && module[j] != NULL; j++) {

         /* copy module enabled flag to ana_module */
         sprintf(str, "/%s/Module switches/%s", analyzer_name, module[j]->name);
         module[j]->enabled = TRUE;
         size = sizeof(BOOL);
         db_get_value(hDB, 0, str, &module[j]->enabled, &size, TID_BOOL, TRUE);

         if (module[j]->init != NULL && module[j]->enabled) {

#ifdef USE_ROOT
            /* create histo subfolder for module */
            sprintf(str, "Histos for module %s", module[j]->name);
            module[j]->histo_folder = (TFolder *) gROOT->FindObjectAny(module[j]->name);
            if (!module[j]->histo_folder)
               module[j]->histo_folder =
                   gManaHistosFolder->AddFolder(module[j]->name, str);
            else if (strcmp(((TObject *) module[j]->histo_folder)->ClassName(), "TFolder")
                     != 0) {
               cm_msg(MERROR, "mana_init",
                      "Fatal error: ROOT Object \"%s\" of class \"%s\" exists but it is not a TFolder, exiting!",
                      module[j]->name,
                      ((TObject *) module[j]->histo_folder)->ClassName());
               exit(1);
            }
            gHistoFolderStack->Clear();
            gHistoFolderStack->Add((TObject *) module[j]->histo_folder);
#endif

            module[j]->init();
         }
      }
   }

   return SUCCESS;
}

/*-- exit routine --------------------------------------------------*/

INT mana_exit()
{
   ANA_MODULE **module;
   INT i, j;

   /* call exit routines from modules */
   for (i = 0; analyze_request[i].event_name[0]; i++) {
      module = analyze_request[i].ana_module;
      for (j = 0; module != NULL && module[j] != NULL; j++)
         if (module[j]->exit != NULL && module[j]->enabled) {
            module[j]->exit();
         }
   }

   /* call main analyzer exit routine */
   return analyzer_exit();
}

/*-- BOR routine ---------------------------------------------------*/

INT bor(INT run_number, char *error)
{
   ANA_MODULE **module;
   INT i, j, size;
   char str[256], file_name[256], *ext_str;
   BANK_LIST *bank_list;

   /* load parameters */
   load_parameters(run_number);

   for (i = 0; analyze_request[i].event_name[0]; i++) {
      /* copy output flag from ODB to bank_list */
      bank_list = analyze_request[i].bank_list;

      if (bank_list != NULL)
         for (; bank_list->name[0]; bank_list++) {
            sprintf(str, "/%s/Bank switches/%s", analyzer_name, bank_list->name);
            bank_list->output_flag = FALSE;
            size = sizeof(DWORD);
            db_get_value(hDB, 0, str, &bank_list->output_flag, &size, TID_DWORD, TRUE);
         }

      /* copy module enabled flag to ana_module */
      module = analyze_request[i].ana_module;
      for (j = 0; module != NULL && module[j] != NULL; j++) {
         sprintf(str, "/%s/Module switches/%s", analyzer_name, module[j]->name);
         module[j]->enabled = TRUE;
         size = sizeof(BOOL);
         db_get_value(hDB, 0, str, &module[j]->enabled, &size, TID_BOOL, TRUE);
      }
   }

   /* clear histos, N-tuples and tests */
   if (clp.online && out_info.clear_histos) {
#ifdef HAVE_HBOOK
      int hid[10000];
      int n;

      for (i = 0; analyze_request[i].event_name[0]; i++)
         if (analyze_request[i].bank_list != NULL)
            if (HEXIST(analyze_request[i].ar_info.event_id))
               HRESET(analyze_request[i].ar_info.event_id, bstr);

      /* get list of all histos */
      HIDALL(hid, n);
      for (i = 0; i < n; i++) {
         for (j = 0; j < 10000; j++)
            if (lock_list[j] == 0 || lock_list[j] == hid[i])
               break;

         /* clear histo if not locked */
         if (lock_list[j] != hid[i])
            HRESET(hid[i], bstr);
      }
#endif                          /* HAVE_HBOOK */

#ifdef USE_ROOT
      /* clear histos */
      if (clp.online && out_info.clear_histos)
         ClearRootHistograms(gManaHistosFolder);
#endif                          /* USE_ROOT */

      /* clear tests */
      test_clear();
   }
#ifdef USE_ROOT
   if (clp.online) {
      /* clear all trees when online */
      for (i = 0; i < tree_struct.n_tree; i++)
         tree_struct.event_tree[i].tree->Reset();
   }
#endif

   /* open output file if not already open (append mode) and in offline mode */
   if (!clp.online && out_file == NULL && !pvm_master
       && !equal_ustring(clp.output_file_name, "OFLN")) {
      if (out_info.filename[0]) {
         strcpy(str, out_info.filename);
         if (strchr(str, '%') != NULL)
            sprintf(file_name, str, run_number);
         else
            strcpy(file_name, str);

         /* check output file extension */
#ifdef HAVE_ZLIB
         out_gzip = FALSE;
#endif
         if (strchr(file_name, '.')) {
            ext_str = file_name + strlen(file_name) - 1;
            while (*ext_str != '.')
               ext_str--;

            if (strncmp(ext_str, ".gz", 3) == 0) {
#ifdef HAVE_ZLIB
               out_gzip = TRUE;
               ext_str--;
               while (*ext_str != '.' && ext_str > file_name)
                  ext_str--;
#else
               strcpy(error,
                      ".gz extension not possible because zlib support is not compiled in.\n");
               cm_msg(MERROR, "bor", error);
               return 0;
#endif
            }

            if (strncmp(ext_str, ".asc", 4) == 0)
               out_format = FORMAT_ASCII;
            else if (strncmp(ext_str, ".mid", 4) == 0)
               out_format = FORMAT_MIDAS;
            else if (strncmp(ext_str, ".rz", 3) == 0)
               out_format = FORMAT_HBOOK;
            else if (strncmp(ext_str, ".root", 5) == 0)
               out_format = FORMAT_ROOT;
            else {
               strcpy(error,
                      "Unknown output data format. Please use file extension .asc, .mid, .rz or .root.\n");
               cm_msg(MERROR, "bor", error);
               return 0;
            }
         } else
            out_format = FORMAT_ASCII;

#ifdef HAVE_PVM
         /* use node name as filename if PVM slave */
         if (pvm_slave) {
            /* extract extension */
            if (strchr(file_name, '.')) {
               strcpy(str, strchr(file_name, '.') + 1);
               sprintf(file_name, "n%d", pvm_client_index);
               strcat(file_name, ".");
               strcat(file_name, str);
            } else {
               sprintf(file_name, "n%d", pvm_client_index);
            }

            PVM_DEBUG("BOR: file_name = %s", file_name);
         }
#endif

         /* open output file */
         if (out_format == FORMAT_HBOOK) {
#ifdef HAVE_HBOOK
            int status, lrec;
            char str2[80];

            lrec = clp.lrec;
#ifdef extname
            quest_[9] = 65000;
#else
            QUEST[9] = 65000;
#endif

            strcpy(str, "BSIZE");
            HBSET(str, HBOOK_LREC, status);
            strcpy(str, "OFFLINE");
            strcpy(str2, "NQP");
            HROPEN(1, str, file_name, str2, lrec, status);
            if (status != 0) {
               sprintf(error, "Cannot open output file %s", out_info.filename);
               cm_msg(MERROR, "bor", error);
               out_file = NULL;
               return 0;
            } else
               out_file = (FILE *) 1;
#else
            cm_msg(MERROR, "bor", "HBOOK support is not compiled in");
#endif                          /* HAVE_HBOOK */
         }

         else if (out_format == FORMAT_ROOT) {
#ifdef USE_ROOT
            // ensure the output file is closed
            assert(gManaOutputFile == NULL);

            gManaOutputFile =
                new TFile(file_name, "RECREATE", "Midas Analyzer output file");
            if (gManaOutputFile == NULL) {
               sprintf(error, "Cannot open output file %s", out_info.filename);
               cm_msg(MERROR, "bor", error);
               out_file = NULL;
               return 0;
            }
            // make all ROOT objects created by user module bor() functions
            // go into the output file
            gManaOutputFile->cd();

            out_file = (FILE *) 1;
#else
            cm_msg(MERROR, "bor", "ROOT support is not compiled in");
#endif                          /* USE_ROOT */
         }

         else {
#ifdef HAVE_ZLIB
            if (out_gzip)
               out_file = (FILE *) gzopen(file_name, "wb");
            else
#endif
            if (out_format == FORMAT_ASCII)
               out_file = fopen(file_name, "wt");
            else
               out_file = fopen(file_name, "wb");
            if (out_file == NULL) {
               sprintf(error, "Cannot open output file %s", file_name);
               cm_msg(MERROR, "bor", error);
               return 0;
            }
         }
      } else
         out_file = NULL;

#ifdef HAVE_HBOOK
      /* book N-tuples */
      if (out_format == FORMAT_HBOOK) {
         int status = book_ntuples();
         if (status != SUCCESS)
            return status;
      }
#endif                          /* HAVE_HBOOK */

#ifdef USE_ROOT
      /* book ROOT TTree */
      if (out_format == FORMAT_ROOT) {
         int status = book_ttree();
         if (status != SUCCESS)
            return status;
      }
#endif                          /* USE_ROOT */

   }

   /* if (out_file == NULL) */
   /* save run number */
   current_run_number = run_number;

   /* call bor for modules */
   for (i = 0; analyze_request[i].event_name[0]; i++) {
      module = analyze_request[i].ana_module;
      for (j = 0; module != NULL && module[j] != NULL; j++)
         if (module[j]->bor != NULL && module[j]->enabled) {
            module[j]->bor(run_number);
         }
   }

   /* call main analyzer BOR routine */
   return ana_begin_of_run(run_number, error);
}

/*-- EOR routine ---------------------------------------------------*/

INT eor(INT run_number, char *error)
{
   ANA_MODULE **module;
   BANK_LIST *bank_list;
   INT i, j, status;
   char str[256], file_name[256];

   /* call EOR routines modules */
   for (i = 0; analyze_request[i].event_name[0]; i++) {
      module = analyze_request[i].ana_module;
      for (j = 0; module != NULL && module[j] != NULL; j++)
         if (module[j]->eor != NULL && module[j]->enabled) {
            module[j]->eor(run_number);
         }
   }

   /* call main analyzer BOR routine */
   status = ana_end_of_run(run_number, error);

   /* save histos if requested */
   if (out_info.histo_dump && clp.online) {
      strcpy(str, out_info.histo_dump_filename);
      if (strchr(str, '%') != NULL)
         sprintf(file_name, str, run_number);
      else
         strcpy(file_name, str);

      add_data_dir(str, file_name);
#ifdef HAVE_HBOOK
      for (i = 0; i < (int) strlen(str); i++)
         if (isupper(str[i]))
            break;

      if (i < (int) strlen(str)) {
         printf
             ("Error: Due to a limitation in HBOOK, directoy names may not contain uppercase\n");
         printf("       characters. Histogram saving to %s will not work.\n", str);
      } else {
         char str2[256];
         strcpy(str2, "NT");
         HRPUT(0, str, str2);
      }
#endif                          /* HAVE_HBOOK */

#ifdef USE_ROOT
      SaveRootHistograms(gManaHistosFolder, str);
#endif                          /* USE_ROOT */
   }

   /* close output file */
   if (out_file && !out_append) {
      if (out_format == FORMAT_HBOOK) {
#ifdef HAVE_HBOOK
         HROUT(0, i, bstr);
         strcpy(str, "OFFLINE");
         HREND(str);
#else
         cm_msg(MERROR, "eor", "HBOOK support is not compiled in");
#endif                          /* HAVE_HBOOK */
      } else if (out_format == FORMAT_ROOT) {
#ifdef USE_ROOT
         CloseRootOutputFile();
#else
         cm_msg(MERROR, "eor", "ROOT support is not compiled in");
#endif                          /* USE_ROOT */
      } else {
#ifdef HAVE_ZLIB
         if (out_gzip)
            gzclose((gzFile)out_file);
         else
#endif
            fclose(out_file);
      }

      out_file = NULL;

      /* free CWNT buffer */
      for (i = 0; analyze_request[i].event_name[0]; i++) {
         bank_list = analyze_request[i].bank_list;

         if (bank_list == NULL) {
            if (analyze_request[i].addr) {
               free(analyze_request[i].addr);
               analyze_request[i].addr = NULL;
            }
         } else {
            for (; bank_list->name[0]; bank_list++)
               if (bank_list->addr) {
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

INT tr_start(INT rn, char *error)
{
   INT status, i;

   /* reset counters */
   for (i = 0; analyze_request[i].event_name[0]; i++) {
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
      for (i = 0; analyze_request[i].event_name[0]; i++) {
         do {
            bm_get_buffer_level(analyze_request[i].buffer_handle, &n_bytes);
            if (n_bytes > 0)
               cm_yield(100);
         } while (n_bytes > 0);
      }

   /* update statistics */
   update_stats();

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


INT write_event_ascii(FILE * file, EVENT_HEADER * pevent, ANALYZE_REQUEST * par)
{
   INT status, size, i, j, count;
   BOOL exclude;
   BANK_HEADER *pbh;
   BANK_LIST *pbl;
   EVENT_DEF *event_def;
   BANK *pbk;
   BANK32 *pbk32;
   void *pdata;
   char *pbuf, name[5], type_name[10];
   LRS1882_DATA *lrs1882;
   LRS1877_DATA *lrs1877;
   LRS1877_HEADER *lrs1877_header;
   HNDLE hKey;
   KEY key;
   char buffer[100000];
   DWORD bkname;
   WORD bktype;

   event_def = db_get_event_definition(pevent->event_id);
   if (event_def == NULL)
      return SS_SUCCESS;

   /* write event header */
   pbuf = buffer;
   name[4] = 0;

   if (pevent->event_id == EVENTID_BOR)
      sprintf(pbuf, "%%ID BOR NR %d\n", (int) pevent->serial_number);
   else if (pevent->event_id == EVENTID_EOR)
      sprintf(pbuf, "%%ID EOR NR %d\n", (int) pevent->serial_number);
   else
      sprintf(pbuf, "%%ID %d TR %d NR %d\n", pevent->event_id, pevent->trigger_mask,
              (int) pevent->serial_number);
   STR_INC(pbuf, buffer);

  /*---- MIDAS format ----------------------------------------------*/
   if (event_def->format == FORMAT_MIDAS) {
      pbh = (BANK_HEADER *) (pevent + 1);
      pbk = NULL;
      pbk32 = NULL;
      do {
         /* scan all banks */
         if (bk_is32(pbh)) {
            size = bk_iterate32(pbh, &pbk32, &pdata);
            if (pbk32 == NULL)
               break;
            bkname = *((DWORD *) pbk32->name);
            bktype = (WORD) pbk32->type;
         } else {
            size = bk_iterate(pbh, &pbk, &pdata);
            if (pbk == NULL)
               break;
            bkname = *((DWORD *) pbk->name);
            bktype = (WORD) pbk->type;
         }

         /* look if bank is in exclude list */
         exclude = FALSE;
         pbl = NULL;
         if (par->bank_list != NULL)
            for (i = 0; par->bank_list[i].name[0]; i++)
               if (*((DWORD *) par->bank_list[i].name) == bkname) {
                  pbl = &par->bank_list[i];
                  exclude = (pbl->output_flag == 0);
                  break;
               }

         if (!exclude) {
            if (rpc_tid_size(bktype & 0xFF))
               size /= rpc_tid_size(bktype & 0xFF);

            lrs1882 = (LRS1882_DATA *) pdata;
            lrs1877 = (LRS1877_DATA *) pdata;

            /* write bank header */
            *((DWORD *) name) = bkname;

            if ((bktype & 0xFF00) == 0)
               strcpy(type_name, rpc_tid_name(bktype & 0xFF));
            else if ((bktype & 0xFF00) == TID_LRS1882)
               strcpy(type_name, "LRS1882");
            else if ((bktype & 0xFF00) == TID_LRS1877)
               strcpy(type_name, "LRS1877");
            else if ((bktype & 0xFF00) == TID_PCOS3)
               strcpy(type_name, "PCOS3");
            else
               strcpy(type_name, "unknown");

            sprintf(pbuf, "BK %s TP %s SZ %d\n", name, type_name, size);
            STR_INC(pbuf, buffer);

            if (bktype == TID_STRUCT) {
               if (pbl == NULL)
                  cm_msg(MERROR, "write_event_ascii", "received unknown bank %s", name);
               else
                  /* write structured bank */
                  for (i = 0;; i++) {
                     status = db_enum_key(hDB, pbl->def_key, i, &hKey);
                     if (status == DB_NO_MORE_SUBKEYS)
                        break;

                     db_get_key(hDB, hKey, &key);
                     sprintf(pbuf, "%s:\n", key.name);
                     STR_INC(pbuf, buffer);

                     /* adjust for alignment */
                     pdata =
                         (void *) VALIGN(pdata,
                                         MIN(ss_get_struct_align(), key.item_size));

                     for (j = 0; j < key.num_values; j++) {
                        db_sprintf(pbuf, pdata, key.item_size, j, key.type);
                        strcat(pbuf, "\n");
                        STR_INC(pbuf, buffer);
                     }

                     /* shift data pointer to next item */
                     pdata = (char *) pdata + key.item_size * key.num_values;
                  }
            } else {
               /* write variable length bank  */
               if ((bktype & 0xFF00) == TID_LRS1877) {
                  for (i = 0; i < size;) {
                     lrs1877_header = (LRS1877_HEADER *) & lrs1877[i];

                     /* print header */
                     sprintf(pbuf, "GA %d BF %d CN %d",
                             lrs1877_header->geo_addr, lrs1877_header->buffer,
                             lrs1877_header->count);
                     strcat(pbuf, "\n");
                     STR_INC(pbuf, buffer);

                     count = lrs1877_header->count;
                     if (count == 0)
                        break;
                     for (j = 1; j < count; j++) {
                        /* print data */
                        sprintf(pbuf, "GA %d CH %02d ED %d DA %1.1lf",
                                lrs1877[i].geo_addr, lrs1877[i + j].channel,
                                lrs1877[i + j].edge, lrs1877[i + j].data * 0.5);
                        strcat(pbuf, "\n");
                        STR_INC(pbuf, buffer);
                     }

                     i += count;
                  }
               } else
                  for (i = 0; i < size; i++) {
                     if ((bktype & 0xFF00) == 0)
                        db_sprintf(pbuf, pdata, size, i, bktype & 0xFF);

                     else if ((bktype & 0xFF00) == TID_LRS1882)
                        sprintf(pbuf, "GA %d CH %02d DA %d",
                                lrs1882[i].geo_addr, lrs1882[i].channel, lrs1882[i].data);

                     else if ((bktype & 0xFF00) == TID_PCOS3)
                        sprintf(pbuf, "TBD");
                     else
                        db_sprintf(pbuf, pdata, size, i, bktype & 0xFF);

                     strcat(pbuf, "\n");
                     STR_INC(pbuf, buffer);
                  }
            }
         }

      } while (1);
   }

  /*---- FIXED format ----------------------------------------------*/
   if (event_def->format == FORMAT_FIXED) {
      if (event_def->hDefKey == 0)
         cm_msg(MERROR, "write_event_ascii", "cannot find event definition");
      else {
         pdata = (char *) (pevent + 1);
         for (i = 0;; i++) {
            status = db_enum_key(hDB, event_def->hDefKey, i, &hKey);
            if (status == DB_NO_MORE_SUBKEYS)
               break;

            db_get_key(hDB, hKey, &key);
            sprintf(pbuf, "%s\n", key.name);
            STR_INC(pbuf, buffer);

            /* adjust for alignment */
            pdata = (void *) VALIGN(pdata, MIN(ss_get_struct_align(), key.item_size));

            for (j = 0; j < key.num_values; j++) {
               db_sprintf(pbuf, pdata, key.item_size, j, key.type);
               strcat(pbuf, "\n");
               STR_INC(pbuf, buffer);
            }

            /* shift data pointer to next item */
            pdata = (char *) pdata + key.item_size * key.num_values;
         }
      }
   }

   /* insert empty line after each event */
   strcat(pbuf, "\n");
   size = strlen(buffer);

   /* write record to device */
#ifdef HAVE_ZLIB
   if (out_gzip)
      status = gzwrite((gzFile)file, buffer, size) == size ? SS_SUCCESS : SS_FILE_ERROR;
   else
#endif
      status =
          fwrite(buffer, 1, size, file) == (size_t) size ? SS_SUCCESS : SS_FILE_ERROR;

   return status;
}

/*---- MIDAS output ------------------------------------------------*/

INT write_event_midas(FILE * file, EVENT_HEADER * pevent, ANALYZE_REQUEST * par)
{
   INT status, size = 0, i;
   BOOL exclude;
   BANK_HEADER *pbh;
   BANK_LIST *pbl;
   EVENT_DEF *event_def;
   BANK *pbk;
   BANK32 *pbk32;
   char *pdata, *pdata_copy;
   char *pbuf;
   EVENT_HEADER *pevent_copy;
   DWORD bkname, bksize;
   WORD bktype;
   static char *buffer = NULL;

   if (buffer == NULL)
      buffer = (char *) malloc(MAX_EVENT_SIZE);

   pevent_copy = (EVENT_HEADER *) ALIGN8((POINTER_T) buffer);

   if (clp.filter) {
      /* use original event */
      size = pevent->data_size + sizeof(EVENT_HEADER);
      memcpy(pevent_copy, pevent, size);
   } else {
      /* copy only banks which are turned on via /analyzer/bank switches */

    /*---- MIDAS format ----------------------------------------------*/

      event_def = db_get_event_definition(pevent->event_id);
      if (event_def == NULL)
         return SUCCESS;

      if (event_def->format == FORMAT_MIDAS) {
         /* copy event header */
         pbuf = (char *) pevent_copy;
         memcpy(pbuf, pevent, sizeof(EVENT_HEADER));
         pbuf += sizeof(EVENT_HEADER);

         pbh = (BANK_HEADER *) (pevent + 1);

         if (bk_is32(pbh))
            bk_init32(pbuf);
         else
            bk_init(pbuf);

         pbk = NULL;
         pbk32 = NULL;
         pdata_copy = pbuf;
         do {
            /* scan all banks */
            if (bk_is32(pbh)) {
               size = bk_iterate32(pbh, &pbk32, &pdata);
               if (pbk32 == NULL)
                  break;
               bkname = *((DWORD *) pbk32->name);
               bktype = (WORD) pbk32->type;
               bksize = pbk32->data_size;
            } else {
               size = bk_iterate(pbh, &pbk, &pdata);
               if (pbk == NULL)
                  break;
               bkname = *((DWORD *) pbk->name);
               bktype = (WORD) pbk->type;
               bksize = pbk->data_size;
            }

            /* look if bank is in exclude list */
            exclude = FALSE;
            pbl = NULL;
            if (par->bank_list != NULL)
               for (i = 0; par->bank_list[i].name[0]; i++)
                  if (*((DWORD *) par->bank_list[i].name) == bkname) {
                     pbl = &par->bank_list[i];
                     exclude = (pbl->output_flag == 0);
                     break;
                  }

            if (!exclude) {
               /* copy bank */
               bk_create(pbuf, (char *) (&bkname), bktype, &pdata_copy);
               memcpy(pdata_copy, pdata, bksize);
               pdata_copy += bksize;
               bk_close(pbuf, pdata_copy);
            }

         } while (1);

         /* set event size in header */
         size = ALIGN8((POINTER_T) pdata_copy - (POINTER_T) pbuf);
         pevent_copy->data_size = size;
         size += sizeof(EVENT_HEADER);
      }

    /*---- FIXED format ----------------------------------------------*/
      if (event_def->format == FORMAT_FIXED) {
         size = pevent->data_size + sizeof(EVENT_HEADER);
         memcpy(pevent_copy, pevent, size);
      }

      if (pevent_copy->data_size == 0)
         return SUCCESS;
   }

   /* write record to device */
#ifdef HAVE_ZLIB
   if (out_gzip)
      status = gzwrite((gzFile)file, pevent_copy, size) == size ? SUCCESS : SS_FILE_ERROR;
   else
#endif
      status =
          fwrite(pevent_copy, 1, size, file) == (size_t) size ? SUCCESS : SS_FILE_ERROR;

   return status;
}

/*---- HBOOK output ------------------------------------------------*/

#ifdef HAVE_HBOOK

INT write_event_hbook(FILE * file, EVENT_HEADER * pevent, ANALYZE_REQUEST * par)
{
   INT i, j, k, n, size, item_size, status;
   BANK *pbk;
   BANK32 *pbk32;
   BANK_LIST *pbl;
   BANK_HEADER *pbh;
   char *pdata;
   BOOL exclude, exclude_all;
   char block_name[5], str[80];
   float rwnt[512];
   EVENT_DEF *event_def;
   HNDLE hkey;
   KEY key;
   DWORD bkname;
   WORD bktype;

   /* return if N-tuples are disabled */
   if (!ntuple_flag)
      return SS_SUCCESS;

   event_def = db_get_event_definition(pevent->event_id);
   if (event_def == NULL)
      return SS_SUCCESS;

   if (event_def->disabled)
      return SS_SUCCESS;

   /* fill number info */
   if (clp.rwnt) {
      memset(rwnt, 0, sizeof(rwnt));
      rwnt[0] = (float) current_run_number;
      rwnt[1] = (float) pevent->serial_number;
      rwnt[2] = (float) pevent->time_stamp;
   } else {
      par->number.run = current_run_number;
      par->number.serial = pevent->serial_number;
      par->number.time = pevent->time_stamp;
   }

  /*---- MIDAS format ----------------------------------------------*/

   if (event_def->format == FORMAT_MIDAS) {
      /* first fill number block */
      strcpy(str, "Number");
      if (!clp.rwnt)
         HFNTB(pevent->event_id, str);

      pbk = NULL;
      pbk32 = NULL;
      exclude_all = TRUE;
      do {
         pbh = (BANK_HEADER *) (pevent + 1);
         /* scan all banks */
         if (bk_is32(pbh)) {
            size = bk_iterate32(pbh, &pbk32, &pdata);
            if (pbk32 == NULL)
               break;
            bkname = *((DWORD *) pbk32->name);
            bktype = (WORD) pbk32->type;
         } else {
            size = bk_iterate(pbh, &pbk, &pdata);
            if (pbk == NULL)
               break;
            bkname = *((DWORD *) pbk->name);
            bktype = (WORD) pbk->type;
         }

         /* look if bank is in exclude list */
         *((DWORD *) block_name) = bkname;
         block_name[4] = 0;

         exclude = FALSE;
         pbl = NULL;
         if (par->bank_list != NULL) {
            for (i = 0; par->bank_list[i].name[0]; i++)
               if (*((DWORD *) par->bank_list[i].name) == bkname) {
                  pbl = &par->bank_list[i];
                  exclude = (pbl->output_flag == 0);
                  break;
               }
            if (par->bank_list[i].name[0] == 0)
               cm_msg(MERROR, "write_event_hbook", "Received unknown bank %s",
                      block_name);
         }

         /* fill CW N-tuple */
         if (!exclude && pbl != NULL && !clp.rwnt) {
            /* set array size in bank list */
            if ((bktype & 0xFF) != TID_STRUCT) {
               item_size = rpc_tid_size(bktype & 0xFF);
               if (item_size == 0) {
                  cm_msg(MERROR, "write_event_hbook",
                         "Received bank %s with unknown item size", block_name);
                  continue;
               }

               pbl->n_data = size / item_size;

               /* check bank size */
               if (pbl->n_data > pbl->size) {
                  cm_msg(MERROR, "write_event_hbook",
                         "Bank %s has more (%d) entries than maximum value (%d)",
                         block_name, pbl->n_data, pbl->size);
                  continue;
               }

               /* copy bank to buffer in bank list, DWORD aligned */
               if (item_size >= 4) {
                  size = MIN((INT) pbl->size * item_size, size);
                  memcpy(pbl->addr, pdata, size);
               } else if (item_size == 2)
                  for (i = 0; i < (INT) pbl->n_data; i++)
                     *((DWORD *) pbl->addr + i) = (DWORD) * ((WORD *) pdata + i);
               else if (item_size == 1)
                  for (i = 0; i < (INT) pbl->n_data; i++)
                     *((DWORD *) pbl->addr + i) = (DWORD) * ((BYTE *) pdata + i);
            } else {
               /* hope that structure members are aligned as HBOOK thinks ... */
               memcpy(pbl->addr, pdata, size);
            }

            /* fill N-tuple */
            HFNTB(pevent->event_id, block_name);
         }

         /* fill RW N-tuple */
         if (!exclude && pbl != NULL && clp.rwnt) {
            exclude_all = FALSE;

            item_size = rpc_tid_size(bktype & 0xFF);
            /* set array size in bank list */
            if ((bktype & 0xFF) != TID_STRUCT) {
               n = size / item_size;

               /* check bank size */
               if (n > (INT) pbl->size) {
                  cm_msg(MERROR, "write_event_hbook",
                         "Bank %s has more (%d) entries than maximum value (%d)",
                         block_name, n, pbl->size);
                  continue;
               }

               /* convert bank to float values */
               for (i = 0; i < n; i++) {
                  switch (bktype & 0xFF) {
                  case TID_BYTE:
                     rwnt[pbl->n_data + i] = (float) (*((BYTE *) pdata + i));
                     break;
                  case TID_WORD:
                     rwnt[pbl->n_data + i] = (float) (*((WORD *) pdata + i));
                     break;
                  case TID_DWORD:
                     rwnt[pbl->n_data + i] = (float) (*((DWORD *) pdata + i));
                     break;
                  case TID_FLOAT:
                     rwnt[pbl->n_data + i] = (float) (*((float *) pdata + i));
                     break;
                  case TID_DOUBLE:
                     rwnt[pbl->n_data + i] = (float) (*((double *) pdata + i));
                     break;
                  }
               }

               /* zero padding */
               for (; i < (INT) pbl->size; i++)
                  rwnt[pbl->n_data + i] = 0.f;
            } else {
               /* fill N-tuple from structured bank */
               k = pbl->n_data;

               for (i = 0;; i++) {
                  status = db_enum_key(hDB, pbl->def_key, i, &hkey);
                  if (status == DB_NO_MORE_SUBKEYS)
                     break;

                  db_get_key(hDB, hkey, &key);

                  /* align data pointer */
                  pdata =
                      (void *) VALIGN(pdata, MIN(ss_get_struct_align(), key.item_size));

                  for (j = 0; j < key.num_values; j++) {
                     switch (key.type & 0xFF) {
                     case TID_BYTE:
                        rwnt[k++] = (float) (*((BYTE *) pdata + j));
                        break;
                     case TID_WORD:
                        rwnt[k++] = (float) (*((WORD *) pdata + j));
                        break;
                     case TID_SHORT:
                        rwnt[k++] = (float) (*((short int *) pdata + j));
                        break;
                     case TID_INT:
                        rwnt[k++] = (float) (*((INT *) pdata + j));
                        break;
                     case TID_DWORD:
                        rwnt[k++] = (float) (*((DWORD *) pdata + j));
                        break;
                     case TID_BOOL:
                        rwnt[k++] = (float) (*((BOOL *) pdata + j));
                        break;
                     case TID_FLOAT:
                        rwnt[k++] = (float) (*((float *) pdata + j));
                        break;
                     case TID_DOUBLE:
                        rwnt[k++] = (float) (*((double *) pdata + j));
                        break;
                     }
                  }

                  /* shift data pointer to next item */
                  pdata += key.item_size * key.num_values * sizeof(char);
               }
            }
         }

      } while (TRUE);

      /* fill RW N-tuple */
      if (clp.rwnt && file != NULL && !exclude_all)
         HFN(pevent->event_id, rwnt);

      /* fill shared memory */
      if (file == NULL)
         HFNOV(pevent->event_id, rwnt);

   }


   /* if (event_def->format == FORMAT_MIDAS) */
 /*---- YBOS format ----------------------------------------------*/
   else if (event_def->format == FORMAT_YBOS) {
     assert(!"YBOS not supported anymore");
   }


 /*---- FIXED format ----------------------------------------------*/
   if (event_def->format == FORMAT_FIXED) {
      if (clp.rwnt) {
         /* fill N-tuple from structured bank */
         pdata = (char *) (pevent + 1);
         k = 3;                 /* index 0..2 for run/serial/time */

         for (i = 0;; i++) {
            status = db_enum_key(hDB, event_def->hDefKey, i, &hkey);
            if (status == DB_NO_MORE_SUBKEYS)
               break;

            db_get_key(hDB, hkey, &key);

            /* align data pointer */
            pdata = (void *) VALIGN(pdata, MIN(ss_get_struct_align(), key.item_size));

            for (j = 0; j < key.num_values; j++) {
               switch (key.type & 0xFF) {
               case TID_BYTE:
                  rwnt[k++] = (float) (*((BYTE *) pdata + j));
                  break;
               case TID_WORD:
                  rwnt[k++] = (float) (*((WORD *) pdata + j));
                  break;
               case TID_SHORT:
                  rwnt[k++] = (float) (*((short int *) pdata + j));
                  break;
               case TID_INT:
                  rwnt[k++] = (float) (*((INT *) pdata + j));
                  break;
               case TID_DWORD:
                  rwnt[k++] = (float) (*((DWORD *) pdata + j));
                  break;
               case TID_BOOL:
                  rwnt[k++] = (float) (*((BOOL *) pdata + j));
                  break;
               case TID_FLOAT:
                  rwnt[k++] = (float) (*((float *) pdata + j));
                  break;
               case TID_DOUBLE:
                  rwnt[k++] = (float) (*((double *) pdata + j));
                  break;
               }
            }

            /* shift data pointer to next item */
            pdata += key.item_size * key.num_values * sizeof(char);
         }

         /* fill RW N-tuple */
         if (file != NULL)
            HFN(pevent->event_id, rwnt);

         /* fill shared memory */
         if (file == NULL)
            HFNOV(pevent->event_id, rwnt);
      } else {
         memcpy(par->addr, pevent + 1, pevent->data_size);

         /* fill N-tuple */
         HFNT(pevent->event_id);
      }

   }

   return SUCCESS;
}
#endif                          /* HAVE_HBOOK */

/*---- ROOT output -------------------------------------------------*/

#ifdef USE_ROOT

INT write_event_ttree(FILE * file, EVENT_HEADER * pevent, ANALYZE_REQUEST * par)
{
   INT i, bklen;
   BANK *pbk;
   BANK32 *pbk32;
   BANK_LIST *pbl;
   BANK_HEADER *pbh;
   void *pdata;
   BOOL exclude, exclude_all;
   char bank_name[5];
   EVENT_DEF *event_def;
   DWORD bkname;
   WORD bktype;
   EVENT_TREE *et;
   TBranch *branch;

   /* return if N-tuples are disabled */
   if (!ntuple_flag)
      return SS_SUCCESS;

   event_def = db_get_event_definition(pevent->event_id);
   if (event_def == NULL)
      return SS_SUCCESS;

   if (event_def->disabled)
      return SS_SUCCESS;

   /* fill number info */
   par->number.run = current_run_number;
   par->number.serial = pevent->serial_number;
   par->number.time = pevent->time_stamp;

  /*---- MIDAS format ----------------------------------------------*/

   if (event_def->format == FORMAT_MIDAS) {
      /* find event in tree structure */
      for (i = 0; i < tree_struct.n_tree; i++)
         if (tree_struct.event_tree[i].event_id == pevent->event_id)
            break;

      if (i == tree_struct.n_tree) {
         cm_msg(MERROR, "write_event_ttree", "Event #%d not booked by book_ttree()",
                pevent->event_id);
         return SS_INVALID_FORMAT;
      }

      et = tree_struct.event_tree + i;

      /* first mark all banks non-filled */
      for (i = 0; i < et->n_branch; i++)
         et->branch_filled[i] = FALSE;

      /* first fill number block */
      et->branch_filled[0] = TRUE;

      pbk = NULL;
      pbk32 = NULL;
      exclude_all = TRUE;
      do {
         pbh = (BANK_HEADER *) (pevent + 1);
         /* scan all banks */
         if (bk_is32(pbh)) {
            bklen = bk_iterate32(pbh, &pbk32, &pdata);
            if (pbk32 == NULL)
               break;
            bkname = *((DWORD *) pbk32->name);
            bktype = (WORD) pbk32->type;
         } else {
            bklen = bk_iterate(pbh, &pbk, &pdata);
            if (pbk == NULL)
               break;
            bkname = *((DWORD *) pbk->name);
            bktype = (WORD) pbk->type;
         }

         if (rpc_tid_size(bktype & 0xFF))
            bklen /= rpc_tid_size(bktype & 0xFF);

         /* look if bank is in exclude list */
         *((DWORD *) bank_name) = bkname;
         bank_name[4] = 0;

         exclude = FALSE;
         pbl = NULL;
         if (par->bank_list != NULL) {
            for (i = 0; par->bank_list[i].name[0]; i++)
               if (*((DWORD *) par->bank_list[i].name) == bkname) {
                  pbl = &par->bank_list[i];
                  exclude = (pbl->output_flag == 0);
                  break;
               }
            if (par->bank_list[i].name[0] == 0) {
               cm_msg(MERROR, "write_event_ttree", "Received unknown bank %s", bank_name);
               continue;
            }
         }

         /* fill leaf */
         if (!exclude && pbl != NULL) {
            for (i = 0; i < et->n_branch; i++)
               if (*((DWORD *) (&et->branch_name[i * NAME_LENGTH])) == bkname)
                  break;

            if (i == et->n_branch) {
               cm_msg(MERROR, "write_event_ttree", "Received unknown bank %s", bank_name);
               continue;
            }

            exclude_all = FALSE;
            branch = et->branch[i];
            et->branch_filled[i] = TRUE;
            et->branch_len[i] = bklen;

            /* structured bank */
            if ((bktype & 0xFF) != TID_STRUCT) {
               TIter next(branch->GetListOfLeaves());
               TLeaf *leaf = (TLeaf *) next();

               /* varibale length array */
               leaf->SetAddress(&et->branch_len[i]);

               leaf = (TLeaf *) next();
               leaf->SetAddress(pdata);
            } else {
               /* hope that structure members are aligned as TTREE thinks ... */
               branch->SetAddress(pdata);
            }
         }

      } while (TRUE);

      /* check if all branches have been filled */
      for (i = 0; i < et->n_branch; i++)
         if (!et->branch_filled[i])
            cm_msg(MERROR, "root_write",
                   "Bank %s booked but not received, tree cannot be filled",
                   et->branch_name + (i * NAME_LENGTH));

      /* delete tree if too many entries, will be obsolete with cyglic trees later */
#if (ROOT_VERSION_CODE < 262401)
      if (clp.online && et->tree->GetEntries() > par->rwnt_buffer_size)
         et->tree->Reset();
#endif

      /* fill tree both online and offline */
      if (!exclude_all)
         et->tree->Fill();

   }                            // if (event_def->format == FORMAT_MIDAS)

   /*---- FIXED format ----------------------------------------------*/

   if (event_def->format == FORMAT_FIXED) {
      /* find event in tree structure */
      for (i = 0; i < tree_struct.n_tree; i++)
         if (tree_struct.event_tree[i].event_id == pevent->event_id)
            break;

      if (i == tree_struct.n_tree) {
         cm_msg(MERROR, "write_event_ttree", "Event #%d not booked by book_ttree()",
                pevent->event_id);
         return SS_INVALID_FORMAT;
      }

      et = tree_struct.event_tree + i;

      et->tree->GetBranch(et->branch_name)->SetAddress(pevent + 1);
      et->tree->Fill();
   }

   return SUCCESS;
}

#endif                          /* USE_ROOT */

/*---- ODB output --------------------------------------------------*/

INT write_event_odb(EVENT_HEADER * pevent)
{
   INT status, size, n_data, i;
   BANK_HEADER *pbh;
   EVENT_DEF *event_def;
   BANK *pbk;
   BANK32 *pbk32;
   void *pdata;
   char name[5];
   HNDLE hKeyRoot, hKey;
   KEY key;
   DWORD bkname;
   WORD bktype;

   event_def = db_get_event_definition(pevent->event_id);
   if (event_def == NULL)
      return SS_SUCCESS;

   /*---- MIDAS format ----------------------------------------------*/

   if (event_def->format == FORMAT_MIDAS) {
      pbh = (BANK_HEADER *) (pevent + 1);
      pbk = NULL;
      pbk32 = NULL;
      do {
         /* scan all banks */
         if (bk_is32(pbh)) {
            size = bk_iterate32(pbh, &pbk32, &pdata);
            if (pbk32 == NULL)
               break;
            bkname = *((DWORD *) pbk32->name);
            bktype = (WORD) pbk32->type;
         } else {
            size = bk_iterate(pbh, &pbk, &pdata);
            if (pbk == NULL)
               break;
            bkname = *((DWORD *) pbk->name);
            bktype = (WORD) pbk->type;
         }

         n_data = size;
         if (rpc_tid_size(bktype & 0xFF))
            n_data /= rpc_tid_size(bktype & 0xFF);

         /* get bank key */
         *((DWORD *) name) = bkname;
         name[4] = 0;

         status = db_find_key(hDB, event_def->hDefKey, name, &hKeyRoot);
         if (status != DB_SUCCESS) {
            cm_msg(MERROR, "write_event_odb", "received unknown bank %s", name);
            continue;
         }

         if (bktype == TID_STRUCT) {
            /* write structured bank */
            for (i = 0;; i++) {
               status = db_enum_key(hDB, hKeyRoot, i, &hKey);
               if (status == DB_NO_MORE_SUBKEYS)
                  break;

               db_get_key(hDB, hKey, &key);

               /* adjust for alignment */
               if (key.type != TID_STRING && key.type != TID_LINK)
                  pdata =
                      (void *) VALIGN(pdata, MIN(ss_get_struct_align(), key.item_size));

               status = db_set_data(hDB, hKey, pdata, key.item_size * key.num_values,
                                    key.num_values, key.type);
               if (status != DB_SUCCESS) {
                  cm_msg(MERROR, "write_event_odb", "cannot write %s to ODB", name);
                  continue;
               }

               /* shift data pointer to next item */
               pdata = (char *) pdata + key.item_size * key.num_values;
            }
         } else {
            db_get_key(hDB, hKeyRoot, &key);

            /* write variable length bank  */
            if (n_data > 0) {
               status = db_set_data(hDB, hKeyRoot, pdata, size, n_data, key.type);
               if (status != DB_SUCCESS) {
                  cm_msg(MERROR, "write_event_odb", "cannot write %s to ODB", name);
                  continue;
               }
            }
         }
      } while (1);
   }

   /*---- FIXED format ----------------------------------------------*/

   if (event_def->format == FORMAT_FIXED && !clp.online) {
      if (db_set_record
          (hDB, event_def->hDefKey, (char *) (pevent + 1), pevent->data_size,
           0) != DB_SUCCESS)
         cm_msg(MERROR, "write_event_odb", "event #%d size mismatch", pevent->event_id);
   }

   return SUCCESS;
}

/*------------------------------------------------------------------*/

static struct {
   short int event_id;
   DWORD last_time;
} last_time_event[50];

ANALYZE_REQUEST *_current_par;

void correct_num_events(INT i)
{
   if (_current_par)
      _current_par->events_received += i - 1;
}

INT process_event(ANALYZE_REQUEST * par, EVENT_HEADER * pevent)
{
   INT i, status = SUCCESS, ch;
   ANA_MODULE **module;
   DWORD actual_time;
   EVENT_DEF *event_def;
   static DWORD last_time_kb = 0;
   static char *orig_event = NULL;

   /* verbose output */
   if (clp.verbose)
      printf("event %d, number %d, total size %d\n",
             (int) pevent->event_id,
             (int) pevent->serial_number,
             (int) (pevent->data_size + sizeof(EVENT_HEADER)));

   /* save analyze_request for event number correction */
   _current_par = par;

   /* check keyboard once every second */
   actual_time = ss_millitime();
   if (!clp.online && actual_time - last_time_kb > 1000 && !clp.quiet && !pvm_slave) {
      last_time_kb = actual_time;

      while (ss_kbhit()) {
         ch = ss_getchar(0);
         if (ch == -1)
            ch = getchar();

         if ((char) ch == '!')
            return RPC_SHUTDOWN;
      }
   }

   if (par == NULL) {
      /* load ODB with BOR event */
      if (pevent->event_id == EVENTID_BOR) {
         /* get run number from BOR event */
         current_run_number = pevent->serial_number;

         cm_msg(MINFO, "process_event", "Set run number %d in ODB", current_run_number);
         assert(current_run_number > 0);

         /* set run number in ODB */
         status = db_set_value(hDB, 0, "/Runinfo/Run number", &current_run_number,
                               sizeof(current_run_number), 1, TID_INT);
         assert(status == SUCCESS);

         /* load ODB from event */
         odb_load(pevent);

#ifdef HAVE_PVM
         PVM_DEBUG("process_event: ODB load");
#endif
      }
   } else
      /* increment event counter */
      par->events_received++;

#ifdef HAVE_PVM

   /* if master, distribute events to clients */
   if (pvm_master) {
      status = pvm_distribute(par, pevent);
      return status;
   }
#endif

   /* don't analyze special (BOR,MESSAGE,...) events */
   if (par == NULL)
      return SUCCESS;

   /* swap event if necessary */
   event_def = db_get_event_definition(pevent->event_id);
   if (event_def == NULL)
      return 0;

   if (event_def->format == FORMAT_MIDAS)
      bk_swap((BANK_HEADER *) (pevent + 1), FALSE);

   /* keep copy of original event */
   if (clp.filter) {
      if (orig_event == NULL)
         orig_event = (char *) malloc(MAX_EVENT_SIZE + sizeof(EVENT_HEADER));
      memcpy(orig_event, pevent, pevent->data_size + sizeof(EVENT_HEADER));
   }

  /*---- analyze event ----*/

   /* call non-modular analyzer if defined */
   if (par->analyzer) {
      status = par->analyzer(pevent, (void *) (pevent + 1));

      /* don't continue if event was rejected */
      if (status == ANA_SKIP)
         return 0;
   }

   /* loop over analyzer modules */
   module = par->ana_module;
   for (i = 0; module != NULL && module[i] != NULL; i++) {
      if (module[i]->enabled) {

         status = module[i]->analyzer(pevent, (void *) (pevent + 1));

         /* don't continue if event was rejected */
         if (status == ANA_SKIP)
            return 0;
      }
   }

   if (event_def->format == FORMAT_MIDAS) {
      /* check if event got too large */
      i = bk_size(pevent + 1);
      if (i > MAX_EVENT_SIZE)
         cm_msg(MERROR, "process_event", "Event got too large (%d Bytes) in analyzer", i);

      /* correct for increased event size */
      pevent->data_size = i;
   }

   if (event_def->format == FORMAT_YBOS) {
     assert(!"YBOS not supported anymore");
   }

   /* increment tests */
   if (par->use_tests)
      test_increment();

   /* in filter mode, use original event */
   if (clp.filter)
      pevent = (EVENT_HEADER *) orig_event;

   /* write resulting event */
   if (out_file) {
#ifdef HAVE_HBOOK
      if (out_format == FORMAT_HBOOK)
         status = write_event_hbook(out_file, pevent, par);
#endif                          /* HAVE_HBOOK */
#ifdef USE_ROOT
      if (out_format == FORMAT_ROOT)
         status = write_event_ttree(out_file, pevent, par);
#endif                          /* USE_ROOT */
      if (out_format == FORMAT_ASCII)
         status = write_event_ascii(out_file, pevent, par);
      if (out_format == FORMAT_MIDAS)
         status = write_event_midas(out_file, pevent, par);

      if (status != SUCCESS) {
         cm_msg(MERROR, "process_event", "Error writing to file (Disk full?)");
         return -1;
      }

      par->events_written++;
   }
#ifdef HAVE_HBOOK
   /* fill shared memory */
   if ((clp.online || equal_ustring(clp.output_file_name, "OFLN"))
       && par->rwnt_buffer_size > 0)
      write_event_hbook(NULL, pevent, par);
#endif                          /* HAVE_HBOOK */
#ifdef USE_ROOT
   /* fill tree, should later be replaced by cyclic filling once it's implemented in ROOT */
   if (clp.online && par->rwnt_buffer_size > 0)
      write_event_ttree(NULL, pevent, par);
#endif


   /* put event in ODB once every second */
   if (out_info.events_to_odb)
      for (i = 0; i < 50; i++) {
         if (last_time_event[i].event_id == pevent->event_id) {
            if (event_def->type == EQ_PERIODIC ||
                event_def->type == EQ_SLOW
                || actual_time - last_time_event[i].last_time > 1000) {
               last_time_event[i].last_time = actual_time;
               write_event_odb(pevent);
            }
            break;
         }
         if (last_time_event[i].event_id == 0) {
            last_time_event[i].event_id = pevent->event_id;
            last_time_event[i].last_time = actual_time;
            write_event_odb(pevent);
            break;
         }
      }

   return SUCCESS;
}

/*------------------------------------------------------------------*/

void receive_event(HNDLE buffer_handle, HNDLE request_id, EVENT_HEADER * pheader,
                   void *pevent)
/* receive online event */
{
   INT i;
   ANALYZE_REQUEST *par;
   static DWORD buffer_size = 0;
   static char *buffer = NULL;
   char *pb;

   if (buffer == NULL) {
      buffer = (char *) malloc(MAX_EVENT_SIZE + sizeof(EVENT_HEADER));

      if (buffer == NULL) {
         cm_msg(MERROR, "receive_event", "Not enough memory to buffer event of size %d",
                buffer_size);
         return;
      }
   }

   /* align buffer */
   pb = (char *) ALIGN8((POINTER_T) buffer);

   /* copy event to local buffer */
   memcpy(pb, pheader, pheader->data_size + sizeof(EVENT_HEADER));

   par = analyze_request;

   for (i = 0; par->event_name[0]; par++)
      if (par->buffer_handle == buffer_handle && par->request_id == request_id) {
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
   for (i = 0; analyze_request[i].event_name[0]; i++)
      if (analyze_request[i].hkey_common == hKey) {
         ar_info = &analyze_request[i].ar_info;

         /* remove previous request */
         if (analyze_request[i].request_id != -1)
            bm_delete_request(analyze_request[i].request_id);

         /* if enabled, add new request */
         if (ar_info->enabled)
            bm_request_event(analyze_request[i].buffer_handle, (short) ar_info->event_id,
                             (short) ar_info->trigger_mask, ar_info->sampling_type,
                             &analyze_request[i].request_id, receive_event);
         else
            analyze_request[i].request_id = -1;
      }

}

/*------------------------------------------------------------------*/

void register_requests(void)
{
   INT index, status;
   char str[256];
   AR_INFO *ar_info;
   AR_STATS *ar_stats;
   HNDLE hKey;

   /* scan ANALYZE_REQUEST table from ANALYZE.C */
   for (index = 0; analyze_request[index].event_name[0]; index++) {
      ar_info = &analyze_request[index].ar_info;
      ar_stats = &analyze_request[index].ar_stats;

      /* create common subtree from analyze_request table in analyze.c */
      sprintf(str, "/%s/%s/Common", analyzer_name, analyze_request[index].event_name);
      db_check_record(hDB, 0, str, ANALYZER_REQUEST_STR, TRUE);
      db_find_key(hDB, 0, str, &hKey);
      analyze_request[index].hkey_common = hKey;

      strcpy(ar_info->client_name, analyzer_name);
      gethostname(ar_info->host, sizeof(ar_info->host));
      db_set_record(hDB, hKey, ar_info, sizeof(AR_INFO), 0);

      /* open hot link to analyzer request info */
      db_open_record(hDB, hKey, ar_info, sizeof(AR_INFO), MODE_READ, update_request,
                     NULL);

      /* create statistics tree */
      sprintf(str, "/%s/%s/Statistics", analyzer_name, analyze_request[index].event_name);
      db_check_record(hDB, 0, str, ANALYZER_STATS_STR, TRUE);
      db_find_key(hDB, 0, str, &hKey);
      assert(hKey);

      ar_stats->events_received = 0;
      ar_stats->events_per_sec = 0;
      ar_stats->events_written = 0;

      /* open hot link to statistics tree */
      status =
          db_open_record(hDB, hKey, ar_stats, sizeof(AR_STATS), MODE_WRITE, NULL, NULL);
      if (status != DB_SUCCESS)
         printf("Cannot open statistics record, probably other analyzer is using it\n");

      if (clp.online) {
         /*---- open event buffer ---------------------------------------*/
         bm_open_buffer(ar_info->buffer, 2*MAX_EVENT_SIZE,
                        &analyze_request[index].buffer_handle);

         /* set the default buffer cache size */
         bm_set_cache_size(analyze_request[index].buffer_handle, 100000, 0);

         /*---- request event -------------------------------------------*/
         if (ar_info->enabled)
            bm_request_event(analyze_request[index].buffer_handle,
                             (short) ar_info->event_id, (short) ar_info->trigger_mask,
                             ar_info->sampling_type, &analyze_request[index].request_id,
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
   static DWORD last_time = 0;
   DWORD actual_time;

   actual_time = ss_millitime();

   if (last_time == 0)
      last_time = actual_time;

   if (actual_time - last_time == 0)
      return;

   for (i = 0; analyze_request[i].event_name[0]; i++) {
      ar_stats = &analyze_request[i].ar_stats;
      ar_stats->events_received += analyze_request[i].events_received;
      ar_stats->events_written += analyze_request[i].events_written;
      ar_stats->events_per_sec =
          (analyze_request[i].events_received / ((actual_time - last_time) / 1000.0));
      analyze_request[i].events_received = 0;
      analyze_request[i].events_written = 0;
   }

   /* propagate new statistics to ODB */
   db_send_changed_records();

   /* save tests in ODB */
   test_write(actual_time - last_time);

   last_time = actual_time;
}

/*-- Book histos --------------------------------------------------*/

#ifdef USE_ROOT

/* h1_book and h2_book are now templates in midas.h */

//==============================================================================

TCutG *cut_book(const char *name)
{

//------------------------------------------------------------------------------

   open_subfolder("cuts");

   TFolder *folder(gHistoFolderStack->Last()? (TFolder *) gHistoFolderStack->
                   Last() : gManaHistosFolder);

   TCutG *cut((TCutG *) folder->FindObject(name));

   if (!cut) {
      cut = new TCutG();
      cut->SetName(name);
      folder->Add(cut);
   }

   close_subfolder();
   return cut;
}

void open_subfolder(char *name)
{

   TFolder *current = (TFolder *) gHistoFolderStack->Last();

   if (!current)
      current = gManaHistosFolder;

   // if the subfolder already exists, use it
   TFolder *subfolder = 0;

   TCollection *listOfSubFolders = current->GetListOfFolders();
   TIter iter(listOfSubFolders);
   while (TObject * obj = iter()) {
      if (strcmp(obj->GetName(), name) == 0 && obj->InheritsFrom("TFolder"))
         subfolder = (TFolder *) obj;
   }

   if (!subfolder) {
      subfolder = new TFolder(name, name);
      current->Add(subfolder);
   }

   gHistoFolderStack->Add(subfolder);
}

void close_subfolder()
{
   if (gHistoFolderStack->Last())
      gHistoFolderStack->Remove(gHistoFolderStack->Last());
}

#endif

/*-- Clear histos --------------------------------------------------*/
#ifdef HAVE_HBOOK
INT clear_histos_hbook(INT id1, INT id2)
{
   INT i;

   if (id1 != id2) {
      printf("Clear ID %d to ID %d\n", id1, id2);
      for (i = id1; i <= id2; i++)
         if (HEXIST(i))
            HRESET(i, bstr);
   } else {
      printf("Clear ID %d\n", id1);
      HRESET(id1, bstr);
   }

   return SUCCESS;
}
#endif                          /* HAVE_HBOOK */

/*------------------------------------------------------------------*/

void lock_histo(INT id)
{
   INT i;

   for (i = 0; i < 10000; i++)
      if (lock_list[i] == 0)
         break;

   lock_list[i] = id;
}

/*------------------------------------------------------------------*/

#ifdef HAVE_HBOOK
INT ana_callback(INT index, void *prpc_param[])
{
   if (index == RPC_ANA_CLEAR_HISTOS)
      clear_histos_hbook(CINT(0), CINT(1));
   return RPC_SUCCESS;
}
#endif

/*------------------------------------------------------------------*/

void load_last_histos()
{
   char str[256];

   /* load previous online histos */
   if (!clp.no_load) {
      strcpy(str, out_info.last_histo_filename);

      if (strchr(str, DIR_SEPARATOR) == NULL)
         add_data_dir(str, out_info.last_histo_filename);

#ifdef HAVE_HBOOK
      {
         FILE *f;
         char str2[256];
         int i;

         for (i = 0; i < (int) strlen(str); i++)
            if (isupper(str[i]))
               break;

         if (i < (int) strlen(str)) {
            printf
                ("Error: Due to a limitation in HBOOK, directoy names may not contain uppercase\n");
            printf("       characters. Histogram loading from %s will not work.\n", str);
         } else {
            f = fopen(str, "r");
            if (f != NULL) {
               fclose(f);
               printf("Loading previous online histos from %s\n", str);
               strcpy(str2, "A");
               HRGET(0, str, str2);

               /* fix wrongly booked N-tuples at ID 100000 */
               if (HEXIST(100000))
                  HDELET(100000);
            }
         }
      }
#endif                          /* HAVE_HBOOK */

#ifdef USE_ROOT
      printf("Loading previous online histos from %s\n", str);
      LoadRootHistograms(gManaHistosFolder, str);
#endif
   }
}

/*------------------------------------------------------------------*/

void save_last_histos()
{
   char str[256];

   /* save online histos */
   strcpy(str, out_info.last_histo_filename);
   if (strchr(str, DIR_SEPARATOR) == NULL)
      add_data_dir(str, out_info.last_histo_filename);

   printf("Saving current online histos to %s\n", str);

#ifdef HAVE_HBOOK
   {
      int i;
      char str2[256];

      for (i = 0; i < (int) strlen(str); i++)
         if (isupper(str[i]))
            break;

      if (i < (int) strlen(str)) {
         printf
             ("Error: Due to a limitation in HBOOK, directoy names may not contain uppercase\n");
         printf("       characters. Histogram saving to %s will not work.\n", str);
      } else {
         strcpy(str2, "NT");
         HRPUT(0, str, str2);
      }
   }
#endif

#ifdef USE_ROOT
   SaveRootHistograms(gManaHistosFolder, str);
#endif

}

/*------------------------------------------------------------------*/

INT loop_online()
{
   INT status = SUCCESS;
   DWORD last_time_loop, last_time_update, actual_time;
   int ch;

   printf("Running analyzer online. Stop with \"!\"\n");

   /* main loop */
   last_time_update = 0;
   last_time_loop = 0;

   do {
      /* calculate events per second */
      actual_time = ss_millitime();

      if (actual_time - last_time_update > 1000) {
         /* update statistics */
         update_stats();
         last_time_update = actual_time;

         /* check keyboard */
         ch = 0;
         while (ss_kbhit()) {
            ch = ss_getchar(0);
            if (ch == -1)
               ch = getchar();

            if ((char) ch == '!')
               break;
         }

         if ((char) ch == '!')
            break;
      }

      if (analyzer_loop_period == 0)
         status = cm_yield(1000);
      else {
         if (actual_time - last_time_loop > (DWORD) analyzer_loop_period) {
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

/*------------------------------------------------------------------*/

INT init_module_parameters(BOOL bclose)
{
   INT i, j, status, size;
   ANA_MODULE **module;
   char str[80];
   HNDLE hkey;

   for (i = 0; analyze_request[i].event_name[0]; i++) {
      module = analyze_request[i].ana_module;
      for (j = 0; module != NULL && module[j] != NULL; j++) {
         if (module[j]->parameters != NULL) {
            sprintf(str, "/%s/Parameters/%s", analyzer_name, module[j]->name);

            if (bclose) {
               db_find_key(hDB, 0, str, &hkey);
               db_close_record(hDB, hkey);
            } else {
               status = db_find_key(hDB, 0, str, &hkey);
               if (status == DB_SUCCESS) {
                  db_get_record_size(hDB, hkey, 0, &size);
                  if (size != module[j]->param_size)
                     status = 0;
               }
               if (status != DB_SUCCESS && module[j]->init_str) {
                  if (db_check_record(hDB, 0, str, strcomb((const char **)module[j]->init_str), TRUE) !=
                      DB_SUCCESS) {
                     cm_msg(MERROR, "init_module_parameters",
                            "Cannot create/check \"%s\" parameters in ODB", str);
                     return 0;
                  }
               }

               db_find_key(hDB, 0, str, &hkey);
               assert(hkey);

               if (db_open_record(hDB, hkey, module[j]->parameters, module[j]->param_size,
                                  MODE_READ, NULL, NULL) != DB_SUCCESS) {
                  cm_msg(MERROR, "init_module_parameters",
                         "Cannot open \"%s\" parameters in ODB", str);
                  return 0;
               }
            }
         }
      }
   }

   return SUCCESS;
}

/*------------------------------------------------------------------*/
void odb_load(EVENT_HEADER * pevent)
{
   BOOL flag;
   int size, i, status;
   char str[256];
   HNDLE hKey, hKeyRoot, hKeyEq;

   flag = TRUE;
   size = sizeof(flag);
   sprintf(str, "/%s/ODB Load", analyzer_name);
   db_get_value(hDB, 0, str, &flag, &size, TID_BOOL, TRUE);

   if (flag) {
      for (i = 0; i < 10; i++)
         if (clp.protect[i][0] && !clp.quiet)
            printf("Protect ODB tree \"%s\"\n", clp.protect[i]);

      if (!clp.quiet)
         printf("Load ODB from run %d...", (int) current_run_number);

      if (flag == 1) {
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
         if (hKeyRoot) {
            db_set_mode(hDB, hKeyRoot, MODE_READ | MODE_WRITE | MODE_DELETE, FALSE);

            for (i = 0;; i++) {
               status = db_enum_key(hDB, hKeyRoot, i, &hKeyEq);
               if (status == DB_NO_MORE_SUBKEYS)
                  break;

               db_set_mode(hDB, hKeyEq, MODE_READ | MODE_WRITE | MODE_DELETE, TRUE);

               db_find_key(hDB, hKeyEq, "Variables", &hKey);
               if (hKey)
                  db_set_mode(hDB, hKey, MODE_READ, TRUE);
            }
         }

         /* lock protected trees */
         for (i = 0; i < 10; i++)
            if (clp.protect[i][0]) {
               db_find_key(hDB, 0, clp.protect[i], &hKey);
               if (hKey)
                  db_set_mode(hDB, hKey, MODE_READ, TRUE);
            }
      }

      /* close open records to parameters */
      init_module_parameters(TRUE);

      if (strncmp((char *) (pevent + 1), "<?xml version=\"1.0\"", 19) == 0)
         db_paste_xml(hDB, 0, (char *) (pevent + 1));
      else
         db_paste(hDB, 0, (char *) (pevent + 1));

      if (flag == 1)
         db_set_mode(hDB, 0, MODE_READ | MODE_WRITE | MODE_DELETE, TRUE);

      /* reinit structured opened by user analyzer */
      analyzer_init();

      /* reload parameter files after BOR event */
      if (!clp.quiet)
         printf("OK\n");
      load_parameters(current_run_number);

      /* open module parameters again */
      init_module_parameters(FALSE);
   }
}

/*------------------------------------------------------------------*/

#define MA_DEVICE_DISK        1
#define MA_DEVICE_TAPE        2
#define MA_DEVICE_FTP         3
#define MA_DEVICE_PVM         4

#define MA_FORMAT_MIDAS       (1<<0)
#define MA_FORMAT_YBOS        (1<<2)
#define MA_FORMAT_GZIP        (1<<3)

typedef struct {
   char file_name[256];
   int format;
   int device;
   int fd;
#ifdef HAVE_ZLIB
   gzFile gzfile;
#else
   FILE *file;
#endif
   char *buffer;
   int wp, rp;
   /*FTP_CON ftp_con; */
} MA_FILE;

/*------------------------------------------------------------------*/

MA_FILE *ma_open(char *file_name)
{
   char *ext_str;
   MA_FILE *file;

   /* allocate MA_FILE structure */
   file = (MA_FILE *) calloc(sizeof(MA_FILE), 1);
   if (file == NULL) {
      cm_msg(MERROR, "ma_open", "Cannot allocate MA file structure");
      return NULL;
   }

   /* save file name */
   strcpy(file->file_name, file_name);

   /* for now, just read from disk */
   file->device = MA_DEVICE_DISK;

   /* or from PVM */
   if (pvm_slave) {
      file->device = MA_DEVICE_PVM;
      file->buffer = (char *) malloc(PVM_BUFFER_SIZE);
      file->wp = file->rp = 0;
   }

   /* check input file extension */
   if (strchr(file_name, '.')) {
      ext_str = file_name + strlen(file_name) - 1;
      while (*ext_str != '.')
         ext_str--;
   } else
      ext_str = "";

   if (strncmp(ext_str, ".gz", 3) == 0) {
#ifdef HAVE_ZLIB
      ext_str--;
      while (*ext_str != '.' && ext_str > file_name)
         ext_str--;
#else
      cm_msg(MERROR, "ma_open",
             ".gz extension not possible because zlib support is not compiled in.\n");
      return NULL;
#endif
   }

   if (strncmp(file_name, "/dev/", 4) == 0)     /* assume MIDAS tape */
      file->format = MA_FORMAT_MIDAS;
   else if (strncmp(ext_str, ".mid", 4) == 0)
      file->format = MA_FORMAT_MIDAS;
   else if (strncmp(ext_str, ".ybs", 4) == 0)
     assert(!"YBOS not supported anymore");
   else {
      printf
          ("Unknown input data format \"%s\". Please use file extension .mid or mid.gz.\n",
           ext_str);
      return NULL;
   }

   if (file->device == MA_DEVICE_DISK) {
      if (file->format == MA_FORMAT_YBOS) {
	assert(!"YBOS not supported anymore");
      } else {
#ifdef HAVE_ZLIB
         file->gzfile = gzopen(file_name, "rb");
         if (file->gzfile == NULL)
            return NULL;
#else
         file->file = fopen(file_name, "rb");
         if (file->file == NULL)
            return NULL;
#endif
      }
   }

   return file;
}

/*------------------------------------------------------------------*/

int ma_close(MA_FILE * file)
{
   if (file->format == MA_FORMAT_YBOS)
     assert(!"YBOS not supported anymore");
   else
#ifdef HAVE_ZLIB
      gzclose((gzFile)file->gzfile);
#else
      fclose(file->file);
#endif

   free(file);
   return SUCCESS;
}

/*------------------------------------------------------------------*/

int ma_read_event(MA_FILE * file, EVENT_HEADER * pevent, int size)
{
   int n;

   if (file->device == MA_DEVICE_DISK) {
      if (file->format == MA_FORMAT_MIDAS) {
         if (size < (int) sizeof(EVENT_HEADER)) {
            cm_msg(MERROR, "ma_read_event", "Buffer size too small");
            return -1;
         }

         /* read event header */
#ifdef HAVE_ZLIB
         n = gzread(file->gzfile, pevent, sizeof(EVENT_HEADER));
#else
         n = sizeof(EVENT_HEADER)*fread(pevent, sizeof(EVENT_HEADER), 1, file->file);
#endif

         if (n < (int) sizeof(EVENT_HEADER)) {
            if (n > 0)
               printf("Unexpected end of file %s, last event skipped\n", file->file_name);
            return -1;
         }

         /* swap event header if in wrong format */
#ifdef SWAP_EVENTS
         WORD_SWAP(&pevent->event_id);
         WORD_SWAP(&pevent->trigger_mask);
         DWORD_SWAP(&pevent->serial_number);
         DWORD_SWAP(&pevent->time_stamp);
         DWORD_SWAP(&pevent->data_size);
#endif

         /* read event */
         n = 0;
         if (pevent->data_size > 0) {
            if (size < (int) pevent->data_size + (int) sizeof(EVENT_HEADER)) {
               cm_msg(MERROR, "ma_read_event", "Buffer size too small");
               return -1;
            }
#ifdef HAVE_ZLIB
            n = gzread(file->gzfile, pevent + 1, pevent->data_size);
#else
            n = pevent->data_size*fread(pevent + 1, pevent->data_size, 1, file->file);
#endif
            if (n != (INT) pevent->data_size) {
               printf("Unexpected end of file %s, last event skipped\n", file->file_name);
               return -1;
            }
         }

         return n + sizeof(EVENT_HEADER);
      } else if (file->format == MA_FORMAT_YBOS) {
	assert(!"YBOS not supported anymore");
      }
   } else if (file->device == MA_DEVICE_PVM) {
#ifdef HAVE_PVM
      int bufid, len, tag, tid;
      EVENT_HEADER *pe;
      struct timeval timeout;

      /* check if anything in buffer */
      if (file->wp > file->rp) {
         pe = (EVENT_HEADER *) (file->buffer + file->rp);
         size = sizeof(EVENT_HEADER) + pe->data_size;
         memcpy(pevent, pe, size);
         file->rp += size;
         return size;
      }

      /* send data request */
      pvm_initsend(PvmDataInPlace);
      pvm_send(pvm_myparent, TAG_DATA);

      /* receive data */
      timeout.tv_sec = 60;
      timeout.tv_usec = 0;

      bufid = pvm_trecv(-1, -1, &timeout);
      if (bufid < 0) {
         pvm_perror("pvm_recv");
         return -1;
      }
      if (bufid == 0) {
         PVM_DEBUG("ma_read_event: timeout receiving data, aborting analyzer.\n");
         return -1;
      }

      status = pvm_bufinfo(bufid, &len, &tag, &tid);
      if (status < 0) {
         pvm_perror("pvm_bufinfo");
         return -1;
      }

      PVM_DEBUG("ma_read_event: receive tag %d, buflen %d", tag, len);

      if (tag == TAG_EOR || tag == TAG_EXIT)
         return -1;

      file->wp = len;
      file->rp = 0;
      status = pvm_upkbyte((char *) file->buffer, len, 1);
      if (status < 0) {
         pvm_perror("pvm_upkbyte");
         return -1;
      }

      /* no new data available, sleep some time to reduce network traffic */
      if (len == 0)
         ss_sleep(200);

      /* re-call this function */
      return ma_read_event(file, pevent, size);
#endif
   }

   return 0;
}

/*------------------------------------------------------------------*/

INT analyze_run(INT run_number, char *input_file_name, char *output_file_name)
{
   EVENT_HEADER *pevent, *pevent_unaligned;
   ANALYZE_REQUEST *par;
   INT i, n, size;
   DWORD num_events_in, num_events_out;
   char error[256], str[256];
   INT status = SUCCESS;
   MA_FILE *file;
   BOOL skip;
   DWORD start_time;

   /* set output file name and flags in ODB */
   sprintf(str, "/%s/Output/Filename", analyzer_name);
   db_set_value(hDB, 0, str, output_file_name, 256, 1, TID_STRING);
#ifdef HAVE_HBOOK
   sprintf(str, "/%s/Output/RWNT", analyzer_name);
   db_set_value(hDB, 0, str, &clp.rwnt, sizeof(BOOL), 1, TID_BOOL);
#endif

   assert(run_number > 0);

   /* set run number in ODB */
   status =
       db_set_value(hDB, 0, "/Runinfo/Run number", &run_number, sizeof(run_number), 1,
                    TID_INT);
   assert(status == SUCCESS);

   /* set file name in out_info */
   strcpy(out_info.filename, output_file_name);

   /* let changes propagate to modules */
   cm_yield(0);

   /* open input file, will be changed to ma_open_file later... */
   file = ma_open(input_file_name);
   if (file == NULL) {
      printf("Cannot open input file \"%s\"\n", input_file_name);
      return -1;
   }

   pevent_unaligned = (EVENT_HEADER *) malloc(EXT_EVENT_SIZE);
   if (pevent_unaligned == NULL) {
      printf("Not enough memeory\n");
      return -1;
   }
   pevent = (EVENT_HEADER *) ALIGN8((POINTER_T) pevent_unaligned);

   /* call analyzer bor routines */
   bor(run_number, error);

   num_events_in = num_events_out = 0;

   start_time = ss_millitime();

   /* event loop */
   do {
      /* read next event */
      n = ma_read_event(file, pevent, EXT_EVENT_SIZE);
      if (n <= 0)
         break;

      num_events_in++;

      /* copy system events (BOR, EOR, MESSAGE) to output file */
      if (pevent->event_id < 0) {
         status = process_event(NULL, pevent);
         if (status < 0 || status == RPC_SHUTDOWN)      /* disk full/stop analyzer */
            break;

         if (out_file && out_format == FORMAT_MIDAS) {
            size = pevent->data_size + sizeof(EVENT_HEADER);
#ifdef HAVE_ZLIB
            if (out_gzip)
               status = gzwrite((gzFile)out_file, pevent, size) == size ? SUCCESS : SS_FILE_ERROR;
            else
#endif
               status =
                   fwrite(pevent, 1, size,
                          out_file) == (size_t) size ? SUCCESS : SS_FILE_ERROR;

            if (status != SUCCESS) {
               cm_msg(MERROR, "analyze_run", "Error writing to file (Disk full?)");
               return -1;
            }

            num_events_out++;
         }

         /* reinit start time after BOR event */
         if (pevent->event_id == EVENTID_BOR)
            start_time = ss_millitime();
      }

      /* check if event is in event limit */
      skip = FALSE;

      if (!pvm_slave) {
         if (clp.n[0] > 0 || clp.n[1] > 0) {
            if (clp.n[1] == 0) {
               /* treat n[0] as upper limit */
               if (num_events_in > clp.n[0]) {
                  num_events_in--;
                  status = SUCCESS;
                  break;
               }
            } else {
               if (num_events_in > clp.n[1]) {
                  status = SUCCESS;
                  break;
               }
               if (num_events_in < clp.n[0])
                  skip = TRUE;
               else if (clp.n[2] > 0 && num_events_in % clp.n[2] != 0)
                  skip = TRUE;
            }
         }
      }

      if (!skip) {
         /* find request belonging to this event */
         par = analyze_request;
         status = SUCCESS;
         for (i = 0; par->event_name[0]; par++)
            if ((par->ar_info.event_id == EVENTID_ALL ||
                 par->ar_info.event_id == pevent->event_id) &&
                (par->ar_info.trigger_mask == TRIGGER_ALL ||
                 (par->ar_info.trigger_mask & pevent->trigger_mask))
                && par->ar_info.enabled) {
               /* analyze this event */
               status = process_event(par, pevent);
               if (status == SUCCESS)
                  num_events_out++;
               if (status < 0 || status == RPC_SHUTDOWN)        /* disk full/stop analyzer */
                  break;

               /* check for Ctrl-C */
               status = cm_yield(0);
            }
         if (status < 0 || status == RPC_SHUTDOWN)
            break;
      }

      /* update ODB statistics once every 100 events */
      if (num_events_in % 100 == 0) {
         update_stats();
         if (!clp.quiet) {
            if (out_file)
               printf("%s:%d  %s:%d  events\r", input_file_name, (int) num_events_in,
                      out_info.filename, (int) num_events_out);
            else
               printf("%s:%d  events\r", input_file_name, (int) num_events_in);

#ifndef OS_WINNT
            fflush(stdout);
#endif
         }
      }
   } while (1);

#ifdef HAVE_PVM
   PVM_DEBUG("analyze_run: event loop finished, status = %d", status);
#endif

   /* signal EOR to slaves */
#ifdef HAVE_PVM
   if (pvm_master) {
      if (status == RPC_SHUTDOWN)
         printf("\nShutting down distributed analyzers, please wait...\n");
      pvm_eor(status == RPC_SHUTDOWN ? TAG_EXIT : TAG_EOR);

      /* merge slave output files */
      if (out_info.filename[0] && !out_append)
         status = pvm_merge();

      start_time = ss_millitime() - start_time;

      update_stats();
      if (!clp.quiet) {
         if (out_file)
            printf("%s:%d  %s:%d  events, %1.2lfs\n", input_file_name, num_events_in,
                   out_info.filename, num_events_out, start_time / 1000.0);
         else
            printf("%s:%d  events, %1.2lfs\n", input_file_name, num_events_in,
                   start_time / 1000.0);
      }
   } else if (pvm_slave) {
      start_time = ss_millitime() - start_time;

      update_stats();
      if (!clp.quiet) {
         if (out_file)
            printf("%s:%d  %s:%d  events, %1.2lfs\n", input_file_name, num_events_in,
                   out_info.filename, num_events_out, start_time / 1000.0);
         else
            printf("%s:%d  events, %1.2lfs\n", input_file_name, num_events_in,
                   start_time / 1000.0);
      }

      eor(current_run_number, error);

      /* send back tests */
      pvm_initsend(PvmDataInPlace);

      for (i = 0; i < n_test; i++)
         pvm_pkbyte((char *) tl[i], sizeof(ANA_TEST), 1);

      PVM_DEBUG("analyze_run: send %d tests back to master", n_test);

      status = pvm_send(pvm_myparent, TAG_EOR);
      if (status < 0) {
         pvm_perror("pvm_send");
         return RPC_SHUTDOWN;
      }
   } else {
      start_time = ss_millitime() - start_time;

      update_stats();
      if (!clp.quiet) {
         if (out_file)
            printf("%s:%d  %s:%d  events, %1.2lfs\n", input_file_name, num_events_in,
                   out_info.filename, num_events_out, start_time / 1000.0);
         else
            printf("%s:%d  events, %1.2lfs\n", input_file_name, num_events_in,
                   start_time / 1000.0);
      }

      /* call analyzer eor routines */
      eor(current_run_number, error);
   }
#else

   start_time = ss_millitime() - start_time;

   update_stats();
   if (!clp.quiet) {
      if (out_file)
         printf("%s:%d  %s:%d  events, %1.2lfs\n", input_file_name, (int) num_events_in,
                out_info.filename, (int) num_events_out, start_time / 1000.0);
      else
         printf("%s:%d  events, %1.2lfs\n", input_file_name, (int) num_events_in,
                start_time / 1000.0);
   }

   /* call analyzer eor routines */
   eor(current_run_number, error);

#endif

   ma_close(file);

   free(pevent_unaligned);

   return status;
}

/*------------------------------------------------------------------*/

INT loop_runs_offline()
{
   INT i, status, run_number;
   char input_file_name[256], output_file_name[256], *prn;
   BANK_LIST *bank_list;

   if (!clp.quiet)
      printf("Running analyzer offline. Stop with \"!\"\n");

   run_number = 0;
   out_append = ((strchr(clp.input_file_name[0], '%') != NULL) &&
                 (strchr(clp.output_file_name, '%') == NULL))
       || clp.input_file_name[1][0];

   /* loop over range of files */
   if (clp.run_number[0] > 0) {
      if (strchr(clp.input_file_name[0], '%') == NULL) {
         printf
             ("Input file name must contain a wildcard like \"%%05d\" when using a range.\n");
         return 0;
      }

      if (clp.run_number[0] == 0) {
         printf("End of range not specified.\n");
         return 0;
      }

      for (run_number = clp.run_number[0]; run_number <= clp.run_number[1]; run_number++) {
         sprintf(input_file_name, clp.input_file_name[0], run_number);
         if (strchr(clp.output_file_name, '%') != NULL)
            sprintf(output_file_name, clp.output_file_name, run_number);
         else
            strcpy(output_file_name, clp.output_file_name);

         status = analyze_run(run_number, input_file_name, output_file_name);
         if (status == RPC_SHUTDOWN)
            break;
      }
   } else {
      /* loop over input file names */
      for (i = 0; clp.input_file_name[i][0] && i < 10; i++) {
         strcpy(input_file_name, clp.input_file_name[i]);

         /* get run number from input file */
         prn = input_file_name;
         while (strchr(prn, DIR_SEPARATOR) != NULL)
            prn = strchr(prn, DIR_SEPARATOR) + 1;

         if (strpbrk(prn, "0123456789"))
            run_number = atoi(strpbrk(prn, "0123456789"));

         if (strchr(clp.output_file_name, '%') != NULL) {
            if (run_number == 0) {
               printf("Cannot extract run number from input file name.\n");
               return 0;
            }
            sprintf(output_file_name, clp.output_file_name, run_number);
         } else
            strcpy(output_file_name, clp.output_file_name);

         status = analyze_run(run_number, input_file_name, output_file_name);
         if (status == RPC_SHUTDOWN)
            break;
      }
   }

   /* close output file in append mode */
   if (out_file && out_append) {
      if (out_format == FORMAT_HBOOK) {
#ifdef HAVE_HBOOK
         char str[80];

         HROUT(0, i, bstr);
         strcpy(str, "OFFLINE");
         HREND(str);
#else
         cm_msg(MERROR, "loop_runs_offline", "HBOOK support is not compiled in");
#endif
      } else if (out_format == FORMAT_ROOT) {
#ifdef USE_ROOT
         CloseRootOutputFile();
#else
         cm_msg(MERROR, "loop_runs_offline", "ROOT support is not compiled in");
#endif                          /* USE_ROOT */
      } else {
#ifdef HAVE_ZLIB
         if (out_gzip)
            gzclose((gzFile)out_file);
         else
#endif
            fclose(out_file);
      }

      /* free bank buffer */
      for (i = 0; analyze_request[i].event_name[0]; i++) {
         bank_list = analyze_request[i].bank_list;

         if (bank_list == NULL)
            continue;

         for (; bank_list->name[0]; bank_list++)
            if (bank_list->addr) {
               free(bank_list->addr);
               bank_list->addr = NULL;
            }
      }
   }
#ifdef HAVE_PVM
   /* merge slave output files */
   if (pvm_master && out_info.filename[0] && out_append)
      pvm_merge();
#endif

   return CM_SUCCESS;
}

/*------------------------------------------------------------------*/

#ifdef HAVE_PVM

int pvm_main(char *argv[])
{
   int mytid, status, i, j, dtid, *pvm_tid, bufid;
   char path[256];
   struct timeval timeout;

   getcwd(path, 256);

   mytid = pvm_mytid();
   if (mytid < 0) {
      pvm_perror("pvm_mytid");
      return 0;
   }

   chdir(path);

   status = pvm_setopt(PvmRoute, PvmRouteDirect);
   if (status < 0) {
      pvm_perror("pvm_setopt");
      pvm_exit();
      return 0;
   }

   pvm_myparent = pvm_parent();
   if (pvm_myparent < 0 && pvm_myparent != PvmNoParent) {
      pvm_perror("pvm_parent");
      pvm_exit();
      return 0;
   }

   /* check if master */
   if (pvm_myparent == PvmNoParent) {
      struct pvmhostinfo *hostp;
      int n_host, n_arch;

#ifdef WIN32
      char *p;

      /* use no path, executable must be under $PVM_ROOT$/bin/WIN32 */
      p = argv[0] + strlen(argv[0]) - 1;
      while (p > argv[0] && *p != '\\')
         p--;
      if (*p == '\\')
         p++;
      strcpy(path, p);
#else
      if (strchr(argv[0], '/') == 0) {
         getcwd(path, 256);
         strcat(path, "/");
         strcat(path, argv[0]);
      } else
         strcpy(path, argv[0]);
#endif

      /* return if no parallelization selected */
      if (clp.n_task == -1)
         return SUCCESS;

      /* Set number of slaves to start */
      pvm_config(&n_host, &n_arch, &hostp);

      pvm_n_task = n_host - 1;
      if (clp.n_task != 0)
         pvm_n_task = clp.n_task;

      if (clp.n_task != 1 && pvm_n_task > n_host - 1)
         pvm_n_task = n_host - 1;

      if (pvm_n_task == 0)
         return SUCCESS;

      pvm_master = TRUE;

      pvm_tid = malloc(sizeof(int) * pvm_n_task);
      pvmc = malloc(sizeof(PVM_CLIENT) * pvm_n_task);

      if (pvm_tid == NULL || pvmc == NULL) {
         free(pvm_tid);
         printf("Not enough memory to allocate PVM structures.\n");
         pvm_exit();
         return 0;
      }

      memset(pvmc, 0, sizeof(PVM_CLIENT) * pvm_n_task);

      for (i = 0; i < pvm_n_task; i++) {
         pvmc[i].buffer = malloc(PVM_BUFFER_SIZE);
         if (pvmc[i].buffer == NULL) {
            free(pvm_tid);
            free(pvmc);
            printf("Not enough memory to allocate PVM buffers.\n");
            pvm_exit();
            return 0;
         }
      }

      /* spawn slaves */
      printf("Parallelizing analyzer on %d machines\n", pvm_n_task);
      if (pvm_n_task == 1)
         status = pvm_spawn(path, argv + 1, PvmTaskDefault, NULL, pvm_n_task, pvm_tid);
      else
         status =
             pvm_spawn(path, argv + 1, PvmTaskHost | PvmHostCompl, ".", pvm_n_task,
                       pvm_tid);
      if (status == 0) {
         pvm_perror("pvm_spawn");
         pvm_exit();
         free(pvm_tid);
         free(pvmc);
         return 0;
      }

      for (i = 0; i < pvm_n_task; i++) {
         pvmc[i].tid = pvm_tid[i];
         pvmc[i].wp = 0;
         pvmc[i].n_events = 0;
         pvmc[i].time = 0;
         dtid = pvm_tidtohost(pvm_tid[i]);
         for (j = 0; j < n_host; j++)
            if (dtid == hostp[j].hi_tid)
               strcpy(pvmc[i].host, hostp[j].hi_name);
      }

      PVM_DEBUG("Spawing on hosts:");
      for (i = 0; i < pvm_n_task; i++)
         PVM_DEBUG("%s", pvmc[i].host);

      if (status < pvm_n_task) {
         printf("Trouble spawning slaves. Aborting. Error codes are:\n");
         for (i = 0; i < pvm_n_task; i++)
            printf("TID %d %d\n", i, pvm_tid[i]);

         pvm_exit();
         free(pvm_tid);
         free(pvmc);
         return 0;
      }

      /* send slave index */
      for (i = 0; i < pvm_n_task; i++) {
         pvm_initsend(PvmDataDefault);

         pvm_pkint(&i, 1, 1);

         PVM_DEBUG("pvm_main: send index to client %d", i);

         status = pvm_send(pvmc[i].tid, TAG_INIT);
         if (status < 0) {
            pvm_perror("pvm_send");
            return 0;
         }
      }

      free(pvm_tid);
   } else {
      char path[256];

      pvm_master = FALSE;
      pvm_slave = TRUE;

      /* go to path from argv[0] */
      strcpy(path, argv[0]);
      for (i = strlen(path); i > 0 && path[i] != DIR_SEPARATOR; i--)
         path[i] = 0;
      if (i > 0)
         path[i] = 0;

      chdir(path);
      PVM_DEBUG("PATH=%s", path);

      /* receive slave index */
      timeout.tv_sec = 10;
      timeout.tv_usec = 0;

      bufid = pvm_trecv(-1, -1, &timeout);
      if (bufid < 0) {
         pvm_perror("pvm_recv");
         return 0;
      }
      if (bufid == 0) {
         PVM_DEBUG("pvm_main: timeout receiving index, aborting analyzer.\n");
         return 0;
      }

      status = pvm_upkint(&pvm_client_index, 1, 1);
      if (status < 0) {
         pvm_perror("pvm_upkint");
         return 0;
      }

      PVM_DEBUG("Received client ID %d", pvm_client_index);
   }

   return SUCCESS;
}

/*------------------------------------------------------------------*/

int pvm_send_event(int index, EVENT_HEADER * pevent)
{
   struct timeval timeout;
   int bufid, len, tag, tid, status;

   if (pevent->data_size + sizeof(EVENT_HEADER) >= PVM_BUFFER_SIZE) {
      printf("Event too large (%d) for PVM buffer (%d), analyzer aborted\n",
             pevent->data_size + sizeof(EVENT_HEADER), PVM_BUFFER_SIZE);
      return RPC_SHUTDOWN;
   }

   /* wait on event request */
   timeout.tv_sec = 60;
   timeout.tv_usec = 0;

   bufid = pvm_trecv(pvmc[index].tid, -1, &timeout);
   if (bufid < 0) {
      pvm_perror("pvm_recv");
      return RPC_SHUTDOWN;
   }
   if (bufid == 0) {
      printf("Timeout receiving data requests from %s, aborting analyzer.\n",
             pvmc[index].host);
      return RPC_SHUTDOWN;
   }

   status = pvm_bufinfo(bufid, &len, &tag, &tid);
   if (status < 0) {
      pvm_perror("pvm_bufinfo");
      return RPC_SHUTDOWN;
   }

   PVM_DEBUG("pvm_send_event: received request from client %d", index);

   /* send event */
   pvm_initsend(PvmDataInPlace);

   pvm_pkbyte((char *) pevent, pevent->data_size + sizeof(EVENT_HEADER), 1);

   PVM_DEBUG("pvm_send_event: send events to client %d", index);

   status = pvm_send(tid, TAG_DATA);
   if (status < 0) {
      pvm_perror("pvm_send");
      return RPC_SHUTDOWN;
   }

   pvmc[index].time = ss_millitime();

   return SUCCESS;
}

/*------------------------------------------------------------------*/

int pvm_send_buffer(int index)
{
   struct timeval timeout;
   int i, bufid, len, tag, tid, status;

   PVM_DEBUG("pvm_send_buffer: index %d", index);

   if (index == -2) {
      bufid = pvm_nrecv(-1, -1);
   } else {
      /* wait on event request with timeout */
      timeout.tv_sec = 60;
      timeout.tv_usec = 0;

      bufid = pvm_trecv(-1, -1, &timeout);
   }

   if (bufid < 0) {
      pvm_perror("pvm_recv");
      return RPC_SHUTDOWN;
   }
   if (bufid == 0) {
      if (index == -2)
         return SUCCESS;

      printf("Timeout receiving data requests, aborting analyzer.\n");
      return RPC_SHUTDOWN;
   }

   status = pvm_bufinfo(bufid, &len, &tag, &tid);
   if (status < 0) {
      pvm_perror("pvm_bufinfo");
      return RPC_SHUTDOWN;
   }

   /* find index of that client */
   for (i = 0; i < pvm_n_task; i++)
      if (pvmc[i].tid == tid)
         break;

   if (i == pvm_n_task) {
      cm_msg(MERROR, "pvm_send_buffer", "received message from unknown client %d", tid);
      return RPC_SHUTDOWN;
   }

   PVM_DEBUG("pvm_send_buffer: received request from client %d", i);

   /* send event */
   pvm_initsend(PvmDataInPlace);

   pvm_pkbyte((char *) pvmc[i].buffer, pvmc[i].wp, 1);

   PVM_DEBUG("pvm_send_buffer: send %d events (%1.1lfkB) to client %d",
             pvmc[i].n_events, pvmc[i].wp / 1024.0, i);

   status = pvm_send(tid, TAG_DATA);
   if (status < 0) {
      pvm_perror("pvm_send");
      return RPC_SHUTDOWN;
   }

   pvmc[i].wp = 0;
   pvmc[i].n_events = 0;
   pvmc[i].time = ss_millitime();

   /* if specific client is requested and not emptied, try again */
   if (index >= 0 && index != i)
      return pvm_send_buffer(index);

   return SUCCESS;
}

/*------------------------------------------------------------------*/

int pvm_distribute(ANALYZE_REQUEST * par, EVENT_HEADER * pevent)
{
   char str[256];
   int i, index, size, status, min, max;

   if (par == NULL && pevent->event_id == EVENTID_BOR) {
      /* distribute ODB dump */
      for (i = 0; i < pvm_n_task; i++) {
         status = pvm_send_event(i, pevent);
         if (status != SUCCESS)
            return status;
      }

      return SUCCESS;
   }

   size = sizeof(EVENT_HEADER) + pevent->data_size;

   if (par == NULL || (par->ar_info.sampling_type & GET_FARM) == 0) {
      /* if not farmed, copy to all client buffers */
      for (i = 0; i < pvm_n_task; i++) {
         /* if buffer full, empty it */
         if (pvmc[i].wp + size >= clp.pvm_buf_size) {
            status = pvm_send_buffer(i);
            if (status != SUCCESS)
               return status;
         }

         if (size >= PVM_BUFFER_SIZE) {
            printf("Event too large (%d) for PVM buffer (%d), analyzer aborted\n", size,
                   PVM_BUFFER_SIZE);
            return RPC_SHUTDOWN;
         }

         memcpy(pvmc[i].buffer + pvmc[i].wp, pevent, size);
         pvmc[i].wp += size;
         pvmc[i].n_events++;
      }
   } else {
      /* farmed: look for buffer with lowest level */
      min = PVM_BUFFER_SIZE;
      index = 0;
      for (i = 0; i < pvm_n_task; i++)
         if (pvmc[i].wp < min) {
            min = pvmc[i].wp;
            index = i;
         }

      /* if buffer full, empty it */
      if (pvmc[index].wp + size >= clp.pvm_buf_size) {
         status = pvm_send_buffer(index);
         if (status != SUCCESS)
            return status;
      }

      if (pvmc[index].wp + size >= PVM_BUFFER_SIZE) {
         printf("Event too large (%d) for PVM buffer (%d), analyzer aborted\n", size,
                PVM_BUFFER_SIZE);
         return RPC_SHUTDOWN;
      }

      /* copy to "index" buffer */
      memcpy(pvmc[index].buffer + pvmc[index].wp, pevent, size);
      pvmc[index].wp += size;
      pvmc[index].n_events++;
   }

   sprintf(str, "%1.3lf:  ", (ss_millitime() - pvm_start_time) / 1000.0);
   for (i = 0; i < pvm_n_task; i++)
      if (i == index)
         sprintf(str + strlen(str), "#%d# ", pvmc[i].wp);
      else
         sprintf(str + strlen(str), "%d ", pvmc[i].wp);
   //PVM_DEBUG(str);

   /* find min/max buffer level */
   min = PVM_BUFFER_SIZE;
   max = 0;
   for (i = 0; i < pvm_n_task; i++) {
      if (pvmc[i].wp > max)
         max = pvmc[i].wp;
      if (pvmc[i].wp < min)
         min = pvmc[i].wp;
   }

   /* don't send events if all buffers are less than half full */
   if (max < clp.pvm_buf_size / 2)
      return SUCCESS;

   /* if all buffer are more than half full, wait for next request */
   if (min > clp.pvm_buf_size / 2) {
      status = pvm_send_buffer(-1);
      return status;
   }

   /* probe new requests */
   status = pvm_send_buffer(-2);

   return status;
}

/*------------------------------------------------------------------*/

int pvm_eor(int eor_tag)
{
   struct timeval timeout;
   int bufid, len, tag, tid, i, j, status, size;
   ANA_TEST *tst_buf;
   DWORD count;
   char str[256];

   printf("\n");

   for (i = 0; i < pvm_n_task; i++)
      pvmc[i].eor_sent = FALSE;

   do {
      /* flush remaining buffers */
      timeout.tv_sec = 60;
      timeout.tv_usec = 0;

      bufid = pvm_trecv(-1, -1, &timeout);
      if (bufid < 0) {
         pvm_perror("pvm_recv");
         return RPC_SHUTDOWN;
      }
      if (bufid == 0) {
         printf("Timeout receiving data request, aborting analyzer.\n");
         return RPC_SHUTDOWN;
      }

      status = pvm_bufinfo(bufid, &len, &tag, &tid);
      if (status < 0) {
         pvm_perror("pvm_bufinfo");
         return RPC_SHUTDOWN;
      }

      /* find index of that client */
      for (j = 0; j < pvm_n_task; j++)
         if (pvmc[j].tid == tid)
            break;

      if (j == pvm_n_task) {
         cm_msg(MERROR, "pvm_eor", "received message from unknown client %d", tid);
         return RPC_SHUTDOWN;
      }

      PVM_DEBUG("pvm_eor: received request from client %d", j);

      /* send remaining buffer if data available */
      if (eor_tag == TAG_EOR && pvmc[j].wp > 0) {
         pvm_initsend(PvmDataInPlace);

         pvm_pkbyte((char *) pvmc[j].buffer, pvmc[j].wp, 1);

         PVM_DEBUG("pvm_eor: send %d events (%1.1lfkB) to client %d",
                   pvmc[j].n_events, pvmc[j].wp / 1024.0, j);

         status = pvm_send(tid, TAG_DATA);
         if (status < 0) {
            pvm_perror("pvm_send");
            return RPC_SHUTDOWN;
         }

         pvmc[j].wp = 0;
         pvmc[j].n_events = 0;
         pvmc[j].time = ss_millitime();
      } else {
         /* send EOR */
         pvm_initsend(PvmDataDefault);

         if (eor_tag == TAG_EOR)
            PVM_DEBUG("pvm_eor: send EOR to client %d", j);
         else
            PVM_DEBUG("pvm_eor: send EXIT to client %d", j);

         printf("Shutting down %s               \r", pvmc[j].host);
         fflush(stdout);

         status = pvm_send(tid, eor_tag);
         if (status < 0) {
            pvm_perror("pvm_send");
            return RPC_SHUTDOWN;
         }

         pvmc[j].eor_sent = TRUE;

         /* wait for EOR reply */
         timeout.tv_sec = 60;
         timeout.tv_usec = 0;

         bufid = pvm_trecv(tid, -1, &timeout);
         if (bufid < 0) {
            pvm_perror("pvm_recv");
            return RPC_SHUTDOWN;
         }
         if (bufid == 0) {
            printf("Timeout receiving EOR request, aborting analyzer.\n");
            return RPC_SHUTDOWN;
         }

         status = pvm_bufinfo(bufid, &len, &tag, &tid);
         if (status < 0) {
            pvm_perror("pvm_bufinfo");
            return RPC_SHUTDOWN;
         }

         PVM_DEBUG("\nGot %d bytes", len);

         tst_buf = malloc(len);
         pvm_upkbyte((char *) tst_buf, len, 1);

         /* write tests to ODB */
         for (i = 0; i < (int) (len / sizeof(ANA_TEST)); i++) {
            sprintf(str, "/%s/Tests/%s", analyzer_name, tst_buf[i].name);
            count = 0;
            size = sizeof(DWORD);
            db_get_value(hDB, 0, str, &count, &size, TID_DWORD, TRUE);
            count += tst_buf[i].count;

            db_set_value(hDB, 0, str, &count, sizeof(DWORD), 1, TID_DWORD);
         }

         free(tst_buf);
      }

      /* check if EOR sent to all clients */
      for (j = 0; j < pvm_n_task; j++)
         if (!pvmc[j].eor_sent)
            break;

   } while (j < pvm_n_task);

   printf("\n");

   return SUCCESS;
}

/*------------------------------------------------------------------*/

int pvm_merge()
{
   int i, j;
   char fn[10][8], str[256], file_name[256], error[256];
   char ext[10], *p;

   strcpy(str, out_info.filename);
   if (strchr(str, '%') != NULL)
      sprintf(file_name, str, current_run_number);
   else
      strcpy(file_name, str);

   /* check output file extension */
   out_gzip = FALSE;
   ext[0] = 0;
   if (strchr(file_name, '.')) {
      p = file_name + strlen(file_name) - 1;
      while (*p != '.')
         p--;
      strcpy(ext, p);
   }
   if (strncmp(ext, ".gz", 3) == 0) {
      out_gzip = TRUE;
      *p = 0;
      p--;
      while (*p != '.' && p > file_name)
         p--;
      strcpy(ext, p);
   }

   if (strncmp(ext, ".asc", 4) == 0)
      out_format = FORMAT_ASCII;
   else if (strncmp(ext, ".mid", 4) == 0)
      out_format = FORMAT_MIDAS;
   else if (strncmp(ext, ".rz", 3) == 0)
      out_format = FORMAT_HBOOK;
   else {
      strcpy(error,
             "Unknown output data format. Please use file extension .asc, .mid or .rz.\n");
      cm_msg(MERROR, "pvm_merge", error);
      return 0;
   }

   if (out_format == FORMAT_HBOOK) {
      if (pvm_n_task <= 10) {
         for (i = 0; i < pvm_n_task; i++)
            sprintf(fn[i], "n%d.rz", i);

         HMERGE(pvm_n_task, fn, file_name);
      } else {
         for (i = 0; i <= pvm_n_task / 10; i++) {
            for (j = 0; j < 10 && j + i * 10 < pvm_n_task; j++)
               sprintf(fn[j], "n%d.rz", j + i * 10);

            sprintf(str, "t%d.rz", i);
            printf("Merging %d files to %s:\n", j, str);
            HMERGE(j, fn, str);
         }
         for (i = 0; i <= pvm_n_task / 10; i++)
            sprintf(fn[i], "t%d.rz", i);

         printf("Merging %d files to %s:\n", i, file_name);
         HMERGE(i, fn, file_name);
      }
   }

   return SUCCESS;
}

#endif                          /* PVM */

/*------------------------------------------------------------------*/

#ifdef USE_ROOT

/*==== ROOT socket histo server ====================================*/

#if defined ( OS_UNIX )
#define THREADRETURN
#define THREADTYPE void
#endif
#if defined( OS_WINNT )
#define THREADRETURN 0
#define THREADTYPE DWORD WINAPI
#endif

/*------------------------------------------------------------------*/

TFolder *ReadFolderPointer(TSocket * fSocket)
{
   //read pointer to current folder
   TMessage *m = 0;
   fSocket->Recv(m);
   POINTER_T p;
   *m >> p;
   return (TFolder *) p;
}

/*------------------------------------------------------------------*/

THREADTYPE root_server_thread(void *arg)
/*
  Serve histograms over TCP/IP socket link
*/
{
   char request[256];

   TSocket *sock = (TSocket *) arg;

   do {

      /* close connection if client has disconnected */
      if (sock->Recv(request, sizeof(request)) <= 0) {
         // printf("Closed connection to %s\n", sock->GetInetAddress().GetHostName());
         sock->Close();
         delete sock;
         return THREADRETURN;

      } else {

         TMessage *message = new TMessage(kMESS_OBJECT);

         if (strcmp(request, "GetListOfFolders") == 0) {

            TFolder *folder = ReadFolderPointer(sock);
            if (folder == NULL) {
               message->Reset(kMESS_OBJECT);
               message->WriteObject(NULL);
               sock->Send(*message);
               delete message;
               continue;
            }
            //get folder names
            TObject *obj;
            TObjArray *names = new TObjArray(100);

            TCollection *folders = folder->GetListOfFolders();
            TIterator *iterFolders = folders->MakeIterator();
            while ((obj = iterFolders->Next()) != NULL)
               names->Add(new TObjString(obj->GetName()));

            //write folder names
            message->Reset(kMESS_OBJECT);
            message->WriteObject(names);
            sock->Send(*message);

            for (int i = 0; i < names->GetLast() + 1; i++)
               delete(TObjString *) names->At(i);

            delete names;

            delete message;

         } else if (strncmp(request, "FindObject", 10) == 0) {

            TFolder *folder = ReadFolderPointer(sock);

            //get object
            TObject *obj;
            if (strncmp(request + 10, "Any", 3) == 0)
               obj = folder->FindObjectAny(request + 14);
            else
               obj = folder->FindObject(request + 11);

            //write object
            if (!obj)
               sock->Send("Error");
            else {
               message->Reset(kMESS_OBJECT);
               message->WriteObject(obj);
               sock->Send(*message);
            }
            delete message;

         } else if (strncmp(request, "FindFullPathName", 16) == 0) {

            TFolder *folder = ReadFolderPointer(sock);

            //find path
            const char *path = folder->FindFullPathName(request + 17);

            //write path
            if (!path) {
               sock->Send("Error");
            } else {
               TObjString *obj = new TObjString(path);
               message->Reset(kMESS_OBJECT);
               message->WriteObject(obj);
               sock->Send(*message);
               delete obj;
            }
            delete message;

         } else if (strncmp(request, "Occurence", 9) == 0) {

            TFolder *folder = ReadFolderPointer(sock);

            //read object
            TMessage *m = 0;
            sock->Recv(m);
            TObject *obj = ((TObject *) m->ReadObject(m->GetClass()));

            //get occurence
            Int_t retValue = folder->Occurence(obj);

            //write occurence
            message->Reset(kMESS_OBJECT);
            *message << retValue;
            sock->Send(*message);

            delete message;

         } else if (strncmp(request, "GetPointer", 10) == 0) {

            //find object
            TObject *obj = gROOT->FindObjectAny(request + 11);

            //write pointer
            message->Reset(kMESS_ANY);
            POINTER_T p = (POINTER_T) obj;
            *message << p;
            sock->Send(*message);

            delete message;

         } else if (strncmp(request, "Command", 7) == 0) {
            char objName[100], method[100];
            sock->Recv(objName, sizeof(objName));
            sock->Recv(method, sizeof(method));
            TObject *object = gROOT->FindObjectAny(objName);
            if (object && object->InheritsFrom(TH1::Class())
                && strcmp(method, "Reset") == 0)
               static_cast < TH1 * >(object)->Reset();

         } else if (strncmp(request, "SetCut", 6) == 0) {

            //read new settings for a cut
            char name[256];
            sock->Recv(name, sizeof(name));
            TCutG *cut = (TCutG *) gManaHistosFolder->FindObjectAny(name);

            TMessage *m = 0;
            sock->Recv(m);
            TCutG *newc = ((TCutG *) m->ReadObject(m->GetClass()));

            if (cut) {
               cm_msg(MINFO, "root server thread", "changing cut %s", newc->GetName());
               newc->TAttMarker::Copy(*cut);
               newc->TAttFill::Copy(*cut);
               newc->TAttLine::Copy(*cut);
               newc->TNamed::Copy(*cut);
               cut->Set(newc->GetN());
               for (int i = 0; i < cut->GetN(); ++i) {
                  cut->SetPoint(i, newc->GetX()[i], newc->GetY()[i]);
               }
            } else {
               cm_msg(MERROR, "root server thread", "ignoring receipt of unknown cut %s",
                      newc->GetName());
            }
            delete newc;

         } else
            printf("SocketServer: Received unknown command \"%s\"\n", request);
      }
   } while (1);

   return THREADRETURN;
}

/*------------------------------------------------------------------*/

THREADTYPE root_socket_server(void *arg)
{
// Server loop listening for incoming network connections on specified port.
// Starts a searver_thread for each connection.
   int port;

   port = *(int *) arg;

   printf("Root server listening on port %d...\n", port);
   TServerSocket *lsock = new TServerSocket(port, kTRUE);

   do {
      TSocket *sock = lsock->Accept();

      // printf("Established connection to %s\n", sock->GetInetAddress().GetHostName());

#if defined ( __linux__ )
      TThread *thread = new TThread("Server", root_server_thread, sock);
      thread->Run();
#endif
#if defined( _MSC_VER )
      LPDWORD lpThreadId = 0;
      CloseHandle(CreateThread(NULL, 1024, &root_server_thread, sock, 0, lpThreadId));
#endif
   } while (1);

   return THREADRETURN;
}

/*------------------------------------------------------------------*/

void start_root_socket_server(int port)
{
   static int pport = port;
#if defined ( __linux__ )
   TThread *thread = new TThread("server_loop", root_socket_server, &pport);
   thread->Run();
#endif
#if defined( _MSC_VER )
   LPDWORD lpThreadId = 0;
   CloseHandle(CreateThread(NULL, 1024, &root_socket_server, &pport, 0, lpThreadId));
#endif
}

/*------------------------------------------------------------------*/

void *root_event_loop(void *arg)
/*
  Thread wrapper around main event loop
*/
{
   if (clp.online)
      loop_online();
   else
      loop_runs_offline();

   gSystem->ExitLoop();

   return NULL;
}

#endif                          /* USE_ROOT */

/*------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
   INT status, size;
   char str[256];
   HNDLE hkey;

#ifdef HAVE_PVM
   int i;

   str[0] = 0;
   for (i = 0; i < argc; i++) {
      strcat(str, argv[i]);
      strcat(str, " ");
   }
   PVM_DEBUG("Analyzer started: %s", str);
#endif

#ifdef USE_ROOT
   int argn = 1;
   char *argp = (char *) argv[0];

   manaApp = new TRint("ranalyzer", &argn, &argp, NULL, 0, true);

   /* default server port */
   clp.root_port = 9090;
#endif

   /* get default from environment */
   cm_get_environment(clp.host_name, sizeof(clp.host_name), clp.exp_name,
                      sizeof(clp.exp_name));

#ifdef HAVE_HBOOK
   /* set default lrec size */
   clp.lrec = HBOOK_LREC;
#endif                          /* HAVE_HBOOK */

   /* read in command line parameters into clp structure */
   status = getparam(argc, argv);
   if (status != CM_SUCCESS)
      return 1;

   /* become a daemon */
   if (clp.daemon) {
      printf("Becoming a daemon...\n");
      clp.quiet = TRUE;
      ss_daemon_init(FALSE);
   }

   /* set default buffer size */
   if (clp.pvm_buf_size == 0)
      clp.pvm_buf_size = 512 * 1024;
   else
      clp.pvm_buf_size *= 1024;
   if (clp.pvm_buf_size > PVM_BUFFER_SIZE) {
      printf("Buffer size cannot be larger than %dkB\n", PVM_BUFFER_SIZE / 1024);
      return 1;
   }

   /* set online mode if no input filename is given */
   clp.online = (clp.input_file_name[0][0] == 0);

#ifdef HAVE_HBOOK
   /* set Ntuple format to RWNT if online */
   if (clp.online || equal_ustring(clp.output_file_name, "OFLN"))
      clp.rwnt = TRUE;
#endif                          /* HAVE_HBOOK */

#ifdef HAVE_PVM
   status = pvm_main(argv);
   if (status != CM_SUCCESS)
      return 1;
#endif

#ifdef USE_ROOT
   /* workaround for multi-threading with midas system calls */
   ss_force_single_thread();
#endif

   /* now connect to server */
   if (clp.online) {
      if (clp.host_name[0])
         printf("Connect to experiment %s on host %s...", clp.exp_name, clp.host_name);
      else
         printf("Connect to experiment %s...", clp.exp_name);
   }

   status =
       cm_connect_experiment1(clp.host_name, clp.exp_name, analyzer_name, NULL, odb_size,
                              DEFAULT_WATCHDOG_TIMEOUT);

   if (status == CM_UNDEF_EXP) {
      printf("\nError: Experiment \"%s\" not defined.\n", clp.exp_name);
      if (getenv("MIDAS_DIR")) {
         printf
             ("Note that \"MIDAS_DIR\" is defined, which results in a single experiment\n");
         printf
             ("called \"Default\". If you want to use the \"exptab\" file, undefine \"MIDAS_DIR\".\n");
      }
      return 1;
   } else if (status != CM_SUCCESS) {
      cm_get_error(status, str);
      printf("\nError: %s\n", str);
      return 1;
   }

   if (clp.online)
      printf("OK\n");

   /* set online/offline mode */
   cm_get_experiment_database(&hDB, NULL);
   db_set_value(hDB, 0, "/Runinfo/Online Mode", &clp.online, sizeof(clp.online), 1,
                TID_INT);

   if (clp.online) {
      /* check for duplicate name */
      status = cm_exist(analyzer_name, FALSE);
      if (status == CM_SUCCESS) {
         cm_disconnect_experiment();
         printf("An analyzer named \"%s\" is already running in this experiment.\n",
                analyzer_name);
         printf
             ("Please select another analyzer name in analyzer.c or stop other analyzer.\n");
         return 1;
      }

      /* register transitions if started online */
      if (cm_register_transition(TR_START, tr_start, 300) != CM_SUCCESS ||
          cm_register_transition(TR_STOP, tr_stop, 700) != CM_SUCCESS ||
          cm_register_transition(TR_PAUSE, tr_pause, 700) != CM_SUCCESS ||
          cm_register_transition(TR_RESUME, tr_resume, 300) != CM_SUCCESS) {
         printf("Failed to start local RPC server");
         return 1;
      }
   } else {
      if (!pvm_slave) {         /* slave could run on same machine... */
         status = cm_exist(analyzer_name, FALSE);
         if (status == CM_SUCCESS) {
            /* kill hanging previous analyzer */
            cm_cleanup(analyzer_name, FALSE);

            status = cm_exist(analyzer_name, FALSE);
            if (status == CM_SUCCESS) {
               /* analyzer may only run once if offline */
               status = cm_shutdown(analyzer_name, FALSE);
               if (status == CM_SHUTDOWN)
                  printf("Previous analyzer stopped\n");
            }
         }
      }
   }

#ifdef HAVE_HBOOK
   /* register callback for clearing histos */
   cm_register_function(RPC_ANA_CLEAR_HISTOS, ana_callback);
#endif

   /* turn on keepalive messages */
   cm_set_watchdog_params(TRUE, DEFAULT_RPC_TIMEOUT);

   /* decrease watchdog timeout in offline mode */
   if (!clp.online)
      cm_set_watchdog_params(TRUE, 2000);

   /* turn off watchdog if in debug mode */
   if (clp.debug)
      cm_set_watchdog_params(0, 0);

   /* initialize module parameters */
   if (init_module_parameters(FALSE) != CM_SUCCESS) {
      cm_disconnect_experiment();
      return 1;
   }

   /* create ODB structure for output */
   sprintf(str, "/%s/Output", analyzer_name);
   db_check_record(hDB, 0, str, ANA_OUTPUT_INFO_STR, TRUE);
   db_find_key(hDB, 0, str, &hkey);
   assert(hkey);
   size = sizeof(out_info);
   db_get_record(hDB, hkey, &out_info, &size, 0);

#ifdef USE_ROOT
   /* create the folder for analyzer histograms */
   gManaHistosFolder =
       gROOT->GetRootFolder()->AddFolder("histos", "MIDAS Analyzer Histograms");
   gHistoFolderStack = new TObjArray();
   gROOT->GetListOfBrowsables()->Add(gManaHistosFolder, "histos");

   /* convert .rz names to .root names */
   if (strstr(out_info.last_histo_filename, ".rz"))
      strcpy(out_info.last_histo_filename, "last.root");

   if (strstr(out_info.histo_dump_filename, ".rz"))
      strcpy(out_info.histo_dump_filename, "his%05d.root");

   db_set_record(hDB, hkey, &out_info, sizeof(out_info), 0);

   /* start socket server */
   if (clp.root_port)
      start_root_socket_server(clp.root_port);

#endif                          /* USE_ROOT */

#ifdef HAVE_HBOOK
   /* convert .root names to .rz names */
   if (strstr(out_info.last_histo_filename, ".root"))
      strcpy(out_info.last_histo_filename, "last.rz");

   if (strstr(out_info.histo_dump_filename, ".root"))
      strcpy(out_info.histo_dump_filename, "his%05d.rz");

   db_set_record(hDB, hkey, &out_info, sizeof(out_info), 0);
#endif

#ifdef HAVE_HBOOK
   /* create global memory */
   if (clp.online) {
      HLIMAP(pawc_size / 4, out_info.global_memory_name);
      printf("\nGLOBAL MEMORY NAME = %s\n", out_info.global_memory_name);
   } else {
      if (equal_ustring(clp.output_file_name, "OFLN")) {
         strcpy(str, "OFLN");
         HLIMAP(pawc_size / 4, str);
         printf("\nGLOBAL MEMORY NAME = %s\n", "OFLN");
      } else
         HLIMIT(pawc_size / 4);
   }
#endif                          /* HAVE_HBOOK */

   /* analyzer init function */
   if (mana_init() != CM_SUCCESS) {
      cm_disconnect_experiment();
      return 1;
   }

   /* load histos from last.xxx */
   if (clp.online)
      load_last_histos();

   /* reqister event requests */
   register_requests();

   /* initialize ss_getchar */
   if (!clp.quiet && !pvm_slave)
      ss_getchar(0);

   /*---- start main loop ----*/

   if (clp.online)
      loop_online();
   else
      loop_runs_offline();

   /* reset terminal */
   if (!clp.quiet && !pvm_slave)
      ss_getchar(TRUE);

   /* call exit function */
   mana_exit();

   /* save histos to last.xxx */
   if (clp.online)
      save_last_histos();

#ifdef HAVE_PVM

   PVM_DEBUG("Analyzer stopped");

   /* exit PVM */
   pvm_exit();

   /* if PVM slave, don't write *SHM file back */
   if (pvm_slave)
      disable_shm_write = TRUE;

#endif

   /* disconnect from experiment */
   cm_disconnect_experiment();

#ifdef USE_ROOT
   if (clp.start_rint)
      manaApp->Run(true);
   printf("\r               \n");       /* overwrite superflous ROOT prompt */
#endif

   return 0;
}

#ifdef LINK_TEST
int   odb_size;
char* analyzer_name;
int   analyzer_loop_period;
ANALYZE_REQUEST analyze_request[1];
int analyzer_init(void) { return 0; }
int analyzer_loop(void) { return 0; }
int analyzer_exit(void) { return 0; }
int ana_end_of_run(INT run_number, char *error) { return 0; }
int ana_begin_of_run(INT run_number, char *error) { return 0; }
int ana_resume_run(INT run_number, char *error) { return 0; }
int ana_pause_run(INT run_number, char *error) { return 0; }
#endif
