/********************************************************************\

  Name:         msc.c
  Created by:   Stefan Ritt

  Contents:     Command-line interface for the Midas Slow Control Bus

  $Log$
  Revision 1.10  2002/10/03 15:30:40  midas
  Added linux support

  Revision 1.9  2002/09/27 11:09:08  midas
  Modified test mode

  Revision 1.8  2002/08/12 12:10:30  midas
  Added reset command

  Revision 1.7  2002/07/10 09:51:25  midas
  Introduced mscb_flash()

  Revision 1.6  2002/07/08 15:00:40  midas
  Added reboot command

  Revision 1.5  2001/10/31 11:15:17  midas
  Added IO check function

  Revision 1.4  2001/09/04 07:33:15  midas
  Added test function

  Revision 1.3  2001/08/31 12:23:50  midas
  Added mutex protection

  Revision 1.2  2001/08/31 11:35:20  midas
  Added "wp" command in msc.c, changed parport to device in mscb.c

  Revision 1.1  2001/08/31 11:05:50  midas
  Initial revision

\********************************************************************/

#ifdef _MSC_VER
#include <windows.h>
#include <conio.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mscb.h"

/*------------------------------------------------------------------*/

void print_help()
{
  puts("Available commands:\n");
  puts("scan                       Scan bus for nodes");
  puts("ping <addr>                Ping and address node");
  puts("addr <addr>                Address node for further commands");
  puts("gaddr <addr>               Address group of nodes");
  puts("info                       Retrive node info");
  puts("sa <addr> <gaddr>          Set node and group address of addressed node");
  puts("debug 0/1                  Turn debuggin off/on on node (LCD output)");
  puts("\nwrite <channel> <value>    Write to node channel");
  puts("read <channel>             Read from node channel");
  puts("wc <index> <value>         Write configuration parameter");
  puts("flash                      Flash parameters into EEPROM");
  puts("rc <index>                 Read configuration parameter");
  puts("reboot                     Reboot addressed node");
  puts("reset                      Reboot whole MSCB system");
  puts("terminal                   Enter teminal mode for SCS-210");
}

/*------------------------------------------------------------------*/

