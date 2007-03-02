/*********************************************************************

  Name:         vppg.c
  Created by:   
        based on v292.c and trPPG.c Pierre-Andre Amaudruz

  Contents:     PPG Pulse Programmer
                
  $Id$
*********************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "unistd.h" // for sleep
#include "vppg.h"

int ddd=1; //debug
FILE  *ppginput;
/*****************************************************************/

/*------------------------------------------------------------------*/

void ppg(void)
{
   printf("       PPG function support\n");
   printf("A Init               B Load\n");
   printf("C StopSequencer      E StartSequencer\n");
   printf("F EnableExtTrig      G DisableExtTrig\n");
   printf("H ExtTrigRegRead     I StatusRead\n");
   printf("J PolmskRead         K PolmskWrite\n");
   printf("L RegWrite           M RegRead\n");
   printf("N StartpatternWrite  O PolzSet\n");
   printf("Q PolzRead           R PolzFlip\n");
   printf("S PolzCtlPPG         T PolzCtlVME\n");
   printf("U BeamOn             V BeamOff \n");
   printf("W BeamCtlPPG         Y BeamCtlRegRead\n");
   printf("D debug (toggles)    P print this list\n");
   printf("X exit \n");
   printf("\n");
}


/*------------------------------------------------------------------*/
/** ppgPolzSet
    Set Polarization bit to a given value.
    @memo Write PPG.
    @param base\_adr PPG VME base addroless
    @param value (8bit)
*/
void VPPGPolzSet(MVME_INTERFACE *mvme, const DWORD base_adr, BYTE value)
{
   VPPGRegWrite(mvme, base_adr,  VPPG_VME_POLZ_SET, value); 
   return;
}

/*------------------------------------------------------------------*/
/** ppgPolzRead
    Read Polarization bit.
    @memo Read PPG.
    @param base\_adr PPG VME base addroless
    @return value (8bit)
*/
BYTE  VPPGPolzRead(MVME_INTERFACE *mvme, const DWORD base_adr)
{
  DWORD value;
  value = VPPGRegRead(mvme, base_adr, VPPG_VME_POLZ_SET);
  value &= 0xFF;

  if (ddd) printf("Read addr 0x%lx  data=%lu\n", (DWORD)(base_adr +VPPG_VME_POLZ_SET),value  );

  return (BYTE)value;
}

/*------------------------------------------------------------------*/
/** VPPGPolzFlip
    Flip the polarization bit.
    @memo Read PPG.
    @param base\_adr PPG VME base address
*/
BYTE  VPPGPolzFlip(MVME_INTERFACE *mvme, const DWORD base_adr)
{
  DWORD value;

  value = (0xFF & (VPPGPolzRead(mvme, base_adr)));
 
  if(ddd)printf("VPPGPolzFlip: read back %lu, now flipping \n",value);
  if(value) {
    VPPGPolzSet(mvme, base_adr, 0);
  }  
  else {
    VPPGPolzSet(mvme, base_adr, 1);
  }
  value = (0xFF & (VPPGPolzRead(mvme, base_adr)));
  if(ddd)printf("VPPGPolzFlip: after flip, read back %lu \n",value); 

 

  return (BYTE)value;
}

/*------------------------------------------------------------------*/
/** VPPGRegWrite
    Write into PPG register.
    @memo Write PPG.
    @param base\_adr PPG VME base address
    @param reg\_offset PPG register
    @param value (8bit)
    @return status register
*/
BYTE VPPGRegWrite(MVME_INTERFACE *mvme, const DWORD base_adr, DWORD reg_offset, BYTE value)
{

  DWORD myval,myreg;
  int  cmode, localam;
  
  mvme_get_dmode(mvme, &cmode);  // remember present VME data mode
  mvme_set_dmode(mvme, MVME_DMODE_D8); // set D8 for PPG

  mvme_get_am(mvme, &localam); // remember present VME addressing mode
  mvme_set_am(mvme, MVME_AM_A16_ND); // set A16 for PPG

  myreg =  base_adr + reg_offset;
  myval = (DWORD)value;
  mvme_write_value(mvme, myreg, value);
  if (ddd) printf("Writing 0x%x to 0x%lx\n", value, myreg );
  myval = 0x1BAD1BAD; // set to a different value
  myval = mvme_read_value(mvme, myreg);
  if(ddd)printf("mvme_read_value returns 0x%lx (%lu)\n",myval,myval);

  mvme_set_dmode(mvme, cmode); // restore VME data mode
  mvme_set_am(mvme, localam); // restore VME addressing mode

  return (BYTE)myval;
}

