/********************************************************************\

  Name:         musbstd.c
  Created by:   Konstantin Olchanski

  Contents:     Midas USB access

  $Id$

\********************************************************************/

#include <stdio.h>
#include "musbstd.h"

#ifdef _MSC_VER                 // Windows includes

#include <windows.h>
#include <conio.h>

#elif defined(OS_DARWIN)

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <fcntl.h>

#include <assert.h>
#include <mach/mach.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>

#elif defined(OS_LINUX)        // Linux includes

#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define HAVE_LIBUSB 1

#endif

#ifdef HAVE_LIBUSB
#include <usb.h>
#endif

#if defined(_MSC_VER)

typedef struct
{
  HANDLE rhandle;
  HANDLE whandle;
} Handle_t;

#elif defined(HAVE_LIBUSB)

typedef struct
{
  usb_dev_handle* dev;
  int             interface;
} Handle_t;

#elif defined(OS_DARWIN)

typedef struct
{
  IOUSBInterfaceInterface** handle;
} Handle_t;

#endif

void* musb_open(int vendor,int product,int instance,int configuration,int interface)
{
#if defined(_MSC_VER)

   Handle_t* handle = calloc(1,sizeof(Handle_t));
   assert(!"FIXME!");
   return NULL;

#elif defined(HAVE_LIBUSB)

   struct usb_bus *bus;
   struct usb_device *dev;
   int count = 0;

   usb_init();
   usb_find_busses();
   usb_find_devices();

   for (bus = usb_get_busses(); bus; bus = bus->next)
     for (dev = bus->devices; dev; dev = dev->next)
       if (dev->descriptor.idVendor  == vendor &&
           dev->descriptor.idProduct == product)
         {
           if (count == instance)
             {
               int status;
               usb_dev_handle *udev;

               udev = usb_open(dev);
               if (!udev)
                 {
                   fprintf(stderr,"musb_open: usb_open() error\n");
                   return NULL;
                 }

               status = usb_set_configuration(udev, configuration);
               if (status < 0)
                 {
                   fprintf(stderr,"musb_open: usb_set_configuration() error %d (%s)\n",status,strerror(-status));

		   fprintf(stderr,"musb_open: Found USB device %04x:%04x instance %d, but cannot initialize it: please check permissions on \"/proc/bus/usb/%s/%s\"\n",vendor,product,instance,bus->dirname,dev->filename);

                   return NULL;
                 }

               /* see if we have write access */
               status = usb_claim_interface(udev, interface);
               if (status < 0)
                 {
                   fprintf(stderr,"musb_open: usb_claim_interface() error %d (%s)\n",status,strerror(-status));

		   fprintf(stderr,"musb_open: Found USB device %04x:%04x instance %d, but cannot initialize it: please check permissions on \"/proc/bus/usb/%s/%s\"\n",vendor,product,instance,bus->dirname,dev->filename);

                   return NULL;
                 }

               Handle_t* handle = calloc(1,sizeof(Handle_t));
               handle->dev       = udev;
	       handle->interface = interface;
               return handle;
             }

           count++;
         }

   return NULL;

#elif defined(OS_DARWIN)

  Handle_t* handle = calloc(1,sizeof(Handle_t));

  kern_return_t status;
  io_iterator_t iter;
  io_service_t  service;
  IOCFPlugInInterface** plugin;
  SInt32 score;
  IOUSBDeviceInterface** device;
  IOUSBInterfaceInterface** uinterface;
  UInt16 xvendor, xproduct;
  UInt8 numend;
  int count = 0;

  status = IORegistryCreateIterator(kIOMasterPortDefault,kIOUSBPlane,kIORegistryIterateRecursively,&iter);
  assert(status==kIOReturnSuccess);

  while ((service = IOIteratorNext(iter)))
    {
      status = IOCreatePlugInInterfaceForService(service, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &plugin, &score);
      assert(status==kIOReturnSuccess);

      status = IOObjectRelease(service);
      assert(status==kIOReturnSuccess);

      status = (*plugin)->QueryInterface(plugin, CFUUIDGetUUIDBytes (kIOUSBDeviceInterfaceID), (void*)&device);
      assert(status==kIOReturnSuccess);

      status = (*plugin)->Release(plugin);
      
      status = (*device)->GetDeviceVendor(device, &xvendor);
      assert(status==kIOReturnSuccess);
      status = (*device)->GetDeviceProduct(device, &xproduct);
      assert(status==kIOReturnSuccess);

      printf("Found USB device: vendor 0x%04x, product 0x%04x\n",xvendor,xproduct);

      if (xvendor==vendor && xproduct==product)
        {
          if (count==instance)
            {
              status = (*device)->USBDeviceOpen(device);
              assert(status==kIOReturnSuccess);
              
              status = (*device)->SetConfiguration(device,1);
              assert(status==kIOReturnSuccess);

              IOUSBFindInterfaceRequest request;

              request.bInterfaceClass = kIOUSBFindInterfaceDontCare;
              request.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;
              request.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
              request.bAlternateSetting = kIOUSBFindInterfaceDontCare;

	      status = (*device)->CreateInterfaceIterator(device,&request,&iter);
	      assert(status==kIOReturnSuccess);

	      while ((service=IOIteratorNext(iter)))
                {
                  status = IOCreatePlugInInterfaceForService(service, kIOUSBInterfaceUserClientTypeID, kIOCFPlugInInterfaceID, &plugin, &score);
                  assert(status==kIOReturnSuccess);
                  
                  status = (*plugin)->QueryInterface(plugin, CFUUIDGetUUIDBytes (kIOUSBInterfaceInterfaceID), (void*)&uinterface);
                  assert(status==kIOReturnSuccess);
                  

                  status = (*uinterface)->USBInterfaceOpen(uinterface);
                  printf("status 0x%x\n",status);
                  assert(status==kIOReturnSuccess);
                  
                  status = (*uinterface)->GetNumEndpoints(uinterface,&numend);
                  assert(status==kIOReturnSuccess);
                  
                  printf("endpoints: %d\n",numend);
                  
                  printf("pipe 1 status: 0x%x\n",(*uinterface)->GetPipeStatus(uinterface,1));
                  printf("pipe 2 status: 0x%x\n",(*uinterface)->GetPipeStatus(uinterface,2));
                  
                  handle->handle = uinterface;
                  return handle;
                }

              fprintf(stderr,"musb_open: cannot find any interfaces!");
              return NULL;
            }

          count++;
        }

      (*device)->Release(device);
    }
#endif
}

