/********************************************************************\

  Name:         cmdedit.c
  Created by:   Stefan Ritt

  Contents:     Command-line editor for ODBEdit


  Revision history
  ------------------------------------------------------------------
  date         by    modification
  ---------    ---   ------------------------------------------------
  24-JUN-97    SR    created


\********************************************************************/

#include "midas.h"
#include "msystem.h"

#ifdef OS_MSDOS
#define MAX_HISTORY 10
#else
#define MAX_HISTORY 50
#endif

#define LINE_LENGTH 256

char history[MAX_HISTORY][LINE_LENGTH];
INT  his_index = 0;

#ifdef OS_VMS

INT cmd_edit(char *prompt, char *cmd, INT (*dir)(char*), INT (*idle)())
{
  printf(prompt);
  ss_gets(cmd, 256);

  return strlen(cmd);
}

#else

INT cmd_edit(char *prompt, char *cmd, INT (*dir)(char*), INT (*idle)())
{
char  line[LINE_LENGTH], *pc;
INT   i, j, k, c, hi;
INT   status;
DWORD last_time = 0;
BOOL  escape_flag = 0;

  if (ss_getchar(0) == -1)
    {
    /* normal input if ss_getchar not supported */
    printf(prompt);
    ss_gets(cmd, 256);
    return strlen(cmd);
    }

  printf(prompt);
  fflush(stdout);

  i  = 0;
  hi = his_index;
  memset(line, 0, LINE_LENGTH);
  memset(history[hi], 0, LINE_LENGTH);

  do
    {
    c = ss_getchar(0);

    if (c == 27)
      escape_flag = TRUE;

    if (c >= ' ' && c < CH_EXT && escape_flag)
      {
      escape_flag = FALSE;
      if (c == 'p')
        c = 6;
      }

    /* normal input */
    if (c >= ' ' && c < CH_EXT)
      {
      if (strlen(line) < LINE_LENGTH-1)
        {
        for (j=strlen(line) ; j>=i ; j--)
          line[j+1] = line[j];
        if (i < LINE_LENGTH-1)
          {
          line[i++] = c;
          printf("%c", c);
          }
        for (j=i ; j<(INT)strlen(line) ; j++)
          printf("%c", line[j]);
        for (j=i ; j<(INT)strlen(line) ; j++)
          printf("\b");
        }
      }

    /* BS */
    if (c == CH_BS && i>0)
      {
      i--;
      printf("\b");
      for (j=i ; j<=(INT) strlen(line) ; j++)
        {
        line[j] = line[j+1];
        if (line[j])
          printf("%c", line[j]);
        else
          printf(" ");
        }
      for (k=0 ; k<j-i ; k++)
        printf("\b");
      }

    /* DELETE/Ctrl-D */
    if (c == CH_DELETE || c == 4)
      {
      for (j=i ; j<=(INT) strlen(line) ; j++)
        {
        line[j] = line[j+1];
        if (line[j])
          printf("%c", line[j]);
        else
          printf(" ");
        }
      for (k=0 ; k<j-i ; k++)
        printf("\b");
      }

    /* Erase line: CTRL-W, CTRL-U */
    if (c == 23 || c == 21)
      {
      i = strlen(line);
      memset(line, 0, sizeof(line));
      printf("\r%s", prompt);
      for (j=0 ; j<i ; j++)
        printf(" ");
      for (j=0 ; j<i ; j++)
        printf("\b");
      i = 0;
      }

    /* Erase line from cursor: CTRL-K */
    if (c == 11)
      {
      for (j=i ; j<(INT)strlen(line) ; j++)
        printf(" ");
      for (j=i ; j<(INT)strlen(line) ; j++)
        printf("\b");
      for (j=strlen(line) ; j>=i ; j--)
        line[j] = 0;
      }

    /* left arrow, CTRL-B */
    if ((c == CH_LEFT || c == 2) && i>0)
      {
      i--;
      printf("\b");
      }

    /* right arrow, CTRL-F */
    if ((c == CH_RIGHT || c == 6) && i<(INT) strlen(line))
      printf("%c", line[i++]);

    /* HOME, CTRL-A */
    if ((c == CH_HOME || c == 1) && i>0)
      {
      for (j=0 ; j<i ; j++)
        printf("\b");
      i = 0;
      }

    /* END, CTRL-E */
    if ((c == CH_END || c == 5) && i<(INT) strlen(line))
      {
      for (j=i ; j<(INT) strlen(line) ; j++)
        printf("%c", line[i++]);
      i = strlen(line);
      }

    /* up arrow / CTRL-P */
    if (c == CH_UP || c == 16)
      {
      if (history[(hi + MAX_HISTORY - 1) % MAX_HISTORY][0])
        {
        hi = (hi + MAX_HISTORY - 1) % MAX_HISTORY;
        for (j=0 ; j<(INT) strlen(line) ; j++)
          printf("\b \b");
        memcpy(line, history[hi], 256);
        printf(line);
        i = strlen(line);
        }
      }

    /* down arrow / CTRL-N */
    if (c == CH_DOWN || c == 14)
      {
      if (history[hi][0])
        {
        hi = (hi+1) % MAX_HISTORY;
        for (j=0 ; j<(INT) strlen(line) ; j++)
          printf("\b \b");
        memcpy(line, history[hi], 256);
        printf(line);
        i = strlen(line);
        }
      }

    /* CTRL-F */
    if (c == 6)
      {
      for (j = (hi+MAX_HISTORY-1) % MAX_HISTORY ; j != hi ; 
           j = (j+MAX_HISTORY-1) % MAX_HISTORY)
        if (history[j][0] && strncmp(line, history[j], i) == 0)
          {
          memcpy(line, history[j], 256);
          printf(line+i);
          i = strlen(line);
          break;
          }
      if (j == hi)
        printf("%c", 7);
      }

    /* tab */
    if (c == 9 && dir != NULL)
      {
      if (strlen(line) == 0)
        status = dir(line);
      else
        {
        if (strchr(line, '"'))
          pc = strchr(line, '"');
        else
          {
          pc = line + strlen(line) - 1;
          while (pc > line && *pc != ' ')
            pc--;
          }
        if (*pc == ' ')
          pc++;

        status = dir(pc);
        }

      if (status)
        /* line extended */
        i = strlen(line);

      /* redraw line */
      printf("\r%s%s", prompt, line);

      for (j=0 ; j<(INT)strlen(line)-i ; j++)
        printf("\b");
      }

    if (c != 0)
      {
      last_time = ss_millitime();
      fflush(stdout);
      }

    if ((ss_millitime() - last_time > 300) && idle != NULL)
      {
      status = idle();

      if (status)
        {
        printf("\r%s%s", prompt, line);

        for (j=0 ; j<(INT)strlen(line)-i ; j++)
          printf("\b");

        fflush(stdout);
        }
      }

    } while (c != CH_CR);

  strcpy(cmd, line);

  if (dir != NULL)
    if (strcmp(cmd, history[(his_index+MAX_HISTORY-1) % MAX_HISTORY]) != 0 &&
        cmd[0])
      {
      strcpy(history[his_index], cmd);
      his_index = (his_index+1) % MAX_HISTORY;
      }

  /* reset terminal */
  ss_getchar(1);

  printf("\n");

  return strlen(line);
}

#endif