/*------------------------------------------------------------------*/
/** VPPGRegRead
    Read PPG register.
    @memo Read PPG.
    @param base\_adr PPG VME base addroless
    @param reg\_offset PPG register
    @return status register (8 bit)
*/
BYTE VPPGRegRead(MVME_INTERFACE *mvme, const DWORD base_adr, DWORD reg_offset)
{
  DWORD value,myreg;
  int  cmode, localam;
  
  mvme_get_dmode(mvme, &cmode); // store present data mode
  mvme_set_dmode(mvme, MVME_DMODE_D8); // set D8 for PPG

  mvme_get_am(mvme, &localam); // store present addressing mode
  mvme_set_am(mvme, MVME_AM_A16_ND); // set A16 for PPG

  myreg =  base_adr + reg_offset;
  if (ddd) printf("Reading from address 0x%lx\n", myreg );
  value = mvme_read_value(mvme, myreg );
  printf("Read back 0x%lx (%lu)\n",value,value);

  mvme_set_dmode(mvme, cmode);// restore data mode
  mvme_set_am(mvme, localam);// restore addressing mode
  return (BYTE) value;
}

/*------------------------------------------------------------------*/
/** VPPGInit
    Initialize the PPG
    @memo Initialize PPG
    @param base\_adr PPG VME base address
    @return void
*/
void VPPGInit(MVME_INTERFACE *mvme, const DWORD base_adr)
{
  VPPGPolmskWrite(mvme, base_adr, VPPG_DEFAULT_PPG_POL_MSK );

  //by default PPG controls these outputs (Titan)
  VPPGPolzCtlPPG(mvme, base_adr);
  VPPGBeamCtlPPG(mvme, base_adr);

  // default.. external trigger input disabled 
  VPPGDisableExtTrig(mvme, base_adr); 

  return;
}

/*------------------------------------------------------------------*/
/** VPPGStatusRead
    Read Status register.
    @memo Read status.
    @param base\_adr PPG VME base address
    @return status register
*/
BYTE VPPGStatusRead(MVME_INTERFACE *mvme, const DWORD base_adr)
{
  BYTE value,itmp;
  value = VPPGRegRead(mvme, base_adr, VPPG_VME_READ_STAT_REG );
    if (value & 1)
      printf("Pulseblaster IS reset\n");
    else
      printf("Pulseblaster is NOT  reset\n");
   if (value & 2)
     printf("Pulseblaster IS running \n");
   else
     printf("Pulseblaster NOT running \n");
   if (value & 4)
     printf("Pulseblaster IS stopped \n");
   else
     printf("Pulseblaster is NOT stopped \n");

   itmp =  (value & 0x18)>> 4;
     printf("VME polarization source control bits = 0x%x\n",itmp);

   if (value & 20)
     printf("VME Polarization control: ACTIVE \n");
   else
     printf("VME Polarization control: OFF \n");
   if (value & 40)
     printf("External clock IS present \n");
   else
     printf("External clock NOT present \n");

   if (value & 80)
     printf("External  Polarization control: ACTIVE \n");
   else
     printf("External  Polarization control: OFF \n");

  return (BYTE)value;
}
/*------------------------------------------------------------------*/
/** VPPGBeamOn
    Directly set Beam On signal.
    @memo Set Beam On (independent of ppg script)
    @param base\_adr PPG VME base address
    @return void
*/
  void VPPGBeamOn(MVME_INTERFACE *mvme, const DWORD base_adr)
{
 
  VPPGRegWrite(mvme, base_adr,VPPG_VME_BEAM_CTL , 1);
   /* channel 14 (beam on/off) set to ON; ignores ppg pulseblaster script */
  return;
}


/*------------------------------------------------------------------*/
/** VPPGBeamOff
    Directly set Beam Off signal.
    @memo Set Beam Off (independent of PPG script)
    @param base\_adr PPG VME base address
    @return void
*/
  void VPPGBeamOff(MVME_INTERFACE *mvme, const DWORD base_adr)
{
  VPPGRegWrite(mvme, base_adr,VPPG_VME_BEAM_CTL , 0);
 /* channel 14 (beam on/off) set to OFF; ignores ppg pulseblaster script */
  return;
}

