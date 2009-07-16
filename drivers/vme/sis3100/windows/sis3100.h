/***************************************************************************/
/*                                                                         */
/*  Filename: sis3100.h                                                    */
/*                                                                         */
/*  Funktion: headerfile for SIS3100  PCI-VME Interface                    */
/*                                                                         */
/*  Autor:                TH                                               */
/*  date:                 10.05.2002                                       */
/*  last modification:    06.06.2002                                       */
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





/* addresses  */

/* general registers */
#define SIS3300_OPT_IO_REG                      0x80	  
#define SIS3300_OPT_IN_IRQ_LATCH                0x84	  
#define SIS3300_OPT_VME_MASTER_CTRL				0x100	  
#define SIS3300_OPT_VME_IRQ_CTRL				0x104	  



/* bits */


/* doorbell IRQ status bits */

#define DOORBELL_DSP_IRQ					0x8000
#define DOORBELL_LEMO_IN3_IRQ				0x4000
#define DOORBELL_LEMO_IN2_IRQ				0x2000
#define DOORBELL_LEMO_IN1_IRQ				0x1000

#define DOORBELL_FLAT_IN4_IRQ				0x800
#define DOORBELL_FLAT_IN3_IRQ				0x400
#define DOORBELL_FLAT_IN2_IRQ				0x200
#define DOORBELL_FLAT_IN1_IRQ				0x100

#define DOORBELL_VME_IRQ7					0x80
#define DOORBELL_VME_IRQ6					0x40
#define DOORBELL_VME_IRQ5					0x20
#define DOORBELL_VME_IRQ4					0x10
#define DOORBELL_VME_IRQ3					0x8
#define DOORBELL_VME_IRQ2					0x4
#define DOORBELL_VME_IRQ1					0x2



/* OPT_IN_IRQ_LATCH clear and status bits */

#define DSP_IRQ_STA_CLR_BIT					0x80000000
#define LEMO_IN3_IRQ_STA_CLR_BIT			0x40000000
#define LEMO_IN2_IRQ_STA_CLR_BIT			0x20000000
#define LEMO_IN1_IRQ_STA_CLR_BIT			0x10000000

#define FLAT_IN4_IRQ_STA_CLR_BIT			0x8000000
#define FLAT_IN3_IRQ_STA_CLR_BIT			0x4000000
#define FLAT_IN2_IRQ_STA_CLR_BIT			0x2000000
#define FLAT_IN1_IRQ_STA_CLR_BIT			0x1000000


#define DOORBELL_IRQ_UPDATE					0x8000


/* OPT_IN_IRQ_LATCH enable bits */

#define DSP_IRQ_ENABLE_BIT					0x80
#define LEMO_IN3_ENABLE_CLR_BIT				0x40
#define LEMO_IN2_ENABLE_CLR_BIT				0x20
#define LEMO_IN1_ENABLE_CLR_BIT				0x10

#define FLAT_IN4_ENABLE_CLR_BIT				0x8
#define FLAT_IN3_ENABLE_CLR_BIT				0x4
#define FLAT_IN2_ENABLE_CLR_BIT				0x2
#define FLAT_IN1_ENABLE_CLR_BIT				0x1


/* OPT_IN/OUT_REG bits */

#define LEMO_OUT3_PULS						0x40000000
#define LEMO_OUT2_PULS						0x20000000
#define LEMO_OUT1_PULS						0x10000000

#define FLAT_OUT4_PULS						0x8000000
#define FLAT_OUT3_PULS						0x4000000
#define FLAT_OUT2_PULS						0x2000000
#define FLAT_OUT1_PULS						0x1000000




#define VME_IRQ7_STA_CLR_BIT				0x800000
#define VME_IRQ6_STA_CLR_BIT				0x400000
#define VME_IRQ5_STA_CLR_BIT				0x200000
#define VME_IRQ4_STA_CLR_BIT				0x100000
#define VME_IRQ3_STA_CLR_BIT				0x80000
#define VME_IRQ2_STA_CLR_BIT				0x40000
#define VME_IRQ1_STA_CLR_BIT				0x20000
