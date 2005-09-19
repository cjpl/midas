/********************************************************************\

  Name:         musbstd.h
  Created by:   Konstantin Olchanski

  Contents:     Midas USB access

  $Log$
  Revision 1.1  2005/09/19 21:33:15  olchanski
  First cut at the standard USB access functions, used by USB-MSCB and USB-CAMAC (CCUSB) drivers


\********************************************************************/


void* musb_open(int vendor,int product,int instance,int configuration,int interface);
int musb_close(void* handle);
int musb_write(void* handle,int endpoint,const void *buf,int count,int timeout_ms);
int musb_read(void* handle,int endpoint,void *buf,int count,int timeout_ms);
int musb_reset(void*handle);

/* end */