/*------------------------------------------------------------------*/
/** VPPGBeamCtlPPG
    PPG controls the Beam On/Off signal.
    @memo Give PPG script control of Beam On/Off signal (ch14)
    @param base\_adr PPG VME base address
    @return void
*/
  void VPPGBeamCtlPPG(MVME_INTERFACE *mvme, const DWORD base_adr)
{
  VPPGRegWrite(mvme, base_adr,VPPG_VME_BEAM_CTL , 3);
 /* channel 14 (beam on/off) follows ppg pulseblaster script */ 
  return;
}

/*------------------------------------------------------------------*/
/** VPPGPolzCtlVME
    VME controls the Pol On/Off signal for helicity
    @memo Give VME control of Pol On/Off signal (DRV POL) (PPG script ignored) 
    @param base\_adr PPG VME base address
    @return void
*/
  void VPPGPolzCtlVME(MVME_INTERFACE *mvme, const DWORD base_adr)
{
  VPPGRegWrite(mvme, base_adr, VPPG_POLZ_SOURCE_CONTROL , 0);
 /* DRV POL (helicity) independent of ppg pulseblaster script */ 
  return;
}
/*------------------------------------------------------------------*/
/** VPPGPolzCtlPPG
    PPG controls the Pol On/Off signal for helicity
    @memo Give PPG script control of Pol On/Off signal (DRV POL) which now follows ch15  
    @param base\_adr PPG VME base address
    @return void
*/
  void VPPGPolzCtlPPG(MVME_INTERFACE *mvme, const DWORD base_adr)
{
  VPPGRegWrite(mvme, base_adr, VPPG_POLZ_SOURCE_CONTROL , 2);
 /* DRV POL (helicity) follows ch15 and ppg pulseblaster script */ 
  return;
}
/*------------------------------------------------------------------*/
/** VPPGBeamCtlRegRead
    Read the Beam Control Register
    @memo Read the Beam Control Register
    @param base\_adr PPG VME base address
    @return void
*/
BYTE VPPGBeamCtlRegRead(MVME_INTERFACE *mvme, const DWORD base_adr)
{
  BYTE data;
  data =  (0xFF &  (VPPGRegRead(mvme, base_adr,  VPPG_VME_BEAM_CTL)));
  if(ddd)printf("VPPGBeamCtlRegRead: read back %d \n",data);
  if(data == 0)
    printf("Beam off independent of PPG script\n");
  else if(data == 1)
    printf("Beam on independent of PPG script\n");
  else
    printf("PPG script controls Beam output\n");
  return(data);
}

/*------------------------------------------------------------------*/
/** VPPGStartSequencer
    Start the PPG sequencer (internal trigger)
    @memo start the PPG sequencer.
    @param base\_adr PPG VME base address
    @return void
*/
  void VPPGStartSequencer(MVME_INTERFACE *mvme, const DWORD base_adr)
{

  VPPGPolmskWrite(mvme, base_adr, VPPG_DEFAULT_PPG_POL_MSK);
  VPPGRegWrite(mvme, base_adr, VPPG_PPG_START_TRIGGER  , 0);
  return;
}

/*------------------------------------------------------------------*/
/** VPPGStopSequencer
    Stop the PPG sequencer.
    @memo Stop the PPG sequencer.
    @param base\_adr PPG VME base address
    @return void
*/
  void VPPGStopSequencer(MVME_INTERFACE *mvme, const DWORD base_adr)
{
  VPPGRegWrite(mvme, base_adr,  VPPG_VME_RESET  , 0);
  return;
}


/*------------------------------------------------------------------*/
/**  VPPGEnableExtTrig(ppg_base)
    Enable front panel trigger input so external inputs can start the sequence
    @memo Enable external trigger  input so external inputs can start the sequence
    @param base\_adr PPG VME base address
    @return data
*/
BYTE  VPPGEnableExtTrig(MVME_INTERFACE *mvme, const DWORD base_adr)
{
  BYTE data;
  data = VPPGRegWrite(mvme, base_adr,  VPPG_VME_TRIG_CTL  , 0);
   return data;
}



