/** WIENER SNMP basic SNMP library to Demonstrate C-Access to WIENER-Crates via SNMP
// modified for LabView imprt 04/23/06, Andreas Ruben
//
// The path to the Net-SNMP include files (default /usr/include) must be added to the 
// include file search path!
// The following libraries must be included:
// netsnmp.lib ws2_32.lib
// The path to the Net-SNMP library must be added to the linker files.
// /usr/lib
// path for the WIENER MIB file (mibdirs) c:/usr/share/snmp/mibs
*/
extern "C" {

#include "WIENER_SNMP.h"

const char* readCommunity = "public";       ///< community name for read operations
const char* writeCommunity = "guru";        ///< community name for write operation

// System Information OIDS
const char *S_sysMainSwitch = "sysMainSwitch.0";
oid oidSysMainSwitch [MAX_OID_LEN];
size_t lengthSysMainSwitch;

const char *S_sysStatus = "sysStatus.0";
oid oidSysStatus [MAX_OID_LEN];
size_t lengthSysStatus;

const char *S_sysVmeSysReset = "sysVmeSysReset.0";
oid oidSysVmeSysReset [MAX_OID_LEN];
size_t lengthSysVmeSysReset;


//Output Count Information
const char *S_outputNumber = "outputNumber.0";
oid oidOutputNumber [MAX_OID_LEN];
size_t lengthOutputNumber;

const char *S_groupsNumber = "groupsNumber.0";
oid oidGroupsNumber [MAX_OID_LEN];
size_t lengthGroupsNumber;


//Ouput Channel information
const char *S_outputNameBase = "outputName.";
char *S_outputName[320];
oid oidOutputName[320] [MAX_OID_LEN];
size_t lengthOutputName[320];

const char *S_outputGroupBase = "outputGroup.";
char *S_outputGroup[320];
oid oidOutputGroup[320] [MAX_OID_LEN];
size_t lengthOutputGroup[320];

const char *S_outputStatusBase = "outputStatus.";
char *S_outputStatus[320];
oid oidOutputStatus[320] [MAX_OID_LEN];
size_t lengthOutputStatus[320];

const char *S_outputMeasurementSenseVoltageBase = "outputMeasurementSenseVoltage.";
char *S_outputMeasurementSenseVoltage[320];
oid oidOutputMeasurementSenseVoltage[320] [MAX_OID_LEN];
size_t lengthOutputMeasurementSenseVoltage[320];

const char *S_outputMeasurementTerminalVoltageBase = "outputMeasurementTerminalVoltage.";
char *S_outputMeasurementTerminalVoltage[320];
oid oidOutputMeasurementTerminalVoltage[320] [MAX_OID_LEN];
size_t lengthOutputMeasurementTerminalVoltage[320];

const char *S_outputMeasurementCurrentBase ="outputMeasurementCurrent.";
char *S_outputMeasurementCurrent[320];
oid oidOutputMeasurementCurrent[320] [MAX_OID_LEN];
size_t lengthOutputMeasurementCurrent[320];

const char *S_outputMeasurementTemperatureBase ="outputMeasurementTemperature.";
char *S_outputMeasurementTemperature[320];
oid oidOutputMeasurementTemperature[320] [MAX_OID_LEN];
size_t lengthOutputMeasurementTemperature[320];

const char *S_outputSwitchBase = "outputSwitch.";
char *S_outputSwitch[320];
oid oidChannelSwitch[320] [MAX_OID_LEN];
size_t lengthChannelSwitch[320];

const char *S_outputVoltageBase ="outputVoltage.";
char *S_outputVoltage[320];
oid oidOutputVoltage[320] [MAX_OID_LEN];
size_t lengthOutputVoltage[320];

const char *S_outputCurrentBase = "outputCurrent.";
char *S_outputCurrent[320];
oid oidOutputCurrent[320] [MAX_OID_LEN];
size_t lengthOutputCurrent[320];

const char *S_outputRiseRateBase ="outputVoltageRiseRate.";
char *S_outputRiseRate[320];
oid oidOutputRiseRate[320] [MAX_OID_LEN];
size_t lengthOutputRiseRate[320];

const char *S_outputFallRateBase ="outputVoltageFallRate.";
char *S_outputFallRate[320];
oid oidOutputFallRate[320] [MAX_OID_LEN];
size_t lengthOutputFallRate[320];

const char *S_outputSupervisionBehaviorBase ="outputSupervisionBehavior.";
char *S_outputSupervisionBehavior[320];
oid oidOutputSupervisionBehavior[320] [MAX_OID_LEN];
size_t lengthOutputSupervisionBehavior[320];

const char *S_outputSupervisionMinSenseVoltageBase ="outputSupervisionMinSenseVoltage.";
char *S_outputSupervisionMinSenseVoltage[320];
oid oidOutputSupervisionMinSenseVoltage[320] [MAX_OID_LEN];
size_t lengthOutputSupervisionMinSenseVoltage[320];

const char *S_outputSupervisionMaxSenseVoltageBase ="outputSupervisionMaxSenseVoltage.";
char *S_outputSupervisionMaxSenseVoltage[320];
oid oidOutputSupervisionMaxSenseVoltage[320] [MAX_OID_LEN];
size_t lengthOutputSupervisionMaxSenseVoltage[320];

const char *S_outputSupervisionMaxTerminalVoltageBase ="outputSupervisionMaxTerminalVoltage.";
char *S_outputSupervisionMaxTerminalVoltage[320];
oid oidOutputSupervisionMaxTerminalVoltage[320] [MAX_OID_LEN];
size_t lengthOutputSupervisionMaxTerminalVoltage[320];

const char *S_outputSupervisionMaxCurrentBase ="outputSupervisionMaxCurrent.";
char *S_outputSupervisionMaxCurrent[320];
oid oidOutputSupervisionMaxCurrent[320] [MAX_OID_LEN];
size_t lengthOutputSupervisionMaxCurrent[320];

const char *S_outputSupervisionMaxTemperatureBase ="outputSupervisionMaxTemperature.";
char *S_outputSupervisionMaxTemperature[320];
oid oidOutputSupervisionMaxTemperature[320] [MAX_OID_LEN];
size_t lengthOutputSupervisionMaxTemperature[320];

const char *S_outputConfigMaxSenseVoltageBase ="outputConfigMaxSenseVoltage.";
char *S_outputConfigMaxSenseVoltage[320];
oid oidOutputConfigMaxSenseVoltage[320] [MAX_OID_LEN];
size_t lengthOutputConfigMaxSenseVoltage[320];

const char *S_outputConfigMaxTerminalVoltageBase ="outputConfigMaxTerminalVoltage.";
char *S_outputConfigMaxTerminalVoltage[320];
oid oidOutputConfigMaxTerminalVoltage[320] [MAX_OID_LEN];
size_t lengthOutputConfigMaxTerminalVoltage[320];

const char *S_outputConfigMaxCurrentBase ="outputConfigMaxCurrent.";
char *S_outputConfigMaxCurrent[320];
oid oidOutputConfigMaxCurrent[320] [MAX_OID_LEN];
size_t lengthOutputConfigMaxCurrent[320];

const char *S_outputConfigMaxPowerBase ="outputSupervisionMaxPower.";
char *S_outputConfigMaxPower[320];
oid oidOutputConfigMaxPower[320] [MAX_OID_LEN];
size_t lengthOutputConfigMaxPower[320];



//Sensor Information
const char *S_sensorNumber ="sensorNumber.0";
oid oidSensorNumber [MAX_OID_LEN];
size_t lengthSensorNumber;

const char *S_sensorTemperatureBase ="sensorTemperature.";
char *S_sensorTemperature[12];
oid oidSensorTemperature[12] [MAX_OID_LEN];
size_t lengthSensorTemperature[12];

const char *S_sensorWarningThresholdBase ="sensorWarningThreshold.";
char *S_sensorWarningThreshold[12];
oid oidSensorWarningThreshold[12] [MAX_OID_LEN];
size_t lengthSensorWarningThreshold[12];

const char *S_sensorFailureThresholdBase ="sensorFailureThreshold.";
char *S_sensorFailureThreshold[12];
oid oidSensorFailureThreshold[12] [MAX_OID_LEN];
size_t lengthSensorFailureThreshold[12];



//SNMP Community Information
const char *S_snmpCommunityNameBase ="snmpCommunityName.";
char *S_snmpCommunityName[4];
oid oidSnmpCommunityName[4] [MAX_OID_LEN];
size_t lengthSnmpCommunityName[4];



// Power Supply Details
const char *S_psFirmwareVersion ="psFirmwareVersion.0";
oid oidPsFirmwareVersion[MAX_OID_LEN];
size_t lengthPsFirmwareVersion;

const char *S_psSerialNumber ="psSerialNumber.0";
oid oidPsSerialNumber[MAX_OID_LEN];
size_t lengthPsSerialNumber;

const char *S_psOperatingTime ="psOperatingTime.0";
oid oidPsOperatingTime[MAX_OID_LEN];
size_t lengthPsOperatingTime;

const char *S_psDirectAccess ="psDirectAccess.0";
oid oidPsDirectAccess[MAX_OID_LEN];
size_t lengthPsDirectAccess;


//Fan Tray Specific OIDs
const char *S_fanFirmwareVersion ="fanFirmwareVersion.0";
oid oidFanFirmwareVersion[MAX_OID_LEN];
size_t lengthFanFirmwareVersion;

const char *S_fanSerialNumber ="fanSerialNumber.0";
oid oidFanSerialNumber[MAX_OID_LEN];
size_t lengthFanSerialNumber;

const char *S_fanOperatingTime ="fanOperatingTime.0";
oid oidFanOperatingTime[MAX_OID_LEN];
size_t lengthFanOperatingTime;

const char *S_fanAirTemperature ="fanAirTemperature.0";
oid oidFanAirTemperature[MAX_OID_LEN];
size_t lengthFanAirTemperature;

const char *S_fanSwitchOffDelay ="fanSwitchOffDelay.0";
oid oidFanSwitchOffDelay[MAX_OID_LEN];
size_t lengthFanSwitchOffDelay;

const char *S_fanNominalSpeed ="fanNominalSpeed.0";
oid oidFanNominalSpeed[MAX_OID_LEN];
size_t lengthFanNominalSpeed;

const char *S_fanNumberOfFans ="fanNumberOfFans.0";
oid oidFanNumberOfFans[MAX_OID_LEN];
size_t lengthFanNumberOfFans;

const char *S_fanSpeedBase ="fanSpeed.";
char *S_fanSpeed[6];
oid oidFanSpeed[6] [MAX_OID_LEN];
size_t lengthFanSpeed[6];


/******************************************************************************/
/** simple syslog replacement for this example.
*/
void syslog(int priority,const char* format,...) {
  va_list vaPrintf; 
  va_start(vaPrintf,format);
  vprintf(format,vaPrintf); putchar('\n');
  va_end(vaPrintf);
}

/******************************************************************************/
/** SNMP Initialization.
*/


int SnmpInit(void) {
   init_snmp("CrateTest");									  // I never saw this name used !?!
   init_mib();													  // init MIB processing
   if(!read_module("WIENER-CRATE-MIB")) {		        // read specific mibs
      syslog(LOG_ERR,"Unable to load SNMP MIB file \"%s\"","WIENER-CRATE-MIB");
      return -1;
   }

  //Translate System OIDS
  lengthSysMainSwitch = MAX_OID_LEN;
  if(!get_node(S_sysMainSwitch,oidSysMainSwitch,&lengthSysMainSwitch)) {
		syslog(LOG_ERR,"OID \"sysMainSwitch.0\"not found"); return false; } 

  lengthSysStatus = MAX_OID_LEN;
  if(!get_node(S_sysStatus,oidSysStatus,&lengthSysStatus)) {
		syslog(LOG_ERR,"OID \"sysStatus.0\"not found"); return false;} 

  lengthSysVmeSysReset = MAX_OID_LEN;
  if(!get_node(S_sysVmeSysReset,oidSysVmeSysReset,&lengthSysVmeSysReset)) {
		syslog(LOG_ERR,"OID \"SysVmeSysReset.0\"not found"); return false;} 



//Translate System Channel Count Information OIDS
  lengthOutputNumber = MAX_OID_LEN;
  if(!get_node(S_outputNumber,oidOutputNumber,&lengthOutputNumber)) {
		syslog(LOG_ERR,"OID \"OutputNumber.0\"not found"); return false; } 

  lengthGroupsNumber = MAX_OID_LEN;
  if(!get_node(S_groupsNumber,oidGroupsNumber,&lengthGroupsNumber)) {
		syslog(LOG_ERR,"OID \"groupsNumber.0\"not found"); return false;} 


//Translate Channel Information OIDS


  char s[100];
  char index[4];
  int base =0;
  int indexBase=0;
  int j=0;

  for( int i = 0; i < 3; i++) {
	//base = 32*i;  // original value
   base = 16*i;    // modified value SR
	indexBase = 100*i;
    //for( j = 1; j < 33; j++) // original value
    for (j = 1; j< 17 ; j++)   // modified value
    {
	  strcpy(s, S_outputNameBase);
	  sprintf(index, "%i", indexBase+j);
	  strcat(s, index);
	  lengthOutputName[base+j] = MAX_OID_LEN;
	  if(!get_node(s,oidOutputName[base+j],&lengthOutputName[base+j])) {
	 	syslog(LOG_ERR,"OID \"outputName\"not found"); return false;}

	  strcpy(s, S_outputGroupBase);
	  sprintf(index, "%i", indexBase+j);
	  strcat(s, index);
	  lengthOutputGroup[base+j] = MAX_OID_LEN;
	  if(!get_node(s,oidOutputGroup[base+j],&lengthOutputGroup[base+j])) {
	 	syslog(LOG_ERR,"OID \"OutputGroup\"not found"); return false;}

      strcpy(s, S_outputStatusBase);
	  sprintf(index, "%i",indexBase+j);
	  strcat(s, index);
	  lengthOutputStatus[base+j] = MAX_OID_LEN;
	  if(!get_node(s,oidOutputStatus[base+j],&lengthOutputStatus[base+j])) {
		syslog(LOG_ERR,"OID \"outputStatus.1\"not found"); return false; } 

	  strcpy(s, S_outputMeasurementSenseVoltageBase);
	  sprintf(index, "%i", indexBase+j);
	  strcat(s, index);
	  lengthOutputMeasurementSenseVoltage[base+j] = MAX_OID_LEN;
	  if(!get_node(s,oidOutputMeasurementSenseVoltage[base+j],&lengthOutputMeasurementSenseVoltage[base+j])) {
	 	syslog(LOG_ERR,"OID \"outputMeasurementSenseVoltage.1\"not found"); return false;}

	  strcpy(s, S_outputMeasurementTerminalVoltageBase);
	  sprintf(index, "%i", indexBase+j);
	  strcat(s, index);
	  lengthOutputMeasurementTerminalVoltage[base+j] = MAX_OID_LEN;
	  if(!get_node(s,oidOutputMeasurementTerminalVoltage[base+j],&lengthOutputMeasurementTerminalVoltage[base+j])) {
	 	syslog(LOG_ERR,"OID \"outputMeasurementTerminalVoltage.1\"not found"); return false;}

	  strcpy(s, S_outputMeasurementCurrentBase);
	  sprintf(index, "%i", indexBase+j);
	  strcat(s, index);
	  lengthOutputMeasurementCurrent[base+j] = MAX_OID_LEN;
		if(!get_node(s,oidOutputMeasurementCurrent[base+j],&lengthOutputMeasurementCurrent[base+j])) {
			syslog(LOG_ERR,"OID \"outputMeasurementCurrent.1\"not found"); return false; }

	  strcpy(s, S_outputMeasurementTemperatureBase);
	  sprintf(index, "%i", indexBase+j);
	  strcat(s, index);
	  lengthOutputMeasurementTemperature[base+j] = MAX_OID_LEN;
		if(!get_node(s,oidOutputMeasurementTemperature[base+j],&lengthOutputMeasurementTemperature[base+j])) {
			syslog(LOG_ERR,"OID \"outputMeasurementTemperature\"not found"); return false; }
    
	  strcpy(s, S_outputSwitchBase);
	  sprintf(index, "%i",indexBase+j);
	  strcat(s, index);
	  lengthChannelSwitch[base+j] = MAX_OID_LEN;
	  if(!get_node(s,oidChannelSwitch[base+j],&lengthChannelSwitch[base+j])) {
		syslog(LOG_ERR,"OID \"channelSwitch.1\"not found"); return false;  } 

	  strcpy(s, S_outputVoltageBase);
	  sprintf(index, "%i", indexBase+j);
	  strcat(s, index);
	  lengthOutputVoltage[base+j] = MAX_OID_LEN;
	  if(!get_node(s,oidOutputVoltage[base+j],&lengthOutputVoltage[base+j])) {
		syslog(LOG_ERR,"OID \"outputVoltage.1\"not found"); return false; }

	  strcpy(s, S_outputCurrentBase);
	  sprintf(index, "%i", indexBase+j);
	  strcat(s, index);
	  lengthOutputCurrent[base+j] = MAX_OID_LEN;
	  if(!get_node(s,oidOutputCurrent[base+j],&lengthOutputCurrent[base+j])) {
		syslog(LOG_ERR,"OID \"outputCurrent\"not found"); return false; }

	  strcpy(s, S_outputRiseRateBase);
	  sprintf(index, "%i", indexBase+j);
	  strcat(s, index);
	  lengthOutputRiseRate[base+j] = MAX_OID_LEN;
	  if(!get_node(s,oidOutputRiseRate[base+j],&lengthOutputRiseRate[base+j])) {
		syslog(LOG_ERR,"OID \"outputRiseRate.1\"not found"); return false;} 
	
	  strcpy(s, S_outputFallRateBase);
	  sprintf(index, "%i", indexBase+j);
	  strcat(s, index);
	  lengthOutputFallRate[base+j] = MAX_OID_LEN;
	  if(!get_node(s,oidOutputFallRate[base+j],&lengthOutputFallRate[base+j])) {
		syslog(LOG_ERR,"OID \"outputRiseRate.1\"not found"); return false; } 
	
	  strcpy(s, S_outputSupervisionBehaviorBase);
	  sprintf(index, "%i", indexBase+j);
	  strcat(s, index);
	  lengthOutputSupervisionBehavior[base+j] = MAX_OID_LEN;
	  if(!get_node(s,oidOutputSupervisionBehavior[base+j],&lengthOutputSupervisionBehavior[base+j])) {
		syslog(LOG_ERR,"OID \"outputSupervisionBehavior\"not found"); return false; } 
	
	  strcpy(s, S_outputSupervisionMinSenseVoltageBase);
	  sprintf(index, "%i", indexBase+j);
	  strcat(s, index);
	  lengthOutputSupervisionMinSenseVoltage[base+j] = MAX_OID_LEN;
	  if(!get_node(s,oidOutputSupervisionMinSenseVoltage[base+j],&lengthOutputSupervisionMinSenseVoltage[base+j])) {
		syslog(LOG_ERR,"OID \"outputSupervisionMinSenseVoltage\"not found"); return false; } 

	  strcpy(s, S_outputSupervisionMaxSenseVoltageBase);
	  sprintf(index, "%i", indexBase+j);
	  strcat(s, index);
	  lengthOutputSupervisionMaxSenseVoltage[base+j] = MAX_OID_LEN;
	  if(!get_node(s,oidOutputSupervisionMaxSenseVoltage[base+j],&lengthOutputSupervisionMaxSenseVoltage[base+j])) {
		syslog(LOG_ERR,"OID \"outputSupervisionMaxSenseVoltage\"not found"); return false; } 

	  strcpy(s, S_outputSupervisionMaxTerminalVoltageBase);
	  sprintf(index, "%i", indexBase+j);
	  strcat(s, index);
	  lengthOutputSupervisionMaxTerminalVoltage[base+j] = MAX_OID_LEN;
	  if(!get_node(s,oidOutputSupervisionMaxTerminalVoltage[base+j],&lengthOutputSupervisionMaxTerminalVoltage[base+j])) {
		syslog(LOG_ERR,"OID \"outputSupervisionMaxTerminalVoltage\"not found"); return false; } 

	  strcpy(s, S_outputSupervisionMaxCurrentBase);
	  sprintf(index, "%i", indexBase+j);
	  strcat(s, index);
	  lengthOutputSupervisionMaxCurrent[base+j] = MAX_OID_LEN;
	  if(!get_node(s,oidOutputSupervisionMaxCurrent[base+j],&lengthOutputSupervisionMaxCurrent[base+j])) {
		syslog(LOG_ERR,"OID \"outputSupervisionMaxCurrent\"not found"); return false; } 
/*
	  strcpy(s, S_outputSupervisionMaxTemperatureBase);
	  sprintf(index, "%i", indexBase+j);
	  strcat(s, index);
	  lengthOutputSupervisionMaxTemperature[base+j] = MAX_OID_LEN;
	  if(!get_node(s,oidOutputSupervisionMaxTemperature[base+j],&lengthOutputSupervisionMaxTemperature[base+j])) {
		syslog(LOG_ERR,"OID \"outputSupervisionMaxTemperature\"not found"); return false; } 
*/
	  strcpy(s, S_outputConfigMaxSenseVoltageBase);
	  sprintf(index, "%i", indexBase+j);
	  strcat(s, index);
	  lengthOutputConfigMaxSenseVoltage[base+j] = MAX_OID_LEN;
	  if(!get_node(s,oidOutputConfigMaxSenseVoltage[base+j],&lengthOutputConfigMaxSenseVoltage[base+j])) {
		syslog(LOG_ERR,"OID \"outputConfigMaxSenseVoltage\"not found"); return false; } 

	  strcpy(s, S_outputConfigMaxTerminalVoltageBase);
	  sprintf(index, "%i", indexBase+j);
	  strcat(s, index);
	  lengthOutputConfigMaxTerminalVoltage[base+j] = MAX_OID_LEN;
	  if(!get_node(s,oidOutputConfigMaxTerminalVoltage[base+j],&lengthOutputConfigMaxTerminalVoltage[base+j])) {
		syslog(LOG_ERR,"OID \"outputConfigMaxTerminalVoltage\"not found"); return false; }
	  
	  strcpy(s, S_outputConfigMaxCurrentBase);
	  sprintf(index, "%i", indexBase+j);
	  strcat(s, index);
	  lengthOutputConfigMaxCurrent[base+j] = MAX_OID_LEN;
	  if(!get_node(s,oidOutputConfigMaxCurrent[base+j],&lengthOutputConfigMaxCurrent[base+j])) {
		syslog(LOG_ERR,"OID \"outputConfigMaxCurrent\"not found"); return false; }

	  strcpy(s, S_outputConfigMaxPowerBase);
	  sprintf(index, "%i", indexBase+j);
	  strcat(s, index);
	  lengthOutputConfigMaxPower[base+j] = MAX_OID_LEN;
	  if(!get_node(s,oidOutputConfigMaxPower[base+j],&lengthOutputConfigMaxPower[base+j])) {
		syslog(LOG_ERR,"OID \"outputConfigMaxPower\"not found"); return false; }
	}  
  } 


//Sensor Information
  lengthSensorNumber = MAX_OID_LEN;
  if(!get_node(S_sensorNumber,oidSensorNumber,&lengthSensorNumber)) {
		syslog(LOG_ERR,"OID \"SensorNumber.0\"not found"); return false;} 

  for(int j = 0; j < 8; j++)
  {
	strcpy(s, S_sensorTemperatureBase);
	sprintf(index, "%i", j+1);
	strcat(s, index);
	lengthSensorTemperature[j] = MAX_OID_LEN;
	if(!get_node(s,oidSensorTemperature[j],&lengthSensorTemperature[j])) {
		syslog(LOG_ERR,"OID \"sensorTemperature.0\"not found"); return false; } 

	strcpy(s, S_sensorWarningThresholdBase);
	sprintf(index, "%i", j+1);
	strcat(s, index);
	lengthSensorWarningThreshold[j] = MAX_OID_LEN;
	if(!get_node(s,oidSensorWarningThreshold[j],&lengthSensorWarningThreshold[j])) {
		syslog(LOG_ERR,"OID \"WarningThreshold\"not found"); return false; } 

	strcpy(s, S_sensorFailureThresholdBase);
	sprintf(index, "%i", j+1);
	strcat(s, index);
	lengthSensorFailureThreshold[j] = MAX_OID_LEN;
	if(!get_node(s,oidSensorFailureThreshold[j],&lengthSensorFailureThreshold[j])) {
		syslog(LOG_ERR,"OID \"FailureThreshold\"not found"); return false; } 
  }


//Translate SNMP community Name OIDs
  for(int j = 0; j < 4; j++)
  {
	strcpy(s, S_snmpCommunityNameBase);
	sprintf(index, "%i", j+1);
	strcat(s, index);
	lengthSnmpCommunityName[j] = MAX_OID_LEN;
	if(!get_node(s,oidSnmpCommunityName[j],&lengthSnmpCommunityName[j])) {
		syslog(LOG_ERR,"OID \"smpCommunityNameBase\"not found"); return false; } 
  }


//Translate PS Specific OIDs
/*
  lengthPsFirmwareVersion = MAX_OID_LEN;
  if(!get_node(S_psFirmwareVersion,oidPsFirmwareVersion,&lengthPsFirmwareVersion)) {
		syslog(LOG_ERR,"OID \"psFirmwareRevision\"not found"); return false; } 
*/
  lengthPsSerialNumber = MAX_OID_LEN;
  if(!get_node(S_psSerialNumber,oidPsSerialNumber,&lengthPsSerialNumber)) {
		syslog(LOG_ERR,"OID \"psSerialNumber\"not found"); return false; } 

  lengthPsOperatingTime = MAX_OID_LEN;
  if(!get_node(S_psOperatingTime,oidPsOperatingTime,&lengthPsOperatingTime)) {
		syslog(LOG_ERR,"OID \"PsOperatingTime\"not found"); return false; } 

  lengthPsDirectAccess = MAX_OID_LEN;
  if(!get_node(S_psDirectAccess,oidPsDirectAccess,&lengthPsDirectAccess)) {
		syslog(LOG_ERR,"OID \"PsDirectAccess\"not found"); return false; } 

/*
//Translate Fan Tray OIDs
  lengthFanFirmwareVersion = MAX_OID_LEN;
  if(!get_node(S_fanFirmwareVersion,oidFanFirmwareVersion,&lengthFanFirmwareVersion)) {
		syslog(LOG_ERR,"OID \"FanFirmwareVersion\"not found"); return false; } 
*/
  lengthFanSerialNumber = MAX_OID_LEN;
  if(!get_node(S_fanSerialNumber,oidFanSerialNumber,&lengthFanSerialNumber)) {
		syslog(LOG_ERR,"OID \"FanSerialNumber\"not found"); return false; } 

  lengthFanOperatingTime = MAX_OID_LEN;
  if(!get_node(S_fanOperatingTime,oidFanOperatingTime,&lengthFanOperatingTime)) {
		syslog(LOG_ERR,"OID \"FanOperatingTime\"not found"); return false; } 

  lengthFanAirTemperature = MAX_OID_LEN;
  if(!get_node(S_fanAirTemperature,oidFanAirTemperature,&lengthFanAirTemperature)) {
		syslog(LOG_ERR,"OID \"fanAirTemperature.0\"not found"); return false; } 

  lengthFanSwitchOffDelay = MAX_OID_LEN;
  if(!get_node(S_fanSwitchOffDelay,oidFanSwitchOffDelay,&lengthFanSwitchOffDelay)) {
		syslog(LOG_ERR,"OID \"FanSwitchOffDelay\"not found"); return false; } 

  lengthFanNominalSpeed = MAX_OID_LEN;
  if(!get_node(S_fanNominalSpeed,oidFanNominalSpeed,&lengthFanNominalSpeed)) {
		syslog(LOG_ERR,"OID \"FanNominalSpeed\"not found"); return false; } 

  lengthFanNumberOfFans = MAX_OID_LEN;
  if(!get_node(S_fanNumberOfFans,oidFanNumberOfFans,&lengthFanNumberOfFans)) {
		syslog(LOG_ERR,"OID \"NumberOfFans\"not found"); return false; } 

  for(int j = 0; j < 6; j++)
  {
	strcpy(s, S_fanSpeedBase);
	sprintf(index, "%i", j+1);
	strcat(s, index);
	lengthFanSpeed[j] = MAX_OID_LEN;
	if(!get_node(s,oidFanSpeed[j],&lengthFanSpeed[j])) {
		syslog(LOG_ERR,"OID \"FanSpeed\"not found"); return false; } 
  }

  SOCK_STARTUP;															// only in main thread
  return 1;
}

/******************************************************************************/
/** SNMP Cleanup.
*/
void SnmpCleanup(void) {
	SOCK_CLEANUP;
}


/******************************************************************************/
/** SNMP Open Session.
*/
//typedef void* HSNMP;                              // SNMP handle (like FILE)

HSNMP SnmpOpen(const char* ipAddress) {
	HSNMP m_sessp;
  struct snmp_session session;
  snmp_sess_init(&session);			                  // structure defaults
  session.version = SNMP_VERSION_2c;
  session.peername = strdup(ipAddress);
  session.community = (u_char*)strdup(readCommunity);
  session.community_len = strlen((char*)session.community);

  session.timeout = 100000;   // timeout (us)
  session.retries = 3;        // retries

  if(!(m_sessp = snmp_sess_open(&session))) {
    int liberr, syserr;
    char *errstr;
    snmp_error(&session, &liberr, &syserr, &errstr);
    syslog(LOG_ERR,"Open SNMP session for host \"%s\": %s",ipAddress,errstr);
    free(errstr);
    return NULL;
  }

//  m_session = snmp_sess_session(m_sessp);     // get the session pointer 

  return m_sessp;
}

/******************************************************************************/
/** SNMP Close Session.
*/
void SnmpClose(HSNMP m_sessp) {
  if(!m_sessp) return;
  if(!snmp_sess_close(m_sessp)) {
    syslog(LOG_ERR,"Close SNMP session: ERROR");
  }
}



//System Information Functions


/******************************************************************************/
/** Get on/off status of crate
*/
int getMainSwitch(HSNMP m_sessp) {
	int value = snmpGetInt(m_sessp, oidSysMainSwitch, lengthSysMainSwitch);
	return value;
}

/******************************************************************************/
/** Write on/off status
*/
int  setMainSwitch(HSNMP m_sessp,int value) {
  	value = snmpSetInt(m_sessp, oidSysMainSwitch, lengthSysMainSwitch, value);
	return value;
}

/******************************************************************************/
/** Get Main Status
*/
int getMainStatus(HSNMP m_sessp){
	int value = snmpGetInt(m_sessp, oidSysStatus, lengthSysStatus);
	return value;
}

/******************************************************************************/
/** Get VmeReset Status
*/
int getVmeReset(HSNMP m_sessp){
	int value = snmpGetInt(m_sessp, oidSysVmeSysReset, lengthSysVmeSysReset);
	return value;
}

/******************************************************************************/
/** Initiate VME SysReset
*/
int setVmeReset(HSNMP m_sessp){
  	int value = snmpSetInt(m_sessp, oidSysVmeSysReset, lengthSysVmeSysReset, 1);
	return value;
}



//System Count Functions

/******************************************************************************/
/** Get the number of channels from power supply
*/
int getOutputNumber(HSNMP m_sessp){
	int value = snmpGetInt(m_sessp, oidOutputNumber, lengthOutputNumber);
	return value;
}

/******************************************************************************/
/** Get the number of channels from power supply
*/
int getOutputGroups(HSNMP m_sessp){
	int value = snmpGetInt(m_sessp, oidGroupsNumber, lengthGroupsNumber);
	return value;
}




//Output Channel Information

/******************************************************************************/
/** I Don't know yet how to set this up
*/
//std::string getOutputName(HSNMP m_sessp,int channel);

int getOutputGroup(HSNMP m_sessp,int channel){
	int value = snmpGetInt(m_sessp, oidOutputGroup[channel], lengthOutputGroup[channel]);
	return value; }

int getChannelStatus(HSNMP m_sessp, int channel){
	int value = snmpGetInt(m_sessp, oidOutputStatus[channel], lengthOutputStatus[channel]);
	return value; }

double getOutputSenseMeasurement(HSNMP m_sessp, int channel){
	double value = snmpGetDouble(m_sessp, oidOutputMeasurementSenseVoltage[channel], 
		                   lengthOutputMeasurementSenseVoltage[channel]);
	return value; }

double getOutputTerminalMeasurement(HSNMP m_sessp, int channel){
	double value = snmpGetDouble(m_sessp, oidOutputMeasurementTerminalVoltage[channel], 
		                   lengthOutputMeasurementTerminalVoltage[channel]);
	return value; }

double getCurrentMeasurement(HSNMP m_sessp, int channel){
	double value = snmpGetDouble(m_sessp, oidOutputMeasurementCurrent[channel], 
		                   lengthOutputMeasurementCurrent[channel]);
	return value; }

int getTemperatureMeasurement(HSNMP m_sessp, int channel){
	int value = snmpGetInt(m_sessp, oidOutputMeasurementTemperature[channel], 
		                   lengthOutputMeasurementTemperature[channel]);
	return value; }

int setChannelSwitch(HSNMP m_sessp, int channel,int value) {
  	value = snmpSetInt(m_sessp, oidChannelSwitch[channel], 
						   lengthChannelSwitch[channel], value);
	return value; }

int getChannelSwitch(HSNMP m_sessmp, int channel){
  	int value = snmpGetInt(m_sessmp, oidChannelSwitch[channel], 
						   lengthChannelSwitch[channel]);
	return value; }

double getOutputVoltage(HSNMP m_sessp, int channel) {
    double value = snmpGetDouble(m_sessp, oidOutputVoltage[channel],
	                           lengthOutputVoltage[channel] );
  return value; }

double setOutputVoltage(HSNMP m_sessp, int channel, double value) {
	value = snmpSetDouble(m_sessp, oidOutputVoltage[channel],
	                      lengthOutputVoltage[channel], value );
	return value; }

double getOutputCurrent(HSNMP m_sessp,int channel) {
	double value = snmpGetDouble(m_sessp, oidOutputCurrent[channel],
	                      lengthOutputCurrent[channel]);
	return value; }

double setOutputCurrent(HSNMP m_sessp,int channel,double value){
	value = snmpSetDouble(m_sessp, oidOutputCurrent[channel],
	                      lengthOutputCurrent[channel], value );
	return value; }

double getOutputRiseRate(HSNMP m_sessp, int channel) {
	double value = snmpGetDouble(m_sessp, oidOutputRiseRate[channel],
	                           lengthOutputRiseRate[channel] );
	return value; }

double  setOutputRiseRate(HSNMP m_sessp,int channel,double value) {
	value = snmpSetDouble(m_sessp, oidOutputRiseRate[channel],
	                      lengthOutputRiseRate[channel], value );
	return value; }

double getOutputFallRate(HSNMP m_sessp, int channel) {
	double value = snmpGetDouble(m_sessp, oidOutputFallRate[channel],
	                           lengthOutputFallRate[channel] );
	return value; }

double  setOutputFallRate(HSNMP m_sessp,int channel,double value) {
	value = snmpSetDouble(m_sessp, oidOutputFallRate[channel],
	                           lengthOutputFallRate[channel], value );
	return value; }

int getOutputSupervisionBehavior(HSNMP m_sessp,int channel) {
	int value = snmpGetInt(m_sessp, oidOutputSupervisionBehavior[channel],
	                           lengthOutputSupervisionBehavior[channel] );
	return value; }

int setOutputSupervisionBehavior(HSNMP m_sessp,int channel,int value) {
	value = snmpSetInt(m_sessp, oidOutputSupervisionBehavior[channel],
	                           lengthOutputSupervisionBehavior[channel], value );
	return value; }

double getOutputSupervisionMinSenseVoltage(HSNMP m_sessp,int channel) {
	double value = snmpGetDouble(m_sessp, oidOutputSupervisionMinSenseVoltage[channel],
	                           lengthOutputSupervisionMinSenseVoltage[channel] );
	return value; }

double setOutputSupervisionMinSenseVoltage(HSNMP m_sessp,int channel,double value) {
	value = snmpSetDouble(m_sessp, oidOutputSupervisionMinSenseVoltage[channel],
	                           lengthOutputSupervisionMinSenseVoltage[channel], value );
	return value; }

double getOutputSupervisionMaxSenseVoltage(HSNMP m_sessp,int channel) {
	double value = snmpGetDouble(m_sessp, oidOutputSupervisionMaxSenseVoltage[channel],
	                           lengthOutputSupervisionMaxSenseVoltage[channel] );
	return value; }

double setOutputSupervisionMaxSenseVoltage(HSNMP m_sessp,int channel,double value) {
	value = snmpSetDouble(m_sessp, oidOutputSupervisionMaxSenseVoltage[channel],
	                           lengthOutputSupervisionMaxSenseVoltage[channel], value );
	return value; }

double getOutputSupervisionMaxTerminalVoltage(HSNMP m_sessp,int channel) {
	double value = snmpGetDouble(m_sessp, oidOutputSupervisionMaxTerminalVoltage[channel],
	                           lengthOutputSupervisionMaxTerminalVoltage[channel] );
	return value; }

double setOutputSupervisionMaxTerminalVoltage(HSNMP m_sessp,int channel,double value) {
	value = snmpSetDouble(m_sessp, oidOutputSupervisionMaxTerminalVoltage[channel],
	                           lengthOutputSupervisionMaxTerminalVoltage[channel], value );
	return value; }

double getOutputSupervisionMaxCurrent(HSNMP m_sessp,int channel) {
	double value = snmpGetDouble(m_sessp, oidOutputSupervisionMaxCurrent[channel],
	                           lengthOutputSupervisionMaxCurrent[channel] );
	return value; }

double setOutputSupervisionMaxCurrent(HSNMP m_sessp,int channel,double value){
	value = snmpSetDouble(m_sessp, oidOutputSupervisionMaxCurrent[channel],
	                           lengthOutputSupervisionMaxCurrent[channel], value );
	return value; }

int getOutputSupervisionMaxTemperature(HSNMP m_sessp,int channel) {
	int value = snmpGetInt(m_sessp, oidOutputSupervisionMaxTemperature[channel],
	                           lengthOutputSupervisionMaxTemperature[channel] );
	return value; }


double getOutputConfigMaxSenseVoltage(HSNMP m_sessp,int channel)  {
	double value = snmpGetInt(m_sessp, oidOutputConfigMaxSenseVoltage[channel],
	                           lengthOutputConfigMaxSenseVoltage[channel] );
	return value; }

double getOutputConfigMaxTerminalVoltage(HSNMP m_sessp,int channel) {
	double value = snmpGetInt(m_sessp, oidOutputConfigMaxTerminalVoltage[channel],
	                           lengthOutputConfigMaxTerminalVoltage[channel] );
	return value; }

double getOutputConfigMaxCurrent(HSNMP m_sessp,int channel) {
	double value = snmpGetInt(m_sessp, oidOutputConfigMaxCurrent[channel],
	                           lengthOutputConfigMaxCurrent[channel] );
	return value; }

double getOutputConfigMaxPower(HSNMP m_sessp,int channel){
	double value = snmpGetInt(m_sessp, oidOutputConfigMaxPower[channel],
	                           lengthOutputConfigMaxPower[channel] );
	return value; }




//Sensor Information functions
int getSensorNumber(HSNMP m_sessp) {
	double value = snmpGetInt(m_sessp, oidSensorNumber,
	                           lengthSensorNumber);
	return (int)value; }

int getSensorTemp(HSNMP m_sessp, int sensor) {
	double value = snmpGetInt(m_sessp, oidSensorTemperature[sensor],
	                           lengthSensorTemperature[sensor]);
	return (int)value; }

int getSensorWarningTemperature(HSNMP m_sessp,int sensor) {
	double value = snmpGetInt(m_sessp, oidSensorWarningThreshold[sensor],
	                           lengthSensorWarningThreshold[sensor]);
	return (int)value; }

int setSensorWarningTemperature(HSNMP m_sessp,int sensor,int value){
	 value = snmpSetInt(m_sessp, oidSensorWarningThreshold[sensor],
	                           lengthSensorWarningThreshold[sensor], value);
	return (int)value; }

int getSensorFailureTemperature(HSNMP m_sessp,int sensor) {
	double value = snmpGetInt(m_sessp, oidSensorFailureThreshold[sensor],
	                           lengthSensorFailureThreshold[sensor]);
	return (int)value; }

int setSensorFailureTemperature(HSNMP m_sessp,int sensor,int value){
	value = snmpSetInt(m_sessp, oidSensorFailureThreshold[sensor],
	                           lengthSensorFailureThreshold[sensor], value);
	return (int)value; }



//Power Supply specific Functions.  
int getPsOperatingTime(HSNMP m_sessp) {
	double value = snmpGetInt(m_sessp, oidPsOperatingTime,
	                           lengthPsOperatingTime);
	return (int)value; }


//Fan Tray Functions
//std::string getFanFirmwareVersion(HSNMP m_sessp);
//std::string getFanSerialNumber(HSNMP m_sessp);
int getFanOperatingTime(HSNMP m_sessp) {
	double value = snmpGetInt(m_sessp, oidFanOperatingTime,
	                           lengthFanOperatingTime );
	return (int)value; } 

int getFanAirTemperature(HSNMP m_sessp) {
	double value = snmpGetInt(m_sessp, oidFanAirTemperature,
	                           lengthFanAirTemperature );
	return (int)value; } 

int getFanSwitchOffDelay(HSNMP m_sessp) {
	double value = snmpGetInt(m_sessp, oidFanSwitchOffDelay,
	                           lengthFanSwitchOffDelay );
	return (int)value; } 

int setFanSwitchOffDelay(HSNMP m_sessp, int value) {
	value = snmpSetInt(m_sessp, oidFanSwitchOffDelay,
	                           lengthFanSwitchOffDelay, value );
	return (int)value; } 

int getFanNominalSpeed(HSNMP m_sessp) {
	double value = snmpGetInt(m_sessp, oidFanNominalSpeed,
	                           lengthFanNominalSpeed );
	return (int)value; } 

int setFanNominalSpeed(HSNMP m_sessp, int value){
	value = snmpSetInt(m_sessp, oidFanNominalSpeed,
	                           lengthFanNominalSpeed, value );
	return (int)value; } 

int getFanNumberOfFans(HSNMP m_sessp){
	double value = snmpGetInt(m_sessp, oidFanNumberOfFans,
	                           lengthFanNumberOfFans );
	return (int)value; } 

int getFanSpeed(HSNMP m_sessp, int fan) {
	double value = snmpGetInt(m_sessp, oidFanSpeed[fan],
	                           lengthFanSpeed[fan]);
	return (int)value; } 



//The rest of the functions are utility functions that actually do the SNMP calls

int snmpGetInt(HSNMP m_sessp, const oid* parameter, size_t length) {
  double value;

  struct snmp_pdu* pdu = snmp_pdu_create(SNMP_MSG_GET);    // prepare get-request pdu

  // for(each GET request to one crate) {
    snmp_add_null_var(pdu,parameter,length);   // generate request data
  // } // endfor

  struct snmp_pdu* response;
	int status = snmp_sess_synch_response(m_sessp,pdu,&response);

  /*
  * Process the response.
  */
  value = -1;
  if (status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
    /*
    * SUCCESS: Print the result variables
    */
    struct variable_list *vars;

    /* manipuate the information ourselves */
    for(vars = response->variables; vars; vars = vars->next_variable) {
			if (vars->type == ASN_OPAQUE_FLOAT) {				    // 0x78
        value = *vars->val.floatVal;
      }
			else if (vars->type == ASN_OPAQUE_DOUBLE) {			// 0x79
        value = *vars->val.doubleVal;
      }
			else if(vars->type == ASN_INTEGER) {				      // 0x02
				value = (double)*vars->val.integer;
      }
			else if(vars->type == ASN_OCTET_STR) {				      // 0x04
            value = vars->val.bitstring[0]*256+vars->val.bitstring[1];
      }
    }
  } else {
    /*
    * FAILURE: print what went wrong!
    */

    if (status == STAT_SUCCESS)
      fprintf(stderr, "Error in packet\nReason: %s\n",
      snmp_errstring(response->errstat));
    // do not clutter up screen
    //else
    //  snmp_sess_perror("snmpget",snmp_sess_session(m_sessp));
    value = -1;
  }
  snmp_free_pdu(response);
  return (int) value;
}



int snmpSetInt(HSNMP m_sessp, const oid* parameter, size_t length, int value) {
  struct snmp_pdu* pdu = snmp_pdu_create(SNMP_MSG_SET);    // prepare set-request pdu
  pdu->community = (u_char*)strdup(writeCommunity);
  pdu->community_len = strlen(writeCommunity);

  // for(each SET request to one crate) {
  int v = (int) value;
  snmp_pdu_add_variable(pdu,parameter,length,ASN_INTEGER,(u_char*)&v,sizeof(v));
  // } // endfor
  value=v;
  struct snmp_pdu* response;
	int status = snmp_sess_synch_response(m_sessp,pdu,&response);

  /*
  * Process the response.
  */
  if (status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
    /*
    * SUCCESS: Print the result variables
    */
    struct variable_list *vars;
    
    // debug print
    //for(vars = response->variables; vars; vars = vars->next_variable)
    //  print_variable(vars->name, vars->name_length, vars);

    /* manipuate the information ourselves */
    for(vars = response->variables; vars; vars = vars->next_variable) {
			if (vars->type == ASN_OPAQUE_FLOAT) {				    // 0x78
        value = (int)*vars->val.floatVal;
      }
			else if (vars->type == ASN_OPAQUE_DOUBLE) {			// 0x79
        value = (int)*vars->val.doubleVal;
      }
			else if(vars->type == ASN_INTEGER) {				      // 0x02
				value = *vars->val.integer;
      }
    }
  } else {
    /*
    * FAILURE: print what went wrong!
    */

    if (status == STAT_SUCCESS)
      fprintf(stderr, "Error in packet\nReason: %s\n",
      snmp_errstring(response->errstat));
    else
      snmp_sess_perror("snmpget",snmp_sess_session(m_sessp));
    return -1;
  }
  snmp_free_pdu(response);
  return (int) value;
}



double snmpGetDouble(HSNMP m_sessp, const oid* parameter, size_t length) {
  double value = 0;

  struct snmp_pdu* pdu = snmp_pdu_create(SNMP_MSG_GET);    // prepare get-request pdu

  // for(each GET request to one crate) {
    snmp_add_null_var(pdu,parameter,length);   // generate request data
  // } // endfor

  struct snmp_pdu* response;
	int status = snmp_sess_synch_response(m_sessp,pdu,&response);

  /*
  * Process the response.
  */
  if (status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
    /*
    * SUCCESS: Print the result variables
    */
    struct variable_list *vars;

    /* manipuate the information ourselves */
    for(vars = response->variables; vars; vars = vars->next_variable) {
			if (vars->type == ASN_OPAQUE_FLOAT) {				    // 0x78
        value = *vars->val.floatVal;
      }
			else if (vars->type == ASN_OPAQUE_DOUBLE) {			// 0x79
        value = *vars->val.doubleVal;
      }
			else if(vars->type == ASN_INTEGER) {				      // 0x02
				value = (double)*vars->val.integer;
      }
    }
  } else {
    /*
    * FAILURE: print what went wrong!
    */

    if (status == STAT_SUCCESS)
      fprintf(stderr, "Error in packet\nReason: %s\n",
      snmp_errstring(response->errstat));
    else
       // suppress timeout's not to clutter the screen
       // snmp_sess_perror("snmpget",snmp_sess_session(m_sessp));
    return -1;
  }
  snmp_free_pdu(response);
  return  value;
}


double snmpSetDouble(HSNMP m_sessp, const oid* parameter, size_t length, double value) {
  struct snmp_pdu* pdu = snmp_pdu_create(SNMP_MSG_SET);    // prepare set-request pdu
  pdu->community = (u_char*)strdup(writeCommunity);
  pdu->community_len = strlen(writeCommunity);

  // for(each SET request to one crate) {
  float v = (float) value;
  snmp_pdu_add_variable(pdu,parameter,length,ASN_OPAQUE_FLOAT,(u_char*)&v,sizeof(v));
  // } // endfor
  
  value =v;
  struct snmp_pdu* response;
	int status = snmp_sess_synch_response(m_sessp,pdu,&response);

  /*
  * Process the response.
  */
  if (status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
    /*
    * SUCCESS: Print the result variables
    */
    struct variable_list *vars;
    
    // debug print
    //for(vars = response->variables; vars; vars = vars->next_variable)
    //  print_variable(vars->name, vars->name_length, vars);

    /* manipuate the information ourselves */
    for(vars = response->variables; vars; vars = vars->next_variable) {
			if (vars->type == ASN_OPAQUE_FLOAT) {				    // 0x78
        value = *vars->val.floatVal;
      }
			else if (vars->type == ASN_OPAQUE_DOUBLE) {			// 0x79
        value = *vars->val.doubleVal;
      }
			else if(vars->type == ASN_INTEGER) {				      // 0x02
				value = (double)*vars->val.integer;
      }
	}
  } else {
    /*
    * FAILURE: print what went wrong!
    */

    if (status == STAT_SUCCESS)
      fprintf(stderr, "Error in packet\nReason: %s\n",
      snmp_errstring(response->errstat));
    else
      snmp_sess_perror("snmpget",snmp_sess_session(m_sessp));
    return -1;
  }
  snmp_free_pdu(response);
  return value;
}

} // extern "C"
