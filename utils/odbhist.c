/********************************************************************\

  Name:         odbhist.c
  Created by:   Stefan Ritt, Ilia Chachkhunashvili

  Contents:     MIDAS history display utility

  $Id$

\********************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>

#ifdef _MSC_VER
#include <io.h>
#else
#include <unistd.h>
#endif

/*------------------------------------------------------------------*/

#define SUCCESS 1
#define TRUE    1
#define FALSE   0

typedef unsigned int DWORD;
typedef int INT;
typedef int BOOL;
typedef char str[256];

/*------------------------------------------------------------------*/

str var_params[100];            /* varables to print 100 each of 256 char lenth */

DWORD status, start_run, end_run, run;
INT i, quiet, add, j, k, eoln, boln;
char file_name[256], var_name[256], mid_name[256];
double total[100], value[100];

/*------------------------------------------------------------------*/

BOOL equal_ustring(char *str1, char *str2)
{
   if (str1 == NULL && str2 != NULL)
      return FALSE;
   if (str1 != NULL && str2 == NULL)
      return FALSE;
   if (str1 == NULL && str2 == NULL)
      return TRUE;

   while (*str1)
      if (toupper(*str1++) != toupper(*str2++))
         return FALSE;

   if (*str2)
      return FALSE;

   return TRUE;
}

/*------------------------------------------------------------------*/

int odb_hist(char *file_name, int run_number, char *var_name, int quiet,
             double *value, int eoln, int boln, int print)
/********************************************************************\

  Routine: odb_hist

  Purpose: Print ODB variable for a single .odb file

  Input:
    char *file_name         Name of ODB file
    INT  run_number         Run number for ODB file
    char *var_name          Name of variable to print
    int  quiet              If set, don't print run number
    int eoln                end of line used to detemine when to print "\n"
    int boln                begin of line used to determin when to print 
                            run number if it's needed
    int print               used to print or not the value of variable. 
                            This it's used for "Emils' function", so it 
                            will take and return the value of variable 
                            without printing it.
    
  Note:    
    There are two variables eoln and boln because there can be begin 
    and end of line together when we have just one variable. 

  Output:
    double *value           ODB value in double format

  Function value:
    SUCCESS                 Successful completion

\********************************************************************/
{
   FILE *f;
   char str[256], path[256], key_name[256], line[256];
   int i, index, a, k;

   /* assemble file name */
   sprintf(str, file_name, run_number);

   /* split var_name into path and key with index */
   strcpy(path, "");
   for (i = strlen(var_name) - 1; i >= 0 && var_name[i] != '/'; i--);
   if (var_name[i] == '/') {
      strcpy(path, "[");
      strcat(path, var_name);
      path[i + 1] = 0;
      strcat(path, "]\n");
      strcpy(key_name, var_name + i + 1);
   }
   if (strchr(key_name, '[')) {
      index = atoi(strchr(key_name, '[') + 1);
      *strchr(key_name, '[') = 0;
   } else
      index = -1;

   f = fopen(str, "r");
   if (f == NULL)
      return 0;

   if ((!quiet) && boln && print)
      printf("%5d: ", run_number);

   /* search path */
   do {
      fgets(line, sizeof(line), f);
      if (line[0] == '[')
         if (equal_ustring(line, path)) {
            /* look for key */
            do {
               fgets(line, sizeof(line), f);
               if (strchr(line, '=') != NULL) {
                  strcpy(str, line);
                  *(strchr(str, '=') - 1) = 0;

                  /* check if key name matches */
                  if (equal_ustring(str, key_name)) {
                     if (index == -1) {
                        /* non-arrays */
                        strcpy(str, strchr(line, '=') + 2);
                        if (strchr(str, ':') != NULL) {
                           strcpy(str, strchr(str, ':') + 2);
                           if (strchr(str, '\n') != NULL)
                              *strchr(str, '\n') = 0;
                           if (str[0] == '[' && strchr(str, ']') != NULL)
                              strcpy(str, strchr(str, ']') + 2);
                           if (print)
                              printf(str);
                           *value = strtod(str, NULL);
                           goto finish;
                        }
                     } else {
                        /* arrays */
                        for (i = 0; i <= index; i++)
                           fgets(line, sizeof(line), f);
                        if (line[0] == '[' && atoi(line + 1) == index) {
                           strcpy(str, strchr(line, ']') + 2);
                           if (strchr(str, '\n') != NULL)
                              *strchr(str, '\n') = 0;
                           if (print)
                              printf(str);
                           *value = strtod(str, NULL);
                        }
                        goto finish;
                     }

                  }
               }

            } while (line[0] != '[' || line[1] != '/');

         }
   } while (!feof(f));

 finish:
   if (print) {
      if (eoln)
         printf("\n");
      else {
         a = 0;
         while (str[a] != '\0')
            a++;
         k = a;
         while ((str[k] != '.') && (k >= 0))
            k--;
         for (i = 0; i < (10 - (a - k)); i++)
            printf(" ");
         printf("\t");
      }
      fclose(f);
   }

   return SUCCESS;
}

