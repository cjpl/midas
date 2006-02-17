/***************************************************************************/
/*                                                                         */
/*  Filename: sis3100_vme_calls.h                                          */
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

typedef unsigned char           u_int8_t;
typedef unsigned short          u_int16_t;
typedef unsigned long           u_int32_t;




/**********************/
/*                    */
/*    VME SYSReset    */
/*                    */
/**********************/

 
int vmesysreset(struct SIS1100_Device_Struct* sis1100_pDevice) ;




/*****************/
/*               */
/*    VME A16    */
/*               */
/*****************/

int vme_A16D8_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int8_t* vme_data ) ;

int vme_A16D16_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int16_t* vme_data ) ;
int vme_A16D32_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data ) ;

int vme_A16D8_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int8_t vme_data ) ;
int vme_A16D16_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int16_t vme_data ) ;
int vme_A16D32_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t vme_data ) ;





/*****************/
/*               */
/*    VME A24    */
/*               */
/*****************/


int vme_A24D8_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int8_t* vme_data ) ;
int vme_A24D16_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int16_t* vme_data ) ;
int vme_A24D32_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data ) ;


int vme_A24BLT32_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* got_no_of_lwords) ;

int vme_A24MBLT64_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* got_no_of_lwords) ;

int vme_A24DMA_D32_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* got_no_of_lwords) ;


int vme_A24BLT32FIFO_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                          u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords) ;

int vme_A24MBLT64FIFO_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                          u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords) ;

int vme_A24DMA_D32FIFO_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* got_no_of_lwords) ;



int vme_A24D8_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int8_t vme_data ) ;
int vme_A24D16_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int16_t vme_data ) ;
int vme_A24D32_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t vme_data ) ;


int vme_A24BLT32_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* put_num_of_lwords) ;


int vme_A24MBLT64_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* put_num_of_lwords) ;


int vme_A24DMA_D32_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* put_num_of_lwords) ;



int vme_A24BLT32FIFO_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* put_num_of_lwords) ;


int vme_A24MBLT64FIFO_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* put_num_of_lwords) ;


int vme_A24DMA_D32FIFO_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* put_num_of_lwords) ;






/*****************/
/*               */
/*    VME A32    */
/*               */
/*****************/


int vme_A32D8_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int8_t* vme_data ) ;
int vme_A32D16_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int16_t* vme_data ) ;
int vme_A32D32_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data ) ;




int vme_A32BLT32_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* got_no_of_lwords) ;



int vme_A32MBLT64_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* got_no_of_lwords) ;


int vme_A32_2EVME_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                          u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords) ;


int vme_A32DMA_D32_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* got_no_of_lwords) ;



int vme_A32BLT32FIFO_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                          u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords) ;

int vme_A32MBLT64FIFO_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                          u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords) ;


int vme_A32_2EVMEFIFO_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                          u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords) ;

int vme_A32DMA_D32FIFO_read(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* got_no_of_lwords) ;



int vme_A32D8_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int8_t vme_data ) ;
int vme_A32D16_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int16_t vme_data ) ;

int vme_A32D32_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t vme_data ) ;




int vme_A32BLT32_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* put_num_of_lwords) ;


int vme_A32MBLT64_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* put_num_of_lwords) ;


int vme_A32DMA_D32_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* put_num_of_lwords) ;



int vme_A32BLT32FIFO_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* put_num_of_lwords) ;


int vme_A32MBLT64FIFO_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* put_num_of_lwords) ;


int vme_A32DMA_D32FIFO_write(struct SIS1100_Device_Struct* pDevice, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* put_num_of_lwords) ;




/***********************/
/*                     */
/*    s1100_control    */
/*                     */
/***********************/

int s1100_control_read(struct SIS1100_Device_Struct* pDevice, int offset, u_int32_t* data) ;
int s1100_control_write(struct SIS1100_Device_Struct* pDevice, int offset, u_int32_t data) ;


/***********************/
/*                     */
/*    s3100_control    */
/*                     */
/***********************/


int s3100_control_read(struct SIS1100_Device_Struct* pDevice, int offset, u_int32_t* data) ;
int s3100_control_write(struct SIS1100_Device_Struct* pDevice, int offset, u_int32_t data) ;



/***********************/
/*                     */
/*    s3100_sharc      */
/*                     */
/***********************/

int s3100_sharc_write(struct SIS1100_Device_Struct* pDevice_sharc_desc, u_int32_t byte_adr, u_int32_t* ptr_data,  u_int32_t num_of_lwords) ;
int s3100_sharc_read(struct SIS1100_Device_Struct* pDevice_sharc_desc, u_int32_t byte_adr, u_int32_t* ptr_data,  u_int32_t num_of_lwords) ;



/***********************/
/*                     */
/*    s3100_sdram      */
/*                     */
/***********************/

int s3100_sdram_write(struct SIS1100_Device_Struct* pDevice_sdram_desc, u_int32_t byte_adr, u_int32_t* ptr_data,  u_int32_t num_of_lwords ) ;
int s3100_sdram_read(struct SIS1100_Device_Struct* pDevice_sdram_desc, u_int32_t byte_adr, u_int32_t* ptr_data,  u_int32_t num_of_lwords )  ;








