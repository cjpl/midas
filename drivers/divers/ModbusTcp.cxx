///
/// ModbusTcp.cxx
///
/// Driver for accessing MODBUS devices using the MODBUS TCP protocol
/// Konstantin Olchanski, TRIUMF, 2008-2009
///
/// This driver is implemented using the public MODBUS documentation:
///
/// http://www.modbus.org/specs.php
/// Modbus_Application_Protocol_V1_1b.pdf
/// Modbus_Messaging_Implementation_Guide_V1_0b.pdf
/// Modbus_over_serial_line_V1_02.pdf
///

#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <fcntl.h>

#include <errno.h>
#include <assert.h>

#include "ModbusTcp.h"
#include "midas.h"

ModbusTcp::ModbusTcp()
{
   fTrace  = false;
   fSocket = -1;
   fReadTimeout_sec = 10;
};

int ModbusTcp::WriteRegister(int slaveId, int ireg, int value)
{
   int f = ireg/1000000;
   int r = ireg%1000000;

   if (f==0 || f==MODBUS_FUNC3 || f==MODBUS_FUNC4)
      f = MODBUS_FUNC6;

   return WriteRegister(slaveId, f, r, value);
}

int ModbusTcp::WriteRegister(int slaveId, int func, int ireg, int value)
{
   assert(func == 6);
   assert(ireg>0);
   assert(ireg<=0x10000);

   int status = Function6(slaveId, ireg-1, value);

   if (status < 0)
      return -1;

   char buf[10000];

   int expected = 12;

   int rd = Read(buf, expected);

   if (rd < 0)
      return -1;

   if (0) {
      for (int i=0; i<rd; i++)
         printf(" %02x", buf[i]&0xFF);
      printf("\n");
   }
   
   if (buf[7] != 6) {
      cm_msg(MERROR, "ModbusTcp::WriteRegister", "reg %d, modbus error code 0x%x, exception code 0x%x", ireg, 0xFF&buf[7], 0xFF&buf[8]);
      return -1;
   }
   
   if (rd != expected) {
      cm_msg(MERROR, "ModbusTcp::WriteRegister", "reg %d, bad modbus packet length %d, expected %d", ireg, rd, expected);
      return -1;
   }
   
   return SUCCESS;
}

int ModbusTcp::ReadRegister(int slaveId, int ireg)
{
   int f = ireg/1000000;
   int r = ireg%1000000;
   if (f==0)
      f = MODBUS_FUNC3;
   return ReadRegister(slaveId, f, r);
}

int ModbusTcp::ReadRegister(int slaveId, int func, int ireg)
{
   assert(func==3 || func==4);
   assert(ireg>0);
   assert(ireg<=0x10000);

   int status;
   
   if (func==3)
      status = Function3(slaveId, ireg-1, 1);
   else if (func==4)
      status = Function4(slaveId, ireg-1, 1);
   else
      assert(!"cannot happen!");
   if (status < 0)
      return -1;
   
   char buf[10000];
  
   int expected = 11;

   int rd = Read(buf, expected);
   if (rd < 0)
      return -1;
   
   if (0) {
      for (int i=0; i<rd; i++)
         printf(" %02x", buf[i]&0xFF);
      printf("\n");
   }
   
   if (buf[7] != func) {
      cm_msg(MERROR, "ModbusTcp::ReadRegister", "reg %d, modbus error code 0x%x, exception code 0x%x", ireg, 0xFF&buf[7], 0xFF&buf[8]);
      return -1;
   }
   
   if (buf[8] != 2 || rd != expected) {
      cm_msg(MERROR, "ModbusTcp::ReadRegister", "reg %d, bad modbus packet length %d, received %d, expected %d", ireg, buf[8], rd, expected);
      return -1;
   }
   
   uint16_t v16 = (buf[9]&0xFF)<<8 | (buf[10]&0xFF);
   
   return v16;
}

int ModbusTcp::ReadRegisters(int slaveId, int ireg, int numReg, uint16_t data[])
{
   int f = ireg/1000000;
   int r = ireg%1000000;
   if (f==0)
      f = MODBUS_FUNC3;
   return ReadRegisters(slaveId, f, r, numReg, data);
}