/*------------------------------------------------------------------*/

int load_pars_from_file(char filename[256])
/********************************************************************\

  Routine: load_pars_from_file

  Purpose: Load parameters for odbhist from file

  Input:
    char *filename          Name of configuration file

  Output:
    <implicit through global variables>

  Function value:
    1                       Successful completion
    0                       Error

\********************************************************************/
{
   FILE *f1;
   char line[256];
   int result;
   int getstr;

   getstr = 1;

   f1 = fopen(filename, "r");
   if (f1 != NULL) {
      result = 1;
      while (result != 0) {
         if (getstr) {
            if (fgets(line, sizeof(line), f1) == NULL)
               break;
         } else
            getstr = 1;

         if (line[0] == '[') {
            switch (line[1]) {
            case 'a':
               if (fgets(line, sizeof(line), f1) != NULL) {
                  switch (line[0]) {
                  case '1':
                     add = TRUE;
                     break;
                  case '0':
                     add = FALSE;
                     break;
                  default:
                     result = 0;
                  }
               } else
                  result = 0;
               break;

            case 'q':
               if (fgets(line, sizeof(line), f1) != NULL) {
                  switch (line[0]) {
                  case '1':
                     quiet = TRUE;
                     break;
                  case '0':
                     quiet = FALSE;
                     break;
                  default:
                     result = 0;
                  }
               } else
                  result = 0;
               break;

            case 'f':
               if ((fgets(line, sizeof(line), f1) != NULL) && (line[0] != '['))
                  strcpy(file_name, line);
               else
                  result = 0;
               break;

            case 'v':
               j = -1;
               while (fgets(line, sizeof(line), f1) != NULL && line[0] != '[') {
                  if (line[0] != '\n')
                     strcpy(var_params[++j], line);
               }
               if (j == -1)
                  result = 0;
               else
                  getstr = 0;

               /* to get correct number of variables in "j" global variable */
               if (line[0] != '[')
                  j--;
               break;

            case 'r':
               if ((fgets(line, sizeof(line), f1) != NULL) && (line[0] != '['))
                  start_run = atoi(line);
               else {
                  result = 0;
                  break;
               }
               if ((fgets(line, sizeof(line), f1) != NULL) && (line[0] != '['))
                  end_run = atoi(line);
               else {
                  result = 0;
                  break;
               }

               break;

            default:
               result = 0;
            }
         }
      }                         /* while */
   } /* if */
   else {
      result = 0;
      printf("\n ERROR:\nCan't open file %s\n", filename);
   }
   if (result != 0)
      if (fclose(f1))
         result = 0;

   return result;
}

/*------------------------------------------------------------------*/

typedef struct {
   short int event_id;           /**< event ID starting from one      */
   short int trigger_mask;       /**< hardware trigger mask           */
   DWORD serial_number;          /**< serial number starting from one */
   DWORD time_stamp;             /**< time of production of event     */
   DWORD data_size;              /**< size of event in bytes w/o header */
} EVENT_HEADER;

#define EVENTID_BOR      ((short int) 0x8000)  /**< Begin-of-run      */
#define EVENTID_EOR      ((short int) 0x8001)  /**< End-of-run        */
#define EVENTID_MESSAGE  ((short int) 0x8002)  /**< Message events    */

int extract(char *mid_file, char *odb_file)
/********************************************************************\

  Routine: extract

  Purpose: Extract ODB file from MID file

  Input:
    char *mid_file          Midas file name, usually runxxxxx.mid
    char *odb_file          ODB file name, usually runxxxxx.odb

  Function value:
    1                       Successful completion
    0                       Error

\********************************************************************/
{
   int fhm, fho, run_number;
   unsigned int n;
   EVENT_HEADER header;
   char *buffer, *p, odb_name[256];

   fhm = open(mid_file, O_RDONLY, 0644);
   if (fhm < 0) {
      printf("Cannot open file \"%s\"\n", mid_file);
      return 0;
   }

   if (strchr(odb_file, '%')) {
      p = mid_file;
      while (*p && !isdigit(*p))
         p++;
      run_number = atoi(p);
      sprintf(odb_name, odb_file, run_number);
   } else
      strcpy(odb_name, odb_file);

   fho = open(odb_name, O_WRONLY | O_CREAT | O_APPEND, 0644);
   if (fho < 0) {
      printf("Cannot open file \"%s\"\n", odb_name);
      return 0;
   }

   n = read(fhm, &header, sizeof(header));
   if (n != sizeof(header)) {
      printf("Cannot read event header from file \"%s\"\n", mid_file);
      return 0;
   }

   if (header.event_id != EVENTID_BOR) {
      printf("First event in \"%s\" is not a BOR event\n", mid_file);
      return 0;
   }

   buffer = malloc(header.data_size);

   n = read(fhm, buffer, header.data_size);
   if (n < header.data_size) {
      printf("Cannot read %d bytes from \"%s\"\n", header.data_size, mid_file);
      return 0;
   }

   n = write(fho, buffer, header.data_size);
   if (n < header.data_size) {
      printf("Cannot write %d bytes to \"%s\"\n", header.data_size, odb_name);
      return 0;
   }

   close(fhm);
   close(fho);
   free(buffer);

   printf("\"%s\" successfully created\n", odb_name);
   return 1;
}

