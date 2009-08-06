
/***************************************************************************/
/*                                                                         */
/*  Filename: sis3100_vme_calls.c                                          */
/*                                                                         */
/*  Funktion:                                                              */
/*                                                                         */
/*  Autor:                TH                                               */
/*  date:                 10.05.2002                                       */
/*  last modification:    11.11.2008                                       */
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
/*  © 2008                                                                 */
/*                                                                         */
/***************************************************************************/

/*---------------------------------------------------------------------------*/
/* Include files                                                             */
/*---------------------------------------------------------------------------*/

#ifndef EXCLUDE_VME // let applications exlucde all VME code

#include <stdio.h>
#include <stdlib.h>
//#include "PlxApi.h"
#include "sis1100w.h" // Header sis1100w.dll

typedef unsigned char           u_int8_t;
typedef unsigned short          u_int16_t;
typedef unsigned long           u_int32_t;

/**********************/
/*                    */
/*    VME SYSReset    */
/*                    */
/**********************/
int vmesysreset(
	struct SIS1100_Device_Struct* pDevice
	)
{
	sis1100w_VmeSysreset(pDevice);
	return 0;
}

int vme_IACK_D8_read(
	struct SIS1100_Device_Struct *pDevice,
	u_int32_t vme_irq_level,
	u_int8_t *vme_data
	)
{
	int return_code;
	unsigned int data;

	return_code = sis1100w_Vme_Single_Read(pDevice, (vme_irq_level << 1) + 1, 0x4000, 1, &data);
	*vme_data = (u_int8_t)data;
	return return_code;
}

/*****************/
/*               */
/*    VME A16    */
/*               */
/*****************/

/* VME A16  Read Cycles */

int vme_A16D8_read(
	struct SIS1100_Device_Struct* pDevice, 
	u_int32_t vme_adr,
	u_int8_t* vme_data
	)
{
	unsigned int readdata;
	int return_code;
	return_code = sis1100w_Vme_Single_Read(pDevice, vme_adr, 0x29, 1, &readdata);
	if(return_code < 0)
		return return_code;
	*vme_data = (u_int8_t)readdata; 
	return return_code;
}


int vme_A16D16_read(
	struct SIS1100_Device_Struct* pDevice,
	u_int32_t vme_adr,
	u_int16_t* vme_data
	)
{
	unsigned int readdata;
	int return_code;
	return_code = sis1100w_Vme_Single_Read(pDevice, vme_adr, 0x29, 2, &readdata);
	if(return_code < 0)
		return return_code;
	*vme_data = (u_int16_t)readdata; 
	return return_code;
}


/* VME A16  Write Cycles */

int vme_A16D8_write(
	struct SIS1100_Device_Struct* pDevice,
	u_int32_t vme_adr,
	u_int8_t vme_data
	)
{
	u_int32_t data_32;
	int return_code;
	data_32 = (u_int32_t)vme_data;
	return_code = sis1100w_Vme_Single_Write(pDevice, vme_adr, 0x29, 1, data_32);
	return return_code;
}


int vme_A16D16_write(
	struct SIS1100_Device_Struct* pDevice,
	u_int32_t vme_adr,
	u_int16_t vme_data
	)
{
	u_int32_t data_32;
	int return_code;
	data_32 = (u_int32_t)vme_data;
	return_code = sis1100w_Vme_Single_Write(pDevice, vme_adr, 0x29, 2, data_32);
	return return_code;
}


/*****************/
/*               */
/*    VME A24    */
/*               */
/*****************/

/* VME A24  Read Cycles */

int vme_A24D8_read(
	struct SIS1100_Device_Struct* pDevice,
	u_int32_t vme_adr,
	u_int8_t* vme_data
	)
{
	unsigned int readdata;
	int return_code;
	return_code = sis1100w_Vme_Single_Read(pDevice, vme_adr, 0x39, 1, &readdata);
	if(return_code < 0)
		return return_code;
	*vme_data = (u_int8_t)readdata; 
	return return_code;
}


int vme_A24D16_read(
	struct SIS1100_Device_Struct* pDevice,
	u_int32_t vme_adr,
	u_int16_t* vme_data
	)
{
	unsigned int readdata;
	int return_code;
	return_code = sis1100w_Vme_Single_Read(pDevice, vme_adr, 0x39, 2, &readdata);
	if(return_code < 0)
		return return_code;
	*vme_data = (u_int16_t)readdata; 
	return return_code;
}