/*------------------------------------------------------------------*/
/**  VPPGDisableExtTrig(ppg_base)
    Disable front panel trigger input so external inputs cannot start the sequence
    @memo Disable external trigger  input so external inputs cannot start the sequence
    @param base\_adr PPG VME base address
    @return data
*/
BYTE VPPGDisableExtTrig(MVME_INTERFACE *mvme, const DWORD base_adr)
{
  return VPPGRegWrite(mvme, base_adr,  VPPG_VME_TRIG_CTL  , 1);
}


/*------------------------------------------------------------------*/
/** VPPGExtTrigRegRead
    Read external trig register (bit 0 int/ext trigger is enabled bit 1 trigger active/inactive)
    @memo Read PPG.
    @param base\_adr PPG VME base addroless
    @return value (8bit)
*/
  BYTE  VPPGExtTrigRegRead(MVME_INTERFACE *mvme, const DWORD base_adr)
{
  BYTE data;
  data = (BYTE) VPPGRegRead(mvme, base_adr,  VPPG_VME_TRIG_CTL);
  if(data == 0)
    printf("External trigger is ENABLED\n");
  else
    printf("External trigger is DISABLED\n");
  return(data);
}

/*------------------------------------------------------------------*/
/** VPPG\_polmsk\_write
    Write the Polarity mask.
    @memo Write and read back polarity mask.
    @param base\_adr PPG VME base address
    @param pol polarity (24bit)
    @return polarity (24bit)
*/
  DWORD VPPGPolmskWrite(MVME_INTERFACE *mvme, const DWORD base_adr, const DWORD pol)
{
  BYTE temp;

  temp  = *((BYTE *) &pol + 2);
  if(ddd)printf("write high bits : 0x%x\n",temp);
  VPPGRegWrite(mvme, base_adr, VPPG_OUTP_POL_MASK_HI, temp); 

  temp  = *((BYTE *) &pol + 1);
  if(ddd)printf("write mid bits : 0x%x\n",temp);
  VPPGRegWrite(mvme, base_adr, VPPG_OUTP_POL_MASK_MID, temp); 

  temp  = *((BYTE *) &pol + 0);
  if(ddd)printf("write low bits : 0x%x\n",temp);
  VPPGRegWrite(mvme, base_adr, VPPG_OUTP_POL_MASK_LO, temp); 

  return VPPGPolmskRead(mvme,base_adr);
}

/*------------------------------------------------------------------*/
/** VPPGPolmskRead 
    Read the Polarity mask.
    @memo Read polarity mask.
    @param base\_adr PPG VME base address
    @return polarity (24bit)
*/
  DWORD VPPGPolmskRead(MVME_INTERFACE *mvme, const DWORD base_adr)
{
  BYTE temp;
  DWORD  pol;
  pol = 0;
  temp = (BYTE)(VPPGRegRead(mvme, base_adr,   VPPG_OUTP_POL_MASK_HI));
  if(ddd)printf("read high bits : 0x%x\n",temp);
  pol |= (temp & 0xFF) << 16;
  if(ddd)printf("pol =  0x%lx\n",pol);

  temp =(BYTE)(VPPGRegRead(mvme, base_adr,   VPPG_OUTP_POL_MASK_MID));
  if(ddd)printf("read mid bits : 0x%x\n",temp);
  pol |= (temp & 0xFF) <<  8;
  if(ddd)printf("pol =  0x%lx\n",pol);

  temp =(BYTE)(VPPGRegRead(mvme, base_adr,   VPPG_OUTP_POL_MASK_LO));
  if(ddd)printf("read low bits : 0x%x\n",temp);
  pol |= (temp & 0xFF);
  printf("pol =  0x%lx\n",pol);

  return pol;
}

