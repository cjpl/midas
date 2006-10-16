// --------------------------------------------------------
//  Include files
// --------------------------------------------------------

#include    <c8051f310.h>
   
// --------------------------------------------------------
//  SMBus_Initialization : 
//      Bus Initialization sequence
// --------------------------------------------------------
void  SMBus_Initialization(void)
{
    const unsigned int  clock_freq = 24500000;
 
    // timer 1 is usually initialized by mscbmain.c
	// If you make no change in that file, you do not need to 
	// activate the following two lines.
	// TMOD = (TMOD & 0x0F) | 0x20;     // 8 bit auto-reloaded mode
    // TR1 = 0;                         // Activate timer 1 

    /*
    // timer 3 configuration (Activate them if you use timer 3)
    // 16-bit, auto-reloaded, low-byte interrupt disabled
    TMR3CN = 0x00;                            
    TMR3   = -(clock_freq/12/40);   // Timer3 configured to overflow after 
    TMR3RL = -(clock_freq/12/40);   // ~25ms (for SMBus low timeout detect)   

    CKCON &= ~0x40;                 // Timer3 uses SYSCLK/12
    TMR3  |= 0x04;                  // Start Timer3
    */


    // SMBus Clock/Configuration
	// SMBus enable, slave inhibit, setup and hold time extension, 
	// timeout detection with timer 3
	// free timeout detection, timer1 overflow as clock source
	// Note :"timer1 overflow" is required for clock source        
//  SMB0CF  = 0x5D;     // SMBus configuration
    SMB0CF  = 0x55;     // SMBus configuration
    SMB0CF |= 0x80;     // Enable SMBus after all setting done.

    // Pin allocation for SCL, SDA for SMB
    // We use default pin allocation used in CF8051F310. (Reset value),
    // namely, SCL = P0 ^ 1, SDA = P0 ^ 0. Make sure SMBbus is activated
    // in XBR0 register together with P0SKIP = 0x00.

    /* need to be checked
    EIE1 |= 1;          // SMBus interrupt enable
    IE   |= 0x20;       // Timer2 interrupt enable
    EA    = 1;          // Global interrupt enable
    */

}

// --------------------------------------------------------
//  SMBus_Write_Byte :
//      Write a single byte to the SMBus as a Master
//
//      Arguments : 
//          slave_address   : 7-bit slave address 
//          value           : data word
//
//      Return    :
//          0               : success
//          non-zero        : error
// --------------------------------------------------------
unsigned char SMBus_Write_Byte(unsigned char slave_address, unsigned char value)
{
    STA = 1;            // make start bit
    while (SI == 0);    // wait SMBus interruption

                        // slave address with R/W_ flag off
    SMB0DAT = (slave_address << 1) & 0xFE;  
    STA     = 0;        // clear start bit
    SI      = 0;        // clear interrupt flag
    while (SI == 0);    // wait SMBus interruption. sending address word ...

    if (!ACK) return 1; // No slave ACK. Address is wrong or slave is missing.

    SMB0DAT = value;    // set data word
    SI      = 0;
    while (SI == 0);    // wait SMBus interruption. 

    if (!ACK) return 2; // Error : No slave ACK. 

    STO = 1;
    SI  = 0;

    return 0;
}

// --------------------------------------------------------
//  SMBus_Read_Byte :
//      Write a single byte to the SMBus as a Master
//
//      Arguments : 
//          slave_address   : 7-bit slave address 
//          * value         : a pointer to store the received data
//
//      Return    :
//          0               : success
//          non-zero        : error
// --------------------------------------------------------
unsigned char SMBus_Read_Byte(unsigned char slave_address, unsigned char * value)
{
    STA = 1;            // make start bit
    while (SI == 0);    // wait SMBus interruption

    // slave address
    // turn on R/W_ flag 
    SMB0DAT = (slave_address << 1) | 0x01;  
    STA     = 0;        // clear start bit
    SI      = 0;        // clear interrupt flag
    while (SI == 0);    // wait SMBus interruption. sending address word ...

    if (!ACK) return 1; // No slave ACK. Address is wrong or slave is missing.

    SI      = 0;
    while (SI == 0);    // wait SMBus interruption. receiving data word ...

    * value = SMB0DAT;  // set data word
    ACK     = 0;        // send back NACK 
    STO     = 1;
    SI      = 0;

    return 0;
}

