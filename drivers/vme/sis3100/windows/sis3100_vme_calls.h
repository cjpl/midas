/***************************************************************************/
/*                                                                         */
/*  Filename: sis3100_vme_calls.h                                          */
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

typedef unsigned char           u_int8_t;
typedef unsigned short          u_int16_t;
typedef unsigned long           u_int32_t;

/**********************/
/*                    */
/*    VME SYSReset    */
/*                    */
/**********************/

int vmesysreset(struct SIS1100_Device_Struct* sis1100_pDevice);

int vme_IACK_D8_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_irq_level, u_int8_t* vme_data);

/*****************/
/*               */
/*    VME A16    */
/*               */
/*****************/

int vme_A16D8_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int8_t* vme_data);
int vme_A16D16_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int16_t* vme_data);
#define vme_A16D32_read(a, b, c) sis1100w_Vme_Single_Read(a, b, 0x29, 4, c)

int vme_A16D8_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int8_t vme_data);
int vme_A16D16_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int16_t vme_data);
#define vme_A16D32_write(a, b, c) sis1100w_Vme_Single_Write(a, b, 0x29, 4, c)

/*****************/
/*               */
/*    VME A24    */
/*               */
/*****************/

int vme_A24D8_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int8_t* vme_data);
int vme_A24D16_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int16_t* vme_data);
#define vme_A24D32_read(a, b, c) sis1100w_Vme_Single_Read(a, b, 0x39, 4, c)

//int vme_A24BLT16_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int16_t* vme_data, u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords);

#define vme_A24BLT32_read(a, b, c, d, e) sis1100w_Vme_Dma_Read(a, b, 0x3b, 4, 0, c, d, e)
#define vme_A24MBLT64_read(a, b, c, d, e) sis1100w_Vme_Dma_Read(a, b, 0x38, 4, 0, c, d, e)

int vme_A24DMA_D16_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int16_t* vme_data, u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords);
#define vme_A24DMA_D32_read(a, b, c, d, e) sis1100w_Vme_Dma_Read(a, b, 0x39, 4, 0, c, d, e)


#define vme_A24BLT32FIFO_read(a, b, c, d, e) sis1100w_Vme_Dma_Read(a, b, 0x3b, 4, 1, c, d, e)
#define vme_A24MBLT64FIFO_read(a, b, c, d, e) sis1100w_Vme_Dma_Read(a, b, 0x38, 4, 1, c, d, e)

int vme_A24DMA_D16FIFO_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int16_t* vme_data, u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords);
#define vme_A24DMA_D32FIFO_read(a, b, c, d, e) sis1100w_Vme_Dma_Read(a, b, 0x39, 4, 1, c, d, e)



int vme_A24D8_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int8_t vme_data);
int vme_A24D16_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int16_t vme_data);
#define vme_A24D32_write(a, b, c) sis1100w_Vme_Single_Write(a, b, 0x39, 4, c)

//int vme_A24BLT16_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int16_t* vme_data, u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords);

#define vme_A24BLT32_write(a, b, c, d, e) sis1100w_Vme_Dma_Write(a, b, 0x3b, 4, 0, c, d, e)
#define vme_A24MBLT64_write(a, b, c, d, e) sis1100w_Vme_Dma_Write(a, b, 0x38, 4, 0, c, d, e)

int vme_A24DMA_D16_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int16_t* vme_data, u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords);
#define vme_A24DMA_D32_write(a, b, c, d, e) sis1100w_Vme_Dma_Write(a, b, 0x39, 4, 0, c, d, e)


#define vme_A24BLT32FIFO_write(a, b, c, d, e) sis1100w_Vme_Dma_Write(a, b, 0x3b, 4, 1, c, d, e)
#define vme_A24MBLT64FIFO_write(a, b, c, d, e) sis1100w_Vme_Dma_Write(a, b, 0x38, 4, 1, c, d, e)
int vme_A24DMA_D16FIFO_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int16_t* vme_data, u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords);
#define vme_A24DMA_D32FIFO_write(a, b, c, d, e) sis1100w_Vme_Dma_Write(a, b, 0x39, 4, 1, c, d, e)

/*****************/
/*               */
/*    VME A32    */
/*               */
/*****************/

int vme_A32D8_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int8_t* vme_data);
int vme_A32D16_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int16_t* vme_data);
#define vme_A32D32_read(a, b, c) sis1100w_Vme_Single_Read(a, b, 0x9, 4, c)

//int vme_A32BLT16_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int16_t* vme_data, u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords);
#define vme_A32BLT32_read(a, b, c, d, e) sis1100w_Vme_Dma_Read(a, b, 0xb, 4, 0, c, d, e)
#define vme_A32MBLT64_read(a, b, c, d, e) sis1100w_Vme_Dma_Read(a, b, 0x8, 4, 0, c, d, e)

#define vme_A32_2EVME_read(a, b, c, d, e) sis1100w_Vme_Dma_Read(a, b, 0x20, 4, 0, c, d, e)