/*------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
   int cfg, print, n_files;

   strcpy(var_name, "/Runinfo/Run number");
   strcpy(file_name, "run%05d.odb");
   start_run = end_run = 0;
   quiet = FALSE;
   add = FALSE;
   print = 1;                   /* print = 1 means that variable will be printed */

   k = 0;
   cfg = 0;
   j = -1;
   n_files = 0;
   mid_name[0] = 0;

   /* parse command line parameters */

   for (i = 1; i < argc; i++) {
      if (argv[i][0] == '-')
         if (argv[i][1] == 'c') {
            printf(argv[i + 1]);
            printf("\n");
            if (!(load_pars_from_file(argv[i + 1])))
               goto usage;
            else
               cfg = 1;
         }
   }

   if (argc <= 1)
      goto usage;

   for (i = 1; i < argc; i++) {
      if (argv[i][0] == '-') {
         if (argv[i][1] == 'q')
            quiet = TRUE;
         else if (argv[i][1] == 'a')
            add = TRUE;
         else {
            if (i + 1 >= argc || argv[i + 1][0] == '-')
               goto usage;
            if (argv[i][1] == 'r') {
               start_run = atoi(argv[++i]);
               end_run = atoi(argv[++i]);
            } else if (argv[i][1] == 'v') {
               j = -1;
               while (i + 1 < argc && argv[i + 1][0] != '-')
                  if (argv[i + 1][0] != '-') {
                     i++;
                     j++;
                     if (argv[i][0] != '-')
                        strcpy(var_params[j], argv[i]);
                  }
            } else if (argv[i][1] == 'f')
               strcpy(file_name, argv[++i]);
            else if (argv[i][1] == 'e')
               strcpy(mid_name, argv[++i]);
            else
               goto usage;
         }
      } else if (!cfg) {

       usage:
         printf("\nusage: odbhist -r <run1> <run2> -v <varname>[index]\n");
         printf("       [-f <filename>] [-q] [-a] [-c <file>] [-e <file>]\n");
         printf("       <run1> <run2>    Range of run numbers (inclusive)\n");
         printf
             ("       <varname>        ODB variable name like \"/Runinfo/Run number\"\n");
         printf("       [index]          Index if <varname> is an array\n");
         printf("       <filename>       run%%05d.odb by default\n");
         printf("       -e <file>        Extract ODB file from MID file\n");
         printf("       -q               Don't display run number\n");
         printf("       -a               Add numbers for all runs\n");
         printf("       -c               load configuration from file\n");
         printf("                        (parameters loaded from cfg file will be\n");
         printf("                        overwriten by parameters from command line)\n");
         return 0;

      }
   }

   if (mid_name[0]) {
      extract(mid_name, file_name);
      return 1;
   }

   if (end_run < start_run) {
      printf("Second run is %d and must be larger or equal than first run %d\n",
             end_run, start_run);
      return 0;
   }

   if (j == -1)
      goto usage;

   /* printing of header is needed here */
   for (run = start_run; run <= end_run; run++) {
      for (k = 0; k <= j; k++) {
         if (k == j)
            eoln = 1;
         else
            eoln = 0;

         if (k == 0)
            boln = 1;
         else
            boln = 0;

         strcpy(var_name, var_params[k]);
         value[k] = 0;

         status = odb_hist(file_name, run, var_name, quiet, &value[k], eoln, boln, print);
         if (status != SUCCESS)
            break;

         total[k] += value[k];
         n_files++;
      }                         /*for k */
   }                            /* for run */

   if (add) {
      printf("\nTotal: ");
      if (quiet)
         printf("\n");
      for (k = 0; k <= j; k++)
         printf("%lf\t", total[k]);
      printf("\n");
   }

   if (n_files == 0) {
      printf("No files found in selected range\n");
   }

   return 1;
}