/*------------------------------------------------------------------*/
/** ppg\_startpattern\_write    ?????????????????????
    Write the Start(Power up or Reset)Pattern.
    @memo Write and read back startup pattern.
    @param base\_adr PPG VME base address
    @param pol polarity (24bit)
    @return polarity (24bit)
*/
  DWORD VPPGStartpatternWrite(MVME_INTERFACE *mvme, const DWORD base_adr, 
			     const DWORD pol)
{
  
  VPPGRegWrite(mvme, base_adr, VPPG_PPG_RESET_REG,0);
  VPPGRegWrite(mvme, base_adr, VPPG_BYTES_PER_WORD,0x0A);
  VPPGRegWrite(mvme, base_adr, VPPG_TOGL_MEM_DEVICE,0);
  VPPGRegWrite(mvme, base_adr, VPPG_CLEAR_ADDR_COUNTER,0x55);
  VPPGRegWrite(mvme, base_adr, VPPG_PROGRAMMING_FIN,7);
  VPPGRegWrite(mvme, base_adr, VPPG_PPG_START_TRIGGER,7);
  VPPGRegWrite(mvme, base_adr, VPPG_PPG_RESET_REG,1);
  VPPGRegWrite(mvme, base_adr, VPPG_BYTES_PER_WORD,3);
  VPPGRegWrite(mvme, base_adr, VPPG_CLEAR_ADDR_COUNTER,0);
  VPPGRegWrite(mvme, base_adr, VPPG_LOAD_MEM,*((BYTE *) &pol + 1));
  VPPGRegWrite(mvme, base_adr, VPPG_LOAD_MEM,*((BYTE *) &pol + 2));
  VPPGRegWrite(mvme, base_adr, VPPG_LOAD_MEM,*((BYTE *) &pol + 3));
  VPPGRegWrite(mvme, base_adr,0x05,0);
  VPPGRegWrite(mvme, base_adr,0x05,0);
  return VPPGPolmskRead(mvme, base_adr);
}
/*------------------------------------------------------------------*/
/** byteOutputOrder
    byte swap for output
    @memo byte swap for PPG.
    @param data 
    @param array 
    @return void
*/
  void byteOutputOrder(PARAM data, char *array)
{
#define LOW_MASK 	0x000000FF
#define LOW_MID_MASK 	0x0000FF00
#define HIGH_MID_MASK 	0x00FF0000
#define HIGH_MASK 	0xFF000000
#define BRANCH1_MASK	0X000FF000
#define BRANCH2_MASK	0X00000FF0
#define BRANCH3_MASK	0X0000000F
  
  //unsigned long total_bits;
  //unsigned int *test;
  
  /*total_bits = data.opcode_width + data.branch_width + data.delay_width  */
  /*			 + data.flag_width;            */
  
  array[0] = (char)( (data.flags & HIGH_MID_MASK) >> 16);
  array[1] = (char)( (data.flags & LOW_MID_MASK) >> 8);
  array[2] = (char)( (data.flags & LOW_MASK));
  
  if (ddd) printf("branch = %lx\n",data.branch_addr);

  array[3] = (char)( (data.branch_addr & BRANCH1_MASK) >> 12);
  array[4] = (char)( (data.branch_addr & BRANCH2_MASK) >> 4);
  array[5] = (char)((data.branch_addr & BRANCH3_MASK) << 4);
  array[5] = array[5] | (char)(data.opcode & BRANCH3_MASK);
  
  array[6] = (char)( (data.delay & HIGH_MASK) >> 24);
  array[7] = (char)( (data.delay & HIGH_MID_MASK) >> 16);
  array[8] = (char)( (data.delay & LOW_MID_MASK) >> 8);
  array[9] = (char)( (data.delay & LOW_MASK));
}

/*------------------------------------------------------------------*/
/** lineRead
    Read line of input file
    @memo read line.
    @param *input FILE pointer
    @return PARAM data structure
*/
  PARAM lineRead(FILE *infile)
{
  PARAM data_struct;
  unsigned int temp;
  //  char line[128];

  /*
  fgets(line, 128, infile);
  printf("line:%s\n", line);
  */
  fscanf(infile,"%x",&temp);
  data_struct.opcode = (char)temp;
  
  fscanf(infile,"%lx",&(data_struct.branch_addr));
  fscanf(infile,"%lx",&(data_struct.delay));
  fscanf(infile,"%lx",&(data_struct.flags));

  return(data_struct);
}