int musb_close(void* handle)
{
   Handle_t* h = (Handle_t*)handle;
#if defined(_MSC_VER)
   CloseHandle(h->rhandle);
   CloseHandle(h->whandle);
#elif defined(HAVE_LIBUSB)
   int status;
   status = usb_release_interface(h->dev,h->interface);
   if (status < 0)
     fprintf(stderr,"musb_close: usb_release_interface() error %d\n",status);
   status = usb_close(h->dev);
   if (status < 0)
     fprintf(stderr,"musb_close: usb_close() error %d\n",status);
#else
   /* FIXME */
#endif
   return 0;
}

int musb_write(void* handle,int endpoint,const void *buf,int count,int timeout)
{
   Handle_t* h = (Handle_t*)handle;
   int n_written;

#if defined(_MSC_VER)
   WriteFile(h->whandle, buf, size, &n_written, NULL);
#elif defined(HAVE_LIBUSB)
   n_written = usb_bulk_write(h->dev, endpoint, buf, count, timeout);
   //usleep(0); // needed for linux not to crash !!!!
#elif defined(OS_DARWIN)
   IOReturn status;
   IOUSBInterfaceInterface** device = h->handle;
   status = (*device)->WritePipe(device,endpoint,buf,count);
   if (status != 0) printf("musb_write: WritePipe() status %d 0x%x\n",status,status);
   n_written = count;
#endif
   return n_written;
}

int musb_read(void* handle,int endpoint,void *buf,int count,int timeout)
{
   Handle_t* h = (Handle_t*)handle;
   int n_read = 0;

#if defined(_MSC_VER)

   OVERLAPPED overlapped;
   int status;

   memset(&overlapped, 0, sizeof(overlapped));
   overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
   n_read = 0;

   status = ReadFile((HANDLE) fd, buf, size, &n_read, &overlapped);
   if (!status) {

      status = GetLastError();
      if (status != ERROR_IO_PENDING)
         return 0;

      /* wait for completion, 1s timeout */
      status = WaitForSingleObject(overlapped.hEvent, 1000);
      if (status == WAIT_TIMEOUT)
         CancelIo((HANDLE) fd);
      else
         GetOverlappedResult((HANDLE) fd, &overlapped, &n_read, FALSE);
   }

   CloseHandle(overlapped.hEvent);

#elif defined(HAVE_LIBUSB)

   n_read = usb_bulk_read(h->dev, endpoint, buf, count, timeout);

#elif defined(OS_DARWIN)

   IOReturn status;
   IOUSBInterfaceInterface** device = h->handle;
   n_read = count;
   status = (*device)->ReadPipe(device,endpoint,buf,&n_read);
   if (status != kIOReturnSuccess) {
      printf("musb_read: requested %d, read %d, ReadPipe() status %d 0x%x\n",count,n_read,status,status);
      return -1;
   }

#endif
   return n_read;
}

int musb_reset(void*handle)
{
   Handle_t* h = (Handle_t*)handle;

#if defined(_MSC_VER)
   assert(!"musb_reset: not implemented");
#elif defined(HAVE_LIBUSB)

   return usb_reset(h->dev);

#elif defined(OS_DARWIN)
   assert(!"musb_reset: not implemented");
#endif
   return 0;
}

/* end */
