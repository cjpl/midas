//
// mhdump: midas history explorer
//
// Author: Konstantin Olchanski, 20-NOV-2007
//
// Compile: 
//   g++ -o mhdump.o -c mhdump.cxx -O2 -g -Wall -Wuninitialized
//   g++ -o mhdump -g -O2 mhdump.o
//
// $Id$
//

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <string>
#include <map>
#include <vector>

////////////////////////////////////////
// Definitions extracted from midas.h //
////////////////////////////////////////

#define DWORD uint32_t

#define NAME_LENGTH            32            /**< length of names, mult.of 8! */

typedef struct {
   DWORD record_type;           /* RT_DATA or RT_DEF */
   DWORD event_id;
   DWORD time;
   DWORD def_offset;            /* place of definition */
   DWORD data_size;             /* data following this header in bytes */
} HIST_RECORD;

typedef struct {
   char name[NAME_LENGTH];             /**< - */
   DWORD type;                         /**< - */
   DWORD n_data;                       /**< - */
} TAG;

////////////////////////////////////////
// Definitions extracted from midas.c //
////////////////////////////////////////

/********************************************************************/
/* data type sizes */
int tid_size[] = {
   0,                           /* tid == 0 not defined                               */
   1,                           /* TID_BYTE      unsigned byte         0       255    */
   1,                           /* TID_SBYTE     signed byte         -128      127    */
   1,                           /* TID_CHAR      single character      0       255    */
   2,                           /* TID_WORD      two bytes             0      65535   */
   2,                           /* TID_SHORT     signed word        -32768    32767   */
   4,                           /* TID_DWORD     four bytes            0      2^32-1  */
   4,                           /* TID_INT       signed dword        -2^31    2^31-1  */
   4,                           /* TID_BOOL      four bytes bool       0        1     */
   4,                           /* TID_FLOAT     4 Byte float format                  */
   8,                           /* TID_DOUBLE    8 Byte float format                  */
   1,                           /* TID_BITFIELD  8 Bits Bitfield    00000000 11111111 */
   0,                           /* TID_STRING    zero terminated string               */
   0,                           /* TID_ARRAY     variable length array of unkown type */
   0,                           /* TID_STRUCT    C structure                          */
   0,                           /* TID_KEY       key in online database               */
   0                            /* TID_LINK      link in online database              */
};

/* data type names */
char *tid_name[] = {
   "NULL",
   "BYTE",
   "SBYTE",
   "CHAR",
   "WORD",
   "SHORT",
   "DWORD",
   "INT",
   "BOOL",
   "FLOAT",
   "DOUBLE",
   "BITFIELD",
   "STRING",
   "ARRAY",
   "STRUCT",
   "KEY",
   "LINK"
};

////////////////////////////////////////

struct Tag
{
  int event_id;
  std::string name;
  int offset;
  int arraySize;
  int typeSize;
  int typeCode;
};

struct Event
{
  bool printAllTags;
  std::map<std::string,Tag*> tags;
  std::vector<std::string> tagNames;
  std::vector<int> tagIndexes;
};

std::map<int,Event*> gTags;