/*------------------------------------------------------------------*/
/** ppgLoad
    Load PPG file into sequencer.
    @memo Load file PPG.
    @param base\_adr PPG VME base address
    @return 1=SUCCESS, -1=file not found
*/
int VPPGLoad(MVME_INTERFACE *mvme, const DWORD base_adr, char *file)
{
  /*  Local Variables  */
  long    j,i;
  int 	index = 0;
  char 	text[100];
  char  num_instructions;
  char  flag_size;
  char  delay_size;
  char  branch_size;
  char  opcode_size;
  unsigned char array[12];
  PARAM  command_info;
  unsigned int 	temp;
  unsigned char result;
  //  FILE  *ppginput;
  unsigned short port_address;
  
  printf("Opening file: %s   ...  ",file);
  ppginput = fopen(file,"r");
  sleep(1) ; // sleep 1s
  if(ppginput == NULL){
    printf("ppgLoad: Byte code file %s could not be opened. [%p]\n", file, ppginput);
    printf("  If file is present, problem may be too many rpc processes.  Reboot ppc and retry\n");
    return -1;
  }
  
  fscanf(ppginput,"%s",text);
  index = strcmp(text,"Op_Code");
  if(index == 0){
    fscanf(ppginput,"%s",text);
    index = strcmp(text,"Size");
    if(index == 0){
      fscanf(ppginput,"%s",text);
    }
    else {
      printf("ppgLoad: Input file has wrong format. Aborting procedure.\n");
      return -1;
    }
  }
  opcode_size = (char)atol(text);
  
  fscanf(ppginput,"%s",text);
  index = strcmp(text,"Branch");
  if(index == 0){
    fscanf(ppginput,"%s",text);
    index = strcmp(text,"Size");
    if(index == 0){
      fscanf(ppginput,"%s",text);
    }
    else {
      printf("ppgLoad: Input file has wrong format. Aborting procedure.\n");
      return -1;
    }
  }
  branch_size = (char)atol(text);
  
  fscanf(ppginput,"%s",text);
  index = strcmp(text,"Delay");
  if(index == 0){
    fscanf(ppginput,"%s",text);
    index = strcmp(text,"Size");
    if(index == 0){
      fscanf(ppginput,"%s",text);
    }
    else {
      printf("ppgLoad: Input file has wrong format. Aborting procedure.\n");
      return -1;
    }
  }
  delay_size = (char)atol(text);
  
  fscanf(ppginput,"%s",text);
  index = strcmp(text,"Flag");
  if(index == 0){
    fscanf(ppginput,"%s",text);
    index = strcmp(text,"Size");
    if(index == 0){
      fscanf(ppginput,"%s",text);
    }
    else {
      printf("ppgLoad: Input file has wrong format. Aborting procedure.\n");
      return -1;
    }
  }
  flag_size = (char)atol(text);
  
  fscanf(ppginput,"%s",text);
  index = strcmp(text,"Instruction");
  if(index == 0){
    fscanf(ppginput,"%s",text);
    index = strcmp(text,"Lines");
    if(index == 0){
      fscanf(ppginput,"%s",text);
    }
    else {
      printf("ppgLoad: Input file has wrong format. Aborting procedure.\n");
      return -1;
    }
  }
  num_instructions = (char)atol(text);
  if (ddd) printf("instruction lines = %d\n",num_instructions);
  
  fscanf(ppginput,"%s",text);
  index = strcmp(text,"Port");
  if(index == 0){
    fscanf(ppginput,"%s",text);
    index = strcmp(text,"Address");
    if(index == 0){
      fscanf(ppginput,"%hx",&port_address);
    }
    else {
      printf("ppgLoad: Input file has wrong format. Aborting procedure.\n");
      return -1;
    }
  }
  if (ddd) printf("Base Address = %lx\n",base_adr);
  
  if (ddd) printf("Reading byte code file...\n");
  
  
  command_info.opcode_width = opcode_size;
  command_info.branch_width = branch_size;
  command_info.delay_width = delay_size;
  command_info.flag_width = flag_size;
  
  /* init ISA BUS Controller  */
  VPPGRegWrite(mvme, base_adr, VPPG_PPG_RESET_REG,0x00);
  VPPGRegWrite(mvme, base_adr, VPPG_BYTES_PER_WORD,0x0A);
  VPPGRegWrite(mvme, base_adr, VPPG_TOGL_MEM_DEVICE,0x00);/*Is this still active?*/
  VPPGRegWrite(mvme, base_adr, VPPG_CLEAR_ADDR_COUNTER,0x00);
  
  for(i=0;i<num_instructions;i++){
    command_info = lineRead(ppginput);
    if (ddd)
      {
	printf("%1x ",command_info.opcode);
	printf("%5lx ",command_info.branch_addr);
	printf("%8lx ",command_info.delay);
	printf("%6lx ",command_info.flags);
	printf("\n");
      }
    byteOutputOrder(command_info,array);
    
    for(j=0;j<10;j++){
      result = array[j];
      temp = (unsigned int) result;
      if (ddd) printf("%x ",temp);
    }
    if (ddd) printf("\n");
    
    for(j=0;j<10;j++)
      {
	VPPGRegWrite(mvme, base_adr, VPPG_LOAD_MEM, array[j]);
      }
  }
  
  /* arm controller        */
  VPPGRegWrite(mvme, base_adr, VPPG_LOAD_MEM,0x00);/*writes the stop command */
  VPPGRegWrite(mvme, base_adr, VPPG_LOAD_MEM,0x00);
  VPPGRegWrite(mvme, base_adr, VPPG_LOAD_MEM,0x00);
  VPPGRegWrite(mvme, base_adr, VPPG_LOAD_MEM,0x00);
  VPPGRegWrite(mvme, base_adr, VPPG_LOAD_MEM,0x00);
  VPPGRegWrite(mvme, base_adr, VPPG_LOAD_MEM,0x01);
  VPPGRegWrite(mvme, base_adr, VPPG_LOAD_MEM,0x00);
  VPPGRegWrite(mvme, base_adr, VPPG_LOAD_MEM,0x00);
  VPPGRegWrite(mvme, base_adr, VPPG_LOAD_MEM,0x00);
  VPPGRegWrite(mvme, base_adr, VPPG_LOAD_MEM,0x02);   /*End of stop command     */
  
  
  VPPGRegWrite(mvme, base_adr, VPPG_PROGRAMMING_FIN,0x00);
  fclose(ppginput);
  if(ddd) printf("Programming ended, controller armed");
  return 1;
}

