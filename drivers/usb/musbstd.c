/********************************************************************\

  Name:         musbstd.c
  Created by:   Konstantin Olchanski, Stefan Ritt

  Contents:     Midas USB access

  $Id$

\********************************************************************/

#include <stdio.h>
#include <assert.h>
#include "musbstd.h"

#ifdef _MSC_VER                 // Windows includes

#include <windows.h>
#include <conio.h>

#include <setupapi.h>
#include <initguid.h>           /* Required for GUID definition */

// link with SetupAPI.Lib.
#pragma comment (lib, "setupapi.lib")

// {CBEB3FB1-AE9F-471c-9016-9B6AC6DCD323}
DEFINE_GUID(GUID_CLASS_MSCB_BULK, 0xcbeb3fb1, 0xae9f, 0x471c, 0x90, 0x16, 0x9b, 0x6a, 0xc6, 0xdc, 0xd3, 0x23);

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

#elif defined(OS_LINUX)         // Linux includes

#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define HAVE_LIBUSB 1

#endif

#ifdef HAVE_LIBUSB
#include <usb.h>
#endif

int musb_open(MUSB_INTERFACE **musb_interface, int vendor, int product, int instance, int configuration, int usbinterface)
{
#if defined(_MSC_VER)

   GUID guid;
   HDEVINFO hDevInfoList;
   SP_DEVICE_INTERFACE_DATA deviceInfoData;
   PSP_DEVICE_INTERFACE_DETAIL_DATA functionClassDeviceData;
   ULONG predictedLength, requiredLength;
   int status;
   char device_name[256], str[256];

   *musb_interface = (MUSB_INTERFACE *)calloc(1, sizeof(MUSB_INTERFACE));

   guid = GUID_CLASS_MSCB_BULK;

   // Retrieve device list for GUID that has been specified.
   hDevInfoList = SetupDiGetClassDevs(&guid, NULL, NULL, (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));

   status = FALSE;
   if (hDevInfoList != NULL) {

      // Clear data structure
      memset(&deviceInfoData, 0, sizeof(deviceInfoData));
      deviceInfoData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

      // retrieves a context structure for a device interface of a device information set.
      if (SetupDiEnumDeviceInterfaces(hDevInfoList, 0, &guid, instance, &deviceInfoData)) {
         // Must get the detailed information in two steps
         // First get the length of the detailed information and allocate the buffer
         // retrieves detailed information about a specified device interface.
         functionClassDeviceData = NULL;

         predictedLength = requiredLength = 0;

         SetupDiGetDeviceInterfaceDetail(hDevInfoList, &deviceInfoData, NULL,   // Not yet allocated
                                         0,     // Set output buffer length to zero
                                         &requiredLength,       // Find out memory requirement
                                         NULL);

         predictedLength = requiredLength;
         functionClassDeviceData = (PSP_DEVICE_INTERFACE_DETAIL_DATA) malloc(predictedLength);
         functionClassDeviceData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

         // Second, get the detailed information
         if (SetupDiGetDeviceInterfaceDetail(hDevInfoList,
                                             &deviceInfoData, functionClassDeviceData,
                                             predictedLength, &requiredLength, NULL)) {

            // Save the device name for subsequent pipe open calls
            strcpy(device_name, functionClassDeviceData->DevicePath);
            free(functionClassDeviceData);

            // Signal device found
            status = TRUE;
         } else
            free(functionClassDeviceData);
      }
   }
   // SetupDiDestroyDeviceInfoList() destroys a device information set
   // and frees all associated memory.
   SetupDiDestroyDeviceInfoList(hDevInfoList);

   if (status) {

      // Get the read handle
      sprintf(str, "%s\\PIPE00", device_name);
      (*musb_interface)->rhandle = CreateFile(str,
                                    GENERIC_WRITE | GENERIC_READ,
                                    FILE_SHARE_WRITE | FILE_SHARE_READ, NULL,
                                    OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

      if ((*musb_interface)->rhandle == INVALID_HANDLE_VALUE)
         return MUSB_ACCESS_ERROR;

      // Get the write handle
      sprintf(str, "%s\\PIPE01", device_name);
      (*musb_interface)->whandle = CreateFile(str,
                                    GENERIC_WRITE | GENERIC_READ,
                                    FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

      if ((*musb_interface)->whandle == INVALID_HANDLE_VALUE)
         return MUSB_ACCESS_ERROR;

      return MUSB_SUCCESS;
   } 
     
   return MUSB_NOT_FOUND;

#elif defined(HAVE_LIBUSB)

   struct usb_bus *bus;
   struct usb_device *dev;
   int count = 0;

   usb_init();
   usb_find_busses();
   usb_find_devices();

   for (bus = usb_get_busses(); bus; bus = bus->next)
      for (dev = bus->devices; dev; dev = dev->next)
         if (dev->descriptor.idVendor == vendor && dev->descriptor.idProduct == product) {
            if (count == instance) {
               int status;
               usb_dev_handle *udev;

               udev = usb_open(dev);
               if (!udev) {
                  fprintf(stderr, "musb_open: usb_open() error\n");
                  return MUSB_ACCESS_ERROR;
               }

               status = usb_set_configuration(udev, configuration);
               if (status < 0) {
                  fprintf(stderr, "musb_open: usb_set_configuration() error %d (%s)\n", status,
                          strerror(-status));

                  fprintf(stderr,
                          "musb_open: Found USB device %04x:%04x instance %d, but cannot initialize it: please check permissions on \"/proc/bus/usb/%s/%s\"\n",
                          vendor, product, instance, bus->dirname, dev->filename);

                  return MUSB_ACCESS_ERROR;
               }

               /* see if we have write access */
               status = usb_claim_interface(udev, usbinterface);
               if (status < 0) {
                  fprintf(stderr, "musb_open: usb_claim_interface() error %d (%s)\n", status,
                          strerror(-status));

                  fprintf(stderr,
                          "musb_open: Found USB device %04x:%04x instance %d, but cannot initialize it: please check permissions on \"/proc/bus/usb/%s/%s\"\n",
                          vendor, product, instance, bus->dirname, dev->filename);

                  return MUSB_ACCESS_ERROR;
               }

               *musb_interface = calloc(1, sizeof(MUSB_INTERFACE));
               *musb_interface->dev = udev;
               *musb_interface->usbinterface = usbinterface;
               return MUSB_SUCCESS;
            }

            count++;
         }

   return MUSB_NOT_FOUND;

#elif defined(OS_DARWIN)

   MUSB_INTERFACE *musb_interface = calloc(1, sizeof(MUSB_INTERFACE));

   kern_return_t status;
   io_iterator_t iter;
   io_service_t service;
   IOCFPlugInInterface **plugin;
   SInt32 score;
   IOUSBDeviceInterface **device;
   IOUSBInterfaceInterface **uinterface;
   UInt16 xvendor, xproduct;
   UInt8 numend;
   int count = 0;

   status = IORegistryCreateIterator(kIOMasterPortDefault, kIOUSBPlane, kIORegistryIterateRecursively, &iter);
   assert(status == kIOReturnSuccess);

   while ((service = IOIteratorNext(iter))) {
      status =
          IOCreatePlugInInterfaceForService(service, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID,
                                            &plugin, &score);
      assert(status == kIOReturnSuccess);

      status = IOObjectRelease(service);
      assert(status == kIOReturnSuccess);

      status =
          (*plugin)->QueryInterface(plugin, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID), (void *) &device);
      assert(status == kIOReturnSuccess);

      status = (*plugin)->Release(plugin);

      status = (*device)->GetDeviceVendor(device, &xvendor);
      assert(status == kIOReturnSuccess);
      status = (*device)->GetDeviceProduct(device, &xproduct);
      assert(status == kIOReturnSuccess);

      printf("Found USB device: vendor 0x%04x, product 0x%04x\n", xvendor, xproduct);

      if (xvendor == vendor && xproduct == product) {
         if (count == instance) {
            status = (*device)->USBDeviceOpen(device);
            assert(status == kIOReturnSuccess);

            status = (*device)->SetConfiguration(device, 1);
            assert(status == kIOReturnSuccess);

            IOUSBFindInterfaceRequest request;

            request.bInterfaceClass = kIOUSBFindInterfaceDontCare;
            request.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;
            request.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
            request.bAlternateSetting = kIOUSBFindInterfaceDontCare;

            status = (*device)->CreateInterfaceIterator(device, &request, &iter);
            assert(status == kIOReturnSuccess);

            while ((service = IOIteratorNext(iter))) {
               status =
                   IOCreatePlugInInterfaceForService(service, kIOUSBInterfaceUserClientTypeID,
                                                     kIOCFPlugInInterfaceID, &plugin, &score);
               assert(status == kIOReturnSuccess);

               status =
                   (*plugin)->QueryInterface(plugin, CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID),
                                             (void *) &uinterface);
               assert(status == kIOReturnSuccess);


               status = (*uinterface)->USBInterfaceOpen(uinterface);
               printf("status 0x%x\n", status);
               assert(status == kIOReturnSuccess);

               status = (*uinterface)->GetNumEndpoints(uinterface, &numend);
               assert(status == kIOReturnSuccess);

               printf("endpoints: %d\n", numend);

               printf("pipe 1 status: 0x%x\n", (*uinterface)->GetPipeStatus(uinterface, 1));
               printf("pipe 2 status: 0x%x\n", (*uinterface)->GetPipeStatus(uinterface, 2));

               musb_interface->handle = uinterface;
               return MUSB_SUCCESS;
            }

            fprintf(stderr, "musb_open: cannot find any interfaces!");
            return MUSB_NOT_FOUND;
         }

         count++;
      }

      (*device)->Release(device);
   }

   return MUSB_NOT_FOUND;
#endif
}

