/********************************************************************\

  Name:         mtape.c
  Created by:   Stefan Ritt

  Contents:     Magnetic tape manipulation program for MIDAS tapes

  $Id$

\********************************************************************/

#include "midas.h"
#include "msystem.h"

BOOL verbose = FALSE;

/*------------------------------------------------------------------*/

INT tape_dir(INT channel, INT count)
{
   EVENT_HEADER *event;
   char buffer[TAPE_BUFFER_SIZE];
   INT status, size, index, blockn;
   time_t t;

   event = (EVENT_HEADER *) buffer;
   for (index = 0; index < count; index++) {
    try_again:
      /* read event header at current position */
      size = sizeof(buffer);
      status = ss_tape_read(channel, buffer, &size);
      if (status != SS_SUCCESS) {
         if (status == SS_END_OF_TAPE) {
            printf("End of tape reached.\n");
            return 1;
         }
         if (status == SS_END_OF_FILE) {
            printf("End of File marker found, skipping...\n");
            goto try_again;
         }
         printf("Cannot read from tape, status=%X\n", status);
         return 1;
      }

      /* check if data is real MIDAS header */
      if (event->event_id != EVENTID_BOR || event->trigger_mask != MIDAS_MAGIC) {
#ifdef OS_UNIX
         FILE *f;

         /* write tape data in temporary file, let 'file' utility determine type */
         f = fopen("/tmp/.mt", "w");
         fwrite(buffer, sizeof(buffer), 1, f);
         fclose(f);
         system("file /tmp/.mt > /tmp/.mtf");
         f = fopen("/tmp/.mtf", "r");
         if (f != NULL) {
            memset(buffer, 0, sizeof(buffer));
            fread(buffer, sizeof(buffer), 1, f);

            /* cut off new line */
            if (strchr(buffer, '\n'))
               *strchr(buffer, '\n') = 0;
            if (strchr(buffer, '\r'))
               *strchr(buffer, '\r') = 0;

            printf("Found file: ");

            /* start after ':' */
            if (strchr(buffer, ':'))
               printf("%s", strchr(buffer, ':') + 2);
            else
               printf("%s", buffer);
            printf("\n");
            fclose(f);
         }
         unlink("/tmp/.mtf");
         unlink("/tmp/.mt");
#else
         printf("File on tape is no MIDAS data\n");
#endif
      } else {
         blockn = ss_tape_get_blockn(channel);
         t = (time_t) event->time_stamp;
         if (blockn > 0)
            printf("Found run #%d at block#:%d recorded on %s", event->serial_number,
                   blockn, ctime(&t));
         else
            printf("Found run #%d recorded on %s", event->serial_number, ctime(&t));
      }
      if (index < count - 1) {
         printf("Spooling tape...\r");
         fflush(stdout);
         status = ss_tape_fskip(channel, 1);
         if (status != SS_SUCCESS) {
            printf("Error spooling tape\n");
            return -1;
         }
      } else
         /* skip back record */
         ss_tape_rskip(channel, -1);
   }

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/

INT tape_restore(INT channel, INT count, char *file_name)
{
   EVENT_HEADER *pevent;
   INT status, index, n, fh, size, mb;
   char buffer[TAPE_BUFFER_SIZE], str[80];

   pevent = (EVENT_HEADER *) buffer;
   for (index = 0; index < count; index++) {
      /* read event header at current position */
    try_again:
      n = TAPE_BUFFER_SIZE;
      status = ss_tape_read(channel, buffer, &n);
      if (status != SS_SUCCESS) {
         if (status == SS_END_OF_TAPE) {
            printf("End of tape reached.\n");
            return SS_SUCCESS;
         } else if (status == SS_END_OF_FILE) {
            printf("End of File marker found, skipping...\n");
            goto try_again;
         }
         return -1;
      }

      /* check if data is real MIDAS header */
      if (pevent->event_id != EVENTID_BOR || pevent->trigger_mask != MIDAS_MAGIC) {
         printf("Data on tape is no MIDAS data\n");
         if (count > 1) {
            printf("Skipping file...\r");
            fflush(stdout);
            ss_tape_fskip(channel, 1);
            continue;
         }
         return -1;
      }

      /* make file name */
      if (strchr(file_name, '%'))
         sprintf(str, file_name, pevent->serial_number);
      else
         strcpy(str, file_name);

      /* print run info */
      printf("Copy run to file %s\n", str);

      /* open file */
      fh = open(str, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, 0644);
      if (fh < 0) {
         printf("Cannot open file %s\n", str);
         return -1;
      }

      size = mb = 0;
      do {
         /* write buffer */
         write(fh, buffer, n);

         size += n;
         if (verbose && size > mb + 1024 * 1024) {
            mb = size;
            printf("%d MB\r", size / 1024 / 1024);
            fflush(stdout);
         }

         /* read next buffer */
         n = TAPE_BUFFER_SIZE;
         ss_tape_read(channel, buffer, &n);
      } while (n == TAPE_BUFFER_SIZE);

      if (verbose)
         printf("%d MB\n", size / 1024 / 1024);

      write(fh, buffer, n);
      close(fh);

      /* skip to next file */
      ss_tape_fskip(channel, 1);
   }

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/

INT tape_backup(INT channel, INT count1, INT count2, char *file_name)
{
   INT i, n, fh, size, mb;
   char buffer[TAPE_BUFFER_SIZE], str[256];

   if (count2 < count1)
      count2 = count1;

   for (i = count1; i <= count2; i++) {
      if (strchr(file_name, '%'))
         sprintf(str, file_name, i);
      else
         strcpy(str, file_name);

      /* open file for read */
      fh = open(str, O_RDONLY | O_BINARY, 0644);
      if (fh < 0) {
         printf("Cannot open file %s\n", str);
         continue;
      }

      /* print run info */
      printf("Copy file %s to tape\n", str);
      size = mb = 0;

      do {
         /* read buffer */
         n = read(fh, buffer, TAPE_BUFFER_SIZE);

         size += n;
         if (verbose && size > mb + 1024 * 1024) {
            mb = size;
            printf("%d MB\r", size / 1024 / 1024);
            fflush(stdout);
         }

         /* write buffer */
         ss_tape_write(channel, buffer, n);

      } while (n > 0);

      if (verbose)
         printf("%d MB\n", size / 1024 / 1024);

      close(fh);

      /* write EOF */
      ss_tape_write_eof(channel);
   }

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
   INT status = 0, i, count1, count2, channel;
   char cmd[100], tape_name[256], file_name[256];

   /* set default tape name */

#ifdef OS_WINNT
   strcpy(tape_name, "\\\\.\\tape0");
#elif defined(OS_UNIX)
   strcpy(tape_name, "/dev/tape");
#elif defined(OS_VMS)
   strcpy(tape_name, "mka0:");
#else
#error This program cannot be compiled under this operating system
#endif

   /* default parameters */
   strcpy(file_name, "run%05d.mid");
   cmd[0] = 0;
   count1 = -100;
   count2 = 0;

   /* if "TAPE" environment variable present, use it */
   if (getenv("TAPE") != NULL)
      strcpy(tape_name, getenv("TAPE"));

   /* parse command line parameters */
   for (i = 1; i < argc; i++) {
      if (argv[i][0] == '-' && argv[i][1] == 'v')
         verbose = TRUE;
      else if (argv[i][0] == '-') {
         if (i + 1 >= argc || argv[i + 1][0] == '-')
            goto usage;
         if (argv[i][1] == 'f')
            strcpy(tape_name, argv[++i]);
         else if (argv[i][1] == 'd')
            strcpy(file_name, argv[++i]);
         else
            goto usage;
      } else if (cmd[0] == 0)
         strcpy(cmd, argv[i]);
      else if (count1 == -100)
         count1 = atoi(argv[i]);
      else
         count2 = atoi(argv[i]);
   }

   if (count1 == -100)
      count1 = 1;

   /* don't produce system messages */
   cm_set_msg_print(0, MT_ALL, puts);

   if (strcmp(cmd, "status") == 0) {
      status = ss_tape_status(tape_name);
      return 0;
   }

   /* open tape device */
   if (ss_tape_open(tape_name, O_RDONLY, &channel) != SS_SUCCESS) {
      printf("Cannot open tape %s.\n", tape_name);
      return 0;
   }

   if (strcmp(cmd, "rewind") == 0)
      status = ss_tape_rewind(channel);

   else if (strcmp(cmd, "online") == 0)
      status = ss_tape_mount(channel);

   else if (strcmp(cmd, "offline") == 0)
      status = ss_tape_unmount(channel);

   else if (strcmp(cmd, "eof") == 0 || strcmp(cmd, "weof") == 0) {
      /* reopen tape in write mode */
      ss_tape_close(channel);
      if (ss_tape_open(tape_name, O_RDWR | O_CREAT | O_TRUNC, &channel) != SS_SUCCESS) {
         printf("Cannot open tape %s.\n", tape_name);
         return 0;
      }

      for (i = 0; i < count1; i++)
         status = ss_tape_write_eof(channel);
   }

   else if (strcmp(cmd, "fsf") == 0 || strcmp(cmd, "ff") == 0)
      status = ss_tape_fskip(channel, count1);

   else if (strcmp(cmd, "fsr") == 0 || strcmp(cmd, "fr") == 0)
      status = ss_tape_rskip(channel, count1);

   else if (strcmp(cmd, "bsf") == 0 || strcmp(cmd, "bf") == 0)
      status = ss_tape_fskip(channel, -count1);

   else if (strcmp(cmd, "bsr") == 0 || strcmp(cmd, "br") == 0)
      status = ss_tape_rskip(channel, -count1);

   else if (strcmp(cmd, "seod") == 0)
      status = ss_tape_spool(channel);

   else if (strcmp(cmd, "dir") == 0)
      status = tape_dir(channel, count1);

   else if (strcmp(cmd, "backup") == 0) {
      /* reopen tape in write mode */
      ss_tape_close(channel);
      if (ss_tape_open(tape_name, O_RDWR | O_CREAT | O_TRUNC, &channel) != SS_SUCCESS) {
         printf("Cannot open tape %s.\n", tape_name);
         return 0;
      }
      status = tape_backup(channel, count1, count2, file_name);
   }

   else if (strcmp(cmd, "restore") == 0)
      status = tape_restore(channel, count1, file_name);

   else {
    usage:
      printf("usage: mtape [-f tape_device] [-d file] [-v] command [count] [last]\n\n");
      printf("Following commands are available:\n");
      printf("  status     Print status information about tape\n");
      printf("  rewind     Rewind tape\n");
      printf
          ("  eof,weof   Write <count> End-of-File marks at the current tape position\n");
      printf("  fsf,ff     Forward spaces <count> files\n");
      printf("  fsr,fr     Forward spaces <count> records\n");
      printf("  bsf,bf     Backspaces <count> files\n");
      printf("  bsr,br     Backspaces <count> records\n");
      printf("  rewind     Rewind tape\n");
      printf("  offline    Places the tape offline (unload)\n");
      printf("  online     Places the tape online (load)\n");
      printf("  seod       Space to end of recorded data\n\n");
      printf("Following commands only work with tapes written in MIDAS format:\n");
      printf("  dir        List next [count] runs on MIDAS tape\n");
      printf("  backup\n");
      printf
          ("             Copy runs with numbers [count] to [last] from disk to tape.\n");
      printf("             If [-d file] is not given, runxxxxx.mid is used where\n");
      printf("             xxxxx is the run number. If [file] contains %%05d, it\n");
      printf("             is replaced by the run number.\n");
      printf("  restore\n");
      printf("             Copy next [count] files from tape to disk.\n");
      printf("             If [-d file] is not given, runxxxxx.mid is used where\n");
      printf("             xxxxx is the run number. If [file] contains %%05d, it\n");
      printf("             is replaced by the run number.\n");
      return 0;
   }

   if (status == -1)
      printf("Error performing operation\n");
   else if (status != SS_SUCCESS)
      printf("Error performing operation, status code = %d\n", status);

   ss_tape_close(channel);

   return 0;
}