#ifdef not_tested
int vme_A24BLT16_read(
	struct SIS1100_Device_Struct* pDevice,
	u_int32_t vme_adr,
	u_int16_t* vme_data,
    u_int32_t req_num_of_lwords,
	u_int32_t* got_num_of_lwords
	)
{
	u_int32_t *buf;
	int return_code;
	unsigned int i;

	buf = (u_int32_t*)malloc(req_num_of_lwords * sizeof(u_int32_t));
	if(buf == NULL){
		free(buf);
		return -1;
	}

	return_code = sis1100w_Vme_Dma_Read(pDevice, vme_adr, 0x3b, 2, 0, buf, req_num_of_lwords, got_num_of_lwords);
	if(return_code)
		return return_code;
	
	for(i = 0;i < req_num_of_lwords;i++){
		vme_data[i] = (u_int16_t)buf[i];
	}
	free(buf);
	return return_code;
}
#endif

int vme_A24DMA_D16_read(
	struct SIS1100_Device_Struct* pDevice,
	u_int32_t vme_adr,
	u_int16_t* vme_data,
	u_int32_t req_num_of_words,
	u_int32_t* got_num_of_words
	)
{
	u_int32_t *buf;
	int return_code;
	unsigned int i;

	buf = (u_int32_t*)malloc(req_num_of_words * sizeof(u_int32_t));
	if(buf == NULL)
		return -1;

	return_code = sis1100w_Vme_Dma_Read(pDevice, vme_adr, 0x39, 2, 0, buf, req_num_of_words, got_num_of_words);
	if(return_code){
		free(buf);
		return return_code;
	}

	for(i = 0;i < req_num_of_words;i++){
		vme_data[i] = (u_int16_t)buf[i];
	}
	free(buf);
	return return_code;
}


int vme_A24DMA_D16FIFO_read(
	struct SIS1100_Device_Struct* pDevice,
	u_int32_t vme_adr,
	u_int16_t* vme_data,
	u_int32_t req_num_of_words,
	u_int32_t* got_num_of_words
	)
{
	u_int32_t *buf;
	int return_code;
	unsigned int i;

	buf = (u_int32_t*)malloc(req_num_of_words * sizeof(u_int32_t));
	if(buf == NULL)
		return -1;

	return_code = sis1100w_Vme_Dma_Read(pDevice, vme_adr, 0x39, 2, 1, buf, req_num_of_words, got_num_of_words);
	if(return_code){
		free(buf);	
		return return_code;
	}

	for(i = 0;i < req_num_of_words;i++){
		vme_data[i] = (u_int16_t)buf[i];
	}
	free(buf);
	return return_code;
}




/* VME A24  Write Cycles */

int vme_A24D8_write(
	struct SIS1100_Device_Struct* pDevice,
	u_int32_t vme_adr,
	u_int8_t vme_data
	)
{
	u_int32_t data_32;
	int return_code;
	data_32 = (u_int32_t)vme_data;
	return_code = sis1100w_Vme_Single_Write(pDevice, vme_adr, 0x39, 1, data_32);
	return return_code;
}


int vme_A24D16_write(
	struct SIS1100_Device_Struct* pDevice,
	u_int32_t vme_adr,
	u_int16_t vme_data)
{
	u_int32_t data_32;
	int return_code;
	data_32 = (u_int32_t)vme_data;
	return_code = sis1100w_Vme_Single_Write(pDevice, vme_adr, 0x39, 2, data_32);
	return return_code;
}


#ifdef not_tested
int vme_A24BLT16_write(
	struct SIS1100_Device_Struct* pDevice,
	u_int32_t vme_adr,
	u_int16_t* vme_data,
    u_int32_t req_num_of_lwords,
	u_int32_t* got_num_of_lwords
	)
{
	u_int32_t *buf;
	int return_code;
	unsigned int i;

	buf = (u_int32_t*)malloc(req_num_of_lwords * sizeof(u_int32_t));
	if(buf == NULL)
		return -1;

	for(i = 0;i < req_num_of_lwords;i++){
		buf[i] = (u_int32_t)vme_data[i];
	}

	return_code = sis1100w_Vme_Dma_Write(pDevice, vme_adr, 0x3b, 2, 0, buf, req_num_of_lwords, got_num_of_lwords);

	free(buf);
	return return_code;
}
#endif



