/********************************************************************\

  Name:         msc.c
  Created by:   Stefan Ritt

  Contents:     Command-line interface for the Midas Slow Control Bus

  $Log$
  Revision 1.29  2003/02/18 11:21:12  midas
  Added repeat scan mode

  Revision 1.28  2003/01/30 08:34:06  midas
  Added [cf] flags to echo

  Revision 1.27  2003/01/24 13:45:12  midas
  Fixed bug in argument analysis

  Revision 1.26  2003/01/15 15:51:11  midas
  Moved error explanation (-1 could be DirectIO)

  Revision 1.25  2003/01/15 13:15:20  midas
  Added hints for problem solving

  Revision 1.24  2002/11/29 08:02:52  midas
  Fixed linux warnings

  Revision 1.23  2002/11/28 14:44:09  midas
  Removed SIZE_XBIT

  Revision 1.22  2002/11/28 13:04:36  midas
  Implemented protocol version 1.2 (echo, read_channels)

  Revision 1.21  2002/11/27 15:40:05  midas
  Added version, fixed few bugs

  Revision 1.20  2002/11/22 15:43:30  midas
  Ouput cycle number in test mode

  Revision 1.19  2002/11/20 12:01:23  midas
  Added host to mscb_init

  Revision 1.18  2002/11/06 16:47:37  midas
  Address only needs 'a'

  Revision 1.17  2002/11/06 16:46:29  midas
  Keep address on reboot

  Revision 1.16  2002/11/06 14:01:20  midas
  Fixed small bugs

  Revision 1.15  2002/10/28 14:26:30  midas
  Changes from Japan

  Revision 1.14  2002/10/22 15:04:34  midas
  Added repeat readout mode

  Revision 1.13  2002/10/09 15:48:13  midas
  Fixed bug with download

  Revision 1.12  2002/10/09 11:06:46  midas
  Protocol version 1.1

  Revision 1.11  2002/10/07 15:16:32  midas
  Added upgrade facility

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
#include <io.h>

#elif defined(__linux__)

#define O_BINARY 0

#include <unistd.h>

#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include "mscb.h"
#include "rpc.h"

/*------------------------------------------------------------------*/

typedef struct {
  unsigned char id;
  char name[32];
} NAME_TABLE;

NAME_TABLE prefix_table[] = {
  { PRFX_PICO,  "Pico",   },
  { PRFX_NANO,  "Nano",   },
  { PRFX_MICRO, "Micro",  },
  { PRFX_MILLI, "Milli",  },
  { PRFX_NONE,  "",       },
  { PRFX_KILO,  "Kilo",   },
  { PRFX_MEGA,  "Mega",   },
  { PRFX_GIGA,  "Giga",   },
  { PRFX_TERA,  "Tera",   },
  { 0                     }
};

NAME_TABLE unit_table[] = {

  { UNIT_METER,      "meter",           },
  { UNIT_GRAM,       "gram",            },
  { UNIT_SECOND,     "second",          },
  { UNIT_MINUTE,     "minute",          },
  { UNIT_HOUR,       "hour",            },
  { UNIT_AMPERE,     "ampere",          },
  { UNIT_KELVIN,     "kelvin",          },
  { UNIT_CELSIUS,    "deg. celsius",    }, 
  { UNIT_FARENHEIT,  "deg. farenheit",  },   

  { UNIT_HERTZ,      "hertz",           },
  { UNIT_PASCAL,     "pascal",          },
  { UNIT_BAR,        "bar",             },
  { UNIT_WATT,       "watt",            },
  { UNIT_VOLT,       "volt",            },
  { UNIT_OHM,        "ohm",             },
  { UNIT_TESLA,      "tesls",           },
  { UNIT_LITERPERSEC,"liter/sec",       },
  { UNIT_RPM,        "RPM",             },

  { UNIT_BOOLEAN,    "boolean",         },
  { UNIT_BYTE,       "byte",            },
  { UNIT_WORD,       "word",            },
  { UNIT_DWORD,      "dword",           },
  { UNIT_ASCII,      "ascii",           },
  { UNIT_STRING,     "string",          },
  { UNIT_BAUD,       "baud",            },

  { UNIT_PERCENT,    "percent",         },
  { UNIT_PPM,        "RPM",             },
  { UNIT_COUNT,      "counts",          },
  { UNIT_FACTOR,     "factor",          },
  { 0                                   }
};

