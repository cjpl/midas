/***************************************************************************/
/*                                                                         */
/*  Filename: sis3100_vme_calls.c                                          */
/*                                                                         */
/*  Funktion:                                                              */
/*                                                                         */
/*  Autor:                TH                                               */
/*  date:                 10.05.2002                                       */
/*  last modification:    01.07.2002                                       */
/*                                                                         */
/* ----------------------------------------------------------------------- */
/*                                                                         */
/*  SIS  Struck Innovative Systeme GmbH                                    */
/*                                                                         */
/*  Harksheider Str. 102A                                                  */
/*  22399 Hamburg                                                          */
/*                                                                         */
/*  Tel. +49 (0)40 60 87 305 0                                             */
/*  Fax  +49 (0)40 60 87 305 20                                            */
/*                                                                         */
/*  http://www.struck.de                                                   */
/*                                                                         */
/*  © 2002                                                                 */
/*                                                                         */
/***************************************************************************/



#if !defined _DEBUG 
  #define _DEBUG
#endif
#if !defined PLX_9054 
  #define PLX_9054
#endif
#if !defined PCI_CODE 
  #define PCI_CODE
#endif
#if !defined LITTLE_ENDIAN 
  #define LITTLE_ENDIAN
#endif
/*---------------------------------------------------------------------------*/
/* Include files                                                             */
/*---------------------------------------------------------------------------*/

#include <stdio.h>
//#include <ansi_c.h>


#include "PlxApi.h"

//#include "sis1100w.h"		// Header sis1100w.dll
#include "sis1100w.h" // Header sis1100w.dll



typedef unsigned char           u_int8_t;
typedef unsigned short          u_int16_t;
typedef unsigned long           u_int32_t;



/**********************/
/*                    */
/*    VME SYSReset    */
/*                    */
/**********************/








int vmesysreset(struct SIS1100_Device_Struct* pDevice)
{
	sis1100w_VmeSysreset(pDevice)  ;
  return 0 ;
}






/*****************/
/*               */
/*    VME A16    */
/*               */
/*****************/

/* VME A16  Read Cycles */

int vme_A16D8_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int8_t* vme_data )
{
  unsigned int readdata ;
  int return_code ;
  return_code = sis1100w_Vme_Single_Read(pDevice, vme_adr,0x29, 1,  &readdata) ;
  if (return_code < 0)  return return_code ;
  *vme_data = (u_int8_t) readdata; 
  return return_code ;
}


int vme_A16D16_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int16_t* vme_data )
{
  unsigned int readdata ;
  int return_code ;
  return_code = sis1100w_Vme_Single_Read(pDevice, vme_adr,0x29, 2,  &readdata) ;
  if (return_code < 0)  return return_code ;
  *vme_data = (u_int16_t) readdata; 
  return return_code ;
}


int vme_A16D32_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data )
{
  unsigned int readdata ;
  int return_code ;
  return_code = sis1100w_Vme_Single_Read(pDevice, vme_adr,0x29, 4,  &readdata) ;
  if (return_code < 0)  return return_code ;
  *vme_data = readdata; 
  return return_code ;
}









/* VME A16  Write Cycles */
int vme_A16D8_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int8_t vme_data )
{
  u_int32_t data_32 ;
  int return_code ;
  data_32 =  (u_int32_t) vme_data ;
  return_code = sis1100w_Vme_Single_Write(pDevice, vme_adr, 0x29, 1,  data_32) ;
  return return_code ;
}

int vme_A16D16_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int16_t vme_data )
{
  u_int32_t data_32 ;
  int return_code ;
  data_32 =  (u_int32_t) vme_data ;
  return_code = sis1100w_Vme_Single_Write(pDevice, vme_adr, 0x29, 2,  data_32) ;
  return return_code ;
}


int vme_A16D32_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t vme_data )
{
  int return_code ;
  return_code = sis1100w_Vme_Single_Write(pDevice, vme_adr, 0x29, 4,  vme_data) ;
  return return_code ;
}













/*****************/
/*               */
/*    VME A24    */
/*               */
/*****************/

/* VME A24  Read Cycles */

int vme_A24D8_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int8_t* vme_data )
{
  unsigned int readdata ;
  int return_code ;
  return_code = sis1100w_Vme_Single_Read(pDevice, vme_adr,0x39, 1,  &readdata) ;
  if (return_code < 0)  return return_code ;
  *vme_data = (u_int8_t) readdata; 
  return return_code ;
}