#define vme_A32_2ESST160_read(a, b, c, d, e) sis1100w_Vme_Dma_Read(a, b, 0x060, 4, 0, c, d, e)
#define vme_A32_2ESST267_read(a, b, c, d, e) sis1100w_Vme_Dma_Read(a, b, 0x160, 4, 0, c, d, e)
#define vme_A32_2ESST320_read(a, b, c, d, e) sis1100w_Vme_Dma_Read(a, b, 0x260, 4, 0, c, d, e)

int vme_A32DMA_D16_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int16_t* vme_data, u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords);
#define vme_A32DMA_D32_read(a, b, c, d, e) sis1100w_Vme_Dma_Read(a, b, 0x9, 4, 0, c, d, e)


#define vme_A32BLT32FIFO_read(a, b, c, d, e) sis1100w_Vme_Dma_Read(a, b, 0xb, 4, 1, c, d, e)

#define vme_A32MBLT64FIFO_read(a, b, c, d, e) sis1100w_Vme_Dma_Read(a, b, 0x8, 4, 1, c, d, e)

#define vme_A32_2EVMEFIFO_read(a, b, c, d, e) sis1100w_Vme_Dma_Read(a, b, 0x20, 4, 1, c, d, e)

#define vme_A32_2ESST160FIFO_read(a, b, c, d, e) sis1100w_Vme_Dma_Read(a, b, 0x060, 4, 1, c, d, e)
#define vme_A32_2ESST267FIFO_read(a, b, c, d, e) sis1100w_Vme_Dma_Read(a, b, 0x160, 4, 1, c, d, e)
#define vme_A32_2ESST320FIFO_read(a, b, c, d, e) sis1100w_Vme_Dma_Read(a, b, 0x260, 4, 1, c, d, e)

int vme_A32DMA_D16FIFO_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int16_t* vme_data, u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords);

#define vme_A32DMA_D32FIFO_read(a, b, c, d, e) sis1100w_Vme_Dma_Read(a, b, 0x9, 4, 1, c, d, e)




int vme_A32D8_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int8_t vme_data);
int vme_A32D16_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int16_t vme_data);

#define vme_A32D32_write(a, b, c) sis1100w_Vme_Single_Write(a, b, 0x9, 4, c)

//int vme_A32BLT16_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int16_t *vme_data, u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords);

#define vme_A32BLT32_write(a, b, c, d, e) sis1100w_Vme_Dma_Write(a, b, 0xb, 4, 0, c, d, e)
#define vme_A32MBLT64_write(a, b, c, d, e) sis1100w_Vme_Dma_Write(a, b, 0x8, 4, 0, c, d, e)

#define vme_A32_2EVME_write(a, b, c, d, e) sis1100w_Vme_Dma_Write(a, b, 0x20, 4, 0, c, d, e)

int vme_A32DMA_D16_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int16_t *vme_data, u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords);
#define vme_A32DMA_D32_write(a, b, c, d, e) sis1100w_Vme_Dma_Write(a, b, 0x9, 4, 0, c, d, e)


#define vme_A32BLT32FIFO_write(a, b, c, d, e) sis1100w_Vme_Dma_Write(a, b, 0xb, 4, 1,  c, d, e)
#define vme_A32MBLT64FIFO_write(a, b, c, d, e) sis1100w_Vme_Dma_Write(a, b, 0x8, 4, 1, c, d, e)
#define vme_A32_2EVMEFIFO_write(a, b, c, d, e) sis1100w_Vme_Dma_Write(a, b, 0x20, 4, 1, c, d, e)
int vme_A32DMA_D16FIFO_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int16_t *vme_data, u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords);
#define vme_A32DMA_D32FIFO_write(a, b, c, d, e) sis1100w_Vme_Dma_Write(a, b, 0x9, 4, 1, c, d, e)


/***********************/
/*                     */
/*    s1100_control    */
/*                     */
/***********************/

#define s1100_control_read(a, b, c) sis1100w_sis1100_Control_Read(a, b, c)
#define s1100_control_write(a, b, c) sis1100w_sis1100_Control_Write(a, b, c)

/***********************/
/*                     */
/*    s3100_control    */
/*                     */
/***********************/

#define s3100_control_read(a, b, c) sis1100w_sis3100_Control_Read(a, b, c)
#define s3100_control_write(a, b, c) sis1100w_sis3100_Control_Write(a, b, c)

/***********************/
/*                     */
/*    s3100_sharc      */
/*                     */
/***********************/

int s3100_sharc_write(struct SIS1100_Device_Struct* pDevice_sharc_desc, u_int32_t byte_adr, u_int32_t* ptr_data, u_int32_t num_of_lwords);
int s3100_sharc_read(struct SIS1100_Device_Struct* pDevice_sharc_desc, u_int32_t byte_adr, u_int32_t* ptr_data, u_int32_t num_of_lwords);

/***********************/
/*                     */
/*    s3100_sdram      */
/*                     */
/***********************/

int s3100_sdram_write(struct SIS1100_Device_Struct* pDevice_sdram_desc, u_int32_t byte_adr, u_int32_t* ptr_data, u_int32_t num_of_lwords);
int s3100_sdram_read(struct SIS1100_Device_Struct* pDevice_sdram_desc, u_int32_t byte_adr, u_int32_t* ptr_data, u_int32_t num_of_lwords);