/*------------------------------------------------------------------*/

void print_help()
{
  puts("Available commands:\n");
  puts("scan [r]                   Scan bus for nodes [repeat mode]");
  puts("ping <addr>                Ping node and set address");
  puts("addr <addr>                Set address");
  puts("gaddr <addr>               Set group address");
  puts("info                       Retrive node info");
  puts("sa <addr> <gaddr>          Set node and group address of addressed node");
  puts("debug 0/1                  Turn debuggin off/on on node (LCD output)");
  puts("\nwrite <channel> <value>  Write to node channel");
  puts("read <channel> [r]         Read from node channel [repeat mode]");
  puts("wc <index> <value>         Write configuration parameter");
  puts("flash                      Flash parameters into EEPROM");
  puts("rc <index>                 Read configuration parameter");
  puts("reboot                     Reboot addressed node");
  puts("reset                      Reboot whole MSCB system");
  puts("terminal                   Enter teminal mode for SCS-210");
  puts("upload <hex-file>          Upload new firmware to node");
  puts("version                    Display version number");
  puts("echo [fc]                  Perform echo test [fast,continuous]");
}

/*------------------------------------------------------------------*/

void print_channel(int index, MSCB_INFO_CHN *info_chn, int data, int verbose)
{
char str[80];
int  i;

  if (verbose)
    {
    memset(str, 0, sizeof(str));
    strncpy(str, info_chn->name, 8);
    for (i=strlen(str) ; i<9 ; i++)
      str[i] = ' ';
    printf("%2d: %s ", index, str);
    }

  switch(info_chn->width)
    {
    case 0:  
      printf(" 0bit"); 
      break;

    case 1:  
      if (info_chn->flags & MSCBF_SIGNED)
        printf(" 8bit %8d (0x%02X)", data, data); 
      else
        printf(" 8bit %8u (0x%02X)", data, data); 
      break;

    case 2: 
      if (info_chn->flags & MSCBF_SIGNED)
        printf("16bit %8d (0x%04X)", data, data); 
      else
        printf("16bit %8u (0x%04X)", data, data); 
      break;

    case 3: 
      if (info_chn->flags & MSCBF_SIGNED)
        printf("24bit %8d (0x%06X)", data, data); 
      else
        printf("24bit %8u (0x%06X)", data, data); 
      break;
      
    case 4: 
      if (info_chn->flags & MSCBF_FLOAT)
        printf("32bit %8.6lg", *((float *)&data));
      else
        {
        if (info_chn->flags & MSCBF_SIGNED)
          printf("32bit %8d (0x%08X)", data, data);
        else
          printf("32bit %8u (0x%08X)", data, data);
        }
      break;
    }

  printf(" ");

  /* evaluate prfix */
  if (info_chn->prefix)
    {
    for (i=0 ; prefix_table[i].id ; i++)
      if (prefix_table[i].id == info_chn->prefix)
        break;
    if (prefix_table[i].id)
      printf(prefix_table[i].name);
    }

  /* evaluate unit */
  if (info_chn->unit)
    {
    for (i=0 ; unit_table[i].id ; i++)
      if (unit_table[i].id == info_chn->unit)
        break;
    if (unit_table[i].id)
      printf(unit_table[i].name);
    }

  if (verbose)
    printf("\n");
  else
    printf("            \r");
}

/*------------------------------------------------------------------*/