int vme_A24DMA_D16_write(
	struct SIS1100_Device_Struct* pDevice,
	u_int32_t vme_adr,
	u_int16_t* vme_data,
    u_int32_t req_num_of_words,
	u_int32_t* got_num_of_words
	)
{
	u_int32_t *buf;
	int return_code;
	unsigned int i;

	buf = (u_int32_t*)malloc(req_num_of_words * sizeof(u_int32_t));
	if(buf == NULL)
		return -1;

	for(i = 0;i < req_num_of_words;i++){
		buf[i] = (u_int32_t)vme_data[i];
	}

	return_code = sis1100w_Vme_Dma_Write(pDevice, vme_adr, 0x39, 2, 0, buf, req_num_of_words, got_num_of_words);

	free(buf);
	return return_code;
}

int vme_A24DMA_D16FIFO_write(
	struct SIS1100_Device_Struct* pDevice,
	u_int32_t vme_adr,
	u_int16_t* vme_data,
    u_int32_t req_num_of_words,
	u_int32_t* got_num_of_words
	)
{

	u_int32_t *buf;
	int return_code;
	unsigned int i;

	buf = (u_int32_t*)malloc(req_num_of_words * sizeof(u_int32_t));
	if(buf == NULL)
		return -1;

	for(i = 0;i < req_num_of_words;i++){
		buf[i] = (u_int32_t)vme_data[i];
	}

	return_code = sis1100w_Vme_Dma_Write(pDevice, vme_adr, 0x39, 2, 1, buf, req_num_of_words, got_num_of_words);

	free(buf);
	return return_code;
}








/*****************/
/*               */
/*    VME A32    */
/*               */
/*****************/

/* VME A32  Read Cycles */

int vme_A32D8_read(
	struct SIS1100_Device_Struct* pDevice,
	u_int32_t vme_adr,
	u_int8_t* vme_data
	)
{
	unsigned int readdata;
	int return_code;
	return_code = sis1100w_Vme_Single_Read(pDevice, vme_adr, 0x9, 1, &readdata);
	if(return_code < 0)
		return return_code;
	*vme_data = (u_int8_t)readdata; 
	return return_code;
}

int vme_A32D16_read(
	struct SIS1100_Device_Struct* pDevice,
	u_int32_t vme_adr,
	u_int16_t* vme_data
	)
{
	unsigned int readdata;
	int return_code;
	return_code = sis1100w_Vme_Single_Read(pDevice, vme_adr, 0x9, 2, &readdata);
	if(return_code < 0)
		return return_code;
	*vme_data = (u_int16_t)readdata; 
	return return_code;
}



#ifdef not_tested
int vme_A32BLT16_read(
	struct SIS1100_Device_Struct* pDevice,
	u_int32_t vme_adr,
	u_int16_t* vme_data,
    u_int32_t req_num_of_lwords,
	u_int32_t* got_num_of_lwords
	)
{
	u_int32_t *buf;
	int return_code;
	unsigned int i;

	buf = (u_int32_t*)malloc(req_num_of_lwords * sizeof(u_int32_t));
	if(buf == NULL){
		free(buf);
		return -1;
	}

	return_code = sis1100w_Vme_Dma_Read(pDevice, vme_adr, 0xb, 2, 0, buf, req_num_of_lwords, got_num_of_lwords);
	if(return_code)
		return return_code;
	
	for(i = 0;i < req_num_of_lwords;i++){
		vme_data[i] = (u_int16_t)buf[i];
	}
	*got_num_of_lwords >>= 1;
	free(buf);
	return return_code;
}
#endif



int vme_A32DMA_D16_read(
	struct SIS1100_Device_Struct* pDevice,
	u_int32_t vme_adr,
	u_int16_t* vme_data,
	u_int32_t req_num_of_words,
	u_int32_t* got_num_of_words
	)
{
	u_int32_t *buf;
	int return_code;
	unsigned int i;

	buf = (u_int32_t*)malloc(req_num_of_words * sizeof(u_int32_t));
	if(buf == NULL)
		return -1;

	return_code = sis1100w_Vme_Dma_Read(pDevice, vme_adr, 0x9, 2, 0, buf, req_num_of_words, got_num_of_words);
	if(return_code){
		free(buf);
		return return_code;
	}

	for(i = 0;i < req_num_of_words;i++){
		vme_data[i] = (u_int16_t)(buf[i]&0xffff);
	}
	free(buf);
	return return_code;
}


