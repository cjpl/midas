#include <math.h>
#include "mscbemb.h"

#include "fgd_008.h"
#include "fgd_008_tsensor.h"
#include "SMBus_handler.h"
   
// Prescale counters for not to read temperature every user_loop.
unsigned int  temp_prescale_counter = 0;

// Request flags for CPU, LM95231(local die), remote diodes
unsigned char xdata temp_request_cpu      = 0;
unsigned char xdata temp_request_local    = 0;
unsigned char xdata temp_request_remote1  = 0;
unsigned char xdata temp_request_remote2  = 0;

extern struct user_data_type xdata user_data[N_HV_CHN];

// toplevel function for temperature measurement
void process_temperature(unsigned char mode)
{
    unsigned char  status;
    float          temperature;

    if (mode == 0) return;    // temperature reading is disactivated.

    // increment prescale counter
    //  temp_prescale_counter = temp_prescale_counter++ % TEMP_PRESCALE;
    temp_prescale_counter = temp_prescale_counter + 1;
	temp_prescale_counter = temp_prescale_counter % TEMP_PRESCALE;
    // At certain point, make a request to measure the temperature
    // Assert the request flag. If the flag is already asserted, just
    // ignore.
    temp_request_cpu = (temp_prescale_counter == (unsigned int) 
                    (0.1 * TEMP_PRESCALE))  ? 1 : temp_request_cpu;

    temp_request_local = (temp_prescale_counter == (unsigned int) 
                    (0.3 * TEMP_PRESCALE))  ? 1 : temp_request_local;

    temp_request_remote1 = (temp_prescale_counter == (unsigned int) 
                    (0.5 * TEMP_PRESCALE))  ? 1 : temp_request_remote1;

    temp_request_remote2 = (temp_prescale_counter == (unsigned int) 
                    (0.7 * TEMP_PRESCALE))  ? 1 : temp_request_remote2;


    if (temp_request_cpu) {
        status = read_temperature_CPU(& temperature);      
        temp_request_cpu = status ? 1 : 0;
        if (! temp_request_cpu) store_temperature(1, temperature);
        return;
    }

    /*
    if (temp_request_local) {
        status = read_temperature_LM95231(0, & temperature);
        // if status has error number, request the remeasurement.
        temp_request_local = status ? 1 : 0;     
        if (! temp_request_local) store_temperature(2, temperature);
        return;
    }
    */
   
    if (temp_request_remote1) {
        status = read_temperature_LM95231(1, & temperature);
        // if status has error number, request the remeasurement.
        temp_request_remote1 = status ? 1 : 0;     
        if (! temp_request_remote1) store_temperature(3, temperature);
        return;
    }
   

    if (temp_request_remote2) {
        led_blink(9, 2, 150);

        status = read_temperature_LM95231(2, & temperature);
        // if status has error number, request the remeasurement.
        temp_request_remote2 = status ? 1 : 0;     
        if (! temp_request_remote2) store_temperature(4, temperature);
        return;
    }

}

// save the temperature to the user data structure.
void store_temperature(unsigned char index, float temperature)
{
    unsigned char i;
    temperature = floor(10 * temperature + 0.5) / 10.; 

    for (i = 0; i < N_HV_CHN; i++) {
        DISABLE_INTERRUPTS;
		if (index == 1) {         // CPU temperature
            user_data[i].temperature_c8051 = temperature;
        } else if (index == 2) {  // LM95231 die temperature
            // nothing at this time.     
		} else if (index == 3) {  // diode 1 temperature
            user_data[i].temperature_diode1 = temperature;
		} else if (index == 4) {  // diode 2 temperature
            user_data[i].temperature_diode2 = temperature;
		}
        ENABLE_INTERRUPTS;
    }
	
}

// ------------------------------------------------------------------
// C8051F310 temperature sensor related items
// ------------------------------------------------------------------
unsigned char read_temperature_CPU(float * temperature)
{
    float   value;

    REF0CN  = 0x0E;         // temp sensor on, internal vol ref, bias generation
//  AD0EN   = 0x80;         // turn on ADC
    ADC0CN  = 0x80;         // turn on ADC

    /* set MUX */
    AMX0P   = 0x1E;         // positive input : temp sensor
    AMX0N   = 0x1F;         // negative input : GND

    DISABLE_INTERRUPTS;
    AD0INT  = 0;            // clear the conversion-done flag
    AD0BUSY = 1;            // Initiate the AD conversion
    while (AD0INT == 0);    // wait the AD conversion done
    ENABLE_INTERRUPTS;

    value = 256. * (ADC0H & 0x03) + ADC0L;   // 2 bit (MSB) + 8 bit (LSB)
    
    // Convert to mV. Reference voltage is 3.3 V. 10 bit resolution
    value = value / 1024. * 3300.;  

    // Convert to degree C, conversion function is from specification.
    * temperature = (value - 897.) / 3.35;

    return 0;
}


