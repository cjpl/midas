// test ss_file_size & co

#include <stdio.h>
#include <stdlib.h>

#include "midas.h"

int main(int argc, char* argv[])
{
   char* f = argv[1];

   double file_size = ss_file_size(f);
   double disk_size = ss_disk_size(f);
   double disk_free = ss_disk_free(f);

   printf("For [%s], file size: %.0f, disk size: %.3f, disk free %.3f\n", f, file_size, disk_size/1024, disk_free/1024);

   char cmd[1024];

   sprintf(cmd, "/bin/ls -ld %s", f);
   system(cmd);

   sprintf(cmd, "/bin/df %s", f);
   system(cmd);

   return 0;
}

// end
