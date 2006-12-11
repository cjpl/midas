/********************************************************************\

  Name:         msc.c
  Created by:   Stefan Ritt

  Contents:     Command-line interface for the Midas Slow Control Bus

  $Id$

\********************************************************************/

#ifdef _MSC_VER

#include <windows.h>
#include <conio.h>
#include <io.h>

#elif defined(OS_LINUX)

#define O_BINARY 0

#include <unistd.h>
#include <ctype.h>

#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include "mscb.h"
#include "mscbrpc.h"
#include "mxml.h"
#include "strlcpy.h"

/*------------------------------------------------------------------*/

typedef struct {
   unsigned char id;
   char name[32];
} NAME_TABLE;

NAME_TABLE prefix_table[] = {
   {PRFX_PICO, "pico",},
   {PRFX_NANO, "nano",},
   {PRFX_MICRO, "micro",},
   {PRFX_MILLI, "milli",},
   {PRFX_NONE, "",},
   {PRFX_KILO, "kilo",},
   {PRFX_MEGA, "mega",},
   {PRFX_GIGA, "giga",},
   {PRFX_TERA, "tera",},
   {0}
};

NAME_TABLE unit_table[] = {

   {UNIT_METER, "meter",},
   {UNIT_GRAM, "gram",},
   {UNIT_SECOND, "second",},
   {UNIT_MINUTE, "minute",},
   {UNIT_HOUR, "hour",},
   {UNIT_AMPERE, "ampere",},
   {UNIT_KELVIN, "kelvin",},
   {UNIT_CELSIUS, "deg. celsius",},
   {UNIT_FARENHEIT, "deg. farenheit",},

   {UNIT_HERTZ, "hertz",},
   {UNIT_PASCAL, "pascal",},
   {UNIT_BAR, "bar",},
   {UNIT_WATT, "watt",},
   {UNIT_VOLT, "volt",},
   {UNIT_OHM, "ohm",},
   {UNIT_TESLA, "tesls",},
   {UNIT_LITERPERSEC, "liter/sec",},
   {UNIT_RPM, "RPM",},
   {UNIT_FARAD, "farad",},

   {UNIT_BOOLEAN, "boolean",},
   {UNIT_BYTE, "byte",},
   {UNIT_WORD, "word",},
   {UNIT_DWORD, "dword",},
   {UNIT_ASCII, "ascii",},
   {UNIT_STRING, "string",},
   {UNIT_BAUD, "baud",},

   {UNIT_PERCENT, "percent",},
   {UNIT_PPM, "RPM",},
   {UNIT_COUNT, "counts",},
   {UNIT_FACTOR, "factor",},
   {0}
};

/*------------------------------------------------------------------*/

void print_help()
{
   puts("Available commands:\n");
   puts("addr <addr>                Address individual node");
   puts("baddr                      Address all nodes (broadcast)");
   puts("debug 0/1                  Turn debuggin off/on on node (LCD output)");
   puts("echo [fc]                  Perform echo test [fast,continuous]");
   puts("flash                      Flash parameters into EEPROM");
   puts("gaddr <addr>               Address group of nodes");
   puts("info                       Retrive node info");
   puts("load <file>                Load node variables");
   puts("ping <addr> [r]            Ping node and set address [repeat mode]");
   puts("read <index> [r] [a]       Read node variable [repeat mode]  [all variables]");
   puts("reboot                     Reboot addressed node");
   puts("reset                      Reboot whole MSCB system");
   puts("sa <addr>                  Set node address of addressed node");
   puts("save <file>                Save current node variables");
   puts("scan [r] [a]               Scan bus for nodes [repeat mode] [all]");
   puts("sg <addr>                  Set group address of addressed node(s)");
   puts("sn <name>                  Set node name (up to 16 characters)");
   puts("terminal                   Enter teminal mode for SCS-210");
   puts("upload <hex-file> [debug]  Upload new firmware to node [with debug info]");
   puts("mup <hex-file> <a1> <a2>   Upload new firmware to nodes a1-a2");
   puts("verify <hex-file>          Compare current firmware with file");
   puts("version                    Display version number");
   puts("write <index> <value> [r]  Write node variable");
   puts("write <i1>-<i2> <value>    Write range of node variables");

   puts("");
}

/*------------------------------------------------------------------*/

