//
// mh2sql: import midas history into an SQL database
//
// Author: Konstantin Olchanski / TRIUMF, August-2008
//
// $Id$
//

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "midas.h"
#include "history_odbc.h"

#include <map>

std::map<int,const char*> gEventName;

int readHstFile(FILE*f)
{
  assert(f!=NULL);

  while (1)
    {
      HIST_RECORD rec;

      int rd = fread(&rec, sizeof(rec), 1, f);
      if (!rd)
	break;

#if 0
      printf("HIST_RECORD:\n");
      printf("  Record type: 0x%x\n", rec.record_type);
      printf("  Event ID: %d\n", rec.event_id);
      printf("  Time: %d\n", rec.time);
      printf("  Offset: 0x%x\n", rec.def_offset);
      printf("  Size: 0x%x\n", rec.data_size);
#endif

      switch (rec.record_type)
	{
	default:
	  printf("Unexpected record type: 0x%x\n", rec.record_type);
	  return -1;
	  break;

	case 0x46445348: // RT_DEF:
	  {
	    char event_name[NAME_LENGTH];
	    rd = fread(event_name, 1, NAME_LENGTH, f);
	    assert(rd == NAME_LENGTH);
	    
	    int size = rec.data_size;
	    int ntags = size/sizeof(TAG);

	    //  printf("Event %d, \"%s\", size %d, %d tags.\n", rec.event_id, event_name, size, ntags);

	    assert(size > 0);
	    assert(size < 1*1024*1024);
	    assert(size == ntags*(int)sizeof(TAG));
	    
	    TAG *tags = new TAG[ntags];
	    rd = fread(tags, 1, size, f);
	    assert(rd == size);

            hs_define_event_odbc(event_name, tags, size);

            gEventName[rec.event_id] = strdup(event_name);

	    delete tags;
	    break;
	  }

	case 0x41445348: // RT_DATA:
	  {
	    int size = rec.data_size;

	    if (0)
	      printf("Data record, size %d.\n", size);

	    assert(size > 0);
	    assert(size < 1*1024*1024);
	    
	    static char *buf = NULL;
            static int bufSize = 0;

            if (size > bufSize)
              {
                bufSize = 1024*((size+1023)/1024);
                buf = (char*)realloc(buf, bufSize);
                assert(buf != NULL);
              }

	    rd = fread(buf, 1, size, f);
	    assert(rd == size);

	    time_t t = (time_t)rec.time;

            hs_write_event_odbc(gEventName[rec.event_id], t, buf, size);
          
            break;
	  }
	}
    }

  return 0;
}

int readHst(const char* name)
{
  FILE* f = fopen(name,"r");
  if (!f)
    {
      fprintf(stderr,"Error: Cannot open \'%s\', errno %d (%s)\n", name, errno, strerror(errno));
      exit(1);
    }

  readHstFile(f);
  fclose(f);
  return 0;
}

void help()
{
  fprintf(stderr,"Usage: mh2sql odbc_dsn file1.hst file2.hst ...\n");
  fprintf(stderr,"\n");
  fprintf(stderr,"Switches:\n");
  fprintf(stderr,"  -h --- print this help message\n");
  fprintf(stderr,"\n");
  fprintf(stderr,"Examples:\n");
  exit(1);
}

int main(int argc,char*argv[])
{
   int status;
   if (argc <= 2)
      help(); // DOES NOT RETURN

   status = cm_connect_experiment("", "", "mh2sql", NULL);
   if (status != SUCCESS)
      exit(1);

   status = hs_connect_odbc(argv[1]);
   if (status != SUCCESS)
      exit(1);
   
   for (int iarg=2; iarg<argc; iarg++)
      if (strcmp(argv[iarg], "-h")==0) {
         help(); // DOES NOT RETURN
      } else if (argv[iarg][0]=='-' && argv[iarg][1]=='v') {
         hs_debug_odbc(atoi(argv[iarg]+2));
      } else {
         readHst(argv[iarg]);
      }

   cm_disconnect_experiment();

   return 0;
}

// end