int musb_close(MUSB_INTERFACE *musb_interface)
{
#if defined(_MSC_VER)
   CloseHandle(musb_interface->rhandle);
   CloseHandle(musb_interface->whandle);
#elif defined(HAVE_LIBUSB)
   int status;
   status = usb_release_interface(musb_interface->dev, musb_interface->usbinterface);
   if (status < 0)
      fprintf(stderr, "musb_close: usb_release_interface() error %d\n", status);
   status = usb_close(musb_interface->dev);
   if (status < 0)
      fprintf(stderr, "musb_close: usb_close() error %d\n", status);
#else
   /* FIXME */
#endif
   return 0;
}

int musb_write(MUSB_INTERFACE *musb_interface, int endpoint, const void *buf, int count, int timeout)
{
   int n_written;

#if defined(_MSC_VER)
   WriteFile(musb_interface->whandle, buf, count, &n_written, NULL);
#elif defined(HAVE_LIBUSB)
   n_written = usb_bulk_write(musb_interface->dev, endpoint, buf, count, timeout);
   //usleep(0); // needed for linux not to crash !!!!
#elif defined(OS_DARWIN)
   IOReturn status;
   IOUSBInterfaceInterface **device = musb_interface->handle;
   status = (*device)->WritePipe(device, endpoint, buf, count);
   if (status != 0)
      printf("musb_write: WritePipe() status %d 0x%x\n", status, status);
   n_written = count;
#endif
   return n_written;
}