void print_channel_str(int index, MSCB_INFO_VAR * info_chn, void *pdata, int verbose,
                       char *line)
{
   char str[80];
   int i, data;

   line[0] = 0;
   if (verbose) {
      memset(str, 0, sizeof(str));
      strncpy(str, info_chn->name, 8);
      for (i = strlen(str); i < 9; i++)
         str[i] = ' ';
      sprintf(line + strlen(line), "%3d: %s ", index, str);
   } else
      sprintf(line, "%s: ", info_chn->name);

   if (info_chn->unit == UNIT_STRING) {
      memset(str, 0, sizeof(str));
      strncpy(str, pdata, info_chn->width);
      if (verbose)
         sprintf(line + strlen(line), "STR%02d    \"", info_chn->width);
      for (i = 0; i < (int) strlen(str); i++)
         switch (str[i]) {
         case 1:
            strcat(line, "\\001");
            break;
         case 2:
            strcat(line, "\\002");
            break;
         case 9:
            strcat(line, "\\t");
            break;
         case 10:
            strcat(line, "\\n");
            break;
         case 13:
            strcat(line, "\\r");
            break;
         default:
            line[strlen(line) + 1] = 0;
            line[strlen(line)] = str[i];
            break;
         }
      strcat(line, "\"");
   } else {
      data = *((int *) pdata);
      switch (info_chn->width) {
      case 0:
         printf(" 0bit");
         break;

      case 1:
         if (info_chn->flags & MSCBF_SIGNED) {
            if (verbose)
               sprintf(line + strlen(line), " 8bit S %15d (0x%02X/", (char) data, data);
            else
               sprintf(line + strlen(line), "%15d (0x%02X/", (char) data, data);
         } else {
            if (verbose)
               sprintf(line + strlen(line), " 8bit U %15u (0x%02X/", data, data);
            else
               sprintf(line + strlen(line), "%15u (0x%02X/", data, data);
         }
         for (i = 0; i < 8; i++)
            if (data & (0x80 >> i))
               sprintf(line + strlen(line), "1");
            else
               sprintf(line + strlen(line), "0");
         sprintf(line + strlen(line), ")");
         break;

      case 2:
         if (info_chn->flags & MSCBF_SIGNED) {
            if (verbose)
               sprintf(line + strlen(line), "16bit S %15d (0x%04X)", (short) data, data);
            else
               sprintf(line + strlen(line), "%15d (0x%04X)", (short) data, data);
         } else {
            if (verbose)
               sprintf(line + strlen(line), "16bit U %15u (0x%04X)", data, data);
            else
               sprintf(line + strlen(line), "%15u (0x%04X)", data, data);
         }
         break;

      case 3:
         if (info_chn->flags & MSCBF_SIGNED) {
            if (verbose)
               sprintf(line + strlen(line), "24bit S %15ld (0x%06X)", (long) data, data);
            else
               sprintf(line + strlen(line), "%15ld (0x%06X)", (long) data, data);
         } else {
            if (verbose)
               sprintf(line + strlen(line), "24bit U %15u (0x%06X)", data, data);
            else
               sprintf(line + strlen(line), "%15ld (0x%06X)", (long) data, data);
         }
         break;

      case 4:
         if (info_chn->flags & MSCBF_FLOAT) {
            if (verbose)
               sprintf(line + strlen(line), "32bit F %15.6lg",
                       *((float *) (void *) &data));
            else
               sprintf(line + strlen(line), "%15.6lg", *((float *) (void *) &data));
         } else {
            if (info_chn->flags & MSCBF_SIGNED) {
               if (verbose)
                  sprintf(line + strlen(line), "32bit S %15d (0x%08X)", data, data);
               else
                  sprintf(line + strlen(line), "%15d (0x%08X)", data, data);
            } else {
               if (verbose)
                  sprintf(line + strlen(line), "32bit U %15u (0x%08X)", data, data);
               else
                  sprintf(line + strlen(line), "%15u (0x%08X)", data, data);
            }
         }
         break;
      }
   }

   sprintf(line + strlen(line), " ");

   /* evaluate prfix */
   if (info_chn->prefix) {
      for (i = 0; prefix_table[i].id; i++)
         if (prefix_table[i].id == info_chn->prefix)
            break;
      if (prefix_table[i].id)
         sprintf(line + strlen(line), prefix_table[i].name);
   }

   /* evaluate unit */
   if (info_chn->unit && info_chn->unit != UNIT_STRING) {
      for (i = 0; unit_table[i].id; i++)
         if (unit_table[i].id == info_chn->unit)
            break;
      if (unit_table[i].id)
         sprintf(line + strlen(line), unit_table[i].name);
   }

   if (verbose)
      sprintf(line + strlen(line), "\n");
   else
      sprintf(line + strlen(line), "                    \r");
}

void print_channel(int index, MSCB_INFO_VAR * info_chn, void *pdata, int verbose)
{
   char str[256];

   print_channel_str(index, info_chn, pdata, verbose, str);
   fputs(str, stdout);
}

/*------------------------------------------------------------------*/

void save_node_xml(MXML_WRITER * writer, int fd, int addr)
{
   int i, j, status, size;
   char str[256], line[256], data[256];
   MSCB_INFO info;
   MSCB_INFO_VAR info_var;

   status = mscb_info(fd, (unsigned short) addr, &info);
   if (status == MSCB_CRC_ERROR) {
      puts("CRC Error");
      return;
   } else if (status != MSCB_SUCCESS) {
      puts("No response from node");
      return;
   }

   mxml_start_element(writer, "Node");

   mxml_start_element(writer, "Name");
   mxml_write_value(writer, info.node_name);
   mxml_end_element(writer);

   mxml_start_element(writer, "NodeAddress");
   sprintf(str, "%d (0x%X)", addr, addr);
   mxml_write_value(writer, str);
   mxml_end_element(writer);

   mxml_start_element(writer, "GroupAddress");
   sprintf(str, "%d (0x%X)", info.group_address, info.group_address);
   mxml_write_value(writer, str);
   mxml_end_element(writer);

   mxml_start_element(writer, "ProtocolVersion");
   sprintf(str, "%d.%d", info.protocol_version / 16, info.protocol_version % 16);
   mxml_write_value(writer, str);
   mxml_end_element(writer);

   mxml_start_element(writer, "Variables");
   for (i = 0; i < info.n_variables; i++) {
      mxml_start_element(writer, "Variable");

      mscb_info_variable(fd, (unsigned short) addr, (unsigned char) i, &info_var);
      size = sizeof(data);
      mscb_read(fd, (unsigned short) addr, (unsigned char) i, data, &size);

      mxml_start_element(writer, "Index");
      sprintf(str, "%d", i);
      mxml_write_value(writer, str);
      mxml_end_element(writer);

      mxml_start_element(writer, "Name");
      strncpy(str, info_var.name, 8);
      str[8] = 0;
      mxml_write_value(writer, str);
      mxml_end_element(writer);

      mxml_start_element(writer, "Width");
      sprintf(str, "%dbit", info_var.width * 8);
      mxml_write_value(writer, str);
      mxml_end_element(writer);

      print_channel_str(i, &info_var, data, 1, line);

      strlcpy(str, line + 21, sizeof(str));
      for (j = 0; j < (int) strlen(str); j++)
         if (str[j] == ' ')
            break;
      str[j] = 0;
      mxml_start_element(writer, "Flags");
      mxml_write_value(writer, str);
      mxml_end_element(writer);

      strlcpy(str, line + 21 + j, sizeof(str));
      for (j += 21; j < (int) strlen(line); j++)
         if (line[j] != ' ')
            break;

      strlcpy(str, line + j, sizeof(str));
      for (j = 0; j < (int) strlen(str); j++)
         if (str[j] == ' ')
            break;
      str[j] = 0;
      mxml_start_element(writer, "Value");
      mxml_write_value(writer, str);
      mxml_end_element(writer);

      if (line[39] == '(') {
         strlcpy(str, line + 40, sizeof(str));
         for (j = 0; j < (int) strlen(str); j++)
            if (str[j] == ')')
               break;
         str[j] = 0;
         mxml_start_element(writer, "HexBinValue");
         mxml_write_value(writer, str);
         mxml_end_element(writer);
         j += 42;
      } else
         j = 39;

      strlcpy(str, line + j, sizeof(str));
      for (j = 0; j < (int) strlen(str); j++)
         if (!isalnum(str[j]))
            break;
      str[j] = 0;
      mxml_start_element(writer, "Unit");
      mxml_write_value(writer, str);
      mxml_end_element(writer);

      mxml_end_element(writer); // "Variable"
   }

   mxml_end_element(writer);    // "Variables"
   mxml_end_element(writer);    // "Node"
}

