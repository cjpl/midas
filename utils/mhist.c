 /********************************************************************\

  Name:         mhist.c
  Created by:   Stefan Ritt

  Contents:     MIDAS history display utility

  $Log$
  Revision 1.2  1998/10/12 12:19:03  midas
  Added Log tag in header


\********************************************************************/

#include "midas.h"
#include "msystem.h"

void tmp()
{
time_t tm;
 
  time(&tm);
  tm = ss_time();
  hs_dump(1, tm-3600, tm, 0);
}

/*------------------------------------------------------------------*/

TAG temp_tag[] = {
  { "Temperatures", TID_FLOAT, 100 },
  { "Humidity",     TID_FLOAT, 99 },
  { "Pressure1",    TID_FLOAT, 1 },
};

TAG hv_tag[] = {
  { "HV",           TID_FLOAT, 100 },
};

float hist[200];
float hv[100];

TAG tag[10];

void write_hist_speed()
/* write some history */
{
DWORD start_time, act_time;
int i, j, bytes;

  hs_define_event(1, "Temperature", temp_tag, sizeof(temp_tag));
  hs_define_event(2, "HV", hv_tag, sizeof(hv_tag));

  start_time = ss_millitime();
  j = bytes = 0;
  do
    {
    for (i=0 ; i<100 ; i++)
      {
      hist[i] = (float)j;
      hs_write_event(1, hist, sizeof(hist));
      hs_write_event(2, hv, sizeof(hv));
      }

    j+=2*i;
    bytes += (sizeof(hist)+sizeof(hv))*i;
    act_time = ss_millitime();

    printf("%ld\n", ss_time());

    } while (act_time - start_time < 10000);

  printf("%d events (%d kB) per sec.\n", j/(act_time-start_time)*1000, 
                                         bytes/1024/(act_time-start_time)*1000);
}

void generate_hist()
/* write some history */
{
int i, j;

  hs_define_event(1, "Temperature", temp_tag, sizeof(temp_tag));
  hs_write_event(1, hist, sizeof(hist));

  hs_define_event(2, "HV", hv_tag, sizeof(hv_tag));
  hs_write_event(2, hv, sizeof(hv));

  for (i=0 ; i<10 ; i++)
    {
    hist[0] = (float)i;
    hist[1] = i/10.f;
    hs_write_event(1, hist, sizeof(hist));

    for (j=0 ; j<100 ; j++)
      hv[j] = j+i/10.f;
    hs_write_event(2, hv, sizeof(hv));
    printf("%d\n", ss_time());
    ss_sleep(1000);
    }
}

/*------------------------------------------------------------------*/

INT query_params(DWORD *event_id, DWORD *start_time, DWORD *end_time,
                 DWORD *interval, TAG *var_tag, DWORD *index)
{
DWORD      status, hour, i, bytes, n;
INT        var_index;
DEF_RECORD *def_rec;
TAG        *tag;

  status = hs_count_events(0, &n);
  if (status != HS_SUCCESS)
    return status;
  bytes = sizeof(DEF_RECORD)*n;
  def_rec = malloc(bytes);
  hs_enum_events(0, def_rec, &bytes);

  printf("Available events:\n");
  for (i=0 ; i<n ; i++)
    printf("[%d] %s (ID %d)\n", i, def_rec[i].event_name, def_rec[i].event_id);

  if (n>1)
    {
    printf("\nSelect event (0..%d): ", i-1);
    scanf("%d", &i);
    *event_id = def_rec[i].event_id;
    }
  else
    *event_id = def_rec[0].event_id;

  hs_count_tags(0, *event_id, &n);
  bytes = sizeof(TAG)*n;
  tag = malloc(bytes);
  hs_enum_tags(0, *event_id, tag, &bytes);

  printf("\nAvailable variables:\n");
  for (i=0 ; i<n ; i++)
    printf("(%d) %s\n", i, tag[i].name);

  *index = var_index = 0;
  if (n>1)
    {
    printf("\nSelect variable (0..%d,-1 for all): ", n-1);
    scanf("%d", &var_index);
    if (var_index >= (INT) n)
      var_index = n-1;
    if (var_index >= 0 && tag[var_index].n_data > 1 && tag[var_index].type != TID_STRING)
      {
      printf("\nSelect index (0..%d): ", tag[var_index].n_data-1);
      scanf("%d", index);
      }
    }

  if (var_index < 0)
    var_tag->name[0] = 0;
  else
    memcpy(var_tag, &tag[var_index], sizeof(TAG));


  printf("\nHow many hours: ");
  scanf("%d", &hour);
  *start_time = ss_time()-hour*3600;
  *end_time = ss_time();

  printf("\nInterval [sec]: ");
  scanf("%d", interval);
  printf("\n");

  free(def_rec);
  free(tag);

  return HS_SUCCESS;
}

