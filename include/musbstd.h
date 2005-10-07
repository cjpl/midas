/********************************************************************\

  Name:         musbstd.h
  Created by:   Konstantin Olchanski

  Contents:     Midas USB access

  $Id:$

\********************************************************************/


void* musb_open(int vendor,int product,int instance,int configuration,int interface);
int musb_close(void* handle);
int musb_write(void* handle,int endpoint,const void *buf,int count,int timeout_ms);
int musb_read(void* handle,int endpoint,void *buf,int count,int timeout_ms);
int musb_reset(void*handle);

/* end */