/*------------------------------------------------------------------*/

void load_nodes_xml(int fd, char *file_name, int flash)
{
   int i, j, index, status, ivar, addr, var_index;
   char str[256], error[256], chn_name[256][9], name[256], value[256];
   unsigned int data;
   float fvalue;
   PMXML_NODE root, node, nvarroot, nvar, nsub;
   MSCB_INFO info;
   MSCB_INFO_VAR info_var;

   root = mxml_parse_file(file_name, error, sizeof(error));
   if (root == NULL) {
      printf("Error loading \"%s\": %s\n", file_name, error);
      return;
   }

   root = mxml_find_node(root, "/MSCBDump");
   if (root == NULL) {
      printf("Error loading \"%s\": No MSCBDump structure in file\n", file_name);
      return;
   }

   /* loop over all nodes in file */
   for (index = 0; index < mxml_get_number_of_children(root); index++) {
      node = mxml_subnode(root, index);

      nsub = mxml_find_node(node, "NodeAddress");
      if (nsub == NULL) {
         printf("Error loading \"%s\": Node #%d does not contain NodeAddress\n",
                file_name, index);
         return;
      }

      printf("\nLoading node #%s\n", mxml_get_value(nsub));
      addr = atoi(mxml_get_value(nsub));

      nvarroot = mxml_find_node(node, "Variables");
      if (nvarroot == NULL) {
         printf("Error loading \"%s\": Node #%d does not contain Variables\n", file_name,
                index);
         return;
      }

      /* get list of channel names from node */
      if (mscb_info(fd, (unsigned short) addr, &info) != MSCB_SUCCESS) {
         printf("No response from node %d, skip node.\n", addr);
         continue;
      }

      memset(chn_name, 0, sizeof(chn_name));
      for (i = 0; i < info.n_variables; i++) {
         mscb_info_variable(fd, (unsigned short) addr, (unsigned char) i, &info_var);
         strncpy(chn_name[i], info_var.name, 8);
      }

      /* loop over variables */
      for (ivar = 0; ivar < mxml_get_number_of_children(nvarroot); ivar++) {

         nvar = mxml_subnode(nvarroot, ivar);
         nsub = mxml_find_node(nvar, "Index");
         if (nsub == NULL) {
            printf("Found variable without index\n");
            continue;
         }
         var_index = atoi(mxml_get_value(nsub));
         nsub = mxml_find_node(nvar, "Name");
         if (nsub == NULL) {
            printf("Found variable without name\n");
            continue;
         }

         strcpy(name, mxml_get_value(nsub));

         nsub = mxml_find_node(nvar, "Value");
         if (nsub == NULL) {
            printf("Found variable without value\n");
            continue;
         }

         strcpy(value, mxml_get_value(nsub));

         /* search for channel with same name */
         for (i = 0; chn_name[i][0]; i++)
            if (strcmp(chn_name[i], name) == 0) {
               mscb_info_variable(fd, (unsigned short) addr, (unsigned char) i,
                                  &info_var);

               if (info_var.unit == UNIT_STRING) {
                  memset(str, 0, sizeof(str));
                  strncpy(str, value, info_var.width);
                  if (strlen(str) > 0 && str[strlen(str) - 1] == '\n')
                     str[strlen(str) - 1] = 0;

                  status =
                      mscb_write(fd, (unsigned short) addr, (unsigned char) i, str,
                                 info_var.width);
                  if (status != MSCB_SUCCESS)
                     printf("Error writing to node %d, variable %d\n", addr, i);

               } else {
                  if (info_var.flags & MSCBF_FLOAT) {
                     fvalue = (float) atof(value);
                     memcpy(&data, &fvalue, sizeof(float));
                  } else {
                     data = atoi(value);
                  }

                  status =
                      mscb_write(fd, (unsigned short) addr, (unsigned char) i, &data,
                                 info_var.width);
                  if (status != MSCB_SUCCESS)
                     printf("Error writing to node %d, variable %d\n", addr, i);

                  /* blank padding */
                  for (j = strlen(name); j < 8; j++)
                     name[j] = ' ';
                  name[j] = 0;

                  printf("%3d: %s %s\n", i, name, value);
               }

               break;
            }

         if (chn_name[i][0] == 0)
            printf("Variable \"%s\" from file not in node, variable skipped\n", name);
      }

      if (flash) {
         mscb_flash(fd, addr, -1, 0);
         Sleep(200);
      }
   }
}

/*------------------------------------------------------------------*/

int match(char *str, char *cmd)
{
   int i;

   if (str[0] == '\r' || str[0] == '\n')
      return 0;

   for (i = 0; i < (int) strlen(str); i++) {
      if (toupper(str[i]) != toupper(cmd[i]) && str[i] != '\r' && str[i] != '\n')
         return 0;
   }

   return 1;
}

/*------------------------------------------------------------------*/

