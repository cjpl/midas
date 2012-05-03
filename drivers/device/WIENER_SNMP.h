#include <net-snmp/net-snmp-config.h>
#define NET_SNMP_SNMPV3_H                   // we don't need SNMPV3 (one include file is missing)
#include <net-snmp/net-snmp-includes.h>

typedef void* HSNMP;   // SNMP handle (like FILE)

void  syslog(int priority,const char* format,...);
int  SnmpInit(void);
void  SnmpCleanup(void);
HSNMP  SnmpOpen(const char* ipAddress);
void  SnmpClose(HSNMP m_sessp);

//System Functions
int  getMainSwitch(HSNMP m_sessp);
int  setMainSwitch(HSNMP m_sessp, int value);
int  getMainStatus(HSNMP m_sessp);
int  getVmeReset(HSNMP m_sessp);
int  setVmeReset(HSNMP m_sessp);

//Output Counts
int  getOutputNumber(HSNMP m_sessp);
int  getOutputGroups(HSNMP m_sessp);

//Output Channel Information
//std::string  getOutputName(HSNMP m_sessp,int channel);
int  getOutputGroup(HSNMP m_sessp,int channel);
int  getChannelStatus(HSNMP m_sessmp, int channel);
double  getOutputSenseMeasurement(HSNMP m_sessp, int channel);
double  getOutputTerminalMeasurement(HSNMP m_sessp, int channel);
double  getCurrentMeasurement(HSNMP m_sessp, int channel);
int  getTemperatureMeasurement(HSNMP m_sessp, int channel);
int  setChannelSwitch(HSNMP m_sessmp, int channel, int value);
int  getChannelSwitch(HSNMP m_sessmp, int channel);
double  getOutputVoltage(HSNMP m_sessp,int channel);
double  setOutputVoltage(HSNMP m_sessp,int channel,double value);
double  getOutputCurrent(HSNMP m_sessp,int channel);
double  setOutputCurrent(HSNMP m_sessp,int channel,double value);
double  getOutputRiseRate(HSNMP m_sessp,int channel);
double  setOutputRiseRate(HSNMP m_sessp,int channel,double value);
double  getOutputFallRate(HSNMP m_sessp,int channel);
double  setOutputFallRate(HSNMP m_sessp,int channel,double value);
int  getOutputSupervisionBehavior(HSNMP m_sessp,int channel);
int  setOutputSupervisionBehavior(HSNMP m_sessp,int channel,int value);
double  getOutputSupervisionMinSenseVoltage(HSNMP m_sessp,int channel);
double  setOutputSupervisionMinSenseVoltage(HSNMP m_sessp,int channel,double value);
double  getOutputSupervisionMaxSenseVoltage(HSNMP m_sessp,int channel);
double  setOutputSupervisionMaxSenseVoltage(HSNMP m_sessp,int channel,double value);
double  getOutputSupervisionMaxTerminalVoltage(HSNMP m_sessp,int channel);
double  setOutputSupervisionMaxTerminalVoltage(HSNMP m_sessp,int channel,double value);
double  getOutputSupervisionMaxCurrent(HSNMP m_sessp,int channel);
double  setOutputSupervisionMaxCurrent(HSNMP m_sessp,int channel,double value);
int  setOutputSupervisionMaxTemperature(HSNMP m_sessp,int channel,int value);
double  getOutputConfigMaxSenseVoltage(HSNMP m_sessp,int channel);
double  getOutputConfigMaxTerminalVoltage(HSNMP m_sessp,int channel);
double  getOutputConfigMaxCurrent(HSNMP m_sessp,int channel);
double  getOutputConfigMaxPower(HSNMP m_sessp,int channel);

//Sensor Information functions
int  getSensorNumber(HSNMP m_sessp);
int  getSensorTemp(HSNMP m_sessp, int sensor);
int  getSensorWarningTemperature(HSNMP m_sessp,int sensor);
int  setSensorWarningTemperature(HSNMP m_sessp,int sensor,int value);
int  getSensorFailureTemperature(HSNMP m_sessp,int sensor);
int  setSensorFailureTemperature(HSNMP m_sessp,int sensor,int value);

//SNMP Community Name Functions
//std::string  getSnmpCommunityName(HSNMP m_sessp,int index);
//std::string  setSnmpCommunityName(HSNMP m_sessp,int index ,int value);

//Power Supply Detail Functions
//std::string  getPsFirmwareVersion(HSNMP m_sessp);
//std::string  getPsSerialNumber(HSNMP m_sessp);
int  getPsOperatingTime(HSNMP m_sessp);
//std::string  getPsDirectAccess(HSNMP m_sessp);
//std::string  setPsDirectAccess(HSNMP m_sessp);


//Fan Tray Functions
//std::string  getFanFirmwareVersion(HSNMP m_sessp);
//std::string  getFanSerialNumber(HSNMP m_sessp);
int  getFanOperatingTime(HSNMP m_sessp);
int  getFanAirTemperature(HSNMP m_sessp);
int  getFanSwitchOffDelay(HSNMP m_sessp);
int  setFanSwitchOffDelay(HSNMP m_sessp, int value);
int  getFanNominalSpeed(HSNMP m_sessp);
int  setFanNominalSpeed(HSNMP m_sessp, int value);
int  getFanNumberOfFans(HSNMP m_sessp);
int  getFanSpeed(HSNMP m_sessp, int fan);


//Utility Functions
double  snmpSetDouble(HSNMP m_sessp, const oid* parameter, size_t length, double value);
double  snmpGetDouble(HSNMP m_sessp, const oid* parameter, size_t length);
int  snmpSetInt(HSNMP m_sessp, const oid* parameter, size_t length, int value);
int  snmpGetInt(HSNMP m_sessp, const oid* parameter, size_t length);

//##SR
void init_mib(void);
void netsnmp_free(void *);
struct tree *read_module(const char *);