int vme_A32DMA_D16FIFO_read(
	struct SIS1100_Device_Struct* pDevice,
	u_int32_t vme_adr,
	u_int16_t* vme_data,
	u_int32_t req_num_of_words,
	u_int32_t* got_num_of_words
	)
{
	u_int32_t *buf;
	int return_code;
	unsigned int i;

	buf = (u_int32_t*)malloc(req_num_of_words * sizeof(u_int32_t));
	if(buf == NULL)
		return -1;

	return_code = sis1100w_Vme_Dma_Read(pDevice, vme_adr, 0x9, 2, 1, buf, req_num_of_words, got_num_of_words);
	if(return_code){
		free(buf);	
		return return_code;
	}

	for(i = 0;i < req_num_of_words;i++){
		vme_data[i] = (u_int16_t)buf[i];
	}
	free(buf);
	return return_code;
}


/* VME A32  Write Cycles */

int vme_A32D8_write(
	struct SIS1100_Device_Struct* pDevice,
	u_int32_t vme_adr,
	u_int8_t vme_data 
	)
{
	u_int32_t data_32;
	int return_code;
	data_32 = (u_int32_t)vme_data;
	return_code = sis1100w_Vme_Single_Write(pDevice, vme_adr, 0x9, 1, data_32);
	return return_code;
}


int vme_A32D16_write(
	struct SIS1100_Device_Struct* pDevice,
	u_int32_t vme_adr,
	u_int16_t vme_data
	)
{
	u_int32_t data_32;
	int return_code;
	data_32 = (u_int32_t)vme_data;
	return_code = sis1100w_Vme_Single_Write(pDevice, vme_adr, 0x9, 2, data_32);
	return return_code;
}


#ifdef not_tested
int vme_A32BLT16_write(
	struct SIS1100_Device_Struct* pDevice,
	u_int32_t vme_adr,
	u_int16_t* vme_data,
    u_int32_t req_num_of_words,
	u_int32_t* got_num_of_words
	)
{
	u_int32_t *buf;
	int return_code;
	unsigned int i;
	unsigned int req_num_of_lwords;
	//unsigned int got_num_of_lwords;

	req_num_of_lwords = 2*req_num_of_words;
	buf = (u_int32_t*)malloc(req_num_of_lwords * sizeof(u_int32_t));
	if(buf == NULL)
		return -1;

	for(i = 0;i < req_num_of_lwords;i++){
		buf[i] = (u_int32_t)vme_data[i];
	}

	return_code = sis1100w_Vme_Dma_Write(pDevice, vme_adr, 0xb, 2, 0, buf, req_num_of_lwords, &got_num_of_words);
	
	free(buf);
	return return_code;
}
#endif


int vme_A32DMA_D16_write(
	struct SIS1100_Device_Struct* pDevice,
	u_int32_t vme_adr,
	u_int16_t* vme_data,
    u_int32_t req_num_of_words,
	u_int32_t* got_num_of_words
	)
{
	u_int32_t *buf;
	int return_code;
	unsigned int i;

	buf = (u_int32_t*)malloc(req_num_of_words * sizeof(u_int32_t));
	if(buf == NULL)
		return -1;

	for(i = 0;i < req_num_of_words;i++){
		buf[i] = (u_int32_t)vme_data[i];
	}

	return_code = sis1100w_Vme_Dma_Write(pDevice, vme_adr, 0x9, 2, 0, buf, req_num_of_words, got_num_of_words);

	free(buf);
	return return_code;
}

int vme_A32DMA_D16FIFO_write(
	struct SIS1100_Device_Struct* pDevice,
	u_int32_t vme_adr,
	u_int16_t* vme_data,
    u_int32_t req_num_of_words,
	u_int32_t* got_num_of_words
	)
{

	u_int32_t *buf;
	int return_code;
	unsigned int i;

	buf = (u_int32_t*)malloc(req_num_of_words * sizeof(u_int32_t));
	if(buf == NULL)
		return -1;

	for(i = 0;i < req_num_of_words;i++){
		buf[i] = (u_int32_t)vme_data[i];
	}

	return_code = sis1100w_Vme_Dma_Write(pDevice, vme_adr, 0x9, 2, 1, buf, req_num_of_words, got_num_of_words);

	free(buf);
	return return_code;
}





