/********************************************************************\

  Name:         epics_ca.c
  Created by:   Stefan Ritt

  Contents:     Epics channel access device driver

  $Id$
            May-14-07: running with base 3.14.8.2
            Had to remove ca_host_name(chid),
            and ca_context_create(ca_disable_preemptive_callback)
\********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <epicsStdlib.h>
#include "cadef.h"
#include "dbDefs.h"
#include "epicsTime.h"

#ifdef _MSC_VER                 /* collision between varargs and stdarg */
#undef va_arg
#undef va_start
#undef va_end
#endif

#include "midas.h"

/*---- globals -----------------------------------------------------*/

#define CHN_NAME_LENGTH 32      /* length of channel names */

typedef struct {
    char  value[20];
    chid  chan_id;  //    chid *pv_handles;
    evid  evt_id;
} CA_NODE;

typedef struct {
    char    * channel_names;
    CA_NODE * caid;
    float   * array;
    INT num_channels;
    DWORD flags;
} CA_INFO;

static void printChidInfo(chid chid, char *message)
{
  printf("\n%s\n",message);
  printf("pv: %s  type(%d) nelements(%ld)", //  host(%s)",
   ca_name(chid),ca_field_type(chid),ca_element_count(chid)); //,
  //   ca_host_name(chid));
  printf(" read(%d) write(%d) state(%d)\n",
   ca_read_access(chid),ca_write_access(chid),ca_state(chid));
}

static void exceptionCallback(struct exception_handler_args args)
{
  chid        chid = args.chid;
  long        stat = args.stat; /* Channel access status code*/
  const char  *channel;
  static char *noname = "unknown";

  channel = (chid ? ca_name(chid) : noname);


  if(chid) printChidInfo(chid,"exceptionCallback");
  printf("exceptionCallback stat %s channel %s\n",
   ca_message(stat),channel);
}

static void connectionCallback(struct connection_handler_args args)
{
  chid        chid = args.chid;

  printChidInfo(chid,"connectionCallback");
}

static void accessRightsCallback(struct access_rights_handler_args args)
{
  chid        chid = args.chid;

  printChidInfo(chid,"accessRightsCallback");
}

void epics_ca_callback(struct event_handler_args args)
{
  CA_INFO *info;
  int i;

  /*
  if(args.status != ECA_NORMAL) {
    printChidInfo(chid, "eventCallback");
  } else {
    *pdata = (char *) args.dbr;
    printf("Event Callback: %s = %s\n",ca_name(chid), pdata);
  }
  */

  /*  if ( args.status == ECA_NORMAL ) {
    ca_dump_dbr ( args.type, args.count, args.dbr );
  }
  else {
    printf ( "%s\t%s\n", dbr_text[args.type], ca_message(args.status) );
  }
  */

  info = (CA_INFO *) args.usr;

  /* search channel index */
  for (i = 0; i < info->num_channels; i++)
    if (info->caid[i].chan_id == args.chid)
      break;
  if (i < info->num_channels)
    info->array[i] = *((float *) args.dbr);

  /*  printf("Ch#%d type:%d count:%d\n", i, ca_field_type(info->caid[i].chan_id), args.count);
   */

}

/*---- device driver routines --------------------------------------*/

