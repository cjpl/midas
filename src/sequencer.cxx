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
   BOOL  running;
   BOOL  finished;
   BOOL  paused;
   int   current_line_number;
   BOOL  stop_after_run;
   BOOL  transition_request;
   int   loop_start_line[4];
   int   loop_end_line[4];
   int   loop_counter[4];
   int   loop_n[4];
   char  subdir[256];
   int   subdir_end_line;
   int   subdir_not_notify;
   int   stack_index;
   int   subroutine_end_line[4];
   int   subroutine_return_line[4];
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
"Running = BOOL : n",\
"Finished = BOOL : y",\
"Paused = BOOL : n",\
"Current line number = INT : 0",\
"Stop after run = BOOL : n",\
"Transition request = BOOL : n",\
"Loop start line = INT[4] :",\
"[0] 0",\
"[1] 0",\
"[2] 0",\
"[3] 0",\
"Loop end line = INT[4] :",\
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
   db_open_record(hDB, hKey, &seq, sizeof(seq), MODE_READ, NULL, NULL);
   if (strlen(seq.path)>0 && seq.path[strlen(seq.path)-1] != DIR_SEPARATOR) {
      strlcat(seq.path, DIR_SEPARATOR_STR, sizeof(seq.path));
   }
   
   if (seq.filename[0]) {
      strlcpy(str, seq.path, sizeof(str));
      strlcat(str, seq.filename, sizeof(str));
      seq.error[0] = 0;
      seq.error_line = 0;
      if (pnseq) {
         mxml_free_tree(pnseq);
         pnseq = NULL;
      }
      pnseq = mxml_parse_file(str, seq.error, sizeof(seq.error));
      if (seq.error[0])
         seq.error_line = atoi(seq.error+21);
   }
   
   seq.transition_request = FALSE;
   
   db_set_record(hDB, hKey, &seq, sizeof(seq), 0);
}
/*------------------------------------------------------------------*/

void seq_error(const char *str)
{
   HNDLE hDB, hKey;
   
   strlcpy(seq.error, str, sizeof(seq.error));
   seq.error_line = seq.current_line_number;
   seq.running = FALSE;
   seq.transition_request = FALSE;

   cm_get_experiment_database(&hDB, NULL);
   db_find_key(hDB, 0, "/Sequencer/State", &hKey);
   assert(hKey);
   db_set_record(hDB, hKey, &seq, sizeof(seq), 0);
   
   cm_msg(MTALK, "sequencer", "Sequencer has stopped with error.");
}

/*------------------------------------------------------------------*/

