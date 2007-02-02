/********************************************************************\

  Name:         TMidasOnline.cxx
  Created by:   Konstantin Olchanski - TRIUMF

  Contents:     C++ MIDAS analyzer

  $Id$

  $Log: TMidasOnline.cxx,v $
  Revision 1.3  2006/08/07 09:19:51  alpha
  RAH - changes made to the connection routine (by KO)

  Revision 1.2  2006/06/05 19:22:22  alpha
  KO- added functions to read values from ODB

  Revision 1.1  2006/05/25 05:58:05  alpha
  First commit


\********************************************************************/

#include "TMidasOnline.h"

#include <string>
#include <assert.h>
#include "midas.h"
#include "msystem.h"
#include "hardware.h"
#include "ybos.h"


TMidasOnline::TMidasOnline() // ctor
{
  fDB = 0;
  fStartHandler  = 0;
  fStopHandler   = 0;
  fPauseHandler  = 0;
  fResumeHandler = 0;
  fEventRequests = 0;
  fEventHandler  = 0;
}

TMidasOnline::~TMidasOnline() // dtor
{
  disconnect();
}

TMidasOnline* TMidasOnline::instance()
{
  if (!gfMidas)
    gfMidas = new TMidasOnline();
  
  return gfMidas;
}

int TMidasOnline::connect(const char*hostname,const char*exptname,const char*progname)
{
  int status;
  
  char xhostname[HOST_NAME_LENGTH];
  char xexptname[NAME_LENGTH];
  
  if (hostname)
    strlcpy(xhostname,hostname,sizeof(xhostname));
  else
    xhostname[0] = 0;
  
  if (exptname)
    strlcpy(xexptname,exptname,sizeof(xexptname));
  else
    xexptname[0] = 0;
  
  /* get default from environment */
  status = cm_get_environment(xhostname, sizeof(xhostname), xexptname, sizeof(xexptname));
  assert(status == CM_SUCCESS);
  
  fHostname = xhostname;
  fExptname = xexptname;

  printf("TMidasOnline::connect: Connecting to experiment \"%s\" on host \"%s\"\n", fExptname.c_str(), fHostname.c_str());
  
  //int watchdog = DEFAULT_WATCHDOG_TIMEOUT;
  int watchdog = 60*1000;

  status = cm_connect_experiment1((char*)fHostname.c_str(), (char*)fExptname.c_str(), (char*)progname, NULL, DEFAULT_ODB_SIZE, watchdog);
  
  if (status == CM_UNDEF_EXP)
    {
      printf("Error: experiment \"%s\" not defined.\n", fExptname.c_str());
      return -1;
    }
  else if (status != CM_SUCCESS)
    {
      printf("Error %d connecting to MIDAS.\n", status);
      return -1;
    }
  
  status = cm_get_experiment_database(&fDB, NULL);
  assert(status == CM_SUCCESS);
  
  cm_set_watchdog_params(true, 60*1000);

  return 0;
}

int TMidasOnline::disconnect()
{
  if (fDB)
    {
      printf("TMidasOnline::disconnect: Disconnecting from experiment \"%s\" on host \"%s\"\n", fExptname.c_str(), fHostname.c_str());
      cm_disconnect_experiment();
      fDB = 0;
    }
  
  return 0;
}

void TMidasOnline::registerTransitions()
{
  cm_register_transition(TR_START,  NULL, 300);
  cm_register_transition(TR_PAUSE,  NULL, 700);
  cm_register_transition(TR_RESUME, NULL, 300);
  cm_register_transition(TR_STOP,   NULL, 700);
}

void TMidasOnline::setTransitionHandlers(TransitionHandler start,TransitionHandler stop,TransitionHandler pause,TransitionHandler resume)
{
  fStartHandler  = start;
  fStopHandler   = stop;
  fPauseHandler  = pause;
  fResumeHandler = resume;
}

bool TMidasOnline::checkTransitions()
{
  int transition, run_number, trans_time;
  
  int status = cm_query_transition(&transition, &run_number, &trans_time);
  if (status != CM_SUCCESS)
    return false;
  
  //printf("cm_query_transition: status %d, tr %d, run %d, time %d\n",status,transition,run_number,trans_time);
  
  if (transition == TR_START)
    {
      if (fStartHandler)
	(*fStartHandler)(transition,run_number,trans_time);
      return true;
    }
  else if (transition == TR_STOP)
    {
      if (fStopHandler)
	(*fStopHandler)(transition,run_number,trans_time);
      return true;
      
    }
  else if (transition == TR_PAUSE)
    {
      if (fPauseHandler)
	(*fPauseHandler)(transition,run_number,trans_time);
      return true;
      
    }
  else if (transition == TR_RESUME)
    {
      if (fResumeHandler)
	(*fResumeHandler)(transition,run_number,trans_time);
      return true;
    }
  
  return false;
}

bool TMidasOnline::poll(int mdelay)
{
  //printf("poll!\n");
  
  if (checkTransitions())
    return true;
  
  int status = cm_yield(mdelay);
  if (status == RPC_SHUTDOWN || status == SS_ABORT)
    {
      printf("TMidasOnline: poll: cm_yield(%d) status %d, shutting down.\n",mdelay,status);
      disconnect();
      return false;
    }
  
  return true;
}

