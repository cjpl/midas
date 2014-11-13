/* CC-USB firmware update utility
 * 
 * Copyright (C) 2005-2009 Jan Toke (toke@chem.rochester.edu)
 *
 *     This program is free software; you can redistribute it and/or
 *     modify it under the terms of the GNU General Public License as
 *     published by the Free Software Foundation, version 2.
 *
 *  This Program is a utility which allows the user of a CC_USB to update the
 *    USB firmware of the CC_USB from Linux  
 * 
*/
#include <stdio.h>
#include <string.h>
#include <usb.h>
#include <time.h>
//#include <libxxusb.h>

#define UCHAR unsigned char

#define XXUSB_WIENER_VENDOR_ID	0x16DC   /* Wiener, Plein & Baus */
#define XXUSB_VMUSB_PRODUCT_ID	0x000B	 /* VM-USB */
#define XXUSB_CCUSB_PRODUCT_ID	0x0001	 /* CC-USB */

#define XXUSB_ENDPOINT_OUT	2  /* Endpoint 2 Out*/
#define XXUSB_ENDPOINT_IN    0x86  /* Endpoint 6 In */

struct xxusb_device_type
{
  struct usb_device *usbdev;
  char SerialString[7];
};

short xxusb_flashblock_program(usb_dev_handle *hDev, UCHAR *config)
{
	int k=0;
	short ret=0;

	UCHAR *pconfig;
	char *pbuf;
	pconfig=config;
	char buf[518] ={(char)0xAA,(char)0xAA,(char)0x55,(char)0x55,(char)0xA0,(char)0xA0};
	pbuf=buf+6;
	for (k=0; k<256; k++)
	{
		*(pbuf++)=(UCHAR)(*(pconfig));
		*(pbuf++)=(UCHAR)(*(pconfig++));
	}
	ret = usb_bulk_write(hDev, XXUSB_ENDPOINT_OUT, buf, 518, 2000);
	return ret;
}

short xxusb_devices_find(xxusb_device_type *xxdev)
{
  short DevFound = 0;
  usb_dev_handle *udev;
  struct usb_bus *bus;
  struct usb_device *dev;  
struct usb_bus *usb_busses;
  char string[256];
  short ret;
   usb_init();
   usb_find_busses();
   usb_busses=usb_get_busses();
  usb_find_devices();
   for (bus=usb_busses; bus; bus = bus->next)
   {
       for (dev = bus->devices; dev; dev= dev->next)
       {
  	   if (dev->descriptor.idVendor==XXUSB_WIENER_VENDOR_ID)
  	   {
  	       udev = usb_open(dev);
	       if (udev) 
               {
		   ret = usb_get_string_simple(udev, dev->descriptor.iSerialNumber, string, sizeof(string));
		   if (ret >0 )  
		     {
		       xxdev[DevFound].usbdev=dev;
		       strcpy(xxdev[DevFound].SerialString, string);
		       DevFound++;
		     }
         	   usb_close(udev);
		}
		   else return -1;
	   }
       }
    }
  return DevFound;
}

short xxusb_device_close(usb_dev_handle *hDev)
{
    short ret;
    ret=usb_release_interface(hDev,0);
    usb_close(hDev);
    return 1;
}

usb_dev_handle* xxusb_device_open(struct usb_device *dev)
{
    short ret;
    usb_dev_handle *udev;
    udev = usb_open(dev);
    ret = usb_set_configuration(udev,1);
    ret = usb_claim_interface(udev,0);
    ret=usb_clear_halt(udev,0x86);
    ret=usb_clear_halt(udev,2); 
    return udev;
}

short xxusb_reset_toggle(usb_dev_handle *hDev)
{
  short ret;
  char buf[2] = {(char)255,(char)255}; 
  int lDataLen=2;
  int timeout=1000;
  ret = usb_bulk_write(hDev, XXUSB_ENDPOINT_OUT, buf,lDataLen, timeout);
  return (short)ret;
}

int main(int argc, char *fname[])
{

  //Make sure the number of arguements is correct
  if(argc != 2)
  {
    printf("\n\n Must provide a firmware file name as an argument\n\n");
    return 0;
  }

  struct usb_device *dev;
  xxusb_device_type devices[100]; 
  time_t t1, t2;
  int i;
  usb_dev_handle *udev;
  unsigned char* pconfig=new unsigned char[220000];
  unsigned char *p;
  int chint;
  long ik;
  FILE *fptr;
  int ret;



	printf("\n*************************************************************************\n\n");
	printf("                   WIENER VM_USB Firmware upgrade\n");
  
  //open CC_USB
  xxusb_devices_find(devices);
  dev = devices[0].usbdev;
  udev = xxusb_device_open(dev); 
	if(!udev)
	{
    printf("\nUnable to open a CC_USB device\n");
    return 0;
  }
	  
	  
	//open Firmware File  
  ik=0;
	if ((fptr=fopen(fname[1],"rb"))==NULL)
	{
    printf("\n File not Found\n");
	  return 0;
	}
	
	
  // Flash VM_USB
	p=pconfig;
	while((chint=getc(fptr)) != EOF)
	{
	 *p=(unsigned char)chint;
	 p++;
	 ik=ik+1;
	}
	printf("\n");
	for (i=0; i < 512; i++)
	{
	 ret=xxusb_flashblock_program(udev,pconfig);
	 pconfig=pconfig+256;
	 t1=(time_t)(.03*CLOCKS_PER_SEC);
	 t1=clock()+(time_t)(.03*CLOCKS_PER_SEC);
	 while (t1>clock());
	   t2=clock();
  printf(".");
	if (i>0)
	 if ((i & 63)==63)
	   printf("\n");
  }
	while (clock() < t1)
	    ;
	  t2=clock();
	
  ret = xxusb_reset_toggle(udev);
	
  sleep(1);

#if 0  
  long fwid;
  ret=CAMAC_register_read(udev,0,&fwid);
	printf("\n\nThe New Firmware ID is %lx\n\n\n",(fwid & 0xffff));
#endif
	if (udev)
	xxusb_device_close(udev);
	return 1;
}

  