int ModbusTcp::ReadRegisters(int slaveId, int func, int ireg, int numReg, uint16_t data[])
{
   assert(func==3 || func==4);
   assert(ireg>0);
   assert(ireg+numReg<0x10000);

   for (int i=0; i<numReg; i++)
      data[i] = 0;

   int status;

   if (func==3)
      status = Function3(slaveId, ireg-1, numReg);
   else if (func==4)
      status = Function4(slaveId, ireg-1, numReg);
   else
      assert(!"cannot happen!");

   if (status < 0)
      return -1;

   char buf[10000];
   
   int expected = 9+numReg*2;
   
   int rd = Read(buf, expected);
   if (rd < 0)
      return -1;
   
   if (0) {
      for (int i=8; i<rd; i++)
         printf(" %02x", buf[i]&0xFF);
      printf("\n");
   }
   
   if (buf[7] != func) {
      cm_msg(MERROR, "ModbusTcp::ReadRegisters", "reg %d, modbus error code 0x%x, exception code 0x%x", ireg, buf[7], buf[8]);
      return -1;
   }
   
   if (buf[8] != 0xFF&(numReg*2) || rd != expected) {
      cm_msg(MERROR, "ModbusTcp::ReadRegisters", "reg %d, bad modbus packet length %d, received %d, expected %d", ireg, 0xFF&buf[8], rd, expected);
      return -1;
   }
   
   for (int i=0; i<numReg; i++)
      data[i] = (buf[9+i*2]&0xFF)<<8 | (buf[10+i*2]&0xFF);
   
   return SUCCESS;
}

int ModbusTcp::Function3(int slaveId, int firstReg, int numReg)
{
   char buf[12];
   
   // Modbus request encoding:
   
   buf[0] = 0; // transaction id MSB
   buf[1] = 1; // transaction id LSB
   buf[2] = 0; // protocol id MSB
   buf[3] = 0; // protocol id LSB
   buf[4] = 0; // packet length MSB
   buf[5] = 6; // packet length LSB
   buf[6] = slaveId; // slave id
   buf[7] = 3; // function code 3: read registers
   buf[8]  = (firstReg & 0xFF00)>>8; 
   buf[9]  = (firstReg & 0x00FF);
   buf[10] = (  numReg & 0xFF00)>>8;
   buf[11] = (  numReg & 0x00FF);
   
   return Write(buf, 12);
}

int ModbusTcp::Function4(int slaveId, int firstReg, int numReg)
{
   char buf[12];
   
   // Modbus request encoding:
   
   buf[0] = 0; // transaction id MSB
   buf[1] = 1; // transaction id LSB
   buf[2] = 0; // protocol id MSB
   buf[3] = 0; // protocol id LSB
   buf[4] = 0; // packet length MSB
   buf[5] = 6; // packet length LSB
   buf[6] = slaveId; // slave id
   buf[7] = 4; // function code 3: read registers
   buf[8]  = (firstReg & 0xFF00)>>8; 
   buf[9]  = (firstReg & 0x00FF);
   buf[10] = (  numReg & 0xFF00)>>8;
   buf[11] = (  numReg & 0x00FF);
   
   return Write(buf, 12);
}

int ModbusTcp::Function6(int slaveId, int ireg, int value)
{
   char buf[12];
   
   // Modbus request encoding:
   
   buf[0] = 0; // transaction id MSB
   buf[1] = 1; // transaction id LSB
   buf[2] = 0; // protocol id MSB
   buf[3] = 0; // protocol id LSB
   buf[4] = 0; // packet length MSB
   buf[5] = 6; // packet length LSB
   buf[6] = slaveId; // slave id
   buf[7] = 6; // function code 6 write register
   buf[8]  = ( ireg & 0xFF00)>>8; 
   buf[9]  = ( ireg & 0x00FF);
   buf[10] = (value & 0xFF00)>>8;
   buf[11] = (value & 0x00FF);
   
   return Write(buf, 12);
}