BOOL evaluate(const char *value, char *result, int size)
{
   HNDLE hDB;
   const char *p;
   char *s;
   char str[256];
   int index, i;
   
   cm_get_experiment_database(&hDB, NULL);
   p = value;
   
   while (*p == ' ')
      p++;
   
   if (*p == '$') {
      p++;
      if (atoi(p) > 0) {
         index = atoi(p);
         if (seq.stack_index > 0) {
            strlcpy(str, seq.subroutine_param[seq.stack_index-1], sizeof(str));
            s = strtok(str, ",");
            if (s == NULL) {
               if (index == 1) {
                  strlcpy(result, str, size);
                  return TRUE;
               } else
                  goto error;
            }
            for (i=1; s != NULL && i<index; i++)
               s = strtok(NULL, ",");
            if (s != NULL) {
               strlcpy(result, s, size);
               return TRUE;
            }
            else 
               goto error;
         } else
            goto error;
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

void seq_start_page()
{
   int i, j, n, size, last_line, status, maxlength;
   HNDLE hDB, hkey, hsubkey, hkeycomm, hkeyc;
   KEY key;
   char data[1000], str[32], name[32];
   char data_str[256], comment[1000];
   MXML_NODE *pn;
   
   cm_get_experiment_database(&hDB, NULL);
   
   show_header(hDB, "Start sequence", "GET", "", 1, 0);
   rsprintf("<tr><th bgcolor=#A0A0FF colspan=2>Start script</th>\n");
   
   /* run parameters from ODB */
   db_find_key(hDB, 0, "/Experiment/Edit on sequence", &hkey);
   db_find_key(hDB, 0, "/Experiment/Parameter Comments", &hkeycomm);
   if (hkey) {
      for (i = 0, n = 0;; i++) {
         db_enum_link(hDB, hkey, i, &hsubkey);
         
         if (!hsubkey)
            break;
         
         db_get_link(hDB, hsubkey, &key);
         strlcpy(str, key.name, sizeof(str));
         
         if (equal_ustring(str, "Edit run number"))
            continue;
         
         db_enum_key(hDB, hkey, i, &hsubkey);
         db_get_key(hDB, hsubkey, &key);
         
         size = sizeof(data);
         status = db_get_data(hDB, hsubkey, data, &size, key.type);
         if (status != DB_SUCCESS)
            continue;
         
         for (j = 0; j < key.num_values; j++) {
            if (key.num_values > 1)
               rsprintf("<tr><td>%s [%d]", str, j);
            else
               rsprintf("<tr><td>%s", str);
            
            if (j == 0 && hkeycomm) {
               /* look for comment */
               if (db_find_key(hDB, hkeycomm, key.name, &hkeyc) == DB_SUCCESS) {
                  size = sizeof(comment);
                  if (db_get_data(hDB, hkeyc, comment, &size, TID_STRING) == DB_SUCCESS)
                     rsprintf("<br>%s\n", comment);
               }
            }
            
            db_sprintf(data_str, data, key.item_size, j, key.type);
            
            maxlength = 80;
            if (key.type == TID_STRING)
               maxlength = key.item_size;
            
            if (key.type == TID_BOOL) {
               if (((DWORD*)data)[j])
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
      
      for (i=1 ; i<last_line ; i++){
         pn = mxml_get_node_at_line(pnseq, i);
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
            
            if (mxml_get_attribute(pn, "type") && equal_ustring(mxml_get_attribute(pn, "type"), "bool")) {
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

void show_seq_page()
{
   INT i, size, n, width, state, eob, last_line;
   HNDLE hDB;
   char str[256], path[256], dir[256], error[256], comment[256], filename[256], data[256], buffer[10000], line[256], name[32];
   time_t now;
   char *flist = NULL, *pc, *pline;
   PMXML_NODE pn;
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
      if (pnseq) {
         mxml_free_tree(pnseq);
         pnseq = NULL;
      }
      pnseq = mxml_parse_file(str, seq.error, sizeof(seq.error));
      if (seq.error[0])
         seq.error_line = atoi(seq.error+21);
      else
         seq.error_line = 0;
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
         if (hparam) {
            for (i = 0, n = 0;; i++) {
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
            seq.loop_end_line[i] = 0;
            seq.loop_counter[i] = 0;
            seq.loop_n[i] = 0;
         }
         seq.current_line_number = 1;
         seq.stack_index = 0;
         seq.error[0] = 0;
         seq.error_line = 0;
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
      f = fopen(str, "wt");
      if (f && isparam("scripttext")) {
         fwrite(getparam("scripttext"), strlen(getparam("scripttext")), 1, f);
         fclose(f);
      }
      strlcpy(str, seq.path, sizeof(str));
      strlcat(str, seq.filename, sizeof(str));
      seq.error[0] = 0;
      if (pnseq) {
         mxml_free_tree(pnseq);
         pnseq = NULL;
      }
      pnseq = mxml_parse_file(str, seq.error, sizeof(seq.error));
      if (seq.error[0])
         seq.error_line = atoi(seq.error+21);
      else
         seq.error_line = -1;
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
   rsprintf("var last_msg = null;\n");
   rsprintf("\n");
   rsprintf("function seq_refresh()\n");
   rsprintf("{\n");
   rsprintf("   seq = ODBGetRecord('/Sequencer/State');\n");
   rsprintf("   var current_line = ODBExtractRecord(seq, 'Current line number');\n");
   rsprintf("   var error_line = ODBExtractRecord(seq, 'Error line');\n");
   rsprintf("   var wait_value = ODBExtractRecord(seq, 'Wait value');\n");
   rsprintf("   var wait_limit = ODBExtractRecord(seq, 'Wait limit');\n");
   rsprintf("   var wait_type = ODBExtractRecord(seq, 'Wait type');\n");
   rsprintf("   var loop_n = ODBExtractRecord(seq, 'Loop n');\n");
   rsprintf("   var loop_counter = ODBExtractRecord(seq, 'Loop counter');\n");
   rsprintf("   var loop_start_line = ODBExtractRecord(seq, 'Loop start line');\n");
   rsprintf("   var loop_end_line = ODBExtractRecord(seq, 'Loop end line');\n");
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
   rsprintf("   if (current_line < last_line-10 && !show_all_lines)\n");
   rsprintf("       document.getElementById('linedots2').style.display = 'inline';\n");
   rsprintf("   else\n");
   rsprintf("       document.getElementById('linedots2').style.display = 'none';\n");
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
   rsprintf("      if (wait_type == '')\n");
   rsprintf("         document.getElementById('wait_row').style.display = 'none';\n");
   rsprintf("      else\n");
   rsprintf("         document.getElementById('wait_row').style.display = 'visible';\n");
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
         rsprintf("<option onClick=\"document.form1.submit()\">[..]</option>\n");
      for (i=0 ; i<n ; i++) {
         if (flist[i*MAX_STRING_LENGTH] != '.')
            rsprintf("<option onClick=\"document.form1.submit()\">[%s]</option>\n", flist+i*MAX_STRING_LENGTH);
      }
      
      /*---- go over XML files in sequencer directory ----*/
      n = ss_file_find(path, (char *)"*.xml", &flist);
      for (i=0 ; i<n ; i++) {
         strlcpy(str, path, sizeof(str));
         strlcat(str, flist+i*MAX_STRING_LENGTH, sizeof(str));
         comment[0] = 0;
         if (pnseq) {
            mxml_free_tree(pnseq);
            pnseq = NULL;
         }
         pnseq = mxml_parse_file(str, error, sizeof(error));
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
      
      free(flist);
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
            rsprintf("<tr><td colspan=2><textarea rows=30 cols=80 name=\"scripttext\">\n");
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
               rsprintf("<tr id=\"wait_row\"><td colspan=2>\n");
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
            strlcpy(str, seq.path, sizeof(str));
            strlcat(str, seq.filename, sizeof(str));
            f = fopen(str, "rt");
            if (f) {
               rsprintf("<tr><td colspan=2>Filename:<b>%s</b></td></tr>\n", seq.filename);
               if (seq.error[0]) {
                  rsprintf("<tr><td bgcolor=red colspan=2><b>");
                  strencode(seq.error);
                  rsprintf("</b></td></tr>\n");
               }
               rsprintf("<tr><td colspan=2>\n");
               rsprintf("<div onClick=\"show_lines();\" id=\"linedots1\" style=\"display:none;\">...<br></div>\n");
               for (int line=1 ; !feof(f) ; line++) {
                  str[0] = 0;
                  pc = fgets(str, sizeof(str), f);
                  if (pc == NULL)
                     break;
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
                     rsprintf("%d ", line);
                     strencode4(str);
                     rsprintf("</font>");
                  }
               }
               rsprintf("<div onClick=\"show_lines();\" id=\"linedots2\" style=\"display:none;\">...<br></div>\n");
               rsprintf("</tr></td>\n");
               fclose(f);
            } else {
               if (str[0]) {
                  rsprintf("<tr><td colspan=2><b>Cannot open file \"%s\"</td></tr>\n", str);
               }
            }
            
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
   int i, n, status, size, index, last_line, state, run_number, cont;
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
   if (!pr)
      return;
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
            seq.loop_end_line[i] = 0;
            seq.loop_n[i] = 0;
            seq.current_line_number++;
         } else {
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
   
   if (equal_ustring(mxml_get_name(pn), "PI") || 
       equal_ustring(mxml_get_name(pn), "RunSequence") ||
       equal_ustring(mxml_get_name(pn), "Comment")) {
      // just skip
      seq.current_line_number++;
      db_set_record(hDB, hKeySeq, &seq, sizeof(seq), 0);
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
         db_set_record(hDB, hKeySeq, &seq, sizeof(seq), 0);
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
         if (strchr(odbpath, '[')) {
            index = atoi(strchr(odbpath, '[')+1);
            *strchr(odbpath, '[') = 0;
         } else
            index = 0;
         if (!evaluate(mxml_get_value(pn), value, sizeof(value)))
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
            
            status = db_set_data_index2(hDB, hKey, data, key.item_size, index, key.type, notify);
            seq.current_line_number++;
         }
         db_set_record(hDB, hKeySeq, &seq, sizeof(seq), 0);
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
         if (!evaluate(mxml_get_value(pn), value, sizeof(value)))
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
         db_set_record(hDB, hKeySeq, &seq, sizeof(seq), 0);
      }
   }
   
   /*---- RunDescription ----*/
   else if (equal_ustring(mxml_get_name(pn), "RunDescription")) {
      db_set_value(hDB, 0, "/Experiment/Run Parameters/Run Description", mxml_get_value(pn), 256, 1, TID_STRING);
      seq.current_line_number++;
      db_set_record(hDB, hKeySeq, &seq, sizeof(seq), 0);
   }
   
   /*---- Script ----*/
   else if (equal_ustring(mxml_get_name(pn), "Script")) {
      if (mxml_get_attribute(pn, "loop_counter"))
         sprintf(str, "%s %d %d %d %d", mxml_get_value(pn), seq.loop_counter[0], seq.loop_counter[1], seq.loop_counter[2], seq.loop_counter[3]);
      else
         sprintf(str, "%s", mxml_get_value(pn));
      ss_system(str);
      seq.current_line_number++;
      db_set_record(hDB, hKeySeq, &seq, sizeof(seq), 0);
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
      db_set_record(hDB, hKeySeq, &seq, sizeof(seq), 0);
   }
   
   /*---- Wait ----*/
   else if (equal_ustring(mxml_get_name(pn), "Wait")) {
      if (equal_ustring(mxml_get_attribute(pn, "for"), "Events")) {
         if (!evaluate(mxml_get_value(pn), str, sizeof(str)))
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
         if (!evaluate(mxml_get_value(pn), str, sizeof(str)))
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
         if (!evaluate(mxml_get_value(pn), str, sizeof(str)))
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
      db_set_record(hDB, hKeySeq, &seq, sizeof(seq), 0);
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
      seq.loop_counter[i] = 1;
      if (equal_ustring(mxml_get_attribute(pn, "n"), "infinite"))
         seq.loop_n[i] = -1;
      else
         seq.loop_n[i] = atoi(mxml_get_attribute(pn, "n"));
      seq.current_line_number++;
      db_set_record(hDB, hKeySeq, &seq, sizeof(seq), 0);
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
      strlcpy(value, mxml_get_value(pn), sizeof(value));
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
}



/**dox***************************************************************/
/** @} *//* end of alfunctioncode */