int vme_A24D16_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int16_t* vme_data )
{
  unsigned int readdata ;
  int return_code ;
  return_code = sis1100w_Vme_Single_Read(pDevice, vme_adr,0x39, 2,  &readdata) ;
  if (return_code < 0)  return return_code ;
  *vme_data = (u_int16_t) readdata; 
  return return_code ;
}


int vme_A24D32_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data )
{
  unsigned int readdata ;
  int return_code ;
  return_code = sis1100w_Vme_Single_Read(pDevice, vme_adr,0x39, 4,  &readdata) ;
  if (return_code < 0)  return return_code ;
  *vme_data = readdata; 
  return return_code ;
}






int vme_A24BLT32_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords)
{
  int return_code ;
	return_code = sis1100w_Vme_Dma_Read(pDevice, vme_adr, 0x3b, 4, 0,vme_data, req_num_of_lwords, got_num_of_lwords);
   return return_code ;
}


int vme_A24MBLT64_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords)
{
  int return_code ;
	return_code = sis1100w_Vme_Dma_Read(pDevice, vme_adr, 0x38, 4, 0,vme_data, req_num_of_lwords, got_num_of_lwords);
   return return_code ;
}


int vme_A24DMA_D32_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords)
{
  int return_code ;
	return_code = sis1100w_Vme_Dma_Read(pDevice, vme_adr, 0x39, 4, 0,vme_data, req_num_of_lwords, got_num_of_lwords);
   return return_code ;
}




int vme_A24BLT32FIFO_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords)
{
  int return_code ;
	return_code = sis1100w_Vme_Dma_Read(pDevice, vme_adr, 0x3b, 4, 1,vme_data, req_num_of_lwords, got_num_of_lwords);
   return return_code ;
}


int vme_A24MBLT64FIFO_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords)
{
  int return_code ;
	return_code = sis1100w_Vme_Dma_Read(pDevice, vme_adr, 0x38, 4, 1,vme_data, req_num_of_lwords, got_num_of_lwords);
   return return_code ;
}


int vme_A24DMA_D32FIFO_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords)
{
  int return_code ;
	return_code = sis1100w_Vme_Dma_Read(pDevice, vme_adr, 0x39, 4, 1,vme_data, req_num_of_lwords, got_num_of_lwords);
   return return_code ;
}









 

/* VME A24  Write Cycles */


int vme_A24D8_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int8_t vme_data )
{
  u_int32_t data_32 ;
  int return_code ;
  data_32 =  (u_int32_t) vme_data ;
  return_code = sis1100w_Vme_Single_Write(pDevice, vme_adr, 0x39, 1,  data_32) ;
  return return_code ;
}

int vme_A24D16_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int16_t vme_data )
{
  u_int32_t data_32 ;
  int return_code ;
  data_32 =  (u_int32_t) vme_data ;
  return_code = sis1100w_Vme_Single_Write(pDevice, vme_adr, 0x39, 2,  data_32) ;
  return return_code ;
}


int vme_A24D32_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t vme_data )
{
  int return_code ;
  return_code = sis1100w_Vme_Single_Write(pDevice, vme_adr, 0x39, 4,  vme_data) ;
  return return_code ;
}




int vme_A24BLT32_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* put_num_of_lwords)
{
  int return_code ;
  	return_code = sis1100w_Vme_Dma_Write(pDevice, vme_adr, 0x3b/*am*/, 4 /*size*/, 0,
  								         vme_data, req_num_of_lwords, put_num_of_lwords ) ;
  return return_code ;
}


int vme_A24MBLT64_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* put_num_of_lwords)
{
  int return_code ;
  	return_code = sis1100w_Vme_Dma_Write(pDevice, vme_adr, 0x38 /*am*/, 4 /*size*/, 0,
  								         vme_data, req_num_of_lwords, put_num_of_lwords ) ;
  return return_code ;
}


int vme_A24DMA_D32_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* put_num_of_lwords)
{
  int return_code ;
  	return_code = sis1100w_Vme_Dma_Write(pDevice, vme_adr, 0x39 /*am*/, 4 /*size*/, 0,
  								         vme_data, req_num_of_lwords, put_num_of_lwords ) ;
  return return_code ;
}




