 /********************************************************************\

  Name:         odbhist.c
  Created by:   Stefan Ritt

  Contents:     MIDAS history display utility

  $Log$
  Revision 1.1  1999/08/23 15:22:39  midas
  Added file


\********************************************************************/

#include "midas.h"
#include "msystem.h"

int odb_hist(char *file_name, int run_number, char *var_name, int quiet)
{
FILE *f;
char str[256], path[256], key_name[256], line[256];
int  i;

  /* assemble file name */
  sprintf(str, file_name, run_number);

  /* split var_name into path and key */
  strcpy(path, "");
  for (i=strlen(var_name)-1 ; i>=0 && var_name[i] != '/' ; i--);
  if (var_name[i] == '/')
    {
    strcpy(path, "[");
    strcat(path, var_name);
    path[i+1] = 0;
    strcat(path, "]\n");
    strcpy(key_name, var_name+i+1);
    }

  f = fopen(str, "r");
  if (f != NULL)
    {
    if (!quiet)
      printf("%5d: ", run_number);

    /* search path */
    do
      {
      fgets(line, sizeof(line), f);
      if (line[0] == '[')
        if (equal_ustring(line, path))
          {
          /* look for key */
          do
            {
            fgets(line, sizeof(line), f);
            if (strchr(line, '=') != NULL)
              {
              strcpy(str, line);
              *(strchr(str, '=')-1) = 0;
              if (equal_ustring(str, key_name))
                {
                strcpy(str, strchr(line, '=')+2);
                if (strchr(str, ':') != NULL)
                  {
                  strcpy(str, strchr(str, ':')+2);
                  if (strchr(str, '\n') != NULL)
                    *strchr(str, '\n') = 0;
                  if (str[0] == '[' && strchr(str, ']') != NULL)
                    strcpy(str, strchr(str, ']')+2);
                  printf(str);
                  goto finish;
                  }

                }
              }

            } while (line[0] != '[' || line[1] != '/');

          }
      } while (!feof(f));

finish:
    printf("\n");
    fclose(f);
    }
  
  return SUCCESS;
}

/*------------------------------------------------------------------*/

main(int argc, char *argv[])
{
DWORD    status, start_run, end_run, run;
INT      i, quiet;
char     file_name[256], var_name[256];

  /* turn off system message */
  cm_set_msg_print(0, MT_ALL, puts);

  strcpy(var_name, "/Runinfo/Run number");
  strcpy(file_name, "run%05d.odb");
  start_run = end_run = 0;
  quiet = FALSE;

  /* parse command line parameters */
  for (i=1 ; i<argc ; i++)
    {
    if (argv[i][0] == '-')
      {
      if (argv[i][1] == 'q')
        quiet = TRUE;
      else
        {
        if (i+1 >= argc || argv[i+1][0] == '-')
          goto usage;
        if (argv[i][1] == 'r')
          {
          start_run = atoi(argv[++i]);
          end_run = atoi(argv[++i]);
          }
        else if (argv[i][1] == 'v')
          strcpy(var_name, argv[++i]);
        else if (argv[i][1] == 'f')
          strcpy(file_name, argv[++i]);
        else
          goto usage;
        }
      }
    else
      {
usage:
      printf("\nusage: odbhist -r <run1> <run2> -v <varname>[index]\n");
      printf("       [-f <filename>] [-q]\n\n");
      printf("       <run1> <run2>    Range of run numbers (inclusive)\n");
      printf("       <varname>        ODB variable name like \"/Runinfo/Run number\"\n");
      printf("       [index]          Index if <varname> is an array\n");
      printf("       -q               Don't display run number\n");
      return 0;
      }
    }

  if (end_run < start_run)
    {
    printf("Second run is %d and must be larger or equal than first run %d\n", 
           end_run, start_run);
    return 0;
    }

  for (run=start_run ; run<=end_run ; run++)
    {
    status = odb_hist(file_name, run, var_name, quiet);
    if (status != SUCCESS)
      break;
    }

  return 1;
}