int ModbusTcp::Connect(const char *addr)
{
   char laddr[256];
   strlcpy(laddr, addr, sizeof(laddr));
   char* s = strchr(laddr,':');
   if (!s) {
      cm_msg(MERROR, "ModbusTcp::Connect", "Invalid address \'%s\': no \':\', should look like \'hostname:tcpport\'", laddr);
      return -1;
   }

   *s = 0;
   
   int port = atoi(s+1);
   if (port == 0) {
      cm_msg(MERROR, "ModbusTcp::Connect", "Invalid address: \'%s\', tcp port number is zero", laddr);
      return -1;
   }

   struct hostent *ph = gethostbyname(laddr);
   if (ph == NULL) {
      cm_msg(MERROR, "ModbusTcp::Connect", "Cannot resolve IP address for \'%s\', h_errno %d (%s)", laddr, h_errno, hstrerror(h_errno));
      return -1;
   }

   int fd = socket (AF_INET, SOCK_STREAM, 0);
   if (fd < 0) {
      cm_msg(MERROR, "ModbusTcp::Connect", "Cannot create TCP socket, socket(AF_INET, SOCK_STREAM) errno %d (%s)", errno, strerror(errno));
      return -1;
   }

   struct sockaddr_in inaddr;

   memset(&inaddr, 0, sizeof(inaddr));
   inaddr.sin_family = AF_INET;
   inaddr.sin_port = htons(port);
   memcpy((char *) &inaddr.sin_addr, ph->h_addr, 4);
  
   cm_msg(MINFO, "ModbusTcp::Connect", "Connecting to \"%s\" port %d", laddr, port);
  
   int status = connect(fd, (sockaddr*)&inaddr, sizeof(inaddr));
   if (status == -1) {
      cm_msg(MERROR, "ModbusTcp::Connect", "Cannot connect to %s:%d, connect() errno %d (%s)", laddr, port, errno, strerror(errno));
      return -1;
   }

   fSocket = fd;

   return SUCCESS;
}

int ModbusTcp::Disconnect()
{
   if (fSocket < 0)
      return SUCCESS;

   close(fSocket);
   fSocket = -1;
   return SUCCESS;
}

int ModbusTcp::Write(const char* buf, int length)
{
   if (1) {
      int count = 0;
      while (1) {
         char buf[256];
         int rd = recv(fSocket, buf, sizeof(buf), MSG_DONTWAIT);
         if (rd <= 0)
            break;
         count += rd;
      }

      if (count > 0)
         cm_msg(MINFO, "ModbusTcp::Write", "Flushed %d bytes", count);
   }
   
  if (fTrace) {
     printf("Writing %p+%d to socket %d: 0x", buf, length, fSocket);
     for (int i=0; i<length; i++)
        printf(" %02x", buf[i]&0xFF);
     printf("\n");
  }

  int wr = send(fSocket, buf, length, 0);

  if (wr != length) {
     cm_msg(MERROR, "ModbusTcp::Write", "TCP I/O error, send() returned %d, errno %d (%s)", wr, errno, strerror(errno));
     return -1;
  }

  return wr;
}

int ModbusTcp::Read(char* buf, int length)
{
   time_t t0 = time(NULL);
   int count = 0;

   while (count < length) {
      int rd = recv(fSocket, buf+count, length-count, MSG_DONTWAIT);
      //int rd = read(fSocket, buf+count, length-count);
      //printf("recv rd %d, errno %d (%s)\n", rd, errno, strerror(errno));

      if (rd == -1 && errno == EAGAIN) {
         if (time(NULL)-t0 > fReadTimeout_sec) {
            cm_msg(MERROR, "ModbusTcp::Read", "TCP connection timeout");
            return count;
         }
         ss_sleep(100);
         continue;
      } else if (rd == 0) {
         cm_msg(MERROR, "ModbusTcp::Read", "TCP connection unexpectedly closed");
         return -1;
      } else if (rd < 0) {
         cm_msg(MERROR, "ModbusTcp::Read", "TCP I/O error, read() returned %d, errno %d (%s)", rd, errno, strerror(errno));
         return -1;
      }

      count += rd;
   }

   if (fTrace) {
      printf("ModbusTcp::Read: received %d bytes: 0x", count);
      
      for (int i=0; i<count; i++)
         printf(" %02x", buf[i]&0xFF);
      printf("\n");
   }
   
   return count;
}

// end
