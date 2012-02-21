#include "mscb.h"
#include "mscbrpc.h"
#include "mxml.h"
#include "strlcpy.h"

void cmd_loop(int fd, char *cmd, unsigned short adr);

void load_nodes_xml(int fd, char *file_name, int flash);

int mainmsc(int argc, char *argv[], mxArray *plhs[ ], bool writeFlag, bool scanFlag);

int match(char *str, char *cmd);

void print_channel(int index, MSCB_INFO_VAR * info_chn, void *pdata, int verbose);

void print_channel_str(int index, MSCB_INFO_VAR * info_chn, void *pdata, int verbose, char *line);

void print_help();

void save_node_xml(MXML_WRITER * writer, int fd, int addr);