void cmd_loop(int fd, char *cmd, unsigned short adr)
{
   int i, fh, status, size, nparam, current_addr, current_group, first, last, broadcast,
       read_all, repeat, index, idx1, idx2, wait = 0;
   unsigned short addr;
   unsigned int data, uptime;
   unsigned char c;
   float value;
   char str[256], line[256], dbuf[100 * 1024], param[10][100], *pc, *buffer, lib[32],
       prot[32];
   FILE *cmd_file = NULL;
   MSCB_INFO info;
   MSCB_INFO_VAR info_var;
   time_t start, now;
   MXML_WRITER *writer;

   /* open command file */
   if (cmd[0] == '@') {
      cmd_file = fopen(cmd + 1, "r");
      if (cmd_file == NULL) {
         printf("Command file %s not found.\n", cmd + 1);
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

   broadcast = 0;

   status = 0;

   do {
      /* print prompt */
      if (!cmd[0]) {
         if (current_addr >= 0)
            printf("node%d(0x%X)> ", current_addr, current_addr);
         else if (current_group >= 0)
            printf("group%d> ", current_group);
         else if (broadcast)
            printf("all> ");
         else
            printf("> ");
         memset(line, 0, sizeof(line));
         fgets(line, sizeof(line), stdin);
      } else if (cmd[0] != '@')
         strcpy(line, cmd);
      else {
         memset(line, 0, sizeof(line));
         fgets(line, sizeof(line), cmd_file);

         if (line[0] == 0)
            break;

         /* cut off CR */
         while (strlen(line) > 0 && line[strlen(line) - 1] == '\n')
            line[strlen(line) - 1] = 0;

         if (line[0] == 0)
            continue;
      }

      /* analyze line */
      nparam = 0;
      pc = line;
      while (*pc == ' ')
         pc++;

      memset(param, 0, sizeof(param));
      do {
         if (*pc == '"') {
            pc++;
            for (i = 0; *pc && *pc != '"'; i++)
               param[nparam][i] = *pc++;
            if (*pc)
               pc++;
         } else if (*pc == '\'') {
            pc++;
            for (i = 0; *pc && *pc != '\''; i++)
               param[nparam][i] = *pc++;
            if (*pc)
               pc++;
         } else if (*pc == '`') {
            pc++;
            for (i = 0; *pc && *pc != '`'; i++)
               param[nparam][i] = *pc++;
            if (*pc)
               pc++;
         } else
            for (i = 0; *pc && *pc != ' '; i++)
               param[nparam][i] = *pc++;
         param[nparam][i] = 0;
         while (*pc == ' ' || *pc == '\r' || *pc == '\n')
            pc++;
         nparam++;
      } while (*pc);

      /* help ---------- */
      if ((param[0][0] == 'h' && param[0][1] == 'e') || param[0][0] == '?')
         print_help();

      /* version ---------- */
      else if (match(param[0], "version")) {
         mscb_get_version(lib, prot);
         printf("MSCB library version  : %s\n", lib);
         printf("MSCB protocol version : %s\n", prot);
      }

      /* scan ---------- */
      else if (match(param[0], "scan")) {
         do {
            /* decimal search */
            for (i = -1; i < 0x10000; i++) {
               if (i == -1)
                  printf("Test address 0xFFFF\r");
               else
                  printf("Test address %d    \r", i);
               fflush(stdout);

               if (i == -1)
                  /* do the first time with retry, to send '0's */
                  status = mscb_addr(fd, MCMD_PING16, (unsigned short) 0xFFFF, 2, 1);
               else
                  status = mscb_ping(fd, (unsigned short) i);

               if (status == MSCB_SUCCESS) {
                  status = mscb_info(fd, (unsigned short) i, &info);
                  strncpy(str, info.node_name, sizeof(info.node_name));
                  str[16] = 0;

                  if (status == MSCB_SUCCESS) {
                     printf
                         ("Found node \"%s\", node addr. %d (0x%04X), group addr. %d (0x%04X)      \n",
                          str, (unsigned short) i, (unsigned short) i, info.group_address,
                          info.group_address);

                     mscb_get_version(lib, prot);
                     if (info.protocol_version != atoi(prot)) {
                        printf
                            ("WARNING: Protocol version on node (%d) differs from local version (%s).\n",
                             info.protocol_version, prot);
                     }

                  }
               } else if (status == MSCB_SUBM_ERROR) {
                  printf("Error: Submaster not responding\n");
                  break;
               }

               if (param[1][0] != 'a' && param[2][0] != 'a')
                  if (i && i % 1000 == 0 && status != MSCB_SUCCESS) {
                     i += 999;
                  }

               if (param[1][0] != 'a' && param[2][0] != 'a')
                  if (i && i % 100 == 0 && status != MSCB_SUCCESS) {
                     i += 99;
                  }

               if (kbhit())
                  break;
            }

            /* hexadecimal search */
            for (i = 0x100; i < 0xFFFF; i++) {
               printf("Test address 0x%x    \r", i);
               fflush(stdout);

               status = mscb_ping(fd, (unsigned short) i);

               if (status == MSCB_SUCCESS) {
                  status = mscb_info(fd, (unsigned short) i, &info);
                  strncpy(str, info.node_name, sizeof(info.node_name));
                  str[16] = 0;

                  if (status == MSCB_SUCCESS) {
                     printf
                         ("Found node \"%s\", node addr. %d (0x%04X), group addr. %d (0x%04X)      \n",
                          str, i, i, info.group_address, info.group_address);
                  }
               } else if (status == MSCB_SUBM_ERROR) {
                  printf("Error: Submaster not responding\n");
                  break;
               }

               if (param[1][0] != 'a' && param[2][0] != 'a')
                  if (i && (i & 0xFF) == 0 && i != 0xFF00 && status != MSCB_SUCCESS) {
                     i += 0xFF;
                  }

               if (kbhit())
                  break;
            }

         } while ((param[1][0] == 'r' || param[2][0] == 'r') && !kbhit());

         while (kbhit())
            getch();

         printf("                       \n");
      }

      /* info ---------- */
      else if (match(param[0], "info")) {
         if (current_addr < 0)
            printf("You must first address an individual node\n");
         else {
            status = mscb_info(fd, (unsigned short) current_addr, &info);
            if (status == MSCB_CRC_ERROR)
               puts("CRC Error");
            else if (status != MSCB_SUCCESS)
               puts("No response from node");
            else {
               strncpy(str, info.node_name, sizeof(info.node_name));
               str[16] = 0;
               printf("Node name        : %s\n", str);
               printf("Node address     : %d (0x%X)\n", info.node_address,
                      info.node_address);
               printf("Group address    : %d (0x%X)\n", info.group_address,
                      info.group_address);
               printf("Protocol version : %d\n", info.protocol_version);
               printf("Watchdog resets  : %d\n", info.watchdog_resets);

               status = mscb_uptime(fd, (unsigned short) current_addr, &uptime);
               if (status == MSCB_SUCCESS)
                  printf("Uptime           : %dd %02dh %02dm %02ds\n",
                         uptime / (3600 * 24),
                         (uptime % (3600 * 24)) / 3600, (uptime % 3600) / 60,
                         (uptime % 60));

               mscb_get_version(lib, prot);
               if (info.protocol_version != atoi(prot)) {
                  printf
                      ("\nWARNING: Protocol version on node (%d) differs from local version (%s).\n",
                       info.protocol_version, prot);
                  printf
                      ("Problems may arise communicating with this node. Please upgrade.\n\n");
               }
            }
         }
      }

      /* ping ---------- */
      else if (match(param[0], "ping")) {
         if (!param[1][0])
            puts("Please specify node address");
         else {
            if (param[1][1] == 'x') {
               sscanf(param[1] + 2, "%x", &i);
               addr = (unsigned short) i;
            } else
               addr = (unsigned short) atoi(param[1]);

            do {
               status = mscb_addr(fd, MCMD_PING16, addr, 10, 1);

               if (status != MSCB_SUCCESS) {
                  if (status == MSCB_MUTEX)
                     printf("MSCB used by other process\n");
                  else if (status == MSCB_SUBM_ERROR)
                     printf("Error: Submaster not responding\n");
                  else
                     printf("Node %d does not respond\n", addr);
                  current_addr = -1;
                  current_group = -1;
                  broadcast = 0;
               } else {
                  printf("Node %d addressed\n", addr);
                  current_addr = addr;
                  current_group = -1;
                  broadcast = 0;
               }

               if (param[2][0] == 'r' && !kbhit())
                  Sleep(1000);

            } while (param[2][0] == 'r' && !kbhit());
         }
      }

      /* address ---------- */
      else if (match(param[0], "addr")) {
         if (!param[1][0])
            puts("Please specify node address");
         else {
            if (param[1][1] == 'x') {
               sscanf(param[1] + 2, "%x", &i);
               addr = (unsigned short) i;
            } else
               addr = (unsigned short) atoi(param[1]);

            mscb_addr(fd, MCMD_ADDR_NODE16, addr, 1, 1);
            current_addr = addr;
            current_group = -1;
            broadcast = 0;
         }
      }

      /* ba, broadcast address ---------- */
      else if (match(param[0], "baddr")) {
         current_addr = -1;
         current_group = -1;
         broadcast = 1;
      }

      /* ga, group address ---------- */
      else if (match(param[0], "gaddr")) {
         if (!param[1][0])
            puts("Please specify group address");
         else {
            if (param[1][1] == 'x') {
               sscanf(param[1] + 2, "%x", &i);
               addr = (unsigned short) i;
            } else
               addr = (unsigned short) atoi(param[1]);

            current_addr = -1;
            current_group = addr;
            broadcast = 0;
         }
      }

      /* sa, set node address ---------- */
      else if (match(param[0], "sa")) {
         if (current_addr < 0 && current_group < 0 && !broadcast)
            printf("You must first address node(s)\n");
         else {
            if (!param[1][0])
               puts("Please specify node address");
            else {
               if (param[1][1] == 'x') {
                  sscanf(param[1] + 2, "%x", &i);
                  addr = (unsigned short) i;
               } else
                  addr = (unsigned short) atoi(param[1]);

               status =
                   mscb_set_node_addr(fd, current_addr, current_group, broadcast, addr);
               if (current_addr >= 0) {
                  if (status == MSCB_ADDR_EXISTS)
                     printf("Error: Address %d exists already on this network\n", addr);
                  else
                     current_addr = addr;
               }
            }
         }
      }

      /* sg, set group address ---------- */
      else if (match(param[0], "sg")) {
         if (current_addr < 0 && current_group < 0 && !broadcast)
            printf("You must first address node(s)\n");
         else {
            if (!param[1][0])
               puts("Please specify group address");
            else {
               if (param[1][1] == 'x') {
                  sscanf(param[1] + 2, "%x", &i);
                  addr = (unsigned short) i;
               } else
                  addr = (unsigned short) atoi(param[1]);

               mscb_set_group_addr(fd, current_addr, current_group, broadcast, addr);
            }
         }
      }

      /* sn, set node name ---------- */
      else if (match(param[0], "sn")) {
         if (current_addr < 0)
            printf("You must first address an individual node\n");
         else {
            if (!param[1][0])
               puts("Please specify node name");
            else {
               strcpy(str, param[1]);
               while (strlen(str) > 0
                      && (str[strlen(str) - 1] == '\r' || str[strlen(str) - 1] == '\n'))
                  str[strlen(str) - 1] = 0;

               mscb_set_name(fd, (unsigned short) current_addr, str);
            }
         }
      }

      /* sm, set MAC address etc. ---------- */
      else if (match(param[0], "sm")) {
         if (set_mac_address(fd))
            break;              // exit program
      }

      /* debug ---------- */
      else if (match(param[0], "debug")) {
         if (current_addr < 0)
            printf("You must first address an individual node\n");
         else {
            if (!param[1][0])
               puts("Please specify 0/1");
            else {
               i = atoi(param[1]);

               /* write CSR register */
               c = i ? CSR_DEBUG : 0;
               mscb_write(fd, (unsigned short) current_addr, 0xFF, &c, 1);
            }
         }
      }

      /* write ---------- */
      else if (match(param[0], "write")) {
         if (current_addr < 0 && current_group < 0)
            printf("You must first address a node or a group\n");
         else {
            if (!param[1][0] || !param[2][0])
               puts("Please specify channel number and data");
            else {
               idx1 = atoi(param[1]);
               if (strchr(param[1], '-'))
                  idx2 = atoi(strchr(param[1], '-')+1);
               else
                  idx2 = idx1;

               if (current_addr < 0) {
                  printf("Enter address of first node in group %d: ", current_group);
                  fgets(str, sizeof(str), stdin);
                  addr = atoi(str);
               } else
                  addr = current_addr;

               for (index=idx1 ; index<=idx2 ; index++) {
                  mscb_info_variable(fd, (unsigned short) addr, (unsigned char) index,
                                    &info_var);

                  if (info_var.unit == UNIT_STRING) {
                     memset(str, 0, sizeof(str));
                     strncpy(str, param[2], info_var.width);
                     if (strlen(str) > 0 && str[strlen(str) - 1] == '\n')
                        str[strlen(str) - 1] = 0;

                     do {
                        if (current_addr >= 0)
                           status = mscb_write(fd, (unsigned short) current_addr,
                                             (unsigned char) index, str, strlen(str) + 1);
                        else
                           status = mscb_write_group(fd, (unsigned short) current_group,
                                                   (unsigned char) index, str,
                                                   strlen(str) + 1);
                        if (param[3][0])
                           Sleep(1000);
                     } while (param[3][0] && !kbhit());

                  } else if (info_var.unit == UNIT_ASCII) {
                     memset(str, 0, sizeof(str));
                     strncpy(str, param[2], sizeof(str));
                     if (strlen(str) > 0 && str[strlen(str) - 1] == '\n')
                        str[strlen(str) - 1] = 0;

                     do {
                        if (current_addr >= 0)
                           status =
                              mscb_write(fd, (unsigned short) current_addr,
                                          (unsigned char) index, str, strlen(str) + 1);
                        Sleep(100);
                     } while (param[3][0] && !kbhit());

                  } else {
                     if (info_var.flags & MSCBF_FLOAT) {
                        value = (float) atof(param[2]);
                        memcpy(&data, &value, sizeof(float));
                     } else {
                        if (param[2][1] == 'x')
                           sscanf(param[2] + 2, "%x", &data);
                        else
                           data = atoi(param[2]);
                     }

                     do {
                        if (current_addr >= 0)
                           status = mscb_write(fd, (unsigned short) current_addr,
                                             (unsigned char) index, &data, info_var.width);
                        else
                           status = mscb_write_group(fd, (unsigned short) current_group,
                                                   (unsigned char) index, &data,
                                                   info_var.width);

                        if (param[3][0])
                           Sleep(100);

                     } while (param[3][0] && !kbhit());

                  }
               }

               if (status != MSCB_SUCCESS)
                  printf("Error: %d\n", status);
            }
         }
      }

      /* read ---------- */
      else if (match(param[0], "read")) {
         if (current_addr < 0)
            printf("You must first address an individual node\n");
         else {

            if (!param[1][0] || param[1][0] == 'a' || param[1][0] == 'r') {
               first = 0;
               last = 256;
            } else {
               first = last = atoi(param[1]);
            }

            read_all = (param[1][0] == 'a' || param[2][0] == 'a' || param[3][0] == 'a');
            repeat = (param[1][0] == 'r' || param[2][0] == 'r' || param[3][0] == 'r');
            if (repeat) {
               wait = 0;
               for (i = 1; i < 4; i++)
                  if (param[i][0] == 'r')
                     break;
               if (atoi(param[i + 1]) > 0)
                  wait = atoi(param[i + 1]);
            }

            do {
               for (i = first; i <= last; i++) {

                  status = mscb_info_variable(fd, (unsigned short) current_addr,
                                              (unsigned char) i, &info_var);

                  
                  if (status == MSCB_NO_VAR) {
                     if (first == last)
                        printf("Node has no variable #%d\n", first);
                     break;
                  } else if (status == MSCB_CRC_ERROR)
                     puts("CRC Error");
                  else if (status != MSCB_SUCCESS)
                     puts("Timeout or invalid channel number");
                  else {
                     if ((info_var.flags & MSCBF_HIDDEN) == 0 || read_all
                         || first == last) {
                        size = info_var.width;
                        memset(dbuf, 0, sizeof(dbuf));
                        status =
                            mscb_read(fd, (unsigned short) current_addr,
                                      (unsigned char) i, dbuf, &size);

                        if (status == MSCB_SUCCESS)
                           print_channel(i, &info_var, dbuf, first != last);
                     }
                  }

                  if (kbhit())
                     break;
               }

               if (first != last && repeat)
                  printf("\n");

               if (repeat) {
                  if (wait) {
                     Sleep(wait);
                     printf("\n");
                  } else
                     Sleep(10);
               }

            } while (!kbhit() && repeat);

            if (first == last)
               printf("\n");
            while (kbhit())
               getch();
         }
      }

      /* save ---------- */
      else if (match(param[0], "save")) {
         if (current_addr < 0 && param[1][0] == 0 && param[2][0] == 0)
            printf("You must first address an individual node or specify a range\n");
         else {
            if (param[1][0] == 0 || strchr(param[1], '.') == NULL) {
               printf("Enter file name: ");
               fgets(str, sizeof(str), stdin);
            } else
               strcpy(str, param[1]);

            if (str[strlen(str) - 1] == '\n')
               str[strlen(str) - 1] = 0;

            if (param[2][0] == 0)
               first = last = current_addr;
            else {
               if (param[3][0]) {
                  first = atoi(param[2]);
                  last = atoi(param[3]);
               } else {
                  first = atoi(param[1]);
                  last = atoi(param[2]);
               }
            }

            writer = mxml_open_file(str);
            if (writer == NULL)
               printf("Cannot open xml file \"%s\"\n", str);
            else {
               mxml_start_element(writer, "MSCBDump");
               for (i = first; i <= last; i++) {
                  printf("%d (0x%04X)\r", i, i);
                  save_node_xml(writer, fd, i);
               }
               printf("\n");
               mxml_end_element(writer);
               mxml_close_file(writer);
            }
         }
      }

      /* load ---------- */
      else if (match(param[0], "load")) {
         if (param[1][0] == 0) {
            printf("Enter file name: ");
            fgets(str, sizeof(str), stdin);
         } else
            strcpy(str, param[1]);

         if (str[strlen(str) - 1] == '\n')
            str[strlen(str) - 1] = 0;

         printf("Write parameters to flash? (y/[n]) ");
         fgets(line, sizeof(line), stdin);

         load_nodes_xml(fd, str, line[0] == 'y');
      }

      /* terminal ---------- */
      else if (match(param[0], "terminal")) {
         int status;

         puts("Exit with <ESC>\n");
         c = 0;

         do {
            if (kbhit()) {
               c = getch();
               if (c == 27)
                  break;

               putchar(c);
               status = mscb_write(fd, (unsigned short) current_addr, 0, &c, 1);
               if (status != MSCB_SUCCESS) {
                  printf("\nError: %d\n", status);
                  break;
               }

               if (c == '\r') {
                  putchar('\n');
                  c = '\n';
                  mscb_write(fd, (unsigned short) current_addr, 0, &c, 1);
               }

            }

            memset(line, 0, sizeof(line));
            size = sizeof(line);
            mscb_read(fd, (unsigned short) current_addr, 0, line, &size);
            if (size > 0)
               fputs(line, stdout);

            Sleep(10);
         } while (TRUE);

         puts("\n");
         while (kbhit())
            getch();
      }

      /* flash ---------- */
      else if (match(param[0], "flash")) {
         if (current_addr < 0 && current_group < 0 && !broadcast)
            printf("You must first address node (s)\n");
         else {
            status = mscb_flash(fd, current_addr, current_group, broadcast);

            if (status != MSCB_SUCCESS)
               printf("Error: %d\n", status);
         }
      }

      /* upload ---------- */
      else if (match(param[0], "upload")) {
         if (current_addr < 0)
            printf("You must first address an individual node\n");
         else {
            if (param[1][0] == 0) {
               printf("Enter name of HEX-file: ");
               fgets(str, sizeof(str), stdin);
            } else
               strcpy(str, param[1]);

            if (str[strlen(str) - 1] == '\n')
               str[strlen(str) - 1] = 0;

            fh = open(str, O_RDONLY | O_BINARY);
            if (fh > 0) {
               if (param[2][0])
                  printf("Uploading file %s\n", str);
               size = lseek(fh, 0, SEEK_END) + 1;
               lseek(fh, 0, SEEK_SET);
               buffer = malloc(size);
               memset(buffer, 0, size);
               read(fh, buffer, size - 1);
               close(fh);

               if (param[2][0] == 'd')
                  status =
                      mscb_upload(fd, (unsigned short) current_addr, buffer, size, TRUE);
               else
                  status =
                      mscb_upload(fd, (unsigned short) current_addr, buffer, size, FALSE);

               if (status == MSCB_FORMAT_ERROR)
                  printf("Syntax error in file \"%s\"\n", str);
               else if (status == MSCB_TIMEOUT)
                  printf("Node %d does not respond\n", current_addr);
               else if (status == MSCB_SUBADDR)
                  printf("Cannot program subaddress\n");
               else if (status == MSCB_NOTREADY)
                  printf("Note just rebooted and not ready for upgrade\n");

               free(buffer);
            } else
               printf("File \"%s\" not found\n", str);
         }
      }

      /* mupload ---------- */
      else if (match(param[0], "mupload")) {

         first = atoi(param[2]);
         last = atoi(param[3]);

         if (last == 0)
            printf("You must specify an address range\n");
         else {
            strcpy(str, param[1]);

            fh = open(str, O_RDONLY | O_BINARY);
            if (fh > 0) {
               if (param[2][0])
                  printf("Uploading file %s\n", str);
               size = lseek(fh, 0, SEEK_END) + 1;
               lseek(fh, 0, SEEK_SET);
               buffer = malloc(size);
               memset(buffer, 0, size);
               read(fh, buffer, size - 1);
               close(fh);

               for (i = first; i <= last; i++) {
                  printf("Node%d: ", i);
                  status = mscb_upload(fd, (unsigned short) i, buffer, size, FALSE);

                  if (status == MSCB_FORMAT_ERROR) {
                     printf("Syntax error in file \"%s\"\n", str);
                     break;
                  } else if (status == MSCB_TIMEOUT)
                     printf("Node %d does not respond\n", i);
                  else if (status == MSCB_SUBADDR)
                     printf("Subchannel %d skipped\n", i);
               }

               free(buffer);
            } else
               printf("File \"%s\" not found\n", str);
         }
      }

      /* verify ---------- */
      else if (match(param[0], "verify")) {
         if (current_addr < 0)
            printf("You must first address an individual node\n");
         else {
            if (param[1][0] == 0) {
               printf("Enter name of HEX-file: ");
               fgets(str, sizeof(str), stdin);
            } else
               strcpy(str, param[1]);

            if (str[strlen(str) - 1] == '\n')
               str[strlen(str) - 1] = 0;

            fh = open(str, O_RDONLY | O_BINARY);
            if (fh > 0) {
               if (param[2][0])
                  printf("Uploading file %s\n", str);
               size = lseek(fh, 0, SEEK_END) + 1;
               lseek(fh, 0, SEEK_SET);
               buffer = malloc(size);
               memset(buffer, 0, size);
               read(fh, buffer, size - 1);
               close(fh);

               status = mscb_verify(fd, (unsigned short) current_addr, buffer, size);

               if (status == MSCB_FORMAT_ERROR)
                  printf("Syntax error in file \"%s\"\n", str);
               else if (status == MSCB_TIMEOUT)
                  printf("Node %d does not respond\n", current_addr);

               free(buffer);
            } else
               printf("File \"%s\" not found\n", str);
         }
      }

      /* reboot ---------- */
      else if (match(param[0], "reboot")) {
         if (current_addr < 0 && current_group < 0 && !broadcast)
            printf("You must first address node (s)\n");
         else {
            status = mscb_reboot(fd, current_addr, current_group, broadcast);

            if (status != MSCB_SUCCESS)
               printf("Error: %d\n", status);
         }
      }

      /* reset ---------- */
      else if (match(param[0], "reset")) {
         mscb_reset(fd);
      }

      /* echo test ---------- */
      else if (match(param[0], "echo")) {
         unsigned char d1, d2;
         int i, status;

         if (current_addr < 0)
            printf("You must first address an individual node\n");
         else {
            d1 = i = 0;
            /* first echo with adressing */
            status = mscb_echo(fd, (unsigned short) current_addr, d1, &d2);
            while (!kbhit()) {
               d1 = (d1 + 1) % 256;

               /* following echos without adressing if no error */
               if (status == MSCB_SUCCESS)
                  status = mscb_echo(fd, 0, d1, &d2);
               else
                  status = mscb_echo(fd, (unsigned short) current_addr, d1, &d2);

               if (status != MSCB_SUCCESS) {
                  printf("Error: %d\n", status);
               }

               if (d2 != d1) {
                  printf("%d\nReceived: %02X, should be %02X, status = %d\n", i, d2, d1,
                         status);

                  if (param[1][0] != 'c' && param[1][1] != 'c')
                     break;
               }

               i++;
               if (i % 100 == 0) {
                  printf("%d\r", i);
                  fflush(stdout);
               }

               if (param[1][0] != 'f' && param[1][1] != 'c')
                  Sleep(10);
            }
            printf("%d\n", i);
            while (kbhit())
               getch();
         }
      }

      /* call user fundtion ----------- */
      else if (match(param[0], "user")) {
         if (current_addr < 0 && current_group < 0 && !broadcast)
            printf("You must first address node (s)\n");
         else {

            if (param[1][0]) {
               data = atoi(param[1]);
               size = sizeof(status);
               status = mscb_user(fd, current_addr, &data, 1, &status, &size);
            } else {
               status = mscb_user(fd, current_addr, NULL, 0, &data, &size);
            }

            if (status != MSCB_SUCCESS)
               printf("Error: %d\n", status);
         }
      }

      /* block transfer test ---------- */
      else if (match(param[0], "block")) {
         int i, status;

         if (current_addr < 0)
            printf("You must first address an individual node\n");
         else {
            i = 0;
            time(&start);
            while (!kbhit()) {
               status =
                   mscb_write_block(fd, (unsigned short) current_addr, 0, dbuf, 1024);
               i += 1024;

               time(&now);
               if (now > start + 1) {
                  start = now;
                  printf("%4.2lf kB/sec\r", i / 1024.0);
                  i = 0;
                  fflush(stdout);
               }
            }
            printf("%d\n", i);
            while (kbhit())
               getch();
         }
      }

      /* test1 ---------- */
      else if (match(param[0], "t1")) {
         data = atoi(param[1]);

         do {
            mscb_link(fd, (unsigned short) current_addr, 4, &data, 1);
            printf("Data: %d\r", data);
         } while (!kbhit());
         while (kbhit())
            getch();
      }

      /* exit/quit ---------- */
      else if (match(param[0], "exit") || match(param[0], "quit"))
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
   int i, fd, server;
   unsigned short adr;
   char cmd[256], device[256], str[256], password[256];
   int debug, check_io;

   cmd[0] = 0;
   adr = 0;

   debug = check_io = server = 0;
   device[0] = password[0] = 0;

   /* parse command line parameters */
   for (i = 1; i < argc; i++) {
      if (argv[i][0] == '-' && argv[i][1] == 'v')
         debug = 1;
      else if (argv[i][0] == '-' && argv[i][1] == 'i')
         check_io = 1;
      else if (argv[i][0] == '-' && argv[i][1] == 's')
         server = 1;
      else if (argv[i][0] == '-') {
         if (i + 1 >= argc || argv[i + 1][0] == '-')
            goto usage;
         else if (argv[i][1] == 'd')
            strcpy(device, argv[++i]);
         else if (argv[i][1] == 'p')
            strcpy(password, argv[++i]);
         else if (argv[i][1] == 'a')
            adr = atoi(argv[++i]);
         else if (argv[i][1] == 'c') {
            if (strlen(argv[i]) >= 256) {
               printf("error: command line too long (>256).\n");
               return 0;
            }
            strcpy(cmd, argv[++i]);
         } else {
          usage:
            printf
                ("usage: msc [-d host:device] [-p password] [-a addr] [-c Command] [-c @CommandFile] [-v] [-i]\n\n");
            printf("       -d     device, usually usb0 for first USB port,\n");
            printf("              or \"<host>:usb0\" for RPC connection\n");
            printf
                ("              or \"mscb<xxx>\" for Ethernet connection to SUBM_260\n");
            printf("       -p     optional password for SUBM_260\n");
            printf("       -s     Start RPC server\n");
            printf("       -a     Address node before executing command\n");
            printf("       -c     Execute command immediately\n");
            printf("       -v     Produce verbose debugging output\n\n");
            printf("       -i     Check IO pins of port\n\n");
            printf("For a list of valid commands start msc interactively\n");
            printf("and type \"help\".\n");
            return 0;
         }
      }
   }

   if (server) {
      mrpc_server_loop();
      return 0;
   }

   if (check_io) {
      mscb_check(device, sizeof(device));
      return 0;
   }

   /* open port */
   fd = mscb_init(device, sizeof(device), password, debug ? 1 : 0);

   if (fd == EMSCB_WRONG_PASSWORD) {
      printf("Enter password to access %s: ", device);
      fgets(str, sizeof(str), stdin);
      while (strlen(str) > 0
             && (str[strlen(str) - 1] == '\r' || str[strlen(str) - 1] == '\n'))
         str[strlen(str) - 1] = 0;
      fd = mscb_init(device, sizeof(device), str, debug ? 1 : 0);
   }

   if (fd < 0) {
      if (fd == EMSCB_LPT_ERROR) {
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
      } else if (fd == EMSCB_NO_DIRECTIO) {
         printf("No DirectIO driver installed\n");
      } else if (fd == EMSCB_COMM_ERROR) {
         printf("\nCannot communicate with MSCB submaster at %s\n", device);
         puts("Please disconnect and reconnect submaster\n");
      } else if (fd == EMSCB_SUBM_VERSION) {
         printf("\nSubmaster at %s has old version\n", device);
         puts("Please upgrade submaster software\n");
      } else if (fd == EMSCB_NOT_FOUND) {
         printf("\nCannot find USB submaster\n");
      } else if (fd == EMSCB_WRONG_PASSWORD) {
         printf("\nWrong password\n");
      } else if (fd == EMSCB_NO_WRITE_ACCESS) {
         printf("\nNo write access to MSCB submaster at %s\n", device);
         puts("Please install hotplug script \"drivers/linux/usb.usermap_scs_250\" to \"/etc/hotplug/\".\n");
      } else if (fd == EMSCB_LOCKED) {
         puts("\nMSCB system is locked by other process");
         puts("Please stop all running MSCB clients\n");
      } else if (device[0])
         printf("Cannot connect to device \"%s\"\n", device);
      else
         printf("No MSCB submaster found\n");

#ifdef _MSC_VER
      puts("\n-- hit any key to exit --");

      while (!kbhit())
         Sleep(100);
      while (kbhit())
         getch();
#endif

      return 0;
   }

   printf("Connected to submaster at %s\n", device);

   cmd_loop(fd, cmd, adr);

   mscb_exit(fd);

   return 0;
}