bool doPrintTags  = true;
bool doPrintNames = true;
bool doPrintData  = true;
bool doAll        = false;

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

	    if (doPrintTags)
	      printf("Event %d, \"%s\", size %d, %d tags.\n", rec.event_id, event_name, size, ntags);

	    assert(size > 0);
	    assert(size < 1*1024*1024);
	    assert(size == ntags*(int)sizeof(TAG));
	    
	    TAG *tags = new TAG[ntags];
	    rd = fread(tags, 1, size, f);
	    assert(rd == size);

	    int offset = 0;

	    for (int itag=0; itag<ntags; itag++)
	      {
		int size = tags[itag].n_data * tid_size[tags[itag].type];

		Tag* t = new Tag;
		t->event_id  = rec.event_id;
		t->name      = tags[itag].name;
		t->offset    = offset;
		t->arraySize = tags[itag].n_data;
		t->typeSize  = tid_size[tags[itag].type];
		t->typeCode  = tags[itag].type;

		Event* e = gTags[t->event_id];
		if (!e)
		  {
		    gTags[t->event_id] = new Event;
		    e = gTags[t->event_id];
		    e->printAllTags = false;
		    if (doAll)
		      e->printAllTags = true;
		  }

		e->tags[t->name] = t;

		if (e->printAllTags)
		  {
		    e->tagNames.push_back(t->name);
		    e->tagIndexes.push_back(-1);
		  }

		if (doPrintTags)
		  printf("  Tag %d: \"%s\"[%d], type \"%s\" (%d), type size %d, offset %d+%d\n", itag, tags[itag].name, tags[itag].n_data, tid_name[tags[itag].type], tags[itag].type, tid_size[tags[itag].type], offset, size);

		offset += size;
	      }

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
	    
	    char *buf = new char[size];
	    rd = fread(buf, 1, size, f);
	    assert(rd == size);

	    time_t t = (time_t)rec.time;

	    Event* e = gTags[rec.event_id];
	    if (e && doPrintData)
	      {
		//printf("event %d, time %s", rec.event_id, ctime(&t));

		int n  = e->tagNames.size();

		if (n>0)
		  printf("%d %d ", rec.event_id, rec.time);

		for (int i=0; i<n; i++)
		  {
		    Tag*t = e->tags[e->tagNames[i]];
		    int index = e->tagIndexes[i];

		    //printf(" dump %s[%d] (%p)\n", e->tagNames[i].c_str(), e->tagIndexes[i], t);

		    if (t)
		      {
			int offset = t->offset;
			void* ptr = (void*)(buf+offset);

			if (doPrintNames)
			  {
			    if (index < 0)
			      printf(" %s=", t->name.c_str());
			    else
			      printf(" %s[%d]=", t->name.c_str(), index);
			  }

			for (int j=0; j<t->arraySize; j++)
			  {
			    if (index >= 0)
			      j = index;

			    switch (t->typeCode)
			      {
			      default:
				printf("unknownType%d ",t->typeCode);
				break;
			      case 1: /* BYTE */
				printf("%u ",((uint8_t*)ptr)[j]);
				break;
			      case 2: /* SBYTE */
				printf("%d ",((int8_t*)ptr)[j]);
				break;
			      case 3: /* CHAR */
				printf("\'%c\' ",((char*)ptr)[j]);
				break;
			      case 4: /* WORD */
				printf("%u ",((uint16_t*)ptr)[j]);
				break;
			      case 5: /* SHORT */
				printf("%d ",((int16_t*)ptr)[j]);
				break;
			      case 6: /* DWORD */
				printf("%u ",((uint32_t*)ptr)[j]);
				break;
			      case 7: /* INT */
				printf("%d ",((int32_t*)ptr)[j]);
				break;
			      case 8: /* BOOL */
				printf("%u ",((uint32_t*)ptr)[j]);
				break;
			      case 9: /* FLOAT */
				printf("%.8g ",((float*)ptr)[j]);
				break;
			      case 10: /* DOUBLE */
				printf("%.16g ",((double*)ptr)[j]);
				break;
			      }

			    if (index >= 0)
			      break;
			  }
		      }
		  }

		if (n>0)
		  printf(" %s", ctime(&t));
	      }

	    delete buf;
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
  fprintf(stderr,"Usage: mhdump [-h] [-L] [-n] [-t] [-E event_id] [-T tag_name] file1.hst file2.hst ...\n");
  fprintf(stderr,"\n");
  fprintf(stderr,"Switches:\n");
  fprintf(stderr,"  -h --- print this help message\n");
  fprintf(stderr,"  -L --- list tag definitions only\n");
  fprintf(stderr,"  -t --- omit tag definitions\n");
  fprintf(stderr,"  -n --- omit variable names\n");
  fprintf(stderr,"\n");
  fprintf(stderr,"Examples:\n");
  fprintf(stderr,"  To list all existing tags: mhdump -L file1.hst file2.hst ...\n");
  fprintf(stderr,"  To show data for all events, all tags: mhdump file1.hst file2.hst ...\n");
  fprintf(stderr,"  To show all data for event 0: mhdump -E 0 file1.hst file2.hst ...\n");
  fprintf(stderr,"  To show data for event 0, tag \"State\": mhdump -n -E 0 -T State file1.hst file2.hst ...\n");
  fprintf(stderr,"  To show data for event 3, tag \"MCRT\", array index 5: mhdump -n -E 3 -T MCRT[5] file1.hst file2.hst ...\n");
  exit(1);
}

int main(int argc,char*argv[])
{
  int event_id = -1;

  if (argc <= 1)
    help(); // DOES NOT RETURN

  for (int iarg=1; iarg<argc; iarg++)
    if (strcmp(argv[iarg], "-h")==0)
      {
        help(); // DOES NOT RETURN
      }
    else if (strcmp(argv[iarg], "-E")==0)
      {
	iarg++;
	event_id = atoi(argv[iarg]);
	if (!gTags[event_id])
	  gTags[event_id] = new Event;
	gTags[event_id]->printAllTags = true;
      }
    else if (strcmp(argv[iarg], "-T")==0)
      {
	iarg++;
	char *s = strchr(argv[iarg],'[');
	int index = -1;
	if (s)
	  {
	    index = atoi(s+1);
	    *s = 0;
	  }

	std::string name = argv[iarg];

	Event* e = gTags[event_id];

	if ((event_id<0) || !e)
	  {
	    fprintf(stderr,"Error: expected \"-E event_id\" before \"-T ...\"\n");
	    exit(1);
	  }

	e->printAllTags = false;
	e->tagNames.push_back(name);
	e->tagIndexes.push_back(index);
      }
    else if (strcmp(argv[iarg], "-t")==0)
      doPrintTags = false;
    else if (strcmp(argv[iarg], "-L")==0)
      {
        doPrintTags = true;
        doPrintData = false;
      }
    else if (strcmp(argv[iarg], "-A")==0)
      doAll = true;
    else if (strcmp(argv[iarg], "-n")==0)
      doPrintNames = false;
    else
      {
	if (gTags.size() == 0)
	  doAll = true;
	readHst(argv[iarg]);
      }

  return 0;
}

// end
