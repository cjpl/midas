/********************************************************************\

  Name:         mtape.c
  Created by:   Stefan Ritt

  Contents:     Magnetic tape manipulation program for MIDAS tapes

  $Log$
  Revision 1.3  1998/10/13 07:31:28  midas
  Fixed minor bugs causing compiler warnings

  Revision 1.2  1998/10/12 12:19:04  midas
  Added Log tag in header


\********************************************************************/

#include "midas.h"
#include "msystem.h"


/*------------------------------------------------------------------*/

INT tape_dir(INT channel, INT count)
{
EVENT_HEADER event;
INT          status, size, index;

  for (index=0 ; index<count ; index++)
    {
    /* read event header at current position */
    size = sizeof(event);
    status = ss_tape_read(channel, &event, &size);
    if (size != sizeof(event))
      {
      if (status == SS_END_OF_TAPE)
        {
        printf("End of tape reached.\n");
        return SS_SUCCESS;
        }
      printf("Cannot read from tape\n");
      return -1;
      }

    /* check if data is real MIDAS header */
    if (event.event_id != EVENTID_BOR || event.trigger_mask != MIDAS_MAGIC)
      {
      printf("Data on tape is no MIDAS data\n");
      return -1;
      }

    /* print run info */
    printf("Found run #%d recorded on %s", event.serial_number, 
            ctime(&event.time_stamp));

    if (index < count-1)
      {
      printf("Spooling tape...\r");
      status = ss_tape_fskip(channel, 1);
      if (status != SS_SUCCESS)
        {
        printf("Error spooling tape\n");
        return -1;
        }
      }
    else
      /* skip back record */
      ss_tape_rskip(channel, -1);
    }

  return SS_SUCCESS;
}

/*------------------------------------------------------------------*/

INT tape_copy(INT channel, INT count)
{
EVENT_HEADER *pevent;
INT          status, index, n, fh;
char         buffer[TAPE_BUFFER_SIZE], str[80];

  pevent = (EVENT_HEADER *) buffer;
  for (index=0 ; index<count ; index++)
    {
    /* read event header at current position */
    n = TAPE_BUFFER_SIZE;
    status = ss_tape_read(channel, buffer, &n);
    if (status != SS_SUCCESS)
      {
      if (status == SS_END_OF_TAPE)
        {
        printf("End of tape reached.\n");
        return SS_SUCCESS;
        }
      printf("Cannot read from tape\n");
      return -1;
      }

    /* check if data is real MIDAS header */
    if (pevent->event_id != EVENTID_BOR || pevent->trigger_mask != MIDAS_MAGIC)
      {
      printf("Data on tape is no MIDAS data\n");
      return -1;
      }

    /* print run info */
    printf("Copy run to file run%05d.mid\n", pevent->serial_number);

    /* open file */
    sprintf(str, "run%05d.mid", pevent->serial_number);
    fh = open(str, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, 0644);
    
    do
      {
      /* write buffer */
      write(fh, buffer, n);

      /* read next buffer */
      n = TAPE_BUFFER_SIZE;
      ss_tape_read(channel, buffer, &n);
      } while (n == TAPE_BUFFER_SIZE);

    write(fh, buffer, n);
    close(fh);
    
    /* skip to next file */
    ss_tape_fskip(channel, 1);
    }

  return SS_SUCCESS;
}

/*------------------------------------------------------------------*/

main(int argc, char *argv[])
{
INT  status, i, count, channel;
char cmd[100], tape_name[100];

  /* set default tape name */
  
#ifdef OS_WINNT
  strcpy(tape_name, "\\\\.\\tape0");
#elif defined(OS_UNIX)
  strcpy(tape_name, "/dev/nrmt0");
#elif defined(OS_VMS)
  strcpy(tape_name, "mka0:");
#else
#error This program cannot be compiled under this operating system
#endif

  /* if "TAPE" environment variable present, use it */
  if (getenv("TAPE") != NULL)
    strcpy(tape_name, getenv("TAPE"));
  
  /* parse command line parameters */
  if (argv[1] == NULL)
    goto usage;

  i = 1;
  if (argv[i][0] == '-')
    {
    if (argc <= 2 || argv[i+1][0] == '-')
      goto usage;
    if (argv[i][1] == 'f')
      strcpy(tape_name, argv[++i]);
    else 
      goto usage;
    i++;
    }
  
  if (argv[i] == NULL)
    goto usage;

  strcpy(cmd, argv[i++]);
  if (argv[i] != NULL)
    count = atoi(argv[i]);
  else
    count = 1;

  /* don't produce system messages */
  cm_set_msg_print(0, MT_ALL, puts);
  
  if (strcmp(cmd, "status") == 0)
    {
    status = ss_tape_status(tape_name);
    return 0;
    }

  /* open tape device */
  if (ss_tape_open(tape_name, &channel) != SS_SUCCESS)
    {
    printf("Cannot open tape %s.\n", tape_name);
    return 0;
    }

  if (strcmp(cmd, "rewind") == 0)
    status = ss_tape_rewind(channel);

  else if (strcmp(cmd, "online") == 0)
    status = ss_tape_mount(channel);

  else if (strcmp(cmd, "offline") == 0)
    status = ss_tape_unmount(channel);

  else if (strcmp(cmd, "eof") == 0 || strcmp(cmd, "weof") == 0)
    for (i=0 ; i<count ; i++)
      status = ss_tape_write_eof(channel);

  else if (strcmp(cmd, "fsf") == 0 || strcmp(cmd, "ff") == 0)
    status = ss_tape_fskip(channel, count);

  else if (strcmp(cmd, "fsr") == 0 || strcmp(cmd, "fr") == 0)
    status = ss_tape_rskip(channel, count);
  
  else if (strcmp(cmd, "bsf") == 0 || strcmp(cmd, "bf") == 0)
    status = ss_tape_fskip(channel, -count);

  else if (strcmp(cmd, "bsr") == 0 || strcmp(cmd, "br") == 0)
    status = ss_tape_rskip(channel, -count);

  else if (strcmp(cmd, "seod") == 0)
    status = ss_tape_spool(channel);

  else if (strcmp(cmd, "dir") == 0)
    status = tape_dir(channel, count);

  else if (strcmp(cmd, "copy") == 0)
    status = tape_copy(channel, count);

  else
    {
usage:
    printf("usage: mtape [-f tape_device] command [count]\n\n");
    printf("Following commands are available:\n");
    printf("  status     Print status information about tape\n");
    printf("  rewind     Rewind tape\n");
    printf("  eof,weof   Write <count> End-of-File marks at the current tape position\n");
    printf("  fsf,ff     Forward spaces <count> files\n");
    printf("  fsr,fr     Forward spaces <count> records\n");
    printf("  bsf,bf     Backspaces <count> files\n");
    printf("  bsr,br     Backspaces <count> records\n");
    printf("  rewind     Rewind tape\n");
    printf("  offline    Places the tape offline (unload)\n");
    printf("  online     Places the tape online (load)\n");
    printf("  seod       Space to end of recorded data\n\n");
    printf("Following commands only work with tapes written in MIDAS format:\n");
    printf("  dir <n>    List next <n> runs on MIDAS tape\n");
    printf("  copy <n>   Copy next <n> runs to disk\n");
    return 0;
    }

  if (status != SS_SUCCESS)
    printf("Error performing operation, status code = %d\n", status);

  ss_tape_close(channel);

  return 0;
}