int vme_A24BLT32FIFO_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* put_num_of_lwords)
{
  int return_code ;
  	return_code = sis1100w_Vme_Dma_Write(pDevice, vme_adr, 0x3b/*am*/, 4 /*size*/, 1,
  								         vme_data, req_num_of_lwords, put_num_of_lwords ) ;
  return return_code ;
}


int vme_A24MBLT64FIFO_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* put_num_of_lwords)
{
  int return_code ;
  	return_code = sis1100w_Vme_Dma_Write(pDevice, vme_adr, 0x38 /*am*/, 4 /*size*/, 1,
  								         vme_data, req_num_of_lwords, put_num_of_lwords ) ;
  return return_code ;
}


int vme_A24DMA_D32FIFO_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* put_num_of_lwords)
{
  int return_code ;
  	return_code = sis1100w_Vme_Dma_Write(pDevice, vme_adr, 0x39 /*am*/, 4 /*size*/, 1,
  								         vme_data, req_num_of_lwords, put_num_of_lwords ) ;
  return return_code ;
}


















/*****************/
/*               */
/*    VME A32    */
/*               */
/*****************/


/* VME A32  Read Cycles */


int vme_A32D8_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int8_t* vme_data )
{
  unsigned int readdata ;
  int return_code ;
  return_code = sis1100w_Vme_Single_Read(pDevice, vme_adr,0x9, 1,  &readdata) ;
  if (return_code < 0)  return return_code ;
  *vme_data = (u_int8_t) readdata; 
  return return_code ;
}


int vme_A32D16_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int16_t* vme_data )
{
  unsigned int readdata ;
  int return_code ;
  return_code = sis1100w_Vme_Single_Read(pDevice, vme_adr,0x9, 2,  &readdata) ;
  if (return_code < 0)  return return_code ;
  *vme_data = (u_int16_t) readdata; 
  return return_code ;
}


int vme_A32D32_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data )
{
  unsigned int readdata ;
  int return_code ;
  return_code = sis1100w_Vme_Single_Read(pDevice, vme_adr,0x9, 4,  &readdata) ;
  if (return_code < 0)  return return_code ;
  *vme_data = readdata; 
  return return_code ;
}












int vme_A32BLT32_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords)
{
  int return_code ;
	return_code = sis1100w_Vme_Dma_Read(pDevice, vme_adr, 0xb, 4, 0,vme_data, req_num_of_lwords, got_num_of_lwords);
   return return_code ;
}


int vme_A32MBLT64_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords)
{
  int return_code ;
	return_code = sis1100w_Vme_Dma_Read(pDevice, vme_adr, 0x8, 4, 0,vme_data, req_num_of_lwords, got_num_of_lwords);
   return return_code ;
}


int vme_A32_2EVME_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords)
{
  int return_code ;
	return_code = sis1100w_Vme_Dma_Read(pDevice, vme_adr, 0x20, 4, 0,vme_data, req_num_of_lwords, got_num_of_lwords);
   return return_code ;
}


int vme_A32DMA_D32_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords)
{
  int return_code ;
	return_code = sis1100w_Vme_Dma_Read(pDevice, vme_adr, 0x9, 4, 0,vme_data, req_num_of_lwords, got_num_of_lwords);
   return return_code ;
}




int vme_A32BLT32FIFO_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords)
{
  int return_code ;
	return_code = sis1100w_Vme_Dma_Read(pDevice, vme_adr, 0xb, 4, 1,vme_data, req_num_of_lwords, got_num_of_lwords);
   return return_code ;
}


int vme_A32MBLT64FIFO_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords)
{
  int return_code ;
	return_code = sis1100w_Vme_Dma_Read(pDevice, vme_adr, 0x8, 4, 1,vme_data, req_num_of_lwords, got_num_of_lwords);
   return return_code ;
}


int vme_A32_2EVMEFIFO_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords)
{
  int return_code ;
	return_code = sis1100w_Vme_Dma_Read(pDevice, vme_adr, 0x20, 4, 1,vme_data, req_num_of_lwords, got_num_of_lwords);
   return return_code ;
}


