/***************************************************************************/
/*                                                                         */
/*  Filename: V1740.h                                                      */
/*                                                                         */
/*  Function: headerfile for V1740                                         */
/*                                                                         */
/* ----------------------------------------------------------------------- */
/*  $Id$                           */
/***************************************************************************/

#ifndef  V1740_INCLUDE_H
#define  V1740_INCLUDE_H

#define V1740_EVENT_READOUT_BUFFER          0x0000

#define V1740_GROUP_CONFIG                  0x8000      /* R/W       ; D32 */ 
#define V1740_GROUP_CFG_BIT_SET             0x8004      /* write only  D32 */ 
#define V1740_GROUP_CFG_BIT_CLR             0x8008      /* write only; D32 */ 
#define V1740_BUFFER_ORGANIZATION           0x800C      /* R/W       ; D32 */ 
#define V1740_BUFFER_FREE                   0x8010      /* R/W       ; D32 */ 
#define V1740_CUSTOM_SIZE                   0x8020      /* R/W       ; D32 */ 
#define V1740_ACQUISITION_CONTROL           0x8100      /* R/W       ; D32 */ 
#define V1740_ACQUISITION_STATUS            0x8104      /* read  only; D32 */ 
#define V1740_SOFTWARE_TRIGGER              0x8108      /* write only; D32 */ 
#define V1740_TRIG_SRCE_EN_MASK             0x810C      /* R/W       ; D32 */ 
#define V1740_FP_TRIGGER_OUT_EN_MASK        0x8110      /* R/W       ; D32 */ 
#define V1740_POST_TRIGGER_SETTING          0x8114      /* R/W       ; D32 */ 
#define V1740_FP_IO_DATA                    0x8118      /* R/W       ; D32 */ 
#define V1740_FP_IO_CONTROL                 0x811C      /* R/W       ; D32 */  
#define V1740_GROUP_EN_MASK                 0x8120      /* R/W       ; D32 */ 
#define V1740_ROC_FPGA_FW_REV               0x8124      /* read  only; D32 */ 
#define V1740_DOWNSAMPLE_FAC                0x8128      /* read only */
#define V1740_EVENT_STORED                  0x812C      /* read  only; D32 */ 
#define V1740_SET_MONITOR_DAC               0x8138      /* R/W       ; D32 */ 
#define V1740_BOARD_INFO                    0x8140      /* read  only; D32 */ 
#define V1740_MONITOR_MODE                  0x8144      /* R/W       ; D32 */ 
#define V1740_EVENT_SIZE                    0x814C      /* read  only; D32 */ 
#define V1740_ALMOST_FULL_LEVEL             0x816C      /* R/W       ; D32 */
#define V1740_FP_LVDS_IO_CRTL               0x81A0      /* R/W       ; D32 */
#define V1740_ALMOST_FULL_LEVEL             0x816C      /* R/W       ; D32 */
#define V1740_FP_LVDS_IO_CRTL               0x81A0      /* R/W       ; D32 */
#define V1740_VME_CONTROL                   0xEF00      /* R/W       ; D32 */ 
#define V1740_VME_STATUS                    0xEF04      /* read  only; D32 */ 
#define V1740_BOARD_ID                      0xEF08      /* R/W       ; D32 */ 
#define V1740_MULTICAST_BASE_ADDCTL         0xEF0C      /* R/W       ; D32 */ 
#define V1740_RELOC_ADDRESS                 0xEF10      /* R/W       ; D32 */ 
#define V1740_INTERRUPT_STATUS_ID           0xEF14      /* R/W       ; D32 */ 
#define V1740_INTERRUPT_EVT_NB              0xEF18      /* R/W       ; D32 */ 
#define V1740_BLT_EVENT_NB                  0xEF1C      /* R/W       ; D32 */ 
#define V1740_SCRATCH                       0xEF20      /* R/W       ; D32 */ 
#define V1740_SW_RESET                      0xEF24      /* write only; D32 */ 
#define V1740_SW_CLEAR                      0xEF28      /* write only; D32 */ 
#define V1740_FLASH_ENABLE                  0xEF2C      /* R/W       ; D32 */ 
#define V1740_FLASH_DATA                    0xEF30      /* R/W       ; D32 */ 
#define V1740_CONFIG_RELOAD                 0xEF34      /* write only; D32 */ 
#define V1740_CONFIG_ROM                    0xF000      /* read  only; D32 */ 

#define V1740_GROUP_THRESHOLD               0x1080      /* For channeln 0x1n80 */
#define V1740_GROUP_STATUS                  0x1088      /* For channel 0 */
#define V1740_GROUP_FW_REV                  0x108C      /* For channel 0 */
#define V1740_GROUP_BUFFER_OCCUPANCY        0x1094      /* For channel 0 */
#define V1740_GROUP_DAC                     0x1098      /* For channel 0 */
#define V1740_GROUP_ADC_CONFIG              0x109C      /* For channel 0 */
#define V1740_CH_TRG_MASK                   0x10A8      /* For channel 0 */
#define V1740_GROUP_CH_TRG_MASK             0x10B0      /* For channel 0 */

#define V1740_RUN_START                           1
#define V1740_RUN_STOP                            2
#define V1740_REGISTER_RUN_MODE                   3
#define V1740_SIN_RUN_MODE                        4
#define V1740_SIN_GATE_RUN_MODE                   5
#define V1740_TRIGGER_OVERTH                      6
#define V1740_TRIGGER_UNDERTH                     7
#define V1740_MULTI_BOARD_SYNC_MODE               8
#define V1740_COUNT_ACCEPTED_TRIGGER              9
#define V1740_COUNT_ALL_TRIGGER                  10
#define V1740_DOWNSAMPLE_ENABLE                  11
#define V1740_DOWNSAMPLE_DISABLE                 12
#define V1740_NO_ZERO_SUPPRESSION                13

#define V1740_EVENT_CONFIG_ALL_ADC        0x01000000
#define V1740_SOFT_TRIGGER                0x80000000
#define V1740_EXTERNAL_TRIGGER            0x40000000
#define V1740_ALIGN64                           0x20

#define V1740_DONE                          0

#endif  //  V1740_INCLUDE_H


