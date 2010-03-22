/***************************************************************************/
/*                                                                         */
/*  Filename: isegvhs.h                                                    */
/*                                                                         */
/***************************************************************************/

/* addresses  */ 

#define ISEGVHS_MODULE_STATUS            0x0000  // D16
#define ISEGVHS_MODULE_CONTROL           0x0002  // D16
#define ISEGVHS_VOLTAGE_RAMP_SPEED       0x0014  // F32
#define ISEGVHS_CURRENT_RAMP_SPEED       0x0018  // F32
#define ISEGVHS_VOLTAGE_MAX              0x001C  // F32
#define ISEGVHS_CURRENT_MAX              0x0020  // F32
#define ISEGVHS_SUPPLY_P5                0x0024  // F32
#define ISEGVHS_SUPPLY_P12               0x0028  // F32
#define ISEGVHS_SUPPLY_N12               0x002C  // F32
#define ISEGVHS_TEMPERATURE              0x0030  // F32
#define ISEGVHS_SERIAL_NUMBER            0x0034  // F32

#define ISEGVHS_VENDOR_ID                0x005C  // "ISEG"

#define ISEGVHS_CHANNEL_BASE             0x0060  
#define ISEGVHS_CHANNEL_OFFSET           0x0030
#define ISEGVHS_CHANNEL_STATUS           0x0000  // D16
#define ISEGVHS_CHANNEL_CONTROL          0x0002  // D16
#define ISEGVHS_VOLTAGE_SET              0x0008  // F32
#define ISEGVHS_CURRENT_SET              0x000C  // F32
#define ISEGVHS_VOLTAGE_MEASURE          0x0010  // F32
#define ISEGVHS_CURRENT_MEASURE          0x0014  // F32
#define ISEGVHS_VOLTAGE_BOUND            0x0018  // F32
#define ISEGVHS_CURRENT_BOUND            0x001C  // F32
#define ISEGVHS_VOLTAGE_NOMINAL          0x0020  // F32
#define ISEGVHS_CURRENT_NOMINAL          0x0024  // F32
#define ISEGVHS_VOLTAGE_MAX_SET          0x0028  // F32
#define ISEGVHS_CURRENT_MAX_SET          0x002C  // F32

#define ISEGVHS_SET_ON                   0x0008
#define ISEGVHS_SET_OFF                  0x0000
#define ISEGVHS_SET_EMCY                 0x0020

#define ISEGVHS_MAX_CHANNELS                 12  // Model...