/*****************************************************************/
/*-PAA- For test purpose only */
#ifdef MAIN_ENABLE
int main (int argc, char* argv[]) {


  DWORD PPG_BASE = 0x8000;
  
  MVME_INTERFACE *mvme;

  int status;
  DWORD ival;
  BYTE data,value;
  char cmd[]="hallo";
  int s;
  char filename[128];

  DWORD rpol,pol,reg_offset;

  if (argc>1) {
    sscanf(argv[1],"%lx",&PPG_BASE);
  }

  // Test under vmic   
  status = mvme_open(&mvme, 0);
  if(status != SUCCESS)
    {
      printf("failure after mvme_open, status = %d\n",status);
      return status;
    }

  // Set am to A16 non-privileged Data
  // Set in the read/write functions
  //  mvme_set_am(mvme, MVME_AM_A16_ND);
  // Set dmode to D8
  //  mvme_set_dmode(mvme, MVME_DMODE_D8);

  VPPGStatusRead(mvme,PPG_BASE);
  data = VPPGExtTrigRegRead(mvme, PPG_BASE);
  printf("External trigger register = 0x%x\n",data);



  ppg();
  while ( isalpha(cmd[0]) )
    {
      printf("\nEnter command (A-Y) X to exit?  ");
      scanf("%s",cmd);
      //  printf("cmd=%s\n",cmd);
      cmd[0]=toupper(cmd[0]);
      s=cmd[0];
      
      switch(s)
	{
	case ('A'):
	  VPPGInit(mvme, PPG_BASE);
	  break;
	case ('B'):
	  printf("Enter PPG filename : ");
	  scanf("%s",filename);
	  VPPGLoad(mvme, PPG_BASE, filename);
	  break;
	case ('C'):
	  VPPGStopSequencer(mvme, PPG_BASE);
	  break;
	case ('E'):
	  VPPGStartSequencer(mvme, PPG_BASE);
	  break;
	case ('F'):
	  data = VPPGEnableExtTrig(mvme, PPG_BASE);
	  printf("Read back data = %d  expect 0 \n",data);
	  break;
	case ('G'):
	  data = VPPGDisableExtTrig(mvme, PPG_BASE);
	  printf("Read back data = %d expect 1 \n",data);
	  break;
	case ('H'):
	  data=VPPGExtTrigRegRead(mvme, PPG_BASE);
	  printf("Read back data =0x%x\n",data);
	  break;
	case ('I'):
	  data = VPPGStatusRead(mvme, PPG_BASE);
	  printf("Read back data =0x%x\n",data);
	  break;
	case ('J'):
	  rpol = VPPGPolmskRead(mvme, PPG_BASE);
	  printf("Read back Pol Mask=0x%lx\n",rpol);
	  break;
	case ('K'):
	  printf("Enter Mask value to write :0x");
	  scanf("%lx",&pol);
	  printf("Writing Pol Mask=0x%lx\n",pol);
	  rpol = VPPGPolmskWrite(mvme, PPG_BASE, pol);
	  printf("Read back Pol Mask=0x%lx\n",rpol);
	  break;
	case ('L'):
	  printf("Enter PPG register offset : 0x");
	  scanf("%lx",&reg_offset);
	  printf("Enter data to write: 0x");
	  scanf("%lx",&ival);
	  value=ival & 0xFF;
	  reg_offset = reg_offset & 0xFF;
	  printf("Writing 0x%x (%d) to register offset 0x%lx (%lu)\n",
		 value,value,reg_offset,reg_offset);
	  data=VPPGRegWrite(mvme, PPG_BASE, reg_offset, value);
	  printf("Read back from offset 0x%lx (%lu)  data = 0x%x (%d)\n",
		 reg_offset,reg_offset,data,data);
	  break;
	case ('M'):
	  printf("Enter PPG register offset : 0x");
	  scanf("%lx",&reg_offset);
	  reg_offset = reg_offset & 0xFF;

	  data=VPPGRegRead(mvme, PPG_BASE, reg_offset);
	  printf("Read back from offset 0x%lx (%lu)    data = 0x%x (%d)\n ",
		 reg_offset,reg_offset,data,data);
	  break;
	case ('N'):
	  printf("Enter Pol Mask : 0x");
	  scanf("%lx",&ival);
	  VPPGStartpatternWrite(mvme, PPG_BASE, ival);
	  break;
	case ('O'):
	  printf("Enter value to set (0 or 1):\n");
	  scanf("%lx",&ival);
	  value = ival && 0xFF;
	  VPPGPolzSet(mvme, PPG_BASE, value);
	  break;
	case ('Q'):
	  data = VPPGPolzRead(mvme, PPG_BASE);
	  printf("data = %d\n",data);
	  break;
	case ('R'):
	  data = VPPGPolzFlip(mvme, PPG_BASE);
	  printf("read back = %d\n",data);
	  break;
	case ('S'):
	  VPPGPolzCtlPPG(mvme, PPG_BASE);
	  break;
	case ('T'):
	  VPPGPolzCtlVME(mvme, PPG_BASE);
	  break;
	case ('U'):
	  VPPGBeamOn(mvme, PPG_BASE);
	  break;
	case ('V'):
	  VPPGBeamOff(mvme, PPG_BASE);
	  break;
	case ('W'):
	  VPPGBeamCtlPPG(mvme, PPG_BASE);
	  break;
	case ('Y'):
	  data=VPPGBeamCtlRegRead(mvme, PPG_BASE);
	  printf("Read back data =0x%x\n",data);
	  break;
	case ('D'):
	  if (ddd)
	    ddd=0;
	  else
	    ddd=1;
	  break;
	case ('P'):
	  ppg();
	  break;
	case ('X'):
	  return 1;
	default:
	  break;
	}
    }

  /*
  VPPGStatusRead(mvme,PPG_BASE);
  data = VPPGExtTrigRegRead(mvme, PPG_BASE);
  printf("External trigger register = 0x%x\n",data);

  printf("\nmain: calling VPPGInit; sets POL mask=0x%x and VME controls hel\n",
	 VPPG_DEFAULT_PPG_POL_MSK);
  VPPGInit(mvme, PPG_BASE);
  VPPGStatusRead(mvme,PPG_BASE);
  data = VPPGExtTrigRegRead(mvme, PPG_BASE);
  printf("External trigger register = 0x%x\n",data);
  */

  status = mvme_close(mvme);
  return 1;
}	
#endif