void cmd_loop(int fd, char *cmd, int adr)
{
int           i, fh, status, size, nparam, addr, gaddr, current_addr, current_group;
unsigned int  data;
unsigned char c;
float         value;
char          str[256], line[256];
char          param[10][100];
char          *pc, *buffer;
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

  if (adr)
    current_addr = adr;

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

    /* version */
    else if (param[0][0] == 'v')
      {
      char lib[32], prot[32];

      mscb_get_version(lib, prot);
      printf("MSCB library version  : %s\n", lib);
      printf("MSCB protocol version : %s\n", prot);
      }

    /* scan */
    else if ((param[0][0] == 's' && param[0][1] == 'c'))
      {
      do
        {
        for (i=0 ; i<0x10000 ; i++)
          {
          printf("Test address %d    \r", i);
          fflush(stdout);
        
          status = mscb_ping(fd, i);

          if (status == MSCB_SUCCESS)
            {
            status = mscb_info(fd, i, &info);
            if (status == MSCB_SUCCESS)
              {
              printf("Found node \"%s\", node addr. %d (0x%04X), group addr. %d (0x%04X)      \n", 
                info.node_name, i, i, info.group_address, info.group_address);
              }
            }

          if (i == 1000)
            i = 0xFFFE;
          }
        } while (param[1][0] == 'r' && !kbhit());

      while (kbhit())
        getch();

      printf("                       \n");
      }

    /* info */
    else if (param[0][0] == 'i')
      {
      if (current_addr < 0)
        printf("You must first address an individual node\n");
      else
        {
        status = mscb_info(fd, current_addr, &info);
        if (status == MSCB_CRC_ERROR)
          puts("CRC Error");
        else if (status != MSCB_SUCCESS)
          puts("No response from node");
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
          printf("Protocol version: %d.%d\n", info.protocol_version/16, info.protocol_version % 16);
          printf("Watchdog resets:  %d\n", info.watchdog_resets);

          printf("\nChannels:\n");
          for (i=0 ; i<info.n_channel ; i++)
            {
            mscb_info_channel(fd, current_addr, GET_INFO_CHANNEL, i, &info_chn);
            size = sizeof(data);
            mscb_read(fd, current_addr, (unsigned char)i, &data, &size);

            print_channel(i, &info_chn, data, 1);
            }

          printf("\nConfiguration Parameters:\n");
          for (i=0 ; i<info.n_conf ; i++)
            {
            mscb_info_channel(fd, current_addr, GET_INFO_CONF, i, &info_chn);
            size = sizeof(data);
            mscb_read_conf(fd, current_addr, (unsigned char)i, &data, &size);

            print_channel(i, &info_chn, data, 1);
            }
          }
        }
      }

    /* reboot */
    else if ((param[0][0] == 'r' && param[0][1] == 'e') && param[0][2] == 'b')
      {
      mscb_reboot(fd, current_addr);
      }

    /* reset */
    else if ((param[0][0] == 'r' && param[0][1] == 'e') && param[0][2] == 's')
      {
      mscb_reset(fd);
      }

    /* ping/address */
    else if ((param[0][0] == 'p') ||
             (param[0][0] == 'a'))
      {
      if (!param[1][0])
        puts("Please specify node address");
      else
        {
        if (param[1][1] == 'x')
          sscanf(param[1]+2, "%x", &addr);
        else
          addr = atoi(param[1]);

        status = mscb_ping(fd, addr);
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

    /* set group address */
    else if ((param[0][0] == 'g' && param[0][1] == 'a'))
      {
      if (!param[1][0])
        puts("Please specify node address");
      else
        {
        if (param[1][1] == 'x')
          sscanf(param[1]+2, "%x", &addr);
        else
          addr = atoi(param[1]);

        current_addr = -1;
        current_group = addr;
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

          mscb_set_addr(fd, current_addr, addr, gaddr);
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
          c = i ? CSR_DEBUG : 0;
          mscb_write_conf(fd, current_addr, 0xFF, &c, 1);
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

          if (current_addr >= 0)
            {
            mscb_info_channel(fd, current_addr, GET_INFO_CHANNEL, addr, &info_chn);
            if (info_chn.flags & MSCBF_FLOAT)
              {
              value = (float)atof(param[2]);
              memcpy(&data, &value, sizeof(float));
              }
            else
              {
              if (param[2][1] == 'x')
                sscanf(param[2]+2, "%x", &data);
              else
                data = atoi(param[2]);
              }

            status = mscb_write(fd, current_addr, (unsigned char)addr, &data, info_chn.width);
            }
          else if (current_group >= 0)
            {
            if (param[2][1] == 'x')
              sscanf(param[2]+2, "%x", &data);
            else
              data = atoi(param[2]);

            status = mscb_write_group(fd, current_group, (unsigned char)addr, &data, 4);
            }

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

          status = mscb_info_channel(fd, current_addr, GET_INFO_CHANNEL, addr, &info_chn);

          if (status == MSCB_CRC_ERROR)
            puts("CRC Error");
          else if (status != MSCB_SUCCESS)
            puts("Timeout or invalid channel number");
          else
            {
            do
              {
              size = sizeof(data);
              status = mscb_read(fd, current_addr, (unsigned char)addr, &data, &size);
              if (status != MSCB_SUCCESS)
                printf("Error: %d\n", status);
              else
                print_channel(addr, &info_chn, data, 0);
              } while (param[2][0] && !kbhit());

            while (kbhit())
              getch();

            printf("\n");
            }
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

          if (current_addr >= 0)
            {
            mscb_info_channel(fd, current_addr, GET_INFO_CONF, addr, &info_chn);
            if (info_chn.flags & MSCBF_FLOAT)
              {
              value = (float)atof(param[2]);
              memcpy(&data, &value, sizeof(float));
              }
            else
              {
              if (param[2][1] == 'x')
                sscanf(param[2]+2, "%x", &data);
              else
                data = atoi(param[2]);
              }

            status = mscb_write_conf(fd, current_addr, (unsigned char)addr, &data, info_chn.width);

            if (status != MSCB_SUCCESS)
              printf("Error: %d\n", status);
            }
          else
            printf("Please address individual node first.\n");
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
          puts("Please specify parameter index");
        else
          {
          addr = atoi(param[1]);

          status = mscb_info_channel(fd, current_addr, GET_INFO_CONF, addr, &info_chn);

          if (status == MSCB_CRC_ERROR)
            puts("CRC Error");
          else if (status != MSCB_SUCCESS)
            puts("Timeout or invalid parameter index");
          else
            {
            do
              {
              size = sizeof(data);
              status = mscb_read_conf(fd, current_addr, (unsigned char)addr, &data, &size);
              if (status != MSCB_SUCCESS)
                printf("Error: %d\n", status);
              else
                print_channel(addr, &info_chn, data, 0);
              } while (param[2][0] && !kbhit());

            while (kbhit())
              getch();

            printf("\n");
            }
          }
        }
      }

    /* terminal */
    else if ((param[0][0] == 't' && param[0][1] == 'e' && param[0][2] == 'r'))
      {
      int  d, status;

      puts("Exit with <ESC>\n");

      /* switch SCS-210 into terminal mode */
      c = 0;
      status = mscb_write(fd, current_addr, 0, &c, 1);

      do
        {
        if (kbhit())
          {
          c = getch();
          putchar(c);
          status = mscb_write(fd, current_addr, 0, &c, 1);
          if (status != MSCB_SUCCESS)
            {
            printf("\nError: %d\n", status);
            break;
            }

          if (c == '\r')
            {
            putchar('\n');
            c = '\n';
            mscb_write(fd, current_addr, 0, &c, 1);
            }

          }

        size = sizeof(d);
        mscb_read(fd, current_addr, 0, &d, &size);
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
        status = mscb_flash(fd, current_addr);

        if (status != MSCB_SUCCESS)
          printf("Error: %d\n", status);
        }
      }

    /* upload */
    else if (param[0][0] == 'u' && param[0][1] == 'p')
      {
      if (current_addr < 0)
        printf("You must first address an individual node\n");
      else
        {
        if (param[1][0] == 0)
          {
          printf("Enter name of HEX-file: ");
          fgets(str, sizeof(str), stdin);
          }
        else
          strcpy(str, param[1]);

        if (str[strlen(str)-1] == '\n')
          str[strlen(str)-1] = 0;

        fh = open(str, O_RDONLY | O_BINARY);
        if (fh > 0)
          {
          size = lseek(fh, 0, SEEK_END)+1;
          lseek(fh, 0, SEEK_SET);
          buffer = malloc(size);
          memset(buffer, 0, size);
          read(fh, buffer, size-1);
          close(fh);
          status = mscb_upload(fd, current_addr, buffer, size);
          if (status != MSCB_SUCCESS)
            printf("Syntax error in file \"%s\"\n", str);
          free(buffer);
          }
        else
          printf("File \"%s\" not found\n", str);
        }
      }

    /* echo test */
    else if (param[0][0] == 'e' && param[0][1] == 'c')
      {
      unsigned char d1, d2;
      int i, status;

      if (current_addr < 0)
        printf("You must first address an individual node\n");
      else
        {
        i = 0;
        while (!kbhit())
          {
          d1 = rand();

          status = mscb_echo(fd, current_addr, d1, &d2);

          if (d2 != d1)
            {
            printf("%d\nReceived: %02X, should be %02X, status = %d\n", i, d2, d1, status);

            if (param[1][0] != 'c' && param[1][1] != 'c')
              break;
            }

          i++;
          if (i % 100 == 0)
            {
            printf("%d\r", i);
            fflush(stdout);
            }

          if (param[1][0] != 'f' && param[1][1] != 'c')
            Sleep(10);
          }
        }
      }

    else if (param[0][0] == 't' && param[0][1] == '1')
      {
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

int main(int argc, char *argv[])
{
int   i, fd, adr, server;
char  cmd[256], device[256];
int   debug, check_io;

  cmd[0] = 0;
  adr = 0;
#ifdef _MSC_VER
  strcpy(device, "lpt1");
#elif defined(__linux__)
  strcpy(device, "/dev/parport0");
#else
  strcpy(device, "");
#endif

  debug = check_io = server = 0;

  /* parse command line parameters */
  for (i=1 ; i<argc ; i++)
    {
    if (argv[i][0] == '-' && argv[i][1] == 'v')
      debug = 1;
    else if (argv[i][0] == '-' && argv[i][1] == 'i')
      check_io = 1;
    else if (argv[i][0] == '-' && argv[i][1] == 's')
      server = 1;
    else if (argv[i][0] == '-')
      {
      if (i+1 >= argc || argv[i+1][0] == '-')
        goto usage;
      else if (argv[i][1] == 'd')
        strcpy(device, argv[++i]);
      else if (argv[i][1] == 'a')
        adr = atoi(argv[++i]);
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
        printf("usage: msc [-d host:device] [-a addr] [-c Command] [-c @CommandFile] [-v] [-i]\n\n");
        printf("       -d     device, usually \"%s\" for parallel port,\n", device);
        printf("              or 0x378 for direct parallel port address,\n");
        printf("              or \"<host>:%s\" for RPC connection\n", device);
        printf("       -s     Start RPC server\n");
        printf("       -a     Address node before executing command\n");
        printf("       -c     Execute command immediately\n");
        printf("       -v     Produce verbose output\n\n");
        printf("       -i     Check IO pins of port\n\n");
        printf("For a list of valid commands start msc interactively\n");
        printf("and type \"help\".\n");
        return 0;
        }
      }
    }

  if (server)
    {
    rpc_server_loop();
    return 0;
    }

  if (check_io)
    {
    mscb_check(device);
    return 0;
    }

  /* open port */
  fd = mscb_init(device);
  if (fd < 0)
    {
    if (fd == -2)
      {
      printf("No MSCB submaster present at port \"%s\"\n", device);

      puts("\nMake sure that");
      puts("");
      puts("o The MSCB submaster is correctly connected to the parallel port");
      puts("o The MSCB submaster has power (green LED on)");
      puts("");
      puts("If this is a new installation, check in the PC BIOS that");
      puts("");
      puts("o LPT1 is in bi-directional (or EPP) mode, NOT ECP mode");
      puts("o LPT1 is configured to address 0x378 (hex)");
      puts("o If BIOS seems ok, you can check the pin I/O with a logic probe");
      puts("  and the \"msc -i\" command which toggles the pins of the parallel port");
      puts("o If the port is configured to another address than 0x378, use");
      puts("  \"msc -d 0xabc\" with \"0xabc\" the correct address");
      }
    else
      printf("Cannot connect to device \"%s\"\n", device);

    puts("\n-- hit any key to exit --");

    while (!kbhit());
    while (kbhit())
      getch();

    return 0;
    }

  cmd_loop(fd, cmd, adr);

  mscb_exit(fd);

  return 0;
}
