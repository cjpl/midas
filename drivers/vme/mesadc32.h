/***************************************************************************/
/*                                                                        */
/*  Filename: MESADC32.h                                                  */
/*                                                                        */
/*  Function: headerfile for MESADC32                                     */
/*                                                                        */
/* ---------------------------------------------------------------------- */
/*                                                                        */
/**************************************************************************/

#ifndef  MESADC32_INCLUDE_H
#define  MESADC32_INCLUDE_H

#define  MESADC32_EMPTY  1
#define  MESADC32_MAX_CHANNELS               32
#define  MESADC32_EVENT_READOUT_BUFFER            0x0000 /* R/W D32, D64 */

// Thershold memory
#define MESADC32_THRESHOLD_0             0x4000      /* R/W def:0 */
/*                                 up to 0x403E for threshold 31 */

// Address Registers
#define MESADC32_ADD_SOURCE              0x6000      /* R/W def:0 D16  */ 
#define MESADC32_ADD_REG                 0x6002      /* R/W def:0 D16  */ 
#define MESADC32_MODULE_ID               0x6004      /* R/W def:0xff D16 */ 
#define MESADC32_SOFT_RESET              0x6008      /* W  D16 */ 
#define MESADC32_FIRM_REV                0x600E      /* R   def:0x01.10  D16 */ 

// Interrupt
#define MESADC32_IRQ_LEVEL               0x6010      /* R/W  def:0 D16 */
 
// MCST, CBLT
#define MESADC32_CBLT_MCST_CTL           0x6020      /* R/W  def:0 */ 
#define MESADC32_CBLT_ADD                0x6022      /* R/W def:0xAA A31..A25 CBLT add */ 
#define MESADC32_MCST_ADD                0x6024      /* R   def:0xbb A31..A25 MCST add */ 

// Fifo handling
#define MESADC32_BUF_DATA_LEN            0x6030      /* R  */ 
#define MESADC32_DATA_LEN_FMT            0x6032      /* R/W def:2 */ 
#define MESADC32_READOUT_RESET           0x6034      /* W */ 
#define MESADC32_MULTIEVENT              0x6036      /* R/W def:0 */ 
#define MESADC32_MARKING_TYPE            0x6038      /* R/W def:0 */  
#define MESADC32_START_ACQ               0x603A      /* R/W def:1 */ 
#define MESADC32_FIFO_RESET              0x603C      /* W */ 
#define MESADC32_DATA_READY              0x603E      /* R */ 

// Operation mode
#define MESADC32_BANK_OPERATION          0x6040      /* R/W def:0 */ 
#define MESADC32_ADC_RESOLUTION          0x6042	     /* R/W def:2 */ 
#define MESADC32_OUTPUT_FMT              0x6044      /* R/W def:0 */ 
#define MESADC32_ADC_OVERRIDE            0x6046	     /* R/W def:2 */ 
#define MESADC32_SLC_OFF                 0x6048      /* R/W def:0 */ 
#define MESADC32_SKIP_OORANGE            0x604A      /* R/W def:0 */ 

// Gate Generator
#define MESADC32_HOLD_DELAY_0            0x6050      /* R/W def:20 */ 
#define MESADC32_HOLD_DELAY_1            0x6052      /* R/W def:20 */ 
#define MESADC32_HOLD_WIDTH_0            0x6054      /* R/W def:50 */ 
#define MESADC32_HOLD_WIDTH_1            0x6056      /* R/W def:50 */ 
#define MESADC32_USE_GG                  0x6058      /* R/W def:0 */

// I/O  
#define MESADC32_INPUT_RANGE             0x6060      /* R/W def:0 */ 
#define MESADC32_ECL_TERM                0x6062      /* R/W def:0 */ 
#define MESADC32_ECL_GATE1_OSC           0x6064      /* R/W def:0 */ 
#define MESADC32_ECL_FC_RESET            0x6066      /* R/W def:0 */ 
#define MESADC32_ECL_BUSY                0x6068      /* R/W def:0 */ 
#define MESADC32_NIM_GATE1_OSC           0x606A      /* R/W def:0 */ 
#define MESADC32_NIM_FC_RESET            0x606C      /* R/W def:0 */ 
#define MESADC32_NIM_BUSY                0x606E      /* R/W def:0 */ 

// Test pulser
#define MESADC32_TESTPULSER              0x6070      /* R/W def:0 */ 

// Module RC-bus for communication to the MSCF16
#define MESADC32_RC_BUSNO                0x6080      /* R/W def:0 */ 
#define MESADC32_RC_MODNUM               0x6082      /* R/W def:0 */ 
#define MESADC32_RC_OPCODE               0x6084      /* R/W  */ 
#define MESADC32_RC_ADDRESS              0x6086      /* R/W  */ 
#define MESADC32_RC_DATA                 0x6088      /* R/W  */ 
#define MESADC32_RC_SENT_RTN_STAT        0x608A      /* R  def:0 */ 

// CTRA
#define MESADC32_RESET_CTL_AB            0x6090      /* R/W */
#define MESADC32_EV_CTL_LOW              0x6092      /* R/W */
#define MESADC32_EV_CTL_HIGH             0x6094      /* R/W */
#define MESADC32_TS_SOURCE               0x6096      /* R/W */
#define MESADC32_TS_DIVIDER              0x6098      /* R/W */
#define MESADC32_TS_COUNTER_LOW          0x609C      /* R */
#define MESADC32_TS_COUNTER_HIGH         0x609E      /* R */

// CTRB
#define MESADC32_ADC_BUSY_TIME_LOW       0x60A0      /* R */
#define MESADC32_ADC_BUSY_TIME_HIHG      0x60A2      /* R */
#define MESADC32_GATE1_TIME_LOW          0x60A4      /* R */
#define MESADC32_GATE1_TIME_HIGH         0x60A6      /* R */
#define MESADC32_TIME_0                  0x60A8      /* R */
#define MESADC32_TIME_1                  0x60AA      /* R */
#define MESADC32_TIME_2                  0x60AC      /* R */
#define MESADC32_STOP_CTR                0x60AE      /* R/W */

#define MESADC32_ACQ_STOP                     0
#define MESADC32_ACQ_START                    1
#define MESADC32_MARK_TIME                    1
#define MESADC32_MARK_EVENT                   0
#define DONE                                  0

#endif  //  MESADC32_INCLUDE_H