// --------------------------------------------------------
//  SMBus_Timeout_Interrupt
//      Interrupt handler for Bus timeout. 
// --------------------------------------------------------
void  SMBus_Timeout_Interrupt(void) interrupt 14
{
    SMB0CF &= ~0x80;        // Disable SMBus
    SMB0CN  =  0x00;        // Clear Bus controller
    TMR3CN  = ~0x80;        // Clear Timer 3 overflow flag
    SMB0CF |=  0x80;        // Enable SMBus
}

// --------------------------------------------------------
//  SMBus_Write_CD :
//      Write a single command and data word to the SMBus as a Master
//
//      Arguments : 
//          slave_address   : 7-bit slave address
//          command         : command word 
//          value           : data word
//
//      Return    :
//          0               : success
//          non-zero        : error
// --------------------------------------------------------
unsigned char SMBus_Write_CD(unsigned char slave_address, 
                             unsigned char command, unsigned char  value)
							 
{
    STA = 1;            // make start bit
    while (SI == 0);    // wait SMBus interruption

    // -----------------------------------------------------
    // Slave address with turning off R/W_ flag 
    // -----------------------------------------------------
    SMB0DAT = (slave_address << 1) & 0xFE;  
    STA     = 0;        // clear start bit
    SI      = 0;        // clear interrupt flag
    while (SI == 0);    // wait SMBus interruption. sending address word ...

    if (!ACK) return 1; // No slave ACK. Address is wrong or slave is missing.

    SMB0DAT = command;  // set command word
    SI      = 0;
    while (SI == 0);    // wait SMBus interruption. 

    if (!ACK) return 2; // Error : No slave ACK.
	
	SMB0DAT = value; 
    SI      = 0;
    while (SI == 0);    // wait SMBus interruption. 

    if (!ACK) return 3; // Error : No slave ACK.

    STO = 1;
    SI  = 0;

    return 0;
}

unsigned char SMBus_Read_CD (unsigned char slave_address, 
                             unsigned char command, unsigned char * value)
{
    STA = 1;            // make start bit
    while (SI == 0);    // wait SMBus interruption

    // slave address
    // turn off R/W_ flag (This is not required, just for confirmation)
    SMB0DAT = (slave_address << 1) & 0xFE;  
    STA     = 0;        // clear start bit
    SI      = 0;        // clear interrupt flag
    while (SI == 0);    // wait SMBus interruption. sending address word ...

    if (!ACK) return 1; // No slave ACK. Address is wrong or slave is missing.

    SMB0DAT = command;  // set data word
    SI      = 0;
    while (SI == 0);    // wait SMBus interruption. 

    if (!ACK) return 2; // Error : No slave ACK. 

    SI  = 0;
    STA = 1;            // make start bit
    while (SI == 0);    // wait SMBus interruption

    // slave address
    // turn on R/W_ flag 
    SMB0DAT = (slave_address << 1) | 0x01;  
    STA     = 0;        // clear start bit
    SI      = 0;        // clear interrupt flag
    while (SI == 0);    // wait SMBus interruption. sending address word ...

    if (!ACK) return 1; // No slave ACK. Address is wrong or slave is missing.

    SI      = 0;
    while (SI == 0);    // wait SMBus interruption. receiving data word ...

    * value = SMB0DAT;  // set data word
    ACK     = 0;        // send back NACK 
    STO     = 1;

    SI      = 0;

    return 0;
}