INT epics_ca_init(HNDLE hKey, void **pinfo, INT channels)
{
  int status, i;
  HNDLE hDB;
  CA_INFO *info;

  /* allocate info structure */
  info = calloc(1, sizeof(CA_INFO));
  *pinfo = info;

  cm_get_experiment_database(&hDB, NULL);

  /* get channel names */
  info->channel_names = calloc(channels, CHN_NAME_LENGTH);
  for (i = 0; i < channels; i++)
    sprintf(info->channel_names + CHN_NAME_LENGTH * i, "Channel%d", i);
  db_merge_data(hDB, hKey, "Channel name",
    info->channel_names, CHN_NAME_LENGTH * channels, channels, TID_STRING);

  /* initialize driver */
  status = ca_task_initialize();
  if (!(status & CA_M_SUCCESS)) {
    cm_msg(MERROR, "epics_ca_init", "unable to initialize");
    return FE_ERR_HW;
  }

  /* allocate arrays */
  info->array = calloc(channels, sizeof(float));
  info->caid = (CA_NODE *) calloc(channels, sizeof(CA_NODE));

  /* search channels */
  info->num_channels = channels;

  //  SEVCHK(ca_context_create(ca_disable_preemptive_callback),"ca_context_create");
  SEVCHK(ca_add_exception_event(exceptionCallback,NULL), "ca_add_exception_event");

  for (i = 0; i < channels; i++) {
    SEVCHK(ca_create_channel(info->channel_names + CHN_NAME_LENGTH * i
           , NULL // connectionCallback
           , &(info->caid[i])
           , 20
           , &(info->caid[i].chan_id))
     , "ca_create_channel");
    SEVCHK(ca_replace_access_rights_event(info->caid[i].chan_id
            , NULL) // accessRightsCallback)
     , "ca_replace_access_rights_event");
    SEVCHK(ca_create_subscription (DBR_FLOAT
           , 0
           , info->caid[i].chan_id
           , DBE_VALUE | DBE_ALARM
           , epics_ca_callback
           , info
           , &(info->caid[i].evt_id))
     , "ca_add_subscription");
  }
  /*
    if (ca_pend_io(5.0) == ECA_TIMEOUT) {
    cm_msg(MERROR, "epics_ca_init", "cannot connect to %s",
    info->channel_names + CHN_NAME_LENGTH * i);
  */
  SEVCHK(ca_pend_event(5.0), "ca_ped_event");

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT epics_ca_exit(CA_INFO * info)
{
   int i;

   if (info->caid) {
      for (i = 0; i < info->num_channels; i++)
        ca_clear_channel(info->caid[i].chan_id);
   }
   free(info->caid);

   ca_task_exit();

   if (info->array)
      free(info->array);
   if (info->channel_names)
      free(info->channel_names);

   free(info);

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT epics_ca_set(CA_INFO * info, INT channel, float value)
{
   ca_put(DBR_FLOAT, info->caid[channel].chan_id, &value);

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT epics_ca_set_all(CA_INFO * info, INT channels, float value)
{
   INT i;

   for (i = 0; i < MIN(info->num_channels, channels); i++)
      ca_put(DBR_FLOAT, info->caid[i].chan_id, &value);

   return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT epics_ca_set_label(CA_INFO * info, INT channels, char *label)
{
  int status;
  char str[256];
  chid chan_id;
  
  printf("%s.DESC label:%s\n", info->channel_names + CHN_NAME_LENGTH * channels, label);
  sprintf(str, "%s.DESC", info->channel_names + CHN_NAME_LENGTH * channels);
  status = ca_search(str, &chan_id);
  
  status = ca_pend_io(0.8);
  if (status != ECA_NORMAL) {
    printf("%s not found\n", str);
  }
  
  // Doesn't work maybe due to the ca_field_tpye which need a ca_pend_io()
  //  status = ca_put(ca_field_type(chan_id), chan_id, label);
  //  printf("ca_put status: %d\n", status);
  
  status = ca_put(DBR_STRING, chan_id, label);
  if (status != ECA_NORMAL) {
    printf("label setting failed (ca_put status: %d)\n", status);
  }
  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT epics_ca_get(CA_INFO * info, INT channel, float *pvalue)
{
  ca_pend_event(0.01);
  *pvalue = info->array[channel];

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

INT epics_ca_get_all(CA_INFO * info, INT channels, float *pvalue)
{
   int i;

   for (i = 0; i < MIN(info->num_channels, channels); i++)
      epics_ca_get(info, i, pvalue + i);

   return FE_SUCCESS;
}

/*---- device driver entry point -----------------------------------*/

INT epics_ca(INT cmd, ...)
{
   va_list argptr;
   HNDLE hKey;
   INT channel, status;
   DWORD flags;
   float value, *pvalue;
   CA_INFO *info;
   char *label;

   va_start(argptr, cmd);
   status = FE_SUCCESS;

   if (cmd == CMD_INIT) {
      void *pinfo;

      hKey = va_arg(argptr, HNDLE);
      pinfo = va_arg(argptr, void *);
      channel = va_arg(argptr, INT);
      flags = va_arg(argptr, DWORD);
      status = epics_ca_init(hKey, pinfo, channel);
      info = *(CA_INFO **) pinfo;
      info->flags = flags;
   } else {
      info = va_arg(argptr, void *);

      /* only execute command if enabled */
      switch (cmd) {
      case CMD_INIT:
  break;

      case CMD_EXIT:
  status = epics_ca_exit(info);
  break;

      case CMD_SET:
  channel = va_arg(argptr, INT);
  value = (float) va_arg(argptr, double);
  status = epics_ca_set(info, channel, value);
  break;

      case CMD_SET_LABEL:
  channel = va_arg(argptr, INT);
  label = va_arg(argptr, char *);
  status = epics_ca_set_label(info, channel, label);
  break;

      case CMD_GET:
  channel = va_arg(argptr, INT);
  pvalue = va_arg(argptr, float *);
  status = epics_ca_get(info, channel, pvalue);
  break;

      default:
  break;
      }
   }

   va_end(argptr);

   return status;
}

/*------------------------------------------------------------------*/