void TMidasOnline::setEventHandler(EventHandler handler)
{
  fEventHandler  = handler;
}

static void eventCallback(HNDLE buffer_handle, HNDLE request_id, EVENT_HEADER* pheader, void* pevent)
{
#if 0
  printf("eventCallback: buffer %d, request %d, pheader %p (event_id: %d, trigger mask: 0x%x, serial: %d, time: %d, size: %d), pevent %p\n",
	 buffer_handle,
	 request_id,
	 pheader,
	 pheader->event_id,
	 pheader->trigger_mask,
	 pheader->serial_number,
	 pheader->time_stamp,
	 pheader->data_size,
	 pevent);
#endif
  
  if (TMidasOnline::instance()->fEventHandler)
    TMidasOnline::instance()->fEventHandler(pheader,pevent,pheader->data_size);
}

int TMidasOnline::eventRequest(const char* bufferName,int eventId,int triggerMask,int samplingType)
{
  int status;
  EventRequest* r = new EventRequest();
  
  r->fNext         = NULL;
  r->fBufferName   = bufferName;
  r->fEventId      = eventId;
  r->fTriggerMask  = triggerMask;
  r->fSamplingType = samplingType;
  
  
  /*---- open event buffer ---------------------------------------*/
  status = bm_open_buffer((char*)bufferName, EVENT_BUFFER_SIZE, &r->fBufferHandle);
  if (status != SUCCESS)
    {
      printf("TMidasOnline::eventRequest: Cannot find data buffer \"%s\", bm_open_buffer() error %d\n", bufferName, status);
      return -1;
    }
  
  /* set the default buffer cache size */
  status = bm_set_cache_size(r->fBufferHandle, 100000, 0);
  assert(status == BM_SUCCESS);
  
  status = bm_request_event(r->fBufferHandle, r->fEventId, r->fTriggerMask, r->fSamplingType, &r->fRequestId, eventCallback);
  assert(status == BM_SUCCESS);
  
  printf("Event request: buffer \"%s\" (%d), event id 0x%x, trigger mask 0x%x, sample %d, request id: %d\n",bufferName,r->fBufferHandle,r->fEventId,r->fTriggerMask,r->fSamplingType,r->fRequestId);
  
  r->fNext = fEventRequests;
  fEventRequests = r;
  
  return r->fRequestId;
};

void TMidasOnline::deleteEventRequest(int requestId)
{
  for (EventRequest* r = fEventRequests; r != NULL; r = r->fNext)
    if (r->fRequestId == requestId)
      {
	int status = bm_delete_request(r->fRequestId);
	assert(status == BM_SUCCESS);
	
	r->fBufferHandle = -1;
	r->fRequestId    = -1;
      }
}

int TMidasOnline::odbReadInt(const char*name,int index,int defaultValue)
{
  int value = defaultValue;
  if (odbReadAny(name,index,TID_INT,&value) == 0)
    return value;
  else
    return defaultValue;
};

uint32_t TMidasOnline::odbReadUint32(const char*name,int index,uint32_t defaultValue)
{
  uint32_t value = defaultValue;
  if (odbReadAny(name,index,TID_DWORD,&value) == 0)
    return value;
  else
    return defaultValue;
};

bool     TMidasOnline::odbReadBool(const char*name,int index,bool defaultValue)
{
  bool value = defaultValue;
  if (odbReadAny(name,index,TID_BOOL,&value) == 0)
    return value;
  else
    return defaultValue;
};

int TMidasOnline::odbReadAny(const char*name,int index,int tid,void* value)
{
  int status;
  int size = rpc_tid_size(tid);
  HNDLE hdir = 0;
  HNDLE hkey;

  status = db_find_key (fDB, hdir, (char*)name, &hkey);
  if (status == SUCCESS)
    {
      status = db_get_data_index(fDB, hkey, value, &size, index, tid);
      if (status != SUCCESS)
        {
          cm_msg(MERROR, "TMidasOnline", "Cannot read \'%s\'[%d] of type %d from odb, db_get_data_index() status %d", name, index, tid, status);
          return -1;
        }

      return 0;
    }
  else if (status == DB_NO_KEY)
    {
      cm_msg(MINFO, "TMidasOnline", "Creating \'%s\'[%d] of type %d", name, index, tid);

      status = db_create_key(fDB, hdir, (char*)name, tid);
      if (status != SUCCESS)
        {
          cm_msg (MERROR, "TMidasOnline", "Cannot create \'%s\' of type %d, db_create_key() status %d", name, tid, status);
          return -1;
        }

      status = db_find_key (fDB, hdir, (char*)name, &hkey);
      if (status != SUCCESS)
        {
          cm_msg(MERROR, "TMidasOnline", "Cannot create \'%s\', db_find_key() status %d", name, status);
          return -1;
        }

      status = db_set_data_index(fDB, hkey, value, size, index, tid);
      if (status != SUCCESS)
        {
          cm_msg(MERROR, "TMidasOnline", "Cannot write \'%s\'[%d] of type %d to odb, db_set_data_index() status %d", name, index, tid, status);
          return -1;
        }

      return 0;
    }
  else
    {
      cm_msg(MERROR, "TMidasOnline", "Cannot read \'%s\'[%d] from odb, db_find_key() status %d", name, index, status);
      return -1;
    }
};

TMidasOnline* TMidasOnline::gfMidas = NULL;

//end