int musb_read(MUSB_INTERFACE *musb_interface, int endpoint, void *buf, int count, int timeout)
{
   int n_read = 0;

#if defined(_MSC_VER)

   OVERLAPPED overlapped;
   int status;

   memset(&overlapped, 0, sizeof(overlapped));
   overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
   n_read = 0;

   status = ReadFile(musb_interface->rhandle, buf, count, &n_read, &overlapped);
   if (!status) {

      status = GetLastError();
      if (status != ERROR_IO_PENDING)
         return 0;

      /* wait for completion, 1s timeout */
      status = WaitForSingleObject(overlapped.hEvent, 1000);
      if (status == WAIT_TIMEOUT)
         CancelIo(musb_interface->rhandle);
      else
         GetOverlappedResult(musb_interface->rhandle, &overlapped, &n_read, FALSE);
   }

   CloseHandle(overlapped.hEvent);

#elif defined(HAVE_LIBUSB)

   n_read = usb_bulk_read(musb_interface->dev, endpoint, buf, count, timeout);

#elif defined(OS_DARWIN)

   IOReturn status;
   IOUSBInterfaceInterface **device = musb_interface->handle;
   n_read = count;
   status = (*device)->ReadPipe(device, endpoint, buf, &n_read);
   if (status != kIOReturnSuccess) {
      printf("musb_read: requested %d, read %d, ReadPipe() status %d 0x%x\n", count, n_read, status, status);
      return -1;
   }
#endif
   return n_read;
}

int musb_reset(MUSB_INTERFACE *musb_interface)
{

#if defined(_MSC_VER)
   assert(!"musb_reset: not implemented");
#elif defined(HAVE_LIBUSB)

   /* Causes re-enumeration: After calling usb_reset, the device will need 
      to re-enumerate and thusly, requires you to find the new device and 
      open a new handle. The handle used to call usb_reset will no longer work */
   return usb_reset(musb_interface->dev);

#elif defined(OS_DARWIN)
   assert(!"musb_reset: not implemented");
#endif
   return 0;
}

/* end */
