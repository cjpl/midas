/********************************************************************\

  Name:         MRPC.C
  Created by:   Stefan Ritt

  Contents:     List of MSCB RPC functions with parameters

  $Log$
  Revision 1.1  2002/10/28 14:26:30  midas
  Changes from Japan


\********************************************************************/

#include "mscb.h"
#include "rpc.h"

/*------------------------------------------------------------------*/

static RPC_LIST rpc_list[] = {

  /* common functions */
  { RPC_MSCB_INIT, "mscb_init",
    {{TID_STRING,     RPC_IN}, 
     {TID_INT,        RPC_IN}, 
     {0} }},

  { RPC_MSCB_EXIT, "mscb_exit",
    {{TID_INT,        RPC_IN}, 
     {0} }},

  { RPC_MSCB_REBOOT, "mscb_reboot",
    {{TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN}, 
     {0} }},

  { RPC_MSCB_RESET, "mscb_reset",
    {{TID_INT,        RPC_IN}, 
     {0} }},

  { RPC_MSCB_PING, "mscb_ping",
    {{TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN}, 
     {0} }},

  { RPC_MSCB_INFO, "mscb_info",
    {{TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN}, 
     {TID_STRUCT,     RPC_OUT, sizeof(MSCB_INFO)}, 
     {0} }},

  { RPC_MSCB_INFO_CHANNEL, "mscb_info_channel",
    {{TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN}, 
     {TID_STRUCT,     RPC_OUT, sizeof(MSCB_INFO_CHN)}, 
     {0} }},

  { RPC_MSCB_SET_ADDR, "mscb_set_addr",
    {{TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN}, 
     {0} }},
  
  { RPC_MSCB_WRITE_GROUP, "mscb_write_group",
    {{TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN}, 
     {TID_BYTE,       RPC_IN},
     {TID_WORD,       RPC_IN}, 
     {TID_INT,        RPC_IN}, 
     {0} }},
 
  { RPC_MSCB_WRITE, "mscb_write",
    {{TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN}, 
     {TID_BYTE,       RPC_IN},
     {TID_WORD,       RPC_IN}, 
     {TID_INT,        RPC_IN}, 
     {0} }},
     
  { RPC_MSCB_WRITE_CONF, "mscb_write_conf",
    {{TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN}, 
     {TID_BYTE,       RPC_IN},
     {TID_WORD,       RPC_IN}, 
     {TID_INT,        RPC_IN}, 
     {0} }},


  { RPC_MSCB_FLASH, "mscb_flash",
    {{TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN}, 
     {0} }},


  { RPC_MSCB_UPLOAD, "mscb_upload",
    {{TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN}, 
     {TID_ARRAY,      RPC_IN | RPC_VARARRAY}, 
     {TID_INT,        RPC_IN}, 
     {0} }},


  { RPC_MSCB_READ, "mscb_read",
    {{TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN}, 
     {TID_BYTE,       RPC_IN},
     {TID_WORD,       RPC_IN}, 
     {0} }},

  { RPC_MSCB_READ_CONF, "mscb_read_conf",
    {{TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN}, 
     {TID_BYTE,       RPC_IN},
     {TID_WORD,       RPC_IN}, 
     {0} }},

  { RPC_MSCB_USER, "mscb_user",
    {{TID_INT,        RPC_IN}, 
     {TID_INT,        RPC_IN}, 
     {TID_ARRAY,      RPC_IN | RPC_VARARRAY},
     {TID_INT,        RPC_IN}, 
     {TID_ARRAY,      RPC_OUT | RPC_VARARRAY},
     {TID_INT,        RPC_IN | RPC_OUT}, 
     {0} }},

   { 0 }

};

/*------------------------------------------------------------------*/