/*------------------------------------------------------------------*/

void display_single_hist(DWORD event_id, DWORD start_time, DWORD end_time,
                         DWORD interval, TAG *tag, DWORD index)
/* read back history */
{
char       buffer[10000];
DWORD      tbuffer[1000];
DWORD      i, size, tbsize, n, type;
char       str[256];
INT        status;

  do
    {
    size = sizeof(buffer);
    tbsize = sizeof(tbuffer);
    status = hs_read(event_id, start_time, end_time, interval, tag->name, index, 
                     tbuffer, &tbsize, buffer, &size, &type, &n);

    if (n == 0)
      printf("No variables \"%s\" found in specified time range\n", tag->name);

    for (i=0 ; i<n ; i++)
      {
      sprintf(str, "%s", ctime(&tbuffer[i])+4);
      str[20] = '\t';
    
      if (type == TID_STRING)
        {
        strcat(str, "\n");
        strcpy(&str[strlen(str)], buffer+(size/n) * i);
        }
      else
        db_sprintf(&str[strlen(str)], buffer, rpc_tid_size(type), i, type);

      strcat(str, "\n");

      printf(str);
      }

    if (status == HS_TRUNCATED)
      start_time = tbuffer[n-1] + (tbuffer[n-1] - tbuffer[n-2]);

    } while (status == HS_TRUNCATED);
}

/*------------------------------------------------------------------*/

main(int argc, char *argv[])
{
DWORD    status, event_id, start_time, end_time, interval, index;
INT      i;
TAG      tag;

  /* turn off system message */
  cm_set_msg_print(0, MT_ALL, puts);

  if (argc == 1)
    {
    status = query_params(&event_id, &start_time, &end_time,
                          &interval, &tag, &index);
    if (status != HS_SUCCESS)
      return 0;
    }
  else
    {
    end_time = ss_time();
    start_time = end_time - 3600;
    interval = 1;
    index = 0;
    tag.type = 0;
    tag.name[0] = 0;

    /* parse command line parameters */
    for (i=1 ; i<argc ; i++)
      {
      if (argv[i][0] == '-')
        {
        if (i+1 >= argc || argv[i+1][0] == '-')
          goto usage;
        if (argv[i][1] == 'e')
          event_id = atoi(argv[++i]);
        else if (argv[i][1] == 'v')
          strcpy(tag.name, argv[++i]);
        else if (argv[i][1] == 'i')
          index = atoi(argv[++i]);
        else if (argv[i][1] == 'h')
          start_time = ss_time() - atoi(argv[++i])*3600;
        else if (argv[i][1] == 'd')
          start_time = ss_time() - atoi(argv[++i])*3600*24;
        else if (argv[i][1] == 't')
          interval = atoi(argv[++i]);
        else
          {
usage:
          printf("\nusage: mhist -e Event ID -v Variable Name\n");
          printf("         [-i Index] [-h Hours] [-d Days] [-t Interval]\n\n");
          printf("where index is for variables which are arrays, hours/days go into the past\n");
          printf("and interval is the minimum interval between two displayed records.\n\n");
          return 0;
          }
        }
      else
        goto usage;
      }
    }

  if (tag.name[0] == 0)
    hs_dump(event_id, start_time, end_time, interval);
  else
    display_single_hist(event_id, start_time, end_time,
                        interval, &tag, index);

  return 1;
}
