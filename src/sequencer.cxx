/********************************************************************\

  Name:         sequencer.cxx
  Created by:   Stefan Ritt

  Contents:     MIDAS sequencer functions

  $Id: alarm.c 5212 2011-10-10 15:55:23Z ritt $

\********************************************************************/

#include "midas.h"
#include "msystem.h"
#include "strlcpy.h"
#include "mxml.h"
#include <assert.h>

/* functions defined in mhttpd.cxx */
extern BOOL isparam(const char *param);
extern const char *getparam(const char *param);
extern void redirect(const char *path);
extern void show_start_page(int script);
extern void rsprintf(const char *format, ...);
extern int mhttpd_revision();
extern void strencode(char *text);
extern void strencode4(char *text);
extern void show_header(HNDLE hDB, const char *title, const char *method, const char *path, int colspan,
                        int refresh);

#undef NAME_LENGTH
#define NAME_LENGTH 256

/**dox***************************************************************/
/** @file sequencer.cxx
The Midas Sequencer file
*/

/** @defgroup alfunctioncode Midas Sequencer Functions
 */

/**dox***************************************************************/
/** @addtogroup seqfunctioncode
 *
 *  @{  */

/*------------------------------------------------------------------*/

typedef struct {
   char  path[256];
   char  filename[256];
   char  error[256];
   int   error_line;
   int   serror_line;
   char  message[256];
   BOOL  message_wait;
   BOOL  running;
   BOOL  finished;
   BOOL  paused;
   int   current_line_number;
   int   scurrent_line_number;
   BOOL  stop_after_run;
   BOOL  transition_request;
   int   loop_start_line[4];
   int   sloop_start_line[4];
   int   loop_end_line[4];
   int   sloop_end_line[4];
   int   loop_counter[4];
   int   loop_n[4];
   char  subdir[256];
   int   subdir_end_line;
   int   subdir_not_notify;
   int   stack_index;
   int   subroutine_end_line[4];
   int   subroutine_return_line[4];
   int   subroutine_call_line[4];
   int   ssubroutine_call_line[4];
   char  subroutine_param[4][256];
   float wait_value;
   float wait_limit;
   DWORD start_time;
   char  wait_type[32];
   char  last_msg[10];
} SEQUENCER;

#define SEQUENCER_STR(_name) const char *_name[] = {\
"[.]",\
"Path = STRING : [256] ",\
"Filename = STRING : [256] ",\
"Error = STRING : [256] ",\
"Error line = INT : 0",\
"SError line = INT : 0",\
"Message = STRING : [256] ",\
"Message Wait = BOOL : n",\
"Running = BOOL : n",\
"Finished = BOOL : y",\
"Paused = BOOL : n",\
"Current line number = INT : 0",\
"SCurrent line number = INT : 0",\
"Stop after run = BOOL : n",\
"Transition request = BOOL : n",\
"Loop start line = INT[4] :",\
"[0] 0",\
"[1] 0",\
"[2] 0",\
"[3] 0",\
"SLoop start line = INT[4] :",\
"[0] 0",\
"[1] 0",\
"[2] 0",\
"[3] 0",\
"Loop end line = INT[4] :",\
"[0] 0",\
"[1] 0",\
"[2] 0",\
"[3] 0",\
"SLoop end line = INT[4] :",\
"[0] 0",\
"[1] 0",\
"[2] 0",\
"[3] 0",\
"Loop counter = INT[4] :",\
"[0] 0",\
"[1] 0",\
"[2] 0",\
"[3] 0",\
"Loop n = INT[4] :",\
"[0] 0",\
"[1] 0",\
"[2] 0",\
"[3] 0",\
"Subdir = STRING : [256] ",\
"Subdir end line = INT : 0",\
"Subdir not notify = INT : 0",\
"Stack index = INT : 0",\
"Subroutine end line = INT[4] :",\
"[0] 0",\
"[1] 0",\
"[2] 0",\
"[3] 0",\
"Subroutine return line = INT[4] :",\
"[0] 0",\
"[1] 0",\
"[2] 0",\
"[3] 0",\
"Subroutine call line = INT[4] :",\
"[0] 0",\
"[1] 0",\
"[2] 0",\
"[3] 0",\
"SSubroutine call line = INT[4] :",\
"[0] 0",\
"[1] 0",\
"[2] 0",\
"[3] 0",\
"Subroutine param = STRING[4] : ",\
"[256] ",\
"[256] ",\
"[256] ",\
"[256] ",\
"Wait value = FLOAT : 0",\
"Wait limit = FLOAT : 0",\
"Start time = DWORD : 0",\
"Wait type = STRING : [32] ",\
"Last msg = STRING : [10] 00:00:00",\
"",\
NULL }

SEQUENCER_STR(sequencer_str);
SEQUENCER seq;
PMXML_NODE pnseq = NULL;

/*------------------------------------------------------------------*/

void seq_error(const char *str)
{
   HNDLE hDB, hKey;
   
   strlcpy(seq.error, str, sizeof(seq.error));
   seq.error_line = seq.current_line_number;
   seq.serror_line = seq.scurrent_line_number;
   seq.running = FALSE;
   seq.transition_request = FALSE;

   cm_get_experiment_database(&hDB, NULL);
   db_find_key(hDB, 0, "/Sequencer/State", &hKey);
   assert(hKey);
   db_set_record(hDB, hKey, &seq, sizeof(seq), 0);
   
   cm_msg(MTALK, "sequencer", "Sequencer has stopped with error.");
}

/*------------------------------------------------------------------*/

int strbreak(char *str, char list[][NAME_LENGTH], int size, const char *brk, BOOL ignore_quotes)
/* break comma-separated list into char array, stripping leading
 and trailing blanks */
{
   int i, j;
   char *p;
   
   memset(list, 0, size * NAME_LENGTH);
   p = str;
   if (!p || !*p)
      return 0;
   
   while (*p == ' ')
      p++;
   
   for (i = 0; *p && i < size; i++) {
      if (*p == '"' && !ignore_quotes) {
         p++;
         j = 0;
         memset(list[i], 0, NAME_LENGTH);
         do {
            /* convert two '"' to one */
            if (*p == '"' && *(p + 1) == '"') {
               list[i][j++] = '"';
               p += 2;
            } else if (*p == '"') {
               break;
            } else
               list[i][j++] = *p++;
            
         } while (j < NAME_LENGTH - 1);
         list[i][j] = 0;
         
         /* skip second '"' */
         p++;
         
         /* skip blanks and break character */
         while (*p == ' ')
            p++;
         if (*p && strchr(brk, *p))
            p++;
         while (*p == ' ')
            p++;
         
      } else {
         strlcpy(list[i], p, NAME_LENGTH);
         
         for (j = 0; j < (int) strlen(list[i]); j++)
            if (strchr(brk, list[i][j])) {
               list[i][j] = 0;
               break;
            }
         
         p += strlen(list[i]);
         while (*p == ' ')
            p++;
         if (*p && strchr(brk, *p))
            p++;
         while (*p == ' ')
            p++;
         
         while (list[i][strlen(list[i]) - 1] == ' ')
            list[i][strlen(list[i]) - 1] = 0;
      }
      
      if (!*p)
         break;
   }
   
   if (i == size)
      return size;
   
   return i + 1;
}

/*------------------------------------------------------------------*/

extern char *stristr(const char *str, const char *pattern);

void strsubst(char *string, int size, const char *pattern, const char *subst)
/* subsitute "pattern" with "subst" in "string" */
{
   char *tail, *p;
   int s;
   
   p = string;
   for (p = stristr(p, pattern); p != NULL; p = stristr(p, pattern)) {
      
      if (strlen(pattern) == strlen(subst)) {
         memcpy(p, subst, strlen(subst));
      } else if (strlen(pattern) > strlen(subst)) {
         memcpy(p, subst, strlen(subst));
         memmove(p + strlen(subst), p + strlen(pattern), strlen(p + strlen(pattern)) + 1);
      } else {
         tail = (char *) malloc(strlen(p) - strlen(pattern) + 1);
         strcpy(tail, p + strlen(pattern));
         s = size - (p - string);
         strlcpy(p, subst, s);
         strlcat(p, tail, s);
         free(tail);
         tail = NULL;
      }
      
      p += strlen(subst);
   }
}

/*------------------------------------------------------------------*/