int vme_A32DMA_D32FIFO_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords)
{
  int return_code ;
	return_code = sis1100w_Vme_Dma_Read(pDevice, vme_adr, 0x9, 4, 1,vme_data, req_num_of_lwords, got_num_of_lwords);
   return return_code ;
}



















/* VME A32  Write Cycles */


int vme_A32D8_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int8_t vme_data )
{
  u_int32_t data_32 ;
  int return_code ;
  data_32 =  (u_int32_t) vme_data ;
  return_code = sis1100w_Vme_Single_Write(pDevice, vme_adr, 0x9, 1,  data_32) ;
  return return_code ;
}

int vme_A32D16_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int16_t vme_data )
{
  u_int32_t data_32 ;
  int return_code ;
  data_32 =  (u_int32_t) vme_data ;
  return_code = sis1100w_Vme_Single_Write(pDevice, vme_adr, 0x9, 2,  data_32) ;
  return return_code ;
}


int vme_A32D32_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t vme_data )
{
  int return_code ;
  return_code = sis1100w_Vme_Single_Write(pDevice, vme_adr, 0x9, 4,  vme_data) ;
  return return_code ;
}



 


int vme_A32BLT32_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* put_num_of_lwords)
{
  int return_code ;
  	return_code = sis1100w_Vme_Dma_Write(pDevice, vme_adr, 0xb/*am*/, 4 /*size*/, 0,
  								         vme_data, req_num_of_lwords, put_num_of_lwords ) ;
  return return_code ;
}


int vme_A32MBLT64_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* put_num_of_lwords)
{
  int return_code ;
  	return_code = sis1100w_Vme_Dma_Write(pDevice, vme_adr, 0x8 /*am*/, 4 /*size*/, 0,
  								         vme_data, req_num_of_lwords, put_num_of_lwords ) ;
  return return_code ;
}


int vme_A32DMA_D32_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* put_num_of_lwords)
{
  int return_code ;
  	return_code = sis1100w_Vme_Dma_Write(pDevice, vme_adr, 0x9 /*am*/, 4 /*size*/, 0,
  								         vme_data, req_num_of_lwords, put_num_of_lwords ) ;
  return return_code ;
}




int vme_A32BLT32FIFO_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* put_num_of_lwords)
{
  int return_code ;
  	return_code = sis1100w_Vme_Dma_Write(pDevice, vme_adr, 0xb/*am*/, 4 /*size*/, 1,
  								         vme_data, req_num_of_lwords, put_num_of_lwords ) ;
  return return_code ;
}


int vme_A32MBLT64FIFO_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* put_num_of_lwords)
{
  int return_code ;
  	return_code = sis1100w_Vme_Dma_Write(pDevice, vme_adr, 0x8 /*am*/, 4 /*size*/, 1,
  								         vme_data, req_num_of_lwords, put_num_of_lwords ) ;
  return return_code ;
}


int vme_A32DMA_D32FIFO_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* put_num_of_lwords)
{
  int return_code ;
  	return_code = sis1100w_Vme_Dma_Write(pDevice, vme_adr, 0x9 /*am*/, 4 /*size*/, 1,
  								         vme_data, req_num_of_lwords, put_num_of_lwords ) ;
  return return_code ;
}










/***********************/
/*                     */
/*    s1100_control    */
/*                     */
/***********************/


int s1100_control_read(struct SIS1100_Device_Struct* p, int offset, u_int32_t* data)
{
int error ;
	error = sis1100w_sis1100_Control_Read(p, offset, data) ;
  	return error ;
}



int s1100_control_write(struct SIS1100_Device_Struct* p, int offset, u_int32_t data)
{
int error ;
	error = sis1100w_sis1100_Control_Write(p, offset, data) ;
  	return error ;
}

/***********************/
/*                     */
/*    s3100_control    */
/*                     */
/***********************/


int s3100_control_read(struct SIS1100_Device_Struct* p, int offset, u_int32_t* data)
{
int error ;
	error = sis1100w_sis3100_Control_Read(p, offset, data) ;
  	return error ;
}



int s3100_control_write(struct SIS1100_Device_Struct* p, int offset, u_int32_t data)
{
int error ;
	error = sis1100w_sis3100_Control_Write(p, offset, data) ;
  	return error ;
}






/***********************/
/*                     */
/*    s3100_sharc      */
/*                     */
/***********************/



