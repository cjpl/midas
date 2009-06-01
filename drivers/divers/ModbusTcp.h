///
/// ModbusTcp.h
///
/// Driver for accessing MODBUS devices using the MODBUS TCP protocol
/// Konstantin Olchanski, TRIUMF, 2008-2009
///

#ifndef INCLUDE_MODBUS_H
#define INCLUDE_MODBUS_H

#include "midas.h"

#define MODBUS_FUNC3  3 ///< MODBUS Read register
#define MODBUS_FUNC4  4 ///< MODBUS Read register
#define MODBUS_FUNC6  6 ///< MODBUS Write register

class ModbusTcp
{
 public:
   bool fTrace; ///< report activity
   int fSocket; ///< TCP socket connection to the Modbus device
   int fReadTimeout_sec; ///< TCP socket read timeout, in seconds
   
 public:
   ModbusTcp(); ///< ctor
   
   int Connect(const char* address);
   int Disconnect();
   
   int ReadRegister(int slaveId,  int ireg); ///< read given register
   int ReadRegisters(int slaveId, int ireg, int numReg, WORD data[]); ///< read multiple registers
   int WriteRegister(int slaveId, int ireg, int value); ///< write given register
            
   int ReadRegister(int slaveId,  int func, int ireg); ///< read given register
   int ReadRegisters(int slaveId, int func, int ireg, int numReg, WORD data[]); ///< read multiple registers
   int WriteRegister(int slaveId, int func, int ireg, int value); ///< write given register

   // low-level access functions

   int Function3(int slaveId, int firstReg, int numReg); ///< send function 3 read request
   int Function4(int slaveId, int firstReg, int numReg); ///< send function 4 read request
   int Function6(int slaveId, int register, int value);  ///< send function 6 write request

   int Write(const char* buf, int length); ///< send TCP data
   int Read(char* buf, int length); ///< receive TCP data with timeout
};

#endif
// end