BOOL msl_parse(char *filename, char *error, int error_size, int *error_line)
{
   char str[256], *buf, *pl, *pe;
   char list[100][NAME_LENGTH], list2[100][NAME_LENGTH], **lines;
   int i, j, n, size, n_lines, endl, line, fhin, nest, incl, library;
   FILE *fout = NULL;
   
   fhin = open(filename, O_RDONLY | O_TEXT);
   if (strchr(filename, '.')) {
      strlcpy(str, filename, sizeof(str));
      *strchr(str, '.') = 0;
      strlcat(str, ".xml", sizeof(str));
      fout = fopen(str, "wt");
   }
   if (fhin > 0 && fout) {
      size = (int)lseek(fhin, 0, SEEK_END);
      lseek(fhin, 0, SEEK_SET);
      buf = (char *)malloc(size+1);
      size = (int)read(fhin, buf, size);
      buf[size] = 0;
      close(fhin);

      /* look for any includes */
      lines = (char **)malloc(sizeof(char *));
      incl = 0;
      pl = buf;
      library = FALSE;
      for (n_lines=0 ; *pl ; n_lines++) {
         lines = (char **)realloc(lines, sizeof(char *)*n_lines+1);
         lines[n_lines] = pl;
         if (strchr(pl, '\n')) {
            pe = strchr(pl, '\n');
            *pe = 0;
            if (*(pe-1) == '\r') {
               *(pe-1) = 0;
            }
            pe++;
         } else
            pe = pl+strlen(pl);
         strlcpy(str, pl, sizeof(str));
         pl = pe;
         strbreak(str, list, 100, ", ", FALSE);
         if (equal_ustring(list[0], "include")) {
            if (!incl) {
               fprintf(fout, "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n");
               fprintf(fout, "<!DOCTYPE RunSequence [\n");
               incl = 1;
            }
            fprintf(fout, "  <!ENTITY %s SYSTEM \"%s.xml\">\n", list[1], list[1]); 
         }
         if (equal_ustring(list[0], "library")) {
            fprintf(fout, "<Library name=\"%s\">\n", list[1]);
            library = TRUE;
         }
      }
      if (incl)
         fprintf(fout, "]>\n");
      else if (!library)
         fprintf(fout, "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n");
            
      /* parse rest of file */
      if (!library)
         fprintf(fout, "<RunSequence xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"\">\n");
      for (line=0 ; line<n_lines ; line++) {
         n = strbreak(lines[line], list, 100, ", ", FALSE);
         
         /* remove any comment */
         for (i=0 ; i<n ; i++) {
            if (list[i][0] == '#') {
               for (j=i ; j<n ; j++)
                  list[j][0] = 0;
               break;
            }
         }
         
         if (equal_ustring(list[0], "library")) {
                     
         } else if (equal_ustring(list[0], "include")) {
               fprintf(fout, "&%s;\n", list[1]); 
            
         } else if (equal_ustring(list[0], "call")) {
            fprintf(fout, "<Call l=\"%d\" name=\"%s\">", line+1, list[1]);
            for (i=2 ; i < 100 && list[i][0] ; i++) {
               if (i > 2)
                  fprintf(fout, ",");
               fprintf(fout, "%s", list[i]);
            }
            fprintf(fout, "</Call>\n");
            
         } else if (equal_ustring(list[0], "cat")) {
            fprintf(fout, "<Cat l=\"%d\" name=\"%s\">", line+1, list[1]);
            for (i=2 ; i < 100 && list[i][0] ; i++) {
               if (i > 2)
                  fprintf(fout, ",");
               fprintf(fout, "\"%s\"", list[i]);
            }
            fprintf(fout, "</Cat>\n");

         } else if (equal_ustring(list[0], "comment")) {
            fprintf(fout, "<Comment l=\"%d\">%s</Comment>\n", line+1, list[1]);
            
         } else if (equal_ustring(list[0], "goto")) {
            fprintf(fout, "<Goto l=\"%d\" sline=\"%s\" />\n", line+1, list[1]);
            
         } else if (equal_ustring(list[0], "if")) {
            fprintf(fout, "<If l=\"%d\" condition=\"%s\">\n", line+1, list[1]);
         } else if (equal_ustring(list[0], "endif")) {
            fprintf(fout, "</If>\n");
            
         } else if (equal_ustring(list[0], "loop")) {
            /* find end of loop */
            for (i=line,nest=0 ; i<n_lines ; i++) {
               strbreak(lines[i], list2, 100, ", ", FALSE);
               if (equal_ustring(list2[0], "loop"))
                  nest++;
               if (equal_ustring(list2[0], "endloop")) {
                  nest--;
                  if (nest == 0)
                     break;
               }
            }
            if (i<n_lines)
               endl = i+1;
            else
               endl = line+1;
            if (list[2][0] == 0)
               fprintf(fout, "<Loop l=\"%d\" le=\"%d\" n=\"%s\">\n", line+1, endl, list[1]);
            else {
               fprintf(fout, "<Loop l=\"%d\" le=\"%d\" var=\"%s\" values=\"", line+1, endl, list[1]);
               for (i=2 ; i < 100 && list[i][0] ; i++) {
                  if (i > 2)
                     fprintf(fout, ",");
                  fprintf(fout, "%s", list[i]);
               }
               fprintf(fout, "\">\n");
            }
         } else if (equal_ustring(list[0], "endloop")) {
            fprintf(fout, "</Loop>\n");
            
         } else if (equal_ustring(list[0], "message")) {
            fprintf(fout, "<Message l=\"%d\"%s>%s</Message>\n", line+1, 
                    list[2][0] == '1'? " wait=\"1\"" : "", list[1]);

         } else if (equal_ustring(list[0], "odbinc")) {
            fprintf(fout, "<ODBInc l=\"%d\" path=\"%s\">%s</ODBInc>\n", line+1, list[1], list[2]);
            
         } else if (equal_ustring(list[0], "odbset")) {
            if (list[3][0])
               fprintf(fout, "<ODBSet l=\"%d\" notify=\"%s\" path=\"%s\">%s</ODBSet>\n", line+1, list[3], list[1], list[2]);
            else
               fprintf(fout, "<ODBSet l=\"%d\" path=\"%s\">%s</ODBSet>\n", line+1, list[1], list[2]);
            
         } else if (equal_ustring(list[0], "odbsubdir")) {
            if (list[2][0])
               fprintf(fout, "<ODBSubdir l=\"%d\" notify=\"%s\" path=\"%s\">\n", line+1, list[2], list[1]);
            else
               fprintf(fout, "<ODBSubdir l=\"%d\" path=\"%s\">\n", line+1, list[1]);
         } else if (equal_ustring(list[0], "endodbsubdir")) {
            fprintf(fout, "</ODBSubdir>\n");
            
         } else if (equal_ustring(list[0], "param")) {
            if (list[2][0] == 0)
               fprintf(fout, "<Param l=\"%d\" name=\"%s\" />\n", line+1, list[1]);
            else if (!list[3][0] && equal_ustring(list[2], "bool")) {
               fprintf(fout, "<Param l=\"%d\" name=\"%s\" type=\"bool\" />\n", line+1, list[1]);
            } else if (!list[3][0]) {
               fprintf(fout, "<Param l=\"%d\" name=\"%s\" comment=\"%s\" />\n", line+1, list[1], list[2]);
            } else {
               fprintf(fout, "<Param l=\"%d\" name=\"%s\" comment=\"%s\" options=\"", line+1, list[1], list[2]);
               for (i=3 ; i < 100 && list[i][0] ; i++) {
                  if (i > 3)
                     fprintf(fout, ",");
                  fprintf(fout, "%s", list[i]);
               }
               fprintf(fout, "\" />\n");
            }

         } else if (equal_ustring(list[0], "rundescription")) {
            fprintf(fout, "<RunDescription l=\"%d\">%s</RunDescription>\n", line+1, list[1]);
            
         } else if (equal_ustring(list[0], "script")) {
            if (list[2][0] == 0)
               fprintf(fout, "<Script l=\"%d\">%s</Script>\n", line+1, list[1]);
            else {
               fprintf(fout, "<Script l=\"%d\" params=\"", line+1);
               for (i=1 ; i < 100 && list[i][0] ; i++) {
                  if (i > 1)
                     fprintf(fout, ",");
                  fprintf(fout, "%s", list[i]);
               }
               fprintf(fout, "\">%s</Script>\n", list[1]);
            }
            
         } else if (equal_ustring(list[0], "set")) {
            fprintf(fout, "<Set l=\"%d\" name=\"%s\">%s</Set>\n", line+1, list[1], list[2]);

         } else if (equal_ustring(list[0], "subroutine")) {
            fprintf(fout, "\n<Subroutine l=\"%d\" name=\"%s\">\n", line+1, list[1]);
         } else if (equal_ustring(list[0], "endsubroutine")) {
            fprintf(fout, "</Subroutine>\n");

         } else if (equal_ustring(list[0], "transition")) {
            fprintf(fout, "<Transition l=\"%d\">%s</Transition>\n", line+1, list[1]);
            
         } else if (equal_ustring(list[0], "wait")) {
            if (!list[2][0])
               fprintf(fout, "<Wait l=\"%d\" for=\"seconds\">%s</Wait>\n", line+1, list[1]);
            else if (!list[3][0])
               fprintf(fout, "<Wait l=\"%d\" for=\"%s\">%s</Wait>\n", line+1, list[1], list[2]);
            else {
               fprintf(fout, "<Wait l=\"%d\" for=\"%s\" path=\"%s\" op=\"%s\">%s</Wait>\n", 
                       line+1, list[1], list[2], list[3], list[4]);
            }
         
         } else if (list[0][0] == 0 || list[0][0] == '#'){
            /* skip empty or outcommented lines */
         } else {
            sprintf(error, "Invalid command \"%s\"", list[0]);
            *error_line = line + 1;
            return FALSE;
         }
      }

      free(lines);
      free(buf);
      if (library)
         fprintf(fout, "\n</Library>\n");
      else
         fprintf(fout, "</RunSequence>\n");
      fclose(fout);
   } else {
      sprintf(error, "File error on \"%s\"", filename);
      return FALSE;
   }

   return TRUE;
}

/*------------------------------------------------------------------*/

BOOL eval_var(const char *value, char *result, int size)
{
   HNDLE hDB, hkey;
   KEY key;
   const char *p;
   char *s;
   char str[256], data[256];
   int index, i;
   
   cm_get_experiment_database(&hDB, NULL);
   p = value;
   
   while (*p == ' ')
      p++;
   
   if (*p == '$') {
      p++;
      strcpy(str, p);
      if (strchr(str, '('))
         *(strchr(str, '(')+1) = 0;
      if (atoi(p) > 0) {
         index = atoi(p);
         if (seq.stack_index > 0) {
            strlcpy(str, seq.subroutine_param[seq.stack_index-1], sizeof(str));
            s = strtok(str, ",");
            if (s == NULL) {
               if (index == 1) {
                  strlcpy(result, str, size);
                  if (result[0] == '$') {
                     strlcpy(str, result, sizeof(str));
                     return eval_var(str, result, size);
                  }                  
                  return TRUE;
               } else
                  goto error;
            }
            for (i=1; s != NULL && i<index; i++)
               s = strtok(NULL, ",");
            if (s != NULL) {
               strlcpy(result, s, size);
               if (result[0] == '$') {
                  strlcpy(str, result, sizeof(str));
                  return eval_var(str, result, size);
               }                  
               return TRUE;
            }
            else 
               goto error;
         } else
            goto error;
      } else if (equal_ustring(str, "ODB(")) {
         strlcpy(str, p+4, sizeof(str));
         if (strchr(str, ')') == NULL)
            return FALSE;
         *strchr(str, ')') = 0;
         if (str[0] == '"' && str[strlen(str)-1] == '"') {
            memmove(str, str+1, strlen(str));
            str[strlen(str)-1] = 0;
         }
         index = 0;
         if (strchr(str, '[')) {
            index = atoi(strchr(str, '[')+1);
            *strchr(str, '[') = 0;
         }
         if (!db_find_key(hDB, 0, str, &hkey))
            return FALSE;
         db_get_key(hDB, hkey, &key);
         i = sizeof(data);
         db_get_data(hDB, hkey, data, &i, key.type);
         db_sprintf(result, data, sizeof(data), index, key.type);
         return TRUE;
         
      } else {
         sprintf(str, "/Sequencer/Variables/%s", value+1);
         if (db_get_value(hDB, 0, str, result, &size, TID_STRING, FALSE) == DB_SUCCESS)
            return TRUE;
      }
   } else {
      strlcpy(result, value, size);
      return TRUE;
   }

error:
   sprintf(str, "Invalid parameter \"%s\"", value);
   seq_error(str);
   return FALSE;
}

/*------------------------------------------------------------------*/

int concatenate(char *result, int size, char *value)
{
   char str[256], list[100][NAME_LENGTH];
   int  i, n;
   
   n = strbreak(value, list, 100, ",", FALSE);
   
   result[0] = 0;
   for (i=0 ; i<n ; i++) {
      if (!eval_var(list[i], str, sizeof(str)))
          return FALSE;
      strlcat(result, str, size);
   }
   
   return TRUE;   
}

/*------------------------------------------------------------------*/

int eval_condition(const char *condition)
{
   int i;
   double value1, value2;
   char value1_str[256], value2_str[256], value1_var[256], value2_var[256], str[256], op[3];
   
   strcpy(str, condition);
   op[1] = op[2] = 0;
   value1 = value2 = 0;
   
   /* find value and operator */
   for (i = 0; i < (int)strlen(str) ; i++)
      if (strchr("<>=!&", str[i]) != NULL)
         break;
   strlcpy(value1_str, str, i+1);
   while (value1_str[strlen(value1_str)-1] == ' ')
      value1_str[strlen(value1_str)-1] = 0;
   op[0] = str[i];
   if (strchr("<>=!&", str[i+1]) != NULL)
      op[1] = str[++i];

   for (i++; str[i] == ' '; i++);
   strlcpy(value2_str, str + i, sizeof(value2_str));
   
   if (!eval_var(value1_str, value1_var, sizeof(value1_var)))
      return -1;
   if (!eval_var(value2_str, value2_var, sizeof(value2_var)))
      return -1;
   for (i=0 ; i<(int)strlen(value1_var) ; i++)
      if (isdigit(value1_var[i]))
         break;
   if (i == (int)strlen(value1_var))
      return -1;
   for (i=0 ; i<(int)strlen(value2_var) ; i++)
      if (isdigit(value2_var[i]))
         break;
   if (i == (int)strlen(value2_var))
      return -1;

   value1 = atof(value1_var);
   value2 = atof(value2_var);
   
   /* now do logical operation */
   if (strcmp(op, "=") == 0)
      if (value1 == value2) return 1;
   if (strcmp(op, "==") == 0)
      if (value1 == value2) return 1;
   if (strcmp(op, "!=") == 0)
      if (value1 != value2) return 1;
   if (strcmp(op, "<") == 0)
      if (value1 < value2) return 1;
   if (strcmp(op, ">") == 0)
      if (value1 > value2) return 1;
   if (strcmp(op, "<=") == 0)
      if (value1 <= value2) return 1;
   if (strcmp(op, ">=") == 0)
      if (value1 >= value2) return 1;
   if (strcmp(op, "&") == 0)
      if (((unsigned int)value1 & (unsigned int)value2) > 0) return 1;
   
   return 0;
}

/*------------------------------------------------------------------*/

void init_sequencer()
{
   int status, size;
   HNDLE hDB;
   HNDLE hKey;
   char str[256];
   
   cm_get_experiment_database(&hDB, NULL);
   status = db_check_record(hDB, 0, "/Sequencer/State", strcomb(sequencer_str), TRUE);
   if (status == DB_STRUCT_MISMATCH) {
      cm_msg(MERROR, "init_sequencer", "Aborting on mismatching /Sequencer/State structure");
      cm_disconnect_experiment();
      abort();
   }
   db_find_key(hDB, 0, "/Sequencer/State", &hKey);
   size = sizeof(seq);
   status = db_open_record(hDB, hKey, &seq, sizeof(seq), MODE_READ, NULL, NULL);
   assert(status == DB_SUCCESS);
   
   if (seq.path[0] == 0)
      getcwd(seq.path, sizeof(seq.path)); 
   if (strlen(seq.path)>0 && seq.path[strlen(seq.path)-1] != DIR_SEPARATOR) {
      strlcat(seq.path, DIR_SEPARATOR_STR, sizeof(seq.path));
   }
   
   if (seq.filename[0]) {
      strlcpy(str, seq.path, sizeof(str));
      strlcat(str, seq.filename, sizeof(str));
      seq.error[0] = 0;
      seq.error_line = 0;
      seq.serror_line = 0;
      if (pnseq) {
         mxml_free_tree(pnseq);
         pnseq = NULL;
      }
      if (stristr(str, ".msl")) {
         if (msl_parse(str, seq.error, sizeof(seq.error), &seq.serror_line)) {
            strsubst(str, sizeof(str), ".msl", ".xml");
            pnseq = mxml_parse_file(str, seq.error, sizeof(seq.error), &seq.error_line);
         }
      } else
         pnseq = mxml_parse_file(str, seq.error, sizeof(seq.error), &seq.error_line);
   }
   
   seq.transition_request = FALSE;
   
   db_set_record(hDB, hKey, &seq, sizeof(seq), 0);
}

/*------------------------------------------------------------------*/

void seq_start_page()
{
   int line, i, n, no, size, last_line, status, maxlength;
   HNDLE hDB, hkey, hsubkey, hkeycomm, hkeyc;
   KEY key;
   char data[1000], str[256], name[32];
   char data_str[256], comment[1000], list[100][NAME_LENGTH];
   MXML_NODE *pn;
   
   cm_get_experiment_database(&hDB, NULL);
   
   show_header(hDB, "Start sequence", "GET", "", 1, 0);
   rsprintf("<tr><th bgcolor=#A0A0FF colspan=2>Start script</th>\n");
  
   if (!pnseq) {
      rsprintf("<tr><td colspan=2 align=\"center\" bgcolor=\"red\"><b>Error in XML script</b></td></tr>\n");
      rsprintf("</table>\n");
      rsprintf("</body></html>\r\n");
      return;
   }
   
   /* run parameters from ODB */
   db_find_key(hDB, 0, "/Experiment/Edit on sequence", &hkey);
   db_find_key(hDB, 0, "/Experiment/Parameter Comments", &hkeycomm);
   n = 0;
   if (hkey) {
      for (line = 0 ;; line++) {
         db_enum_link(hDB, hkey, line, &hsubkey);
         
         if (!hsubkey)
            break;
         
         db_get_link(hDB, hsubkey, &key);
         strlcpy(str, key.name, sizeof(str));
         
         if (equal_ustring(str, "Edit run number"))
            continue;
         
         db_enum_key(hDB, hkey, line, &hsubkey);
         db_get_key(hDB, hsubkey, &key);
         
         size = sizeof(data);
         status = db_get_data(hDB, hsubkey, data, &size, key.type);
         if (status != DB_SUCCESS)
            continue;
         
         for (i = 0; i < key.num_values; i++) {
            if (key.num_values > 1)
               rsprintf("<tr><td>%s [%d]", str, i);
            else
               rsprintf("<tr><td>%s", str);
            
            if (i == 0 && hkeycomm) {
               /* look for comment */
               if (db_find_key(hDB, hkeycomm, key.name, &hkeyc) == DB_SUCCESS) {
                  size = sizeof(comment);
                  if (db_get_data(hDB, hkeyc, comment, &size, TID_STRING) == DB_SUCCESS)
                     rsprintf("<br>%s\n", comment);
               }
            }
            
            db_sprintf(data_str, data, key.item_size, i, key.type);
            
            maxlength = 80;
            if (key.type == TID_STRING)
               maxlength = key.item_size;
            
            if (key.type == TID_BOOL) {
               if (((DWORD*)data)[i])
                  rsprintf("<td><input type=checkbox checked name=x%d value=1></td></tr>\n", n++);
               else
                  rsprintf("<td><input type=checkbox name=x%d value=1></td></tr>\n", n++);
            } else
               rsprintf("<td><input type=text size=%d maxlength=%d name=x%d value=\"%s\"></tr>\n",
                        (maxlength<80)?maxlength:80, maxlength-1, n++, data_str);
         }
      }
   }
   
   /* parameters from script */
   pn = mxml_find_node(pnseq, "RunSequence");
   if (pn) {
      last_line = mxml_get_line_number_end(pn);
      
      for (line=1 ; line<last_line ; line++){
         pn = mxml_get_node_at_line(pnseq, line);
         if (!pn)
            continue;
            
         if (equal_ustring(mxml_get_name(pn), "Param")) {
            strlcpy(name, mxml_get_attribute(pn, "name"), sizeof(name));

            rsprintf("<tr><td>%s", name);
            if (mxml_get_attribute(pn, "comment"))
               rsprintf("<br>%s\n", mxml_get_attribute(pn, "comment"));
                     
            size = sizeof(data_str);
            sprintf(str, "/Sequencer/Variables/%s", name);
            data_str[0] = 0;
            db_get_value(hDB, 0, str, data_str, &size, TID_STRING, FALSE);
            
            if (mxml_get_attribute(pn, "options")) {
               strlcpy(data, mxml_get_attribute(pn, "options"), sizeof(data));
               no = strbreak(mxml_get_attribute(pn, "options"), list, 100, ",", FALSE);
               rsprintf("<td><select name=x%d>\n", n++);
               for (i=0 ; i<no ; i++)
                  rsprintf("<option>%s</option>\n", list[i]);
               rsprintf("</select></td></tr>\n");
            } else if (mxml_get_attribute(pn, "type") && equal_ustring(mxml_get_attribute(pn, "type"), "bool")) {
               if (data_str[0] == '1')
                  rsprintf("<td><input type=checkbox checked name=x%d value=1></tr>\n", n++);
               else
                  rsprintf("<td><input type=checkbox name=x%d value=1></tr>\n", n++);
            } else
               rsprintf("<td><input type=text name=x%d value=\"%s\"></tr>\n", n++, data_str);
         }
         
      }
   }
        
   rsprintf("<tr><td align=center colspan=2>\n");
   rsprintf("<input type=submit name=cmd value=\"Start Script\">\n");
   rsprintf("<input type=hidden name=params value=1>\n");
   rsprintf("<input type=submit name=cmd value=Cancel>\n");
   rsprintf("</tr>\n");
   rsprintf("</table>\n");
   
   if (isparam("redir"))
      rsprintf("<input type=hidden name=\"redir\" value=\"%s\">\n", getparam("redir"));
   
   rsprintf("</body></html>\r\n");
}

/*------------------------------------------------------------------*/

const char *bar_col[] = {"#B0B0FF", "#C0C0FF", "#D0D0FF", "#E0E0FF"};
const char *call_col[] = {"#B0FFB0", "#C0FFC0", "#D0FFD0", "#E0FFE0"};

void show_seq_page()
{
   INT i, size, n,  width, state, eob, last_line, error_line;
   HNDLE hDB;
   char str[256], path[256], dir[256], error[256], comment[256], filename[256], data[256], buffer[10000], line[256], name[32];
   time_t now;
   char *flist = NULL, *pc, *pline, *buf;
   PMXML_NODE pn;
   int fh;
   FILE *f;
   HNDLE hKey, hparam, hsubkey;
   KEY key;
   
   cm_get_experiment_database(&hDB, NULL);
   db_find_key(hDB, 0, "/Sequencer/State", &hKey);
   assert(hKey);
   
   /*---- load XML file ----*/
   if (equal_ustring(getparam("cmd"), "Load")) {
      if (isparam("dir"))
         strlcpy(seq.filename, getparam("dir"), sizeof(seq.filename));
      else
         filename[0] = 0;
      if (isparam("fs"))
         strlcat(seq.filename, getparam("fs"), sizeof(seq.filename));
      
      strlcpy(str, seq.path, sizeof(str));
      strlcat(str, seq.filename, sizeof(str));
      seq.error[0] = 0;
      seq.error_line = 0;
      seq.serror_line = 0;
      if (pnseq) {
         mxml_free_tree(pnseq);
         pnseq = NULL;
      }
      if (stristr(str, ".msl")) {
         if (msl_parse(str, seq.error, sizeof(seq.error), &seq.serror_line)) {
            strsubst(str, sizeof(str), ".msl", ".xml");
            pnseq = mxml_parse_file(str, seq.error, sizeof(seq.error), &seq.error_line);
         }
      } else
         pnseq = mxml_parse_file(str, seq.error, sizeof(seq.error), &seq.error_line);
      
      seq.finished = FALSE;
      db_set_record(hDB, hKey, &seq, sizeof(seq), 0);
      redirect("");
      return;
   }
   
   /*---- start script ----*/
   if (equal_ustring(getparam("cmd"), "Start Script")) {
      if (isparam("params")) {
         
         /* set parameters from ODB */
         db_find_key(hDB, 0, "/Experiment/Edit on sequence", &hparam);
         n = 0;
         if (hparam) {
            for (i = 0 ;; i++) {
               db_enum_key(hDB, hparam, i, &hsubkey);
               
               if (!hsubkey)
                  break;
               
               db_get_key(hDB, hsubkey, &key);
               
               for (int j = 0; j < key.num_values; j++) {
                  size = key.item_size;
                  sprintf(str, "x%d", n++);
                  db_sscanf(getparam(str), data, &size, 0, key.type);
                  db_set_data_index(hDB, hsubkey, data, key.item_size, j, key.type);
               }
            }
         }
         
         /* set parameters from script */
         pn = mxml_find_node(pnseq, "RunSequence");
         if (pn) {
            last_line = mxml_get_line_number_end(pn);
            
            for (i=1 ; i<last_line ; i++){
               pn = mxml_get_node_at_line(pnseq, i);
               if (!pn)
                  continue;
               
               if (equal_ustring(mxml_get_name(pn), "Param")) {
                  strlcpy(name, mxml_get_attribute(pn, "name"), sizeof(name));
                  sprintf(str, "x%d", n++);
                  strlcpy(buffer, getparam(str), sizeof(buffer));
                  sprintf(str, "/Sequencer/Variables/%s", name);
                  db_set_value(hDB, 0, str, buffer, strlen(buffer)+1, 1, TID_STRING);
               }
            }
         }
         
         /* start sequencer */
         seq.running = TRUE;
         seq.finished = FALSE;
         seq.paused = FALSE;
         seq.transition_request = FALSE;
         seq.wait_limit = 0;
         seq.wait_value = 0;
         seq.start_time = 0;
         seq.wait_type[0] = 0;
         for (i=0 ; i<4 ; i++) {
            seq.loop_start_line[i] = 0;
            seq.sloop_start_line[i] = 0;
            seq.loop_end_line[i] = 0;
            seq.sloop_end_line[i] = 0;
            seq.loop_counter[i] = 0;
            seq.loop_n[i] = 0;
         }
         for (i=0 ; i<4 ; i++) {
            seq.subroutine_end_line[i] = 0;
            seq.subroutine_return_line[i] = 0;
            seq.subroutine_call_line[i] = 0;
            seq.ssubroutine_call_line[i] = 0;
            seq.subroutine_param[i][0] = 0;
         }
         seq.current_line_number = 1;
         seq.scurrent_line_number = 1;
         seq.stack_index = 0;
         seq.error[0] = 0;
         seq.error_line = 0;
         seq.serror_line = 0;
         seq.subdir[0] = 0;
         seq.subdir_end_line = 0;
         seq.subdir_not_notify = 0;
         db_set_record(hDB, hKey, &seq, sizeof(seq), 0);
         cm_msg(MTALK, "show_seq_page", "Sequencer has been started.");
         redirect("");
         return;
         
      } else {

         seq_start_page();
         return;
      }
   }
   
   /*---- save script ----*/
   if (equal_ustring(getparam("cmd"), "Save")) {
      strlcpy(str, seq.path, sizeof(str));
      strlcat(str, seq.filename, sizeof(str));
      fh = open(str, O_RDWR | O_TRUNC | O_TEXT, 0644);
      if (fh > 0 && isparam("scripttext")) {
         i = strlen(getparam("scripttext"));
         write(fh, getparam("scripttext"), i);
         close(fh);
      }
      strlcpy(str, seq.path, sizeof(str));
      strlcat(str, seq.filename, sizeof(str));
      seq.error[0] = 0;
      if (pnseq) {
         mxml_free_tree(pnseq);
         pnseq = NULL;
      }
      seq.error_line = 0;
      seq.serror_line = 0;
      if (stristr(str, ".msl")) {
         if (msl_parse(str, seq.error, sizeof(seq.error), &seq.serror_line)) {
            strsubst(str, sizeof(str), ".msl", ".xml");
            pnseq = mxml_parse_file(str, seq.error, sizeof(seq.error), &seq.error_line);
         }
      } else
         pnseq = mxml_parse_file(str, seq.error, sizeof(seq.error), &seq.error_line);

      db_set_record(hDB, hKey, &seq, sizeof(seq), 0);
      redirect("");
      return;
   }
   
   /*---- stop after current run ----*/
   if (equal_ustring(getparam("cmd"), "Stop after current run")) {
      seq.stop_after_run = TRUE;
      db_set_record(hDB, hKey, &seq, sizeof(seq), 0);
      redirect("");
      return;
   }
   if (equal_ustring(getparam("cmd"), "Cancel 'Stop after current run'")) {
      seq.stop_after_run = FALSE;
      db_set_record(hDB, hKey, &seq, sizeof(seq), 0);
      redirect("");
      return;
   }
   
   /*---- stop immediately ----*/
   if (equal_ustring(getparam("cmd"), "Stop immediately")) {
      seq.running = FALSE;
      seq.finished = FALSE;
      seq.paused = FALSE;
      seq.wait_limit = 0;
      seq.wait_value = 0;
      seq.wait_type[0] = 0;
      for (i=0 ; i<4 ; i++) {
         seq.loop_start_line[i] = 0;
         seq.loop_end_line[i] = 0;
         seq.loop_counter[i] = 0;
         seq.loop_n[i] = 0;
      }
      seq.stop_after_run = FALSE;
      seq.subdir[0] = 0;
      
      /* stop run if not already stopped */
      size = sizeof(state);
      db_get_value(hDB, 0, "/Runinfo/State", &state, &size, TID_INT, FALSE);
      if (state != STATE_STOPPED)
         cm_transition(TR_STOP, 0, str, sizeof(str), DETACH, FALSE);
      
      db_set_record(hDB, hKey, &seq, sizeof(seq), 0);
      cm_msg(MTALK, "show_seq_page", "Sequencer is finished.");
      redirect("");
      return;
   }
   
   /*---- pause script ----*/
   if (equal_ustring(getparam("cmd"), "SPause")) {
      seq.paused = TRUE;
      db_set_record(hDB, hKey, &seq, sizeof(seq), 0);
      redirect("");
      return;
   }
   
   /*---- resume script ----*/
   if (equal_ustring(getparam("cmd"), "SResume")) {
      seq.paused = FALSE;
      db_set_record(hDB, hKey, &seq, sizeof(seq), 0);
      redirect("");
      return;
   }

   /* header */
   rsprintf("HTTP/1.0 200 Document follows\r\n");
   rsprintf("Server: MIDAS HTTP %d\r\n", mhttpd_revision());
   rsprintf("Content-Type: text/html; charset=iso-8859-1\r\n\r\n");
   
   rsprintf("<html><head>\n");
   rsprintf("<link rel=\"icon\" href=\"favicon.png\" type=\"image/png\" />\n");
   rsprintf("<link rel=\"stylesheet\" href=\"mhttpd.css\" type=\"text/css\" />\n");
   
   if (!equal_ustring(getparam("cmd"), "Load Script") && !isparam("fs") &&
       !equal_ustring(getparam("cmd"), "Edit Script"))
      rsprintf("<meta http-equiv=\"Refresh\" content=\"60\">\n");
   
   /* update script */
   rsprintf("<script type=\"text/javascript\" src=\"../mhttpd.js\"></script>\n");
   rsprintf("<script type=\"text/javascript\">\n");
   rsprintf("<!--\n");
   rsprintf("var show_all_lines = false;\n");
   rsprintf("var sshow_all_lines = false;\n");
   rsprintf("var show_xml = false;\n");
   rsprintf("var last_msg = null;\n");
   rsprintf("var last_paused = null;\n");
   rsprintf("\n");
   rsprintf("function seq_refresh()\n");
   rsprintf("{\n");
   rsprintf("   seq = ODBGetRecord('/Sequencer/State');\n");
   rsprintf("   var current_line = ODBExtractRecord(seq, 'Current line number');\n");
   rsprintf("   var scurrent_line = ODBExtractRecord(seq, 'SCurrent line number');\n");
   rsprintf("   var subroutine_call_line = ODBExtractRecord(seq, 'Subroutine call line');\n");
   rsprintf("   var ssubroutine_call_line = ODBExtractRecord(seq, 'SSubroutine call line');\n");
   rsprintf("   var error_line = ODBExtractRecord(seq, 'Error line');\n");
   rsprintf("   var serror_line = ODBExtractRecord(seq, 'SError line');\n");
   rsprintf("   var message = ODBExtractRecord(seq, 'Message');\n");
   rsprintf("   var wait_value = ODBExtractRecord(seq, 'Wait value');\n");
   rsprintf("   var wait_limit = ODBExtractRecord(seq, 'Wait limit');\n");
   rsprintf("   var wait_type = ODBExtractRecord(seq, 'Wait type');\n");
   rsprintf("   var loop_n = ODBExtractRecord(seq, 'Loop n');\n");
   rsprintf("   var loop_counter = ODBExtractRecord(seq, 'Loop counter');\n");
   rsprintf("   var loop_start_line = ODBExtractRecord(seq, 'Loop start line');\n");
   rsprintf("   var sloop_start_line = ODBExtractRecord(seq, 'SLoop start line');\n");
   rsprintf("   var loop_end_line = ODBExtractRecord(seq, 'Loop end line');\n");
   rsprintf("   var sloop_end_line = ODBExtractRecord(seq, 'SLoop end line');\n");
   rsprintf("   var finished = ODBExtractRecord(seq, 'Finished');\n");
   rsprintf("   var paused = ODBExtractRecord(seq, 'Paused');\n");
   rsprintf("   var start_time = ODBExtractRecord(seq, 'Start time');\n");
   rsprintf("   var msg = ODBExtractRecord(seq, 'Last msg');\n");
   rsprintf("   \n");
   rsprintf("   if (last_msg == null)\n");
   rsprintf("      last_msg = msg;\n");
   rsprintf("   else if (last_msg != msg)\n");
   rsprintf("      window.location.href = '.';\n");
   rsprintf("   \n");
   rsprintf("   if (last_paused == null)\n");
   rsprintf("      last_paused = paused;\n");
   rsprintf("   else if (last_paused != paused)\n");
   rsprintf("      window.location.href = '.';\n");
   rsprintf("   \n");
   rsprintf("   for (var sl=1 ; ; sl++) {\n");
   rsprintf("      sline = document.getElementById('sline'+sl);\n");
   rsprintf("      if (sline == null) {\n");
   rsprintf("         var slast_line = sl-1;\n");
   rsprintf("         break;\n");
   rsprintf("      }\n");
   rsprintf("      if (Math.abs(sl - scurrent_line) > 10 && !sshow_all_lines)\n");
   rsprintf("         sline.style.display = 'none';\n");
   rsprintf("      else\n");
   rsprintf("         sline.style.display = 'inline';\n");
   rsprintf("      if (scurrent_line > 10 || sshow_all_lines)\n");
   rsprintf("          document.getElementById('slinedots1').style.display = 'inline';\n");
   rsprintf("      else\n");
   rsprintf("          document.getElementById('slinedots1').style.display = 'none';\n");
   rsprintf("      if (sl == serror_line)\n");
   rsprintf("         sline.style.backgroundColor = '#FF0000';\n");
   rsprintf("      else if (sl == scurrent_line)\n");
   rsprintf("         sline.style.backgroundColor = '#80FF80';\n");
   rsprintf("      else if (sl == ssubroutine_call_line[3])\n");
   rsprintf("         sline.style.backgroundColor = '%s';\n", call_col[3]);
   rsprintf("      else if (sl == ssubroutine_call_line[2])\n");
   rsprintf("         sline.style.backgroundColor = '%s';\n", call_col[2]);
   rsprintf("      else if (sl == ssubroutine_call_line[1])\n");
   rsprintf("         sline.style.backgroundColor = '%s';\n", call_col[1]);
   rsprintf("      else if (sl == ssubroutine_call_line[0])\n");
   rsprintf("         sline.style.backgroundColor = '%s';\n", call_col[0]);
   rsprintf("      else if (sl >= sloop_start_line[3] && sl <= sloop_end_line[3])\n");
   rsprintf("         sline.style.backgroundColor = '%s';\n", bar_col[3]);
   rsprintf("      else if (sl >= sloop_start_line[2] && sl <= sloop_end_line[2])\n");
   rsprintf("         sline.style.backgroundColor = '%s';\n", bar_col[2]);
   rsprintf("      else if (sl >= sloop_start_line[1] && sl <= sloop_end_line[1])\n");
   rsprintf("         sline.style.backgroundColor = '%s';\n", bar_col[1]);
   rsprintf("      else if (sl >= sloop_start_line[0] && sl <= sloop_end_line[0])\n");
   rsprintf("         sline.style.backgroundColor = '%s';\n", bar_col[0]);
   rsprintf("      else\n");
   rsprintf("         sline.style.backgroundColor = '#FFFFFF';\n");
   rsprintf("   }\n");
   rsprintf("   for (var l=1 ; ; l++) {\n");
   rsprintf("      line = document.getElementById('line'+l);\n");
   rsprintf("      if (line == null) {\n");
   rsprintf("         var last_line = l-1;\n");
   rsprintf("         break;\n");
   rsprintf("      }\n");
   rsprintf("      if (Math.abs(l - current_line) > 10 && !show_all_lines)\n");
   rsprintf("         line.style.display = 'none';\n");
   rsprintf("      else\n");
   rsprintf("         line.style.display = 'inline';\n");
   rsprintf("      if (current_line > 10 || show_all_lines)\n");
   rsprintf("          document.getElementById('linedots1').style.display = 'inline';\n");
   rsprintf("      else\n");
   rsprintf("          document.getElementById('linedots1').style.display = 'none';\n");
   rsprintf("      if (l == error_line)\n");
   rsprintf("         line.style.backgroundColor = '#FF0000';\n");
   rsprintf("      else if (l == current_line)\n");
   rsprintf("         line.style.backgroundColor = '#80FF80';\n");
   rsprintf("      else if (l == subroutine_call_line[3])\n");
   rsprintf("         line.style.backgroundColor = '%s';\n", call_col[3]);
   rsprintf("      else if (l == subroutine_call_line[2])\n");
   rsprintf("         line.style.backgroundColor = '%s';\n", call_col[2]);
   rsprintf("      else if (l == subroutine_call_line[1])\n");
   rsprintf("         line.style.backgroundColor = '%s';\n", call_col[1]);
   rsprintf("      else if (l == subroutine_call_line[0])\n");
   rsprintf("         line.style.backgroundColor = '%s';\n", call_col[0]);
   rsprintf("      else if (l >= loop_start_line[3] && l <= loop_end_line[3])\n");
   rsprintf("         line.style.backgroundColor = '%s';\n", bar_col[3]);
   rsprintf("      else if (l >= loop_start_line[2] && l <= loop_end_line[2])\n");
   rsprintf("         line.style.backgroundColor = '%s';\n", bar_col[2]);
   rsprintf("      else if (l >= loop_start_line[1] && l <= loop_end_line[1])\n");
   rsprintf("         line.style.backgroundColor = '%s';\n", bar_col[1]);
   rsprintf("      else if (l >= loop_start_line[0] && l <= loop_end_line[0])\n");
   rsprintf("         line.style.backgroundColor = '%s';\n", bar_col[0]);
   rsprintf("      else\n");
   rsprintf("         line.style.backgroundColor = '#FFFFFF';\n");
   rsprintf("   }\n");
   rsprintf("   \n");
   rsprintf("   if (document.getElementById('linedots2')) {\n");
   rsprintf("      if (current_line < last_line-10 && !show_all_lines)\n");
   rsprintf("         document.getElementById('linedots2').style.display = 'inline';\n");
   rsprintf("      else\n");
   rsprintf("         document.getElementById('linedots2').style.display = 'none';\n");
   rsprintf("   }\n");
   rsprintf("   if (document.getElementById('slinedots2')) {\n");
   rsprintf("      if (scurrent_line < slast_line-10 && !show_all_lines)\n");
   rsprintf("         document.getElementById('slinedots2').style.display = 'inline';\n");
   rsprintf("      else\n");
   rsprintf("         document.getElementById('slinedots2').style.display = 'none';\n");
   rsprintf("   }\n");
   rsprintf("   \n");
   rsprintf("   var wl = document.getElementById('wait_label');\n");
   rsprintf("   if (wl != null) {\n");
   rsprintf("      if (wait_type == 'Seconds')\n");
   rsprintf("         wl.innerHTML = 'Wait:&nbsp['+wait_value+'/'+wait_limit+'&nbsp;s]';\n");
   rsprintf("      else if (wait_type == 'Events')\n");
   rsprintf("         wl.innerHTML = 'Run:&nbsp['+wait_value+'/'+wait_limit+'&nbsp;events]';\n");
   rsprintf("      else if (wait_type.substr(0, 3) == 'ODB') {\n");
   rsprintf("         op = wait_type.substr(3);\n");
   rsprintf("         if (op == '')\n");
   rsprintf("            op = '>=';\n");
   rsprintf("         op = op.replace(/>/g, '&gt;').replace(/</g, '&lt;');\n");
   rsprintf("         wl.innerHTML = 'ODB:&nbsp['+wait_value+'&nbsp;'+op+'&nbsp;'+wait_limit+']';\n");
   rsprintf("      } else {\n");
   rsprintf("         wl.innerHTML = '';\n");
   rsprintf("      }\n");
   rsprintf("      wr = document.getElementById('wait_row');\n"); 
   rsprintf("      if (wait_type == '')\n");
   rsprintf("         wr.style.display = 'none';\n");
   rsprintf("      else\n");
   rsprintf("         wr.style.display = 'table-row';\n");
   rsprintf("   }\n");
   rsprintf("   var rp = document.getElementById('runprgs');\n");
   rsprintf("   if (rp != null) {\n");
   rsprintf("      var width = Math.round(100.0*wait_value/wait_limit);\n");
   rsprintf("      if (width > 100)\n");
   rsprintf("         width = 100;\n");
   rsprintf("      rp.width = width+'%%';\n");
   rsprintf("   }\n");
   rsprintf("   \n");
   rsprintf("   for (var i=0 ; i<4 ; i++) {\n");
   rsprintf("      var l = document.getElementById('loop'+i);\n");
   rsprintf("      if (loop_start_line[i] > 0) {\n");
   rsprintf("         var ll = document.getElementById('loop_label'+i);\n");
   rsprintf("         if (loop_n[i] == -1)\n");
   rsprintf("            ll.innerHTML = 'Loop:&nbsp['+loop_counter[i]+']';\n");
   rsprintf("         else\n");
   rsprintf("            ll.innerHTML = 'Loop:&nbsp['+loop_counter[i]+'/'+loop_n[i]+']';\n");
   rsprintf("         l.style.display = 'table-row';\n");
   rsprintf("         var lp = document.getElementById('loopprgs'+i);\n");
   rsprintf("         if (loop_n[i] == -1)\n");
   rsprintf("            lp.style.display = 'none';\n");
   rsprintf("         else\n");
   rsprintf("            lp.width = Math.round(100.0*loop_counter[i]/loop_n[i])+'%%';\n");
   rsprintf("      } else\n");
   rsprintf("         l.style.display = 'none';\n");
   rsprintf("   }\n");
   rsprintf("   \n");
   rsprintf("   if (message != '') {\n");
   rsprintf("      alert(message);\n");
   rsprintf("      ODBSet('/Sequencer/State/Message', '');\n");
   rsprintf("      window.location.href = '.';\n");
   rsprintf("   }\n");
   rsprintf("   \n");
   rsprintf("   if (finished == 'y' || error_line > 0) {\n");
   rsprintf("      window.location.href = '.';\n");
   rsprintf("   }\n");
   rsprintf("\n");
   rsprintf("   refreshID = setTimeout('seq_refresh()', 1000);\n");
   rsprintf("}\n");
   rsprintf("\n");
   rsprintf("function show_lines()\n");
   rsprintf("{\n");
   rsprintf("   show_all_lines = !show_all_lines;\n");
   rsprintf("   seq_refresh();\n");
   rsprintf("}\n");
   rsprintf("\n");
   rsprintf("function sshow_lines()\n");
   rsprintf("{\n");
   rsprintf("   sshow_all_lines = !sshow_all_lines;\n");
   rsprintf("   seq_refresh();\n");
   rsprintf("}\n");
   rsprintf("\n");
   rsprintf("function load()\n");
   rsprintf("{\n");
   rsprintf("   if (document.getElementById('fs').selectedIndex == -1)\n");
   rsprintf("      alert('Please select a file to load');\n");
   rsprintf("   else {\n");
   rsprintf("      var o = document.createElement('input');\n");
   rsprintf("      o.type = 'hidden'; o.name='cmd'; o.value='Load';\n");
   rsprintf("      document.form1.appendChild(o);\n");
   rsprintf("      document.form1.submit();\n");
   rsprintf("   }\n");
   rsprintf("}\n");
   rsprintf("\n");
   rsprintf("function toggle_xml()\n");
   rsprintf("{\n");
   rsprintf("   show_xml = !show_xml;\n");
   rsprintf("   if (show_xml) {\n");
   rsprintf("      document.getElementById('xml_pane').style.display = 'inline';\n");
   rsprintf("      document.getElementById('txml').innerHTML = 'Hide XML';\n");
   rsprintf("   } else {\n");
   rsprintf("      document.getElementById('xml_pane').style.display = 'none';\n");
   rsprintf("      document.getElementById('txml').innerHTML = 'Show XML';\n");
   rsprintf("   }\n");
   rsprintf("}\n");
   
   rsprintf("//-->\n");
   rsprintf("</script>\n");
   
   rsprintf("<title>Sequencer</title></head>\n");
   if (seq.running)
      rsprintf("<body onLoad=\"seq_refresh();\">\n");
   
   /* title row */
   size = sizeof(str);
   str[0] = 0;
   db_get_value(hDB, 0, "/Experiment/Name", str, &size, TID_STRING, TRUE);
   time(&now);
   
   rsprintf("<form name=\"form1\" method=\"GET\" action=\".\">\n");
   
   rsprintf("<table border=3 cellpadding=2>\n");
   rsprintf("<tr><th bgcolor=#A0A0FF>MIDAS experiment \"%s\"", str);
   rsprintf("<th bgcolor=#A0A0FF>%s</tr>\n", ctime(&now));
   
   /*---- menu buttons ----*/
   
   if (!equal_ustring(getparam("cmd"), "Load Script") && !isparam("fs")) {
      rsprintf("<tr>\n");
      rsprintf("<td colspan=2 bgcolor=#C0C0C0>\n");
      
      if (seq.running) {
         if (seq.stop_after_run)
            rsprintf("<input type=submit name=cmd value=\"Cancel 'Stop after current run'\">\n");
         else
            rsprintf("<input type=submit name=cmd value=\"Stop after current run\">\n");
         
         rsprintf("<script type=\"text/javascript\">\n");
         rsprintf("<!--\n");
         rsprintf("function stop_immediately()\n");
         rsprintf("{\n");
         rsprintf("   flag = confirm('Are you sure to stop the script immediately?');\n");
         rsprintf("   if (flag == true)\n");
         rsprintf("      window.location.href = '?cmd=Stop immediately';\n");
         rsprintf("}\n");
         rsprintf("//-->\n");
         rsprintf("</script>\n");
         rsprintf("<input type=button onClick=\"stop_immediately()\" value=\"Stop immediately\">");
         
         if (!seq.paused) {
            rsprintf("<script type=\"text/javascript\">\n");
            rsprintf("<!--\n");
            rsprintf("function pause()\n");
            rsprintf("{\n");
            rsprintf("   flag = confirm('Are you sure to pause the script ?');\n");
            rsprintf("   if (flag == true)\n");
            rsprintf("      window.location.href = '?cmd=SPause';\n");
            rsprintf("}\n");
            rsprintf("//-->\n");
            rsprintf("</script>\n");
            rsprintf("<input type=button onClick=\"pause()\" value=\"Pause script\">");
         } else {
            rsprintf("<input type=button onClick=\"window.location.href=\'?cmd=SResume\';\" value=\"Resume script\">");
         }
         
      } else {
         rsprintf("<input type=submit name=cmd value=\"Load Script\">\n");
         rsprintf("<input type=submit name=cmd value=\"Start Script\">\n");
      }
      if (seq.filename[0] && !seq.running && !equal_ustring(getparam("cmd"), "Load Script") && !isparam("fs"))
         rsprintf("<input type=submit name=cmd value=\"Edit Script\">\n");
      rsprintf("<input type=submit name=cmd value=Status>\n");
      
      rsprintf("</td></tr>\n");
   }
   
   /*---- file selector ----*/
   
   if (equal_ustring(getparam("cmd"), "Load Script") || isparam("fs")) {
      rsprintf("<tr><td align=center colspan=2 bgcolor=#FFFFFF>\n");
      rsprintf("<b>Select a sequence file:</b><br>\n");
      rsprintf("<select name=\"fs\" id=\"fs\" size=20 style=\"width:300\">\n");
      
      if (isparam("dir"))
         strlcpy(dir, getparam("dir"), sizeof(dir));
      else
         dir[0] = 0;
      strlcpy(path, seq.path, sizeof(path));

      if (isparam("fs")) {
         strlcpy(str, getparam("fs"), sizeof(str));
         if (str[0] == '[') {
            strcpy(str, str+1);
            str[strlen(str)-1] = 0;
         }
         if (equal_ustring(str, "..")) {
            pc = dir+strlen(dir)-1;
            if (pc > dir && *pc == '/')
               *(pc--) = 0;
            while (pc >= dir && *pc != '/')
               *(pc--) = 0;
         } else {
            strlcat(dir, str, sizeof(dir));
            strlcat(dir, DIR_SEPARATOR_STR, sizeof(dir));
         }
      }
      strlcat(path, dir, sizeof(path));
      
      /*---- go over subdirectories ----*/
      n = ss_dir_find(path, (char *)"*", &flist);     
      if (dir[0])
         rsprintf("<option onDblClick=\"document.form1.submit()\">[..]</option>\n");
      for (i=0 ; i<n ; i++) {
         if (flist[i*MAX_STRING_LENGTH] != '.')
            rsprintf("<option onDblClick=\"document.form1.submit()\">[%s]</option>\n", flist+i*MAX_STRING_LENGTH);
      }
      
      /*---- go over XML files in sequencer directory ----*/
      /*
      n = ss_file_find(path, (char *)"*.xml", &flist);
      for (i=0 ; i<n ; i++) {
         strlcpy(str, path, sizeof(str));
         strlcat(str, flist+i*MAX_STRING_LENGTH, sizeof(str));
         comment[0] = 0;
         if (pnseq) {
            mxml_free_tree(pnseq);
            pnseq = NULL;
         }
         pnseq = mxml_parse_file(str, error, sizeof(error), &error_line);
         if (error[0])
            sprintf(comment, "Error in XML: %s", error);
         else {
            if (pnseq) {
               pn = mxml_find_node(pnseq, "RunSequence/Comment");
               if (pn)
                  strlcpy(comment, mxml_get_value(pn), sizeof(comment));
               else
                  strcpy(comment, "<No description in XML file>");
            }
         }
         if (pnseq) {
            mxml_free_tree(pnseq);
            pnseq = NULL;
         }
         
         rsprintf("<option onClick=\"document.getElementById('cmnt').innerHTML='%s'\">%s</option>\n", comment, flist+i*MAX_STRING_LENGTH);
      }
      */
      
      /*---- go over MSL files in sequencer directory ----*/
      n = ss_file_find(path, (char *)"*.msl", &flist);
      for (i=0 ; i<n ; i++) {
         strlcpy(str, path, sizeof(str));
         strlcat(str, flist+i*MAX_STRING_LENGTH, sizeof(str));
         
         if (msl_parse(str, error, sizeof(error), &error_line)) {
            if (strchr(str, '.')) {
               *strchr(str, '.') = 0;
               strlcat(str, ".xml", sizeof(str));
            }
            comment[0] = 0;
            if (pnseq) {
               mxml_free_tree(pnseq);
               pnseq = NULL;
            }
            pnseq = mxml_parse_file(str, error, sizeof(error), &error_line);
            if (error[0]) {
               strlcpy(comment, error, sizeof(comment));
            } else {
               if (pnseq) {
                  pn = mxml_find_node(pnseq, "RunSequence/Comment");
                  if (pn)
                     strlcpy(comment, mxml_get_value(pn), sizeof(comment));
                  else
                     strcpy(comment, "<No description in XML file>");
               }
            }
            if (pnseq) {
               mxml_free_tree(pnseq);
               pnseq = NULL;
            }
         } else
            sprintf(comment, "Error in MSL: %s", error);
         
         strsubst(comment, sizeof(comment), "\"", "\\\'");
         rsprintf("<option onClick=\"document.getElementById('cmnt').innerHTML='%s'\"", comment);
         rsprintf(" onDblClick=\"load();\">%s</option>\n", flist+i*MAX_STRING_LENGTH);
      }

      free(flist);
      flist = NULL;
      rsprintf("</select>\n");
      rsprintf("<input type=hidden name=dir value=\"%s\">", dir);
      rsprintf("</td></tr>\n");
      
      rsprintf("<tr><td align=center colspan=2 id=\"cmnt\">&nbsp;</td></tr>\n");
      rsprintf("<tr><td align=center colspan=2>\n");
      rsprintf("<input type=button onClick=\"load();\" value=\"Load\">\n");
      rsprintf("<input type=submit name=cmd value=\"Cancel\">\n");
      rsprintf("</td></tr>\n");
   }
   
   /*---- show XML file ----*/
   
   else {
      if (seq.filename[0]) {
         if (equal_ustring(getparam("cmd"), "Edit Script")) {
            rsprintf("<tr><td colspan=2>Filename:<b>%s</b>&nbsp;&nbsp;", seq.filename);
            rsprintf("<input type=submit name=cmd value=\"Save\">\n");
            rsprintf("<input type=submit name=cmd value=\"Cancel\">\n");
            rsprintf("</td></tr>\n");
            rsprintf("<tr><td colspan=2><textarea rows=30 cols=80 name=\"scripttext\" style=\"font-family:monospace;font-size:medium;\">\n");
            strlcpy(str, seq.path, sizeof(str));
            strlcat(str, seq.filename, sizeof(str));
            f = fopen(str, "rt");
            if (f) {
               for (int line=0 ; !feof(f) ; line++) {
                  str[0] = 0;
                  fgets(str, sizeof(str), f);
                  rsprintf(str);
               }
               fclose(f);
            }
            rsprintf("</textarea></td></tr>\n");
            rsprintf("<tr><td align=center colspan=2>\n");
            rsprintf("<input type=submit name=cmd value=\"Save\">\n");
            rsprintf("<input type=submit name=cmd value=\"Cancel\">\n");
            rsprintf("</td></tr>\n");
         } else {
            if (seq.stop_after_run)
               rsprintf("<tr id=\"msg\" style=\"display:table-row\"><td colspan=2><b>Sequence will be stopped after current run</b></td></tr>\n");
            else
               rsprintf("<tr id=\"msg\" style=\"display:none\"><td colspan=2><b>Sequence will be stopped after current run</b></td></tr>\n");
            
            for (i=0 ; i<4 ; i++) {
               rsprintf("<tr id=\"loop%d\" style=\"display:none\"><td colspan=2>\n", i);
               rsprintf("<table width=\"100%%\"><tr><td id=\"loop_label%d\">Loop&nbsp;%d:</td>\n", i, i);
               if (seq.loop_n[i] <= 0)
                  width = 0;
               else
                  width = (int)(((double)seq.loop_counter[i]/seq.loop_n[i])*100+0.5);
               rsprintf("<td width=\"100%%\"><table id=\"loopprgs%d\" width=\"%d%%\" height=\"25\">\n", i, width);
               rsprintf("<tr><td style=\"background-color:%s;", bar_col[i]);
               rsprintf("border:2px solid #000080;border-top:2px solid #E0E0FF;border-left:2px solid #E0E0FF;\">&nbsp;\n");
               rsprintf("</td></tr></table></td></tr></table></td></tr>\n");
            }
            if (seq.running) {
               rsprintf("<tr id=\"wait_row\" style=\"visible: none;\"><td colspan=2>\n");
               if (seq.wait_value <= 0)
                  width = 0;
               else
                  width = (int)(((double)seq.wait_value/seq.wait_limit)*100+0.5);
               rsprintf("<table width=\"100%%\"><tr><td id=\"wait_label\">Run:</td>\n");
               rsprintf("<td width=\"100%%\"><table id=\"runprgs\" width=\"%d%%\" height=\"25\">\n", width);
               rsprintf("<tr><td style=\"background-color:#80FF80;border:2px solid #008000;border-top:2px solid #E0E0FF;border-left:2px solid #E0E0FF;\">&nbsp;\n");
               rsprintf("</td></tr></table></td></tr></table></td></tr>\n");
            }
            if (seq.paused) {
               rsprintf("<tr><td align=\"center\" colspan=2 style=\"background-color:#FFFF80;\"><b>Sequencer is paused</b>\n");
               rsprintf("</td></tr>\n");
            }
            if (seq.finished) {
               rsprintf("<tr><td colspan=2 style=\"background-color:#80FF80;\"><b>Sequence is finished</b>\n");
               rsprintf("</td></tr>\n");
            }
            
            rsprintf("<tr><td colspan=2><table width=100%%><tr><td>Filename:<b>%s</b></td>", seq.filename);
            if (stristr(seq.filename, ".msl"))
               rsprintf("<td align=\"right\"><a onClick=\"toggle_xml();\" id=\"txml\" href=\"#\">Show XML</a></td>");
            rsprintf("</td></tr></table></td></tr>\n");
            
            if (seq.error[0]) {
               rsprintf("<tr><td bgcolor=red colspan=2><b>");
               strencode(seq.error);
               rsprintf("</b></td></tr>\n");
            }
   
            rsprintf("<tr><td colspan=2><table width=100%%>");

            /*---- Left (MSL) pane ---- */
               
            if (stristr(seq.filename, ".msl")) {
               strlcpy(str, seq.path, sizeof(str));
               strlcat(str, seq.filename, sizeof(str));
               fh = open(str, O_RDONLY | O_TEXT, 0644);
               if (fh > 0) {
                  size = (int)lseek(fh, 0, SEEK_END);
                  lseek(fh, 0, SEEK_SET);
                  buf = (char *)malloc(size+1);
                  size = (int)read(fh, buf, size);
                  buf[size] = 0;
                  close(fh);
                  
                  rsprintf("<tr><td colspan=2 valign=\"top\">\n");
                  rsprintf("<div onClick=\"sshow_lines();\" id=\"slinedots1\" style=\"display:none;\">...<br></div>\n");
                  
                  pline = buf;
                  for (int line=1 ; *pline ; line++) {
                     strlcpy(str, pline, sizeof(str));
                     if (strchr(str, '\n'))
                        *(strchr(str, '\n')+1) = 0;
                     if (str[0]) {
                        if (line == seq.serror_line)
                           rsprintf("<font id=\"sline%d\" style=\"font-family:monospace;background-color:red;\">", line);
                        else if (seq.running && line == seq.current_line_number)
                           rsprintf("<font id=\"sline%d\" style=\"font-family:monospace;background-color:#80FF00\">", line);
                        else
                           rsprintf("<font id=\"sline%d\" style=\"font-family:monospace\">", line);
                        if (line < 10)
                           rsprintf("&nbsp;");
                        if (line < 100)
                           rsprintf("&nbsp;");
                        rsprintf("%d&nbsp;", line);
                        strencode4(str);
                        rsprintf("</font>");
                     }
                     if (strchr(pline, '\n'))
                        pline = strchr(pline, '\n')+1;
                     else
                        pline += strlen(pline);
                     if (*pline == '\r')
                        pline++;
                  }
                  rsprintf("<div onClick=\"sshow_lines();\" id=\"slinedots2\" style=\"display:none;\">...<br></div>\n");
                  rsprintf("</td>\n");
                  free(buf);
                  buf = NULL;
                  
               } else {
                  if (str[0]) {
                     rsprintf("<tr><td colspan=2><b>Cannot open file \"%s\"</td></tr>\n", str);
                  }
               }
            }
            
            /*---- Right (XML) pane ----*/
            
            if (stristr(seq.filename, ".msl"))
               rsprintf("<td id=\"xml_pane\" style=\"border-left-width:1px;border-left-style:solid;border-color:black;display:none;\">\n");
            else
               rsprintf("<td colspan=2 id=\"xml_pane\">\n");

            rsprintf("<div onClick=\"show_lines();\" id=\"linedots1\" style=\"display:none;\">...<br></div>\n");
            
            strlcpy(str, seq.path, sizeof(str));
            strlcat(str, seq.filename, sizeof(str));
            if (strchr(str, '.')) {
               *strchr(str, '.') = 0;
               strlcat(str, ".xml", sizeof(str));
            }
            fh = open(str, O_RDONLY | O_TEXT, 0644);
            if (fh > 0) {
               size = (int)lseek(fh, 0, SEEK_END);
               lseek(fh, 0, SEEK_SET);
               buf = (char *)malloc(size+1);
               size = (int)read(fh, buf, size);
               buf[size] = 0;
               close(fh);
               if (mxml_parse_entity(&buf, str, NULL, 0, NULL) != 0) {
                  /* show error */
               }
               
               pline = buf;
               for (int line=1 ; *pline ; line++) {
                  strlcpy(str, pline, sizeof(str));
                  if (strchr(str, '\n'))
                     *(strchr(str, '\n')+1) = 0;
                  if (str[0]) {
                     if (line == seq.error_line)
                        rsprintf("<font id=\"line%d\" style=\"font-family:monospace;background-color:red;\">", line);
                     else if (seq.running && line == seq.current_line_number)
                        rsprintf("<font id=\"line%d\" style=\"font-family:monospace;background-color:#80FF00\">", line);
                     else
                        rsprintf("<font id=\"line%d\" style=\"font-family:monospace\">", line);
                     if (line < 10)
                        rsprintf("&nbsp;");
                     if (line < 100)
                        rsprintf("&nbsp;");
                     rsprintf("%d&nbsp;", line);
                     strencode4(str);
                     rsprintf("</font>");
                  }
                  if (strchr(pline, '\n'))
                     pline = strchr(pline, '\n')+1;
                  else
                     pline += strlen(pline);
                  if (*pline == '\r')
                     pline++;
               }
               rsprintf("<div onClick=\"show_lines();\" id=\"linedots2\" style=\"display:none;\">...<br></div>\n");
               rsprintf("</td>\n");
               free(buf);
               buf = NULL;
            } else {
               if (str[0]) {
                  rsprintf("<tr><td colspan=2><b>Cannot open file \"%s\"</td></tr>\n", str);
               }
            }
         }
         
         rsprintf("</tr></table></td></tr>\n");
            
         /*---- show messages ----*/
         if (seq.running) {
            rsprintf("<tr><td colspan=2>\n");
            rsprintf("<font style=\"font-family:monospace\">\n");
            rsprintf("<a href=\"../?cmd=Messages\">...</a><br>\n");
            
            cm_msg_retrieve(10, buffer, sizeof(buffer));
            
            pline = buffer;
            eob = FALSE;
            
            do {
               strlcpy(line, pline, sizeof(line));
               
               /* extract single line */
               if (strchr(line, '\n'))
                  *strchr(line, '\n') = 0;
               if (strchr(line, '\r'))
                  *strchr(line, '\r') = 0;
               
               pline += strlen(line);
               
               while (*pline == '\r' || *pline == '\n')
                  pline++;
               
               strlcpy(str, line+11, sizeof(str));
               pc = strchr(line+25, ' ');
               if (pc)
                  strlcpy(str+8, pc, sizeof(str)-9);
               
               /* check for error */
               if (strstr(line, ",ERROR]"))
                  rsprintf("<span style=\"color:white;background-color:red\">%s</span>", str);
               else
                  rsprintf("%s", str);
               
               rsprintf("<br>\n");
            } while (!eob && *pline);
            
            
            rsprintf("</font></td></tr>\n");
         }
      } else {
         rsprintf("<tr><td colspan=2><b>No script loaded</b></td></tr>\n");
      }
   }
   
   rsprintf("</table>\n");
   rsprintf("</form></body></html>\r\n");
}