/***********************/
/*                     */
/*    s3100_sharc      */
/*                     */
/***********************/

int s3100_sharc_write(
	struct SIS1100_Device_Struct* p_sharc_desc,
	u_int32_t byte_adr,
	u_int32_t* ptr_data,
	u_int32_t num_of_lwords
	)
{
	int return_code = 0;
	unsigned int put_num_of_lwords;
	unsigned int i,j;

	if(num_of_lwords <= 0x0)
		return 0;

	if(num_of_lwords <= 0x4){
  		j = 0;
		for(i = 0;i < num_of_lwords;i++){
  			return_code = sis1100w_SdramSharc_Single_Write(p_sharc_desc, byte_adr + j, ptr_data[i]); 
    		j = j + 4;
			if(return_code != 0)
				return -1;
  		}
  		return j;
	}else{
		return_code = sis1100w_SdramSharc_Dma_Write(p_sharc_desc, byte_adr, ptr_data, num_of_lwords, &put_num_of_lwords);
		if(return_code != 0)
			return -1;
		return (int) (put_num_of_lwords << 2);
	}	
	return 0;
}

int s3100_sharc_read(
	struct SIS1100_Device_Struct* p_sharc_desc,
	u_int32_t byte_adr,
	u_int32_t* ptr_data,
	u_int32_t num_of_lwords
	)
{
	int return_code;
	unsigned int got_num_of_lwords;
	unsigned int i,j;

	if(num_of_lwords <= 0x0)
		return 0;

	if(num_of_lwords <= 0x4){
  		j = 0;
  		for(i = 0;i < num_of_lwords;i++){
    		return_code = sis1100w_SdramSharc_Single_Read(p_sharc_desc, byte_adr + j, &ptr_data[i]); 
    		j = j + 4;
			if(return_code != 0)
				return -1;
  	 	}
	  	return j;
	}else{
		return_code = sis1100w_SdramSharc_Dma_Read(p_sharc_desc, byte_adr, ptr_data, num_of_lwords, &got_num_of_lwords);
		if(return_code != 0)
			return -1;
		return (int) (got_num_of_lwords << 2);
	}
	return 0;
}

/***********************/
/*                     */
/*    s3100_sdram      */
/*                     */
/***********************/

int s3100_sdram_write(
	struct SIS1100_Device_Struct* p_sdram_desc,
	u_int32_t byte_adr,
	u_int32_t* ptr_data,
	u_int32_t num_of_lwords
	)
{
	int return_code = 0;
	unsigned int put_num_of_lwords;
	unsigned int i,j;
	
	if(num_of_lwords <= 0x0)
		return 0;

  	if(num_of_lwords <= 0x4){
  		j = 0;
		for(i = 0;i < num_of_lwords;i++){
  			return_code = sis1100w_SdramSharc_Single_Write(p_sdram_desc, byte_adr + j, ptr_data[i]); 
    		j = j + 4;
			if(return_code != 0)
				return -1;
  		}
  		return j;
	}else{
		return_code = sis1100w_SdramSharc_Dma_Write(p_sdram_desc, byte_adr, ptr_data, num_of_lwords, &put_num_of_lwords);
		if(return_code != 0)
			return -1;
		return (int) (put_num_of_lwords << 2);
	}	
	return 0;
}

int s3100_sdram_read(
	struct SIS1100_Device_Struct* p_sdram_desc,
	u_int32_t byte_adr,
	u_int32_t* ptr_data,
	u_int32_t num_of_lwords 
	)
{
	int return_code;
	unsigned int got_num_of_lwords;
	unsigned int i,j;

	if(num_of_lwords <= 0)
		return 0;
  	if(num_of_lwords <= 0x4){
  		j = 0;
  		for(i = 0;i < num_of_lwords;i++){
    		return_code = sis1100w_SdramSharc_Single_Read(p_sdram_desc, byte_adr + j, &ptr_data[i]); 
    		j = j + 4;
			if(return_code != 0)
				return -1;
  	 	}
	  	return j;
	}else{
		return_code =  sis1100w_SdramSharc_Dma_Read(p_sdram_desc, byte_adr, ptr_data, num_of_lwords, &got_num_of_lwords);
		if(return_code != 0)
			return -1;
		return (int) (got_num_of_lwords << 2);
	}
	return 0;
}

#endif // EXCLUDE_VME