int s3100_sharc_write(struct SIS1100_Device_Struct* p_sharc_desc, u_int32_t byte_adr, u_int32_t* ptr_data,  u_int32_t num_of_lwords )
{
int return_code = 0;
unsigned int put_num_of_lwords ;
unsigned int i,j ;
	if (num_of_lwords <= 0x0) return 0 ;

  	if (num_of_lwords <= 0x4) {
  		j = 0;
		for (i=0;i<num_of_lwords;i++) {
  		return_code =  sis1100w_SdramSharc_Single_Write(p_sharc_desc, byte_adr+j, ptr_data[i])  ; 
    	j=j+4;
		if (return_code != 0) return -1 ;
  		}
  		return j ;
	  }	
	 else {
		return_code =  sis1100w_SdramSharc_Dma_Write(p_sharc_desc, byte_adr, ptr_data, num_of_lwords, &put_num_of_lwords)  ;
		if (return_code != 0) return -1 ;
	  	return (int) (put_num_of_lwords << 2);
	  }	
		
  return 0 ;
}




int s3100_sharc_read(struct SIS1100_Device_Struct* p_sharc_desc, u_int32_t byte_adr, u_int32_t* ptr_data, u_int32_t num_of_lwords )
{
int return_code ;
unsigned int got_num_of_lwords ;
unsigned int i,j ;
	if (num_of_lwords <= 0x0) return 0 ;
  	if (num_of_lwords <= 0x4) {
  		j = 0;
  		for (i=0;i<num_of_lwords;i++) {
    		return_code =  sis1100w_SdramSharc_Single_Read(p_sharc_desc, byte_adr+j, &ptr_data[i])  ; 
    		j=j+4;
			if (return_code != 0) return -1 ;
  	 	}
	  	return j ;
	  }	
	 else {
		return_code =  sis1100w_SdramSharc_Dma_Read(p_sharc_desc, byte_adr, ptr_data, num_of_lwords, &got_num_of_lwords)  ;
		if (return_code != 0) return -1 ;
	  	return (int) (got_num_of_lwords << 2);
	  }	
	 
//  return return_code ;
  return 0 ;
}





/***********************/
/*                     */
/*    s3100_sdram      */
/*                     */
/***********************/


int s3100_sdram_write(struct SIS1100_Device_Struct* p_sdram_desc, u_int32_t byte_adr, u_int32_t* ptr_data,  u_int32_t num_of_lwords )
{
int return_code = 0;
unsigned int put_num_of_lwords ;
unsigned int i,j ;
	if (num_of_lwords <= 0x0) return 0 ;

  	if (num_of_lwords <= 0x4) {
  		j = 0;
		for (i=0;i<num_of_lwords;i++) {
  		return_code =  sis1100w_SdramSharc_Single_Write(p_sdram_desc, byte_adr+j, ptr_data[i])  ; 
    	j=j+4;
		if (return_code != 0) return -1 ;
  		}
  		return j ;
	  }	
	 else {
		return_code =  sis1100w_SdramSharc_Dma_Write(p_sdram_desc, byte_adr, ptr_data, num_of_lwords, &put_num_of_lwords)  ;
		if (return_code != 0) return -1 ;
	  	return (int) (put_num_of_lwords << 2);
	  }	
		
  return 0 ;
}




int s3100_sdram_read(struct SIS1100_Device_Struct* p_sdram_desc, u_int32_t byte_adr, u_int32_t* ptr_data,  u_int32_t num_of_lwords )
{
int return_code ;
unsigned int got_num_of_lwords ;
unsigned int i,j ;
	if (num_of_lwords <= 0x0) return 0 ;
  	if (num_of_lwords <= 0x4) {
  		j = 0;
  		for (i=0;i<num_of_lwords;i++) {
    		return_code =  sis1100w_SdramSharc_Single_Read(p_sdram_desc, byte_adr+j, &ptr_data[i])  ; 
    		j=j+4;
			if (return_code != 0) return -1 ;
  	 	}
	  	return j ;
	  }	
	 else {
		return_code =  sis1100w_SdramSharc_Dma_Read(p_sdram_desc, byte_adr, ptr_data, num_of_lwords, &got_num_of_lwords)  ;
		if (return_code != 0) return -1 ;
	  	return (int) (got_num_of_lwords << 2);
	  }	
	 
//  return return_code ;
  return 0 ;
}







		  






