/*------------------------------------------------------------------*/

void sequencer()
{
   PMXML_NODE pn, pr, pt;
   char odbpath[256], value[256], data[256], str[256], name[32], op[32];
   char list[100][NAME_LENGTH], *pc;
   int i, l, n, status, size, index, index1, index2, last_line, state, run_number, cont;
   HNDLE hDB, hKey, hKeySeq;
   KEY key;
   double d;
   float v;
   
   if (!seq.running || seq.paused || pnseq == NULL)
      return;
   
   cm_get_experiment_database(&hDB, NULL);
   db_find_key(hDB, 0, "/Sequencer/State", &hKeySeq);
   if (!hKeySeq)
      return;
   
   cm_msg_retrieve(1, str, sizeof(str));
   str[19] = 0;
   strcpy(seq.last_msg, str+11);
   
   pr = mxml_find_node(pnseq, "RunSequence");
   if (!pr) {
      seq_error("Cannot find <RunSequence> tag in XML file");
      return;
   }

   last_line = mxml_get_line_number_end(pr);
   
   /* check for Subroutine end */
   if (seq.stack_index > 0 && seq.current_line_number == seq.subroutine_end_line[seq.stack_index-1]) {
      seq.subroutine_end_line[seq.stack_index-1] = 0;
      seq.current_line_number = seq.subroutine_return_line[seq.stack_index-1];
      seq.stack_index --;
   }

   /* check for last line of script */
   if (seq.current_line_number >= last_line) {
      seq.running = FALSE;
      seq.finished = TRUE;
      db_set_record(hDB, hKeySeq, &seq, sizeof(seq), 0);
      cm_msg(MTALK, "sequencer", "Sequencer is finished.");
      return;
   }
   
   /* check for loop end */
   for (i=3 ; i>=0 ; i--)
      if (seq.loop_start_line[i] > 0)
         break;
   if (i >= 0) {
      if (seq.current_line_number == seq.loop_end_line[i]) {
         if (seq.loop_counter[i] == seq.loop_n[i]) {
            seq.loop_counter[i] = 0;
            seq.loop_start_line[i] = 0;
            seq.sloop_start_line[i] = 0;
            seq.loop_end_line[i] = 0;
            seq.sloop_end_line[i] = 0;
            seq.loop_n[i] = 0;
            seq.current_line_number++;
         } else {
            pn = mxml_get_node_at_line(pnseq, seq.loop_start_line[i]);
            if (mxml_get_attribute(pn, "var") && mxml_get_attribute(pn, "values")) {
               strlcpy(data, mxml_get_attribute(pn, "values"), sizeof(data));
               strbreak(data, list, 100, ",", FALSE);
               strlcpy(name, mxml_get_attribute(pn, "var"), sizeof(name));
               if (!eval_var(list[seq.loop_counter[i]], value, sizeof(value)))
                  return;
               sprintf(str, "/Sequencer/Variables/%s", name);
               db_set_value(hDB, 0, str, value, strlen(value)+1, 1, TID_STRING);
            }
            seq.loop_counter[i]++;
            seq.current_line_number = seq.loop_start_line[i]+1;
         }
         db_set_record(hDB, hKeySeq, &seq, sizeof(seq), 0);
         return;
      }
   }
   
   /* check for ODBSubdir end */
   if (seq.current_line_number == seq.subdir_end_line) {
      seq.subdir_end_line = 0;
      seq.subdir[0] = 0;
      seq.subdir_not_notify = FALSE;
   }
   
   /* find node belonging to current line */
   pn = mxml_get_node_at_line(pnseq, seq.current_line_number);
   if (!pn) {
      seq.current_line_number++;
      db_set_record(hDB, hKeySeq, &seq, sizeof(seq), 0);
      return;
   }
   
   /* set MSL line from current element */
   if (mxml_get_attribute(pn, "l"))
      seq.scurrent_line_number = atoi(mxml_get_attribute(pn, "l"));
   
   if (equal_ustring(mxml_get_name(pn), "PI") || 
       equal_ustring(mxml_get_name(pn), "RunSequence") ||
       equal_ustring(mxml_get_name(pn), "Comment")) {
      // just skip
      seq.current_line_number++;
   }
   
   /*---- ODBSubdir ----*/
   else if (equal_ustring(mxml_get_name(pn), "ODBSubdir")) {
      if (!mxml_get_attribute(pn, "path")) {
         seq_error("Missing attribute \"path\"");
      } else {
         strlcpy(seq.subdir, mxml_get_attribute(pn, "path"), sizeof(seq.subdir));
         if (mxml_get_attribute(pn, "notify"))
            seq.subdir_not_notify = !atoi(mxml_get_attribute(pn, "notify"));
         seq.subdir_end_line = mxml_get_line_number_end(pn);
         seq.current_line_number++;
      }
   }
   
   /*---- ODBSet ----*/
   else if (equal_ustring(mxml_get_name(pn), "ODBSet")) {
      if (!mxml_get_attribute(pn, "path")) {
         seq_error("Missing attribute \"path\"");
      } else {
         strlcpy(odbpath, seq.subdir, sizeof(odbpath));
         if (strlen(odbpath) > 0 && odbpath[strlen(odbpath)-1] != '/')
            strlcat(odbpath, "/", sizeof(odbpath));
         strlcat(odbpath, mxml_get_attribute(pn, "path"), sizeof(odbpath));
         
         /* check if index is supplied */
         index1 = index2 = 0;
         if (odbpath[strlen(odbpath) - 1] == ']') {
            if (strchr(odbpath, '[')) {
               if (*(strchr(odbpath, '[') + 1) == '*')
                  index1 = -1;
               else if (strchr((strchr(odbpath, '[') + 1), '.') ||
                        strchr((strchr(odbpath, '[') + 1), '-')) {
                  index1 = atoi(strchr(odbpath, '[') + 1);
                  pc = strchr(odbpath, '[') + 1;
                  while (*pc != '.' && *pc != '-')
                     pc++;
                  while (*pc == '.' || *pc == '-')
                     pc++;
                  index2 = atoi(pc);
               } else
                  index1 = atoi(strchr(odbpath, '[') + 1);
            }
            *strchr(odbpath, '[') = 0;
         }

         if (!eval_var(mxml_get_value(pn), value, sizeof(value)))
            return;
         status = db_find_key(hDB, 0, odbpath, &hKey);
         if (status != DB_SUCCESS) {
            sprintf(str, "Cannot find ODB key \"%s\"", odbpath);
            seq_error(str);
            return;
         } else {
            db_get_key(hDB, hKey, &key);
            size = sizeof(data);
            db_sscanf(value, data, &size, 0, key.type);
            
            int notify = TRUE;
            if (seq.subdir_not_notify)
               notify = FALSE;
            if (mxml_get_attribute(pn, "notify"))
               notify = atoi(mxml_get_attribute(pn, "notify"));

            if (key.num_values > 1 && index1 == -1) {
               for (i=0 ; i<key.num_values ; i++)
                  status = db_set_data_index2(hDB, hKey, data, key.item_size, i, key.type, notify);
            } else if (key.num_values > 1 && index2 > index1) {
               for (i=index1; i<key.num_values && i<=index2; i++)
                  status = db_set_data_index2(hDB, hKey, data, key.item_size, i, key.type, notify);
            } else
               status = db_set_data_index2(hDB, hKey, data, key.item_size, index1, key.type, notify);
            
            if (status != DB_SUCCESS) {
               sprintf(str, "Cannot set ODB key \"%s\"", odbpath);
               seq_error(str);
               return;
            }
            size = sizeof(seq);
            db_get_record(hDB, hKeySeq, &seq, &size, 0); // could have changed seq tree
            seq.current_line_number++;
         }
      }
   }
   
   /*---- ODBInc ----*/
   else if (equal_ustring(mxml_get_name(pn), "ODBInc")) {
      if (!mxml_get_attribute(pn, "path")) {
         seq_error("Missing attribute \"path\"");
      } else {
         strlcpy(odbpath, seq.subdir, sizeof(odbpath));
         if (strlen(odbpath) > 0 && odbpath[strlen(odbpath)-1] != '/')
            strlcat(odbpath, "/", sizeof(odbpath));
         strlcat(odbpath, mxml_get_attribute(pn, "path"), sizeof(odbpath));
         if (strchr(odbpath, '[')) {
            index = atoi(strchr(odbpath, '[')+1);
            *strchr(odbpath, '[') = 0;
         } else
            index = 0;
         if (!eval_var(mxml_get_value(pn), value, sizeof(value)))
            return;
         status = db_find_key(hDB, 0, odbpath, &hKey);
         if (status != DB_SUCCESS) {
            sprintf(str, "Cannot find ODB key \"%s\"", odbpath);
            seq_error(str);
         } else {
            db_get_key(hDB, hKey, &key);
            size = sizeof(data);
            db_get_data_index(hDB, hKey, data, &size, index, key.type);
            db_sprintf(str, data, size, 0, key.type);
            d = atof(str);
            d += atof(value);
            sprintf(str, "%lf", d);
            size = sizeof(data);
            db_sscanf(str, data, &size, 0, key.type);
            
            int notify = TRUE;
            if (seq.subdir_not_notify)
               notify = FALSE;
            if (mxml_get_attribute(pn, "notify"))
               notify = atoi(mxml_get_attribute(pn, "notify"));
            
            db_set_data_index2(hDB, hKey, data, key.item_size, index, key.type, notify);
            seq.current_line_number++;
         }
      }
   }
   
   /*---- RunDescription ----*/
   else if (equal_ustring(mxml_get_name(pn), "RunDescription")) {
      db_set_value(hDB, 0, "/Experiment/Run Parameters/Run Description", mxml_get_value(pn), 256, 1, TID_STRING);
      seq.current_line_number++;
   }
   
   /*---- Script ----*/
   else if (equal_ustring(mxml_get_name(pn), "Script")) {
      if (mxml_get_attribute(pn, "loop_counter"))
         sprintf(str, "%s %d %d %d %d", mxml_get_value(pn), seq.loop_counter[0], seq.loop_counter[1], seq.loop_counter[2], seq.loop_counter[3]);
      else
         sprintf(str, "%s", mxml_get_value(pn));
      ss_system(str);
      seq.current_line_number++;
   }
   
   /*---- Transition ----*/
   else if (equal_ustring(mxml_get_name(pn), "Transition")) {
      if (equal_ustring(mxml_get_value(pn), "Start")) {
         if (!seq.transition_request) {
            seq.transition_request = TRUE;
            size = sizeof(state);
            db_get_value(hDB, 0, "/Runinfo/State", &state, &size, TID_INT, FALSE);
            if (state != STATE_RUNNING) {
               size = sizeof(run_number);
               db_get_value(hDB, 0, "/Runinfo/Run number", &run_number, &size, TID_INT, FALSE);
               status = cm_transition(TR_START, run_number+1, str, sizeof(str), DETACH, FALSE);
               if (status != CM_SUCCESS) {
                  sprintf(str, "Cannot start run: %s", str);
                  seq_error(str);
               }
            }
         } else {
            // Wait until transition has finished
            size = sizeof(state);
            db_get_value(hDB, 0, "/Runinfo/State", &state, &size, TID_INT, FALSE);
            if (state == STATE_RUNNING) {
               seq.transition_request = FALSE;
               seq.current_line_number++;
            }
         }
      } else if (equal_ustring(mxml_get_value(pn), "Stop")) {
         if (!seq.transition_request) {
            seq.transition_request = TRUE;
            size = sizeof(state);
            db_get_value(hDB, 0, "/Runinfo/State", &state, &size, TID_INT, FALSE);
            if (state != STATE_STOPPED) {
               status = cm_transition(TR_STOP, 0, str, sizeof(str), DETACH, FALSE);
               if (status != CM_SUCCESS) {
                  sprintf(str, "Cannot stop run: %s", str);
                  seq_error(str);
               }
            }
         } else {
            // Wait until transition has finished
            size = sizeof(state);
            db_get_value(hDB, 0, "/Runinfo/State", &state, &size, TID_INT, FALSE);
            if (state == STATE_STOPPED) {
               seq.transition_request = FALSE;
               
               if (seq.stop_after_run) {
                  seq.stop_after_run = FALSE;
                  seq.running = FALSE;
                  seq.finished = TRUE;
                  cm_msg(MTALK, "sequencer", "Sequencer is finished.");
               } else
                  seq.current_line_number++;
               
               db_set_record(hDB, hKeySeq, &seq, sizeof(seq), 0);
            }
         }
      } else {
         sprintf(str, "Invalid transition \"%s\"", mxml_get_value(pn));
         seq_error(str);
         return;
      }
   }
   
   /*---- Wait ----*/
   else if (equal_ustring(mxml_get_name(pn), "Wait")) {
      if (equal_ustring(mxml_get_attribute(pn, "for"), "Events")) {
         if (!eval_var(mxml_get_value(pn), str, sizeof(str)))
            return;
         n = atoi(str);
         seq.wait_limit = n;
         strcpy(seq.wait_type, "Events");
         size = sizeof(d);
         db_get_value(hDB, 0, "/Equipment/Trigger/Statistics/Events sent", &d, &size, TID_DOUBLE, FALSE);
         seq.wait_value = d;
         if (d >= n) {
            seq.current_line_number = mxml_get_line_number_end(pn) + 1;
            seq.wait_limit = 0;
            seq.wait_value = 0;
            seq.wait_type[0] = 0;
         }
         seq.wait_value = d;
      } else  if (equal_ustring(mxml_get_attribute(pn, "for"), "ODBValue")) {
         if (!eval_var(mxml_get_value(pn), str, sizeof(str)))
            return;
         v = atof(str);
         seq.wait_limit = v;
         strcpy(seq.wait_type, "ODB");
         if (!mxml_get_attribute(pn, "path")) {
            seq_error("\"path\" must be given for ODB values");
            return;
         } else {
            strlcpy(odbpath, mxml_get_attribute(pn, "path"), sizeof(odbpath));
            if (strchr(odbpath, '[')) {
               index = atoi(strchr(odbpath, '[')+1);
               *strchr(odbpath, '[') = 0;
            } else
               index = 0;
            status = db_find_key(hDB, 0, odbpath, &hKey);
            if (status != DB_SUCCESS) {
               sprintf(str, "Cannot find ODB key \"%s\"", odbpath);
               seq_error(str);
               return;
            } else {
               db_get_key(hDB, hKey, &key);
               size = sizeof(data);
               db_get_data_index(hDB, hKey, data, &size, index, key.type);
               db_sprintf(str, data, size, 0, key.type);
               seq.wait_value = atof(str);
               if (mxml_get_attribute(pn, "op"))
                  strlcpy(op, mxml_get_attribute(pn, "op"), sizeof(op));
               else
                  strcpy(op, ">=");
               strlcat(seq.wait_type, op, sizeof(seq.wait_type));
               
               cont = FALSE;
               if (equal_ustring(op, ">=")) {
                  cont = (seq.wait_value >= seq.wait_limit);
               } else if (equal_ustring(op, ">")) { 
                  cont = (seq.wait_value > seq.wait_limit);
               } else if (equal_ustring(op, "<=")) {
                  cont = (seq.wait_value >= seq.wait_limit);
               } else if (equal_ustring(op, "<")) {
                  cont = (seq.wait_value >= seq.wait_limit);
               } else if (equal_ustring(op, "==")) {
                  cont = (seq.wait_value >= seq.wait_limit);
               } else if (equal_ustring(op, "!=")) {
                  cont = (seq.wait_value >= seq.wait_limit);
               } else {
                  sprintf(str, "Invalid comaprison \"%s\"", op);
                  seq_error(str);
                  return;
               }
               
               if (cont) {
                  seq.current_line_number = mxml_get_line_number_end(pn) + 1;
                  seq.wait_limit = 0;
                  seq.wait_value = 0;
                  seq.wait_type[0] = 0;
               }
            }
         }
      } else if (equal_ustring(mxml_get_attribute(pn, "for"), "Seconds")) {
         if (!eval_var(mxml_get_value(pn), str, sizeof(str)))
            return;
         n = atoi(str);
         seq.wait_limit = n;
         strcpy(seq.wait_type, "Seconds");
         if (seq.start_time == 0) {
            seq.start_time = ss_time();
            seq.wait_value = 0;
         } else {
            seq.wait_value = ss_time() - seq.start_time;
            if (seq.wait_value > seq.wait_limit)
               seq.wait_value = seq.wait_limit;
         }
         if (ss_time() - seq.start_time > (DWORD)seq.wait_limit) {
            seq.current_line_number++;
            seq.start_time = 0;
            seq.wait_limit = 0;
            seq.wait_value = 0;
            seq.wait_type[0] = 0;
         }
      } else {
         sprintf(str, "Invalid wait attribute \"%s\"", mxml_get_attribute(pn, "for"));
         seq_error(str);
      }
   }
   
   /*---- Loop start ----*/
   else if (equal_ustring(mxml_get_name(pn), "Loop")) {
      for (i=0 ; i<4 ; i++)
         if (seq.loop_start_line[i] == 0)
            break;
      if (i == 4) {
         seq_error("Maximum loop nesting exceeded");
         return;
      }
      seq.loop_start_line[i] = seq.current_line_number;
      seq.loop_end_line[i] = mxml_get_line_number_end(pn);
      if (mxml_get_attribute(pn, "l"))
         seq.sloop_start_line[i] = atoi(mxml_get_attribute(pn, "l"));
      if (mxml_get_attribute(pn, "le"))
         seq.sloop_end_line[i] = atoi(mxml_get_attribute(pn, "le"));
      seq.loop_counter[i] = 1;
      if (mxml_get_attribute(pn, "var")) {
         if (!mxml_get_attribute(pn, "values")) {
            seq_error("Missing \"value\" attribute");
            return;
         }
         strlcpy(data, mxml_get_attribute(pn, "values"), sizeof(data));
         seq.loop_n[i] = strbreak(data, list, 100, ",", FALSE);
         
         strlcpy(name, mxml_get_attribute(pn, "var"), sizeof(name));
         if (!eval_var(list[0], value, sizeof(value)))
            return;
         sprintf(str, "/Sequencer/Variables/%s", name);
         db_set_value(hDB, 0, str, value, strlen(value)+1, 1, TID_STRING);
         
      } else if (mxml_get_attribute(pn, "n")) {
         if (equal_ustring(mxml_get_attribute(pn, "n"), "infinite"))
            seq.loop_n[i] = -1;
         else {
            if (!eval_var(mxml_get_attribute(pn, "n"), value, sizeof(value)))
               return;
            seq.loop_n[i] = atoi(value);
         }
      }
      seq.current_line_number++;
   }
   
   /*---- If ----*/
   else if (equal_ustring(mxml_get_name(pn), "If")) {
      strlcpy(str, mxml_get_attribute(pn, "condition"), sizeof(str));
      i = eval_condition(str);
      if (i < 0) {
         seq_error("Invalid number in comparison");
         return;
      }
      if (i == 1)
         seq.current_line_number++;
      else
         seq.current_line_number = mxml_get_line_number_end(pn) + 1;
   }

   /*---- Goto ----*/
   else if (equal_ustring(mxml_get_name(pn), "Goto")) {
      if (!mxml_get_attribute(pn, "line") && !mxml_get_attribute(pn, "sline")) {
         seq_error("Missing line number");
         return;
      }
      if (mxml_get_attribute(pn, "line")) {
         if (!eval_var(mxml_get_attribute(pn, "line"), str, sizeof(str)))
            return;
         seq.current_line_number = atoi(str);
      }
      if (mxml_get_attribute(pn, "sline")) {
         if (!eval_var(mxml_get_attribute(pn, "sline"), str, sizeof(str)))
            return;
         for (i=0 ; i<last_line ; i++) {
            pt = mxml_get_node_at_line(pnseq, i);
            if (pt && mxml_get_attribute(pt, "l")) {
               l = atoi(mxml_get_attribute(pt, "l"));
               if (atoi(str) == l) {
                  seq.current_line_number = i;
                  break;
               }
            }
         } 
      }
   }

   /*---- Library ----*/
   else if (equal_ustring(mxml_get_name(pn), "Library")) {
      // simply skip libraries
      seq.current_line_number = mxml_get_line_number_end(pn) + 1;
   }
   
   /*---- Subroutine ----*/
   else if (equal_ustring(mxml_get_name(pn), "Subroutine")) {
      // simply skip subroutines
      seq.current_line_number = mxml_get_line_number_end(pn) + 1;
   }

   /*---- Param ----*/
   else if (equal_ustring(mxml_get_name(pn), "Param")) {
      // simply skip parameters
      seq.current_line_number = mxml_get_line_number_end(pn) + 1;
   }

   /*---- Set ----*/
   else if (equal_ustring(mxml_get_name(pn), "Set")) {
      if (!mxml_get_attribute(pn, "name")) {
         seq_error("Missing variable name");
         return;
      }
      strlcpy(name, mxml_get_attribute(pn, "name"), sizeof(name));
      if (!eval_var(mxml_get_value(pn), value, sizeof(value)))
         return;
      sprintf(str, "/Sequencer/Variables/%s", name);
      db_set_value(hDB, 0, str, value, strlen(value)+1, 1, TID_STRING);
      
      seq.current_line_number = mxml_get_line_number_end(pn)+1;
   }

   /*---- Message ----*/
   else if (equal_ustring(mxml_get_name(pn), "Message")) {
      if (!eval_var(mxml_get_value(pn), value, sizeof(value)))
         return;
      if (!seq.message_wait)
         strlcpy(seq.message, value, sizeof(seq.message));
      if (mxml_get_attribute(pn, "wait")) {
         if (atoi(mxml_get_attribute(pn, "wait")) == 1) {
            seq.message_wait = TRUE;
            db_set_record(hDB, hKeySeq, &seq, sizeof(seq), 0);
            if (seq.message[0] != 0)
               return; // don't increment line number until message is cleared from within browser
            seq.message_wait = FALSE;
         }
      }
      seq.current_line_number = mxml_get_line_number_end(pn)+1;
   }

   /*---- Cat ----*/
   else if (equal_ustring(mxml_get_name(pn), "Cat")) {
      if (!mxml_get_attribute(pn, "name")) {
         seq_error("Missing variable name");
         return;
      }
      strlcpy(name, mxml_get_attribute(pn, "name"), sizeof(name));
      if (!concatenate(value, sizeof(value), mxml_get_value(pn)))
         return;
      sprintf(str, "/Sequencer/Variables/%s", name);
      db_set_value(hDB, 0, str, value, strlen(value)+1, 1, TID_STRING);
      
      seq.current_line_number = mxml_get_line_number_end(pn)+1;
   }

   /*---- Call ----*/
   else if (equal_ustring(mxml_get_name(pn), "Call")) {
      if (seq.stack_index == 4) {
         seq_error("Maximum subroutine level exceeded");
         return;
      } else {
         // put current line number on stack
         seq.subroutine_call_line[seq.stack_index] = mxml_get_line_number_end(pn);
         seq.ssubroutine_call_line[seq.stack_index] = atoi(mxml_get_attribute(pn, "l"));
         seq.subroutine_return_line[seq.stack_index] = mxml_get_line_number_end(pn)+1;
         
         // search subroutine
         for (i=1 ; i<mxml_get_line_number_end(mxml_find_node(pnseq, "RunSequence")) ; i++) {
            pt = mxml_get_node_at_line(pnseq, i);
            if (pt) {
               if (equal_ustring(mxml_get_name(pt), "Subroutine")) {
                  if (equal_ustring(mxml_get_attribute(pt, "name"), mxml_get_attribute(pn, "name"))) {
                     // put routine end line on end stack
                     seq.subroutine_end_line[seq.stack_index] = mxml_get_line_number_end(pt);
                     // go to first line of subroutine
                     seq.current_line_number = mxml_get_line_number_start(pt) + 1;
                     // put parameter(s) on stack
                     if (mxml_get_value(pn))
                        strlcpy(seq.subroutine_param[seq.stack_index], mxml_get_value(pn), 256);
                     // increment stack
                     seq.stack_index ++;
                     break;
                  }
               }
            }
         }
         if (i == mxml_get_line_number_end(mxml_find_node(pnseq, "RunSequence"))) {
            sprintf(str, "Subroutine '%s' not found", mxml_get_attribute(pn, "name"));
            seq_error(str);
         }
      }
   }
   
   /*---- <unknown> ----*/   
   else {
      sprintf(str, "Unknown statement \"%s\"", mxml_get_name(pn));
      seq_error(str);
   }
   
   /* set MSL line from current element */
   pn = mxml_get_node_at_line(pnseq, seq.current_line_number);
   if (pn) { 
      /* check if node belongs to library */
      pt = mxml_get_parent(pn);
      while (pt) {
         if (equal_ustring(mxml_get_name(pt), "Library"))
            break;
         pt = mxml_get_parent(pt);
      }
      if (pt)
         seq.scurrent_line_number = -1;
      else if(mxml_get_attribute(pn, "l"))
         seq.scurrent_line_number = atoi(mxml_get_attribute(pn, "l"));
   }

   /* update current line number */
   db_set_record(hDB, hKeySeq, &seq, sizeof(seq), 0);
}

/**dox***************************************************************/
/** @} *//* end of alfunctioncode */

