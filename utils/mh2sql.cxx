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
#include "history.h"

#include <map>

std::map<int,const char*> gEventName;

int copyHstFile(FILE*f, MidasHistoryInterface *mh)
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

            mh->hs_define_event(event_name, ntags, tags);

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

            mh->hs_write_event(gEventName[rec.event_id], t, size, buf);
          
            break;
	  }
	}
    }

  return 0;
}

int copyHst(const char* name, MidasHistoryInterface* mh)
{
  FILE* f = fopen(name,"r");
  if (!f)
    {
      fprintf(stderr,"Error: Cannot open \'%s\', errno %d (%s)\n", name, errno, strerror(errno));
      exit(1);
    }

  copyHstFile(f, mh);
  fclose(f);
  mh->hs_flush_buffers();
  return 0;
}

void help()
{
  fprintf(stderr,"Usage: mh2sql [-h] [switches...] file1.hst file2.hst ...\n");
  fprintf(stderr,"\n");
  fprintf(stderr,"Switches:\n");
  fprintf(stderr,"  -h --- print this help message\n");
  fprintf(stderr,"  --hs-debug <hs_debug_flag> --- set the history debug flag\n");
  fprintf(stderr,"  --odbc <ODBC_DSN> --- write to ODBC (SQL) history using given ODBC DSN\n");
  fprintf(stderr,"  --sqlite <path> --- write to SQLITE database at the given path\n");
  fprintf(stderr,"\n");
  fprintf(stderr,"Examples:\n");
  fprintf(stderr,"  mh2sql --hs-debug 1 --sqlite . 130813.hst\n");
  exit(1);
}

int main(int argc,char*argv[])
{
   int status;
   int hs_debug_flag = 0;
   HNDLE hDB;
   MidasHistoryInterface *mh = NULL;

   if (argc <= 2)
      help(); // DOES NOT RETURN

   status = cm_connect_experiment("", "", "mh2sql", NULL);
   if (status != SUCCESS)
      exit(1);

   status = cm_get_experiment_database(&hDB, NULL);
   assert(status == HS_SUCCESS);

   for (int iarg=1; iarg<argc; iarg++) {
      const char* arg = argv[iarg];
      if (arg[0] != '-')
         continue;
      
      if (strcmp(arg, "-h")==0) {
         help(); // DOES NOT RETURN
      } else if (strcmp(arg, "--hs-debug") == 0) {
         hs_debug_flag = atoi(argv[iarg+1]);
         iarg++;
      } else if (strcmp(arg, "--odbc") == 0) {
         mh = MakeMidasHistoryODBC();
         assert(mh);
         mh->hs_set_debug(hs_debug_flag);
         status = mh->hs_connect(argv[iarg+1]);
         if (status != HS_SUCCESS)
            exit(1);
         iarg++;
      } else if (strcmp(arg, "--sqlite") == 0) {
         mh = MakeMidasHistorySqlite();
         assert(mh);
         mh->hs_set_debug(hs_debug_flag);
         status = mh->hs_connect(argv[iarg+1]);
         if (status != HS_SUCCESS)
            exit(1);
         iarg++;
      }
   }

   if (!mh) {
      status = hs_get_history(hDB, 0, HS_GET_DEFAULT|HS_GET_WRITER|HS_GET_INACTIVE, hs_debug_flag, &mh);
      assert(status == HS_SUCCESS);
      assert(mh);
   }
   
   for (int iarg=1; iarg<argc; iarg++) {
      const char* arg = argv[iarg];

      if (arg[0] != '-')
         continue;
      
      if (strcmp(arg, "--hs-debug") == 0) {
         mh->hs_set_debug(atoi(argv[iarg+1]));
         iarg++;
      }
   }

   for (int iarg=1; iarg<argc; iarg++) {
      const char* arg = argv[iarg];

      if (strstr(arg, ".hst") != NULL) {
         copyHst(arg, mh);
      }
   }

   mh->hs_disconnect();

   cm_disconnect_experiment();

   return 0;
}

// end