void cmd_loop(int fd, char *cmd)
{
int           i, j, status, nparam, addr, gaddr, current_addr, current_group;
unsigned int  data;
char          str[256], line[256];
char          param[10][100];
char          *pc;
FILE          *cmd_file = NULL;
MSCB_INFO     info;
MSCB_INFO_CHN info_chn;


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

  if (cmd[0])
    /* assumed some node correctly addressed in command mode */
    current_addr = 0;
  else
    current_addr = -1;

  current_group = -1;

  do
    {
    /* print prompt */
    if (!cmd[0])
      {
      if (current_addr >= 0)
        printf("node0x%X> ", current_addr);
      else if (current_group >= 0)
        printf("group%d> ", current_group);
      else
        printf("> ");
      fgets(line, sizeof(line), stdin);
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
      print_help();

    /* scan */
    else if ((param[0][0] == 's' && param[0][1] == 'c'))
      {
      for (i=0 ; i<1000 ; i++)
        {
        printf("Test address %d\r", i);
        fflush(stdout);
        status = mscb_addr(fd, CMD_PING16, i);
        if (status == MSCB_SUCCESS)
          printf("Found node %d       \n", i);
        }
      status = mscb_addr(fd, CMD_PING16, 0xFFFF);
      if (status == MSCB_SUCCESS)
        printf("Found node 0xFFFF\n");
      else
        printf("                    \n");

      current_addr = -1;
      }

    /* info */
    else if ((param[0][0] == 'i' && param[0][1] == 'n') && param[0][2] == 'f')
      {
      if (current_addr < 0)
        printf("You must first address an individual node\n");
      else
        {
        status = mscb_info(fd, &info);
        if (status != MSCB_SUCCESS)
          {
          printf("No response from node\n");
          }
        else
          {
          printf("Node name:        %s\n", info.node_name);
          printf("Node status:      0x%02X", info.node_status);
          if (info.node_status > 0)
            {
            printf(" (");
            if ((info.node_status & CSR_DEBUG) > 0)
              printf("DEBUG ");
            if ((info.node_status & CSR_LCD_PRESENT) > 0)
              printf("LCD ");
            if ((info.node_status & CSR_SYNC_MODE) > 0)
              printf("SYNC ");
            if ((info.node_status & CSR_FREEZE_MODE) > 0)
              printf("FREEZE ");
            printf("\b)\n");
            }
          else
            printf("\n");
          printf("Node address:     %d (0x%X)\n", info.node_address, info.node_address);
          printf("Group address:    %d (0x%X)\n", info.group_address, info.group_address);
          printf("Protocol version: %02X\n", info.protocol_version);
          printf("Watchdog resets:  %d\n", info.watchdog_resets);

          printf("\nChannels:\n");
          for (i=0 ; i<info.n_channel ; i++)
            {
            mscb_info_channel(fd, GET_INFO_CHANNEL, i, &info_chn);
            mscb_read(fd, (unsigned char)i, &data);
            memset(str, 0, sizeof(str));
            strncpy(str, info_chn.channel_name, 8);
            for (j=strlen(str) ; j<9 ; j++)
              str[j] = ' ';
            printf("%2d: %s ", i, str);
            switch(info_chn.channel_width)
              {
              case SIZE_0BIT:  printf(" 0bit"); break;
              case SIZE_8BIT:  printf(" 8bit %8d (0x%02X)", data, data); break;
              case SIZE_16BIT: printf("16bit %8d (0x%04X)", data, data); break;
              case SIZE_24BIT: printf("24bit %8d (0x%06X)", data, data); break;
              case SIZE_32BIT: if (info_chn.flags & MSCBF_FLOAT)
                                 printf("32bit %8.6lg", *((float *)&data));
                               else
                                 printf("32bit %8d (0x%08X)", data, data);
                               break;
              }
            printf("\n");
            }
          printf("\nConfiguration Parameters:\n");
          for (i=0 ; i<info.n_conf ; i++)
            {
            mscb_info_channel(fd, GET_INFO_CONF, i, &info_chn);
            mscb_read_conf(fd, (unsigned char)i, &data);
            memset(str, 0, sizeof(str));
            strncpy(str, info_chn.channel_name, 8);
            for (j=strlen(str) ; j<9 ; j++)
              str[j] = ' ';
            printf("%2d: %s ", i, str);
            switch(info_chn.channel_width)
              {
              case SIZE_0BIT:  printf(" 0bit"); break;
              case SIZE_8BIT:  printf(" 8bit %8d (0x%02X)", data, data); break;
              case SIZE_16BIT: printf("16bit %8d (0x%04X)", data, data); break;
              case SIZE_24BIT: printf("24bit %8d (0x%06X)", data, data); break;
              case SIZE_32BIT: if (info_chn.flags & MSCBF_FLOAT)
                                 printf("32bit %8.6lg", *((float *)&data));
                               else
                                 printf("32bit %8d (0x%08X)", data, data);
                               break;
              }
            printf("\n");
            }
          }
        }
      }

    /* reboot */
    else if ((param[0][0] == 'r' && param[0][1] == 'e') && param[0][2] == 'b')
      {
      mscb_reboot(fd);
      current_addr = -1;
      current_group = -1;
      }

    /* reset */
    else if ((param[0][0] == 'r' && param[0][1] == 'e') && param[0][2] == 's')
      {
      mscb_reset(fd);
      current_addr = -1;
      current_group = -1;
      }

    /* ping */
    else if ((param[0][0] == 'p' && param[0][1] == 'i') ||
             (param[0][0] == 'a' && param[0][1] == 'd'))
      {
      if (!param[1][0])
        puts("Please specify node address");
      else
        {
        if (param[1][1] == 'x')
          sscanf(param[1]+2, "%x", &addr);
        else
          addr = atoi(param[1]);

        status = mscb_addr(fd, CMD_PING16, addr);
        if (status != MSCB_SUCCESS)
          {
          if (status == MSCB_MUTEX)
            printf("MSCB used by other process\n");
          else
            printf("Node %d does not respond\n", addr);
          current_addr = -1;
          current_group = -1;
          }
        else
          {
          printf("Node %d addressed\n", addr);
          current_addr = addr;
          current_group = -1;
          }
        }
      }

    /* gaddr */
    else if ((param[0][0] == 'g' && param[0][1] == 'a'))
      {
      if (!param[1][0])
        puts("Please specify group address");
      else
        {
        if (param[1][1] == 'x')
          sscanf(param[1]+2, "%x", &addr);
        else
          addr = atoi(param[1]);

        status = mscb_addr(fd, CMD_ADDR_GRP16, addr);
        if (status != MSCB_SUCCESS)
          {
          printf("Error %d\n", status);
          current_addr = -1;
          current_group = -1;
          }
        else
          {
          current_addr = -1;
          current_group = addr;
          }
        }
      }

    /* sa */
    else if ((param[0][0] == 's' && param[0][1] == 'a'))
      {
      if (current_addr < 0)
        printf("You must first address an individual node\n");
      else
        {
        if (!param[1][0] || !param[2][0])
          puts("Please specify node and group address");
        else
          {
          if (param[1][1] == 'x')
            sscanf(param[1]+2, "%x", &addr);
          else
            addr = atoi(param[1]);

          if (param[2][1] == 'x')
            sscanf(param[2]+2, "%x", &gaddr);
          else
            gaddr = atoi(param[2]);

          mscb_set_addr(fd, addr, gaddr);
          }
        }
      }

    /* debug */
    else if ((param[0][0] == 'd' && param[0][1] == 'e'))
      {
      if (current_addr < 0)
        printf("You must first address an individual node\n");
      else
        {
        if (!param[1][0])
          puts("Please specify 0/1");
        else
          {
          i = atoi(param[1]);

          /* write CSR register */
          mscb_write_conf(fd, 0xFF, i ? CSR_DEBUG : 0, 1);
          }
        }
      }

    /* write */
    else if ((param[0][0] == 'w' && param[0][1] == 'r'))
      {
      if (current_addr < 0 && current_group < 0)
        printf("You must first address a node or a group\n");
      else
        {
        if (!param[1][0] || !param[2][0])
          puts("Please specify channel number and data");
        else
          {
          addr = atoi(param[1]);

          if (param[2][1] == 'x')
            sscanf(param[2]+2, "%lx", &data);
          else
            data = atoi(param[2]);

          if (current_addr >= 0)
            status = mscb_write(fd, (unsigned char)addr, data, 4);
          else if (current_group >= 0)
            status = mscb_write_na(fd, (unsigned char)addr, data, 4);

          if (status != MSCB_SUCCESS)
            printf("Error: %d\n", status);
          }
        }
      }

    /* read */
    else if ((param[0][0] == 'r' && param[0][1] == 'e'))
      {
      if (current_addr < 0)
        printf("You must first address an individual node\n");
      else
        {
        if (!param[1][0])
          puts("Please specify channel number");
        else
          {
          addr = atoi(param[1]);

          status = mscb_read(fd, (unsigned char)addr, &data);
          if (status != MSCB_SUCCESS)
            printf("Error: %d\n", status);
          else
            printf("%d (0x%X)\n", data, data);
          }
        }
      }

    /* wc */
    else if (param[0][0] == 'w' && param[0][1] == 'c')
      {
      if (current_addr < 0)
        printf("You must first address an individual node\n");
      else
        {
        if (!param[1][0] || !param[2][0])
          puts("Please specify parameter index and data");
        else
          {
          addr = atoi(param[1]);

          if (param[2][1] == 'x')
            sscanf(param[2]+2, "%lx", &data);
          else
            data = atoi(param[2]);

          status = mscb_write_conf(fd, (unsigned char)addr, data, 4);

          if (status != MSCB_SUCCESS)
            printf("Error: %d\n", status);
          }
        }
      }

    /* rc */
    else if ((param[0][0] == 'r' && param[0][1] == 'c'))
      {
      if (current_addr < 0)
        printf("You must first address an individual node\n");
      else
        {
        if (!param[1][0])
          puts("Please specify channel number");
        else
          {
          addr = atoi(param[1]);

          status = mscb_read_conf(fd, (unsigned char)addr, &data);
          if (status != MSCB_SUCCESS)
            printf("Error: %d\n", status);
          else
            printf("%d (0x%X)\n", data, data);
          }
        }
      }

    /* terminal */
    else if ((param[0][0] == 't' && param[0][1] == 'e' && param[0][2] == 'r'))
      {
      char c;
      int  d, status;

      puts("Exit with <ESC>\n");

      /* switch SCS-210 into terminal mode */
      c = 0;
      status = mscb_write(fd, 0, (unsigned int) c, 1);

      do
        {
        if (kbhit())
          {
          c = getch();
          putchar(c);
          status = mscb_write(fd, 0, (unsigned int) c, 1);
          if (status != MSCB_SUCCESS)
            {
            printf("\nError: %d\n", status);
            break;
            }

          if (c == '\r')
            {
            putchar('\n');
            mscb_write(fd, 0, (unsigned int) '\n', 1);
            }

          }

        mscb_read(fd, 0, &d);
        if (d > 0)
          putchar(d);

        } while (c != 27);

      puts("\n");
      while (kbhit())
        getch();
      }

    /* flash */
    else if (param[0][0] == 'f' && param[0][1] == 'l')
      {
      if (current_addr < 0)
        printf("You must first address an individual node\n");
      else
        {
        status = mscb_flash(fd);

        if (status != MSCB_SUCCESS)
          printf("Error: %d\n", status);
        }
      }

    /* test */
    else if ((param[0][0] == 't' && param[0][1] == 'e' && param[0][2] == 's'))
      {
      unsigned short d1, d2;
      int size, i, status;

      i = 0;
      while (!kbhit())
        {
        d1 = rand();

        status = mscb_user(fd, &d1, sizeof(d1), &d2, &size);

        if (d2 != d1)
          printf("Received: %04X, should be %04X, status = %d\n", d2, d1, status);

        i++;
        if (i % 100 == 0)
          {
          printf("%d\r", i);
          fflush(stdout);
          }

        //Sleep(10);
        }

      }

    /* exit/quit */
    else if ((param[0][0] == 'e' && param[0][1] == 'x') ||
        (param[0][0] == 'q'))
      break;

    else if (param[0][0] == 0);

    else if (param[0][0] == '\n');

    else
      printf("Unknown command %s %s %s\n", param[0], param[1], param[2]);

    /* exit after single command */
    if (cmd[0] && cmd[0] != '@')
      break;

    } while (1);
}

/*------------------------------------------------------------------*/

main(int argc, char *argv[])
{
int   i, fd;
char  cmd[256], port[100];
int   debug, check_io;

  cmd[0] = 0;
#ifdef _MSC_VER
  strcpy(port, "lpt1");
#elif defined(__linux__)
  strcpy(port, "/dev/parport0");
#else
  strcpy(port, "");
#endif

  debug = check_io = 0;

  /* parse command line parameters */
  for (i=1 ; i<argc ; i++)
    {
    if (argv[i][0] == '-' && argv[i][1] == 'v')
      debug = 1;
    if (argv[i][0] == '-' && argv[i][1] == 'i')
      check_io = 1;
    else if (argv[i][0] == '-')
      {
      if (i+1 >= argc || argv[i+1][0] == '-')
        goto usage;
      if (argv[i][1] == 'p')
        strcpy(port, argv[++i]);
      else if (argv[i][1] == 'c')
        {
        if (strlen(argv[i]) >= 256)
          {
          printf("error: command line too long (>256).\n");
          return 0;
          }
        strcpy(cmd, argv[++i]);
        }
      else
        {
usage:
        printf("usage: msc [-p port] [-c Command] [-c @CommandFile] [-v] [-i]\n\n");
        printf("       -p     Specify port (LPT1 by default)\n");
        printf("       -c     Execute command immediately\n");
        printf("       -v     Produce verbose output\n\n");
        printf("       -i     Check IO pins of port\n\n");
        printf("For a list of valid commands start msc interactively\n");
        printf("and type \"help\".\n");
        return 0;
        }
      }
    }

  if (check_io)
    {
    mscb_check(port);
    return 0;
    }

  /* open port */
  fd = mscb_init(port);
  if (fd < 0)
    {
    if (fd == -2)
      printf("No MSCB submaster present at port \"%s\"\n", port);
    return 0;
    }

  cmd_loop(fd, cmd);

  mscb_exit(fd);

  return 0;
}