// ------------------------------------------------------------------
// LM95231 temperature sensor related items
//          - K. Mizouchi    Sep/20/2006
// Note :
// [1] SMBus clock frequency (f_SMB) should be 10 kHz < f_SMB < 100 kHz.
//     Please be careful when you change the sytem clock.
// [2] LM95231 has a 7-bit SMBus slave address. The address cannot be
//     changed by software or hardware. It depends on the part number 
//     you ordered. You need to confirm the part number.
// ------------------------------------------------------------------


// ------------------------------------------------------------------
// LM95231 temperature sensor initialization processes
//      return value : 0         success
//                     non zero  error
// ------------------------------------------------------------------
unsigned char init_LM95231(unsigned char init) 
{

    unsigned char   manufacturer, revision, status;

    watchdog_refresh(0);
    if (init == 1) {    // device check sequence
        // Read Manufacturer and Revision ID to confirm
        // LM95231 is alive.
        manufacturer = read_LM95231(REG_TEMP_MANUFACTURER);
        revision     = read_LM95231(REG_TEMP_REVISION);
        // if ((manufacturer != ) || (revision !=) ) return 1; 
        // Error. Unknow device or unavailable.


        // Check two remote diodes, MMBT9304, are alive.
        status = read_LM95231(REG_TEMP_STATUS);
        if (status & 0x03 != 0x03) return 2;    // error remode diode is missing

    } 
    // Set configuration register
    // 1 Hz continuous mode, unsigned temperature format
    write_LM95231(REG_TEMP_CONFIG, 0x20);   
    
    // Activate diode noise filter on
    write_LM95231(REG_TEMP_FILTER, 0x05);

    // Set remote diode model type to 2N3904
    write_LM95231(REG_TEMP_MODEL, 0x00);

    // Disable TruTherm mode
    write_LM95231(REG_TEMP_TRUTHERM, 0x00);

    return 0;       // initialization succeeded
}


// ------------------------------------------------------------------
// LM95231 temperature sensor : Read temperature
//      Argument(s) : 
//                  diode == 0 for local die temperature
//                        == 1 for remote diode1 temperature
//                        == 2 for remote diode2 temperature
//
//                  * temp     Measured temperature is stored to the address
//
//      Return      : 0        success
//                    non-zero failure
//
//      Note :
//      [1] A remote diode conversion takes long time, typically, 30 ms.
//          We do not wait the conversion (= non-blocking), release the 
//          CPU for other tasks.
// ------------------------------------------------------------------
unsigned char read_temperature_LM95231(unsigned char diode, float * temp)
{
    unsigned char status;
    unsigned char address_MSB, address_LSB, data_MSB, data_LSB;

    // Check the BUSY flag to see the conversion is done.
    status = read_LM95231(REG_TEMP_STATUS);
    if (status & 0x80) return 1;        // BUSY flag. Now AD converting ...    

    if (diode == 0) {
        address_MSB = REG_TEMP_LOCAL_MSB;                
        address_LSB = REG_TEMP_LOCAL_LSB;
    } else if (diode == 1) {
        address_MSB = REG_TEMP_REMOTE1_MSB;
        address_LSB = REG_TEMP_REMOTE1_LSB;
    } else if (diode == 2) {
        address_MSB = REG_TEMP_REMOTE2_MSB;
        address_LSB = REG_TEMP_REMOTE2_LSB;
    } else return 2;                    // unknown diode

    // Read MSB first because reading MSB blocks the LSB update.
    // see the specification of LM95231.
    data_MSB = read_LM95231(address_MSB);   
    data_LSB = read_LM95231(address_LSB);

    // Again, check the BUSY flag to confirm the conversion does not restart 
    // while reading the temperature.
    status = read_LM95231(REG_TEMP_STATUS);
    if (status & 0x80) return 3;        // BUSY flag. Now AD converting ...    

    // Data reconstruction
    if (diode == 0) {
        * temp = (float) (data_MSB & 0x7F) * 1.     // mask sign bit
                + (float) (data_LSB >> 6) * 0.25;   // 6 bit shift

    } else {
        * temp = (float) data_MSB * 1.   
                + (float) (data_LSB >> 3) * 0.03125;    // 3 bit shift
    }
	   
    return  0;
}

// ------------------------------------------------------------------
// LM95231 temperature sensor : write access sequence
// ------------------------------------------------------------------
void write_LM95231(unsigned char address, unsigned char value)
{
 
    watchdog_refresh(0);
    SMBus_Write_CD(ADDR_LM95231, address, value);
}

// ------------------------------------------------------------------
// LM95231 temperature sensor : read access sequence
//
//      Note :
//      [1] Single call of this read sequcence at least takes 
//          DELAY_LM95231 * 105. (525 us @ DELAY_LM95231 = 5 us) 
//          When you makes the DELAY_LM95231 longer, be careful
//          not to forget calling watchdog_refresh.
// ------------------------------------------------------------------
unsigned char read_LM95231(unsigned char address)     
{

    unsigned char  value;	
    watchdog_refresh(0);
	SMBus_Read_CD (ADDR_LM95231, address, &value);
    return value;
}
