/********************************************************************\

  Name:         nitronic.c
  Created by:   Stefan Ritt

  Contents:     Nitronics High Voltage Device Driver

  $Log$
  Revision 1.2  1998/10/12 12:18:57  midas
  Added Log tag in header


\********************************************************************/

#include <stdio.h>
#include <math.h>

#include "midas.h"
#include "rs232.h"

static HNDLE  hDB;
static HNDLE  hKeyControl;
static HNDLE  hKeyEvent, hKeyNames, hKeyDemand, hKeyMeasured, hKeyHVon;
static EQUIPMENT *pEquipment;

static int    hDevComm;
static INT    num_channels;
static float  voltage_limit;
static float  update_sensitivity;
static float  *demand;
static float  *measured;
static float  *demand_mirror;
static float  *measured_mirror;
static char   *names;
static DWORD  *last_change;
static BOOL   HVon;

static void read_channel(int ch);
static void update_demand(INT, INT);

/*----------------------------------------------------------------------------*/

#include <conio.h>

int init_nitronic(HNDLE hdb, HNDLE hRootKey, EQUIPMENT *pEquip)
{
int  status, size;
WORD current_limit, address;
char com_port[15];
char str[256];
KEY  key;
BOOL enabled;

  hDB = hdb;
  pEquipment = pEquip;

  // Find LRS1440 subkey
  status = db_find_key(hDB, hRootKey, "Nitronic", &hKeyControl);
  if (status != DB_SUCCESS)
    {
    cm_msg(MERROR, "init_nitronic", "Cannot find Nitronic entry in online database");
    return FE_ERR_ODB;
    }

  // Check 'enabled' flag
  enabled = FALSE;
  size = sizeof(enabled);
  db_get_value(hDB, hKeyControl, "Enabled", &enabled, &size, TID_BOOL);

  // Don't go on if Nitronic is not on
  if (!enabled)
    return FE_ERR_DISABLED;

  // Find event key
  status = db_find_key(hDB, hKeyControl, "Event", &hKeyEvent);
  if (status != DB_SUCCESS)
    {
    cm_msg(MERROR, "init_nitronic", "Cannot find event entry in online database specified in 'Event key'");
    return FE_ERR_ODB;
    }
  status = db_find_key(hDB, hKeyEvent, "Names", &hKeyNames);
  if (status != DB_SUCCESS)
    {
    cm_msg(MERROR, "init_nitronic", "Cannot find 'Names' entry in online database");
    return FE_ERR_ODB;
    }
  status = db_find_key(hDB, hKeyEvent, "Demand", &hKeyDemand);
  if (status != DB_SUCCESS)
    {
    cm_msg(MERROR, "init_nitronic", "Cannot find 'Demand' entry in online database");
    return FE_ERR_ODB;
    }
  status = db_find_key(hDB, hKeyEvent, "Measured", &hKeyMeasured);
  if (status != DB_SUCCESS)
    {
    cm_msg(MERROR, "init_nitronic", "Cannot find 'Measured' entry in online database");
    return FE_ERR_ODB;
    }
  status = db_find_key(hDB, hKeyEvent, "HV on", &hKeyHVon);
  if (status != DB_SUCCESS)
    {
    cm_msg(MERROR, "init_nitronic", "Cannot find 'HV on' entry in online database");
    return FE_ERR_ODB;
    }

  // Get configuration settings
  num_channels = 32;
  size = sizeof(INT);
  status = db_get_value(hDB, hKeyControl, "Channels", &num_channels, &size, TID_INT);
  if (status != DB_SUCCESS)
    {
    cm_msg(MERROR, "init_nitronic", "Cannot find 'Channels' entry in online database");
    return FE_ERR_ODB;
    }

  // Allocate memory for buffers
  demand          = (float *) calloc(num_channels, sizeof(float));
  demand_mirror   = (float *) calloc(num_channels, sizeof(float));
  measured        = (float *) calloc(num_channels, sizeof(float));
  measured_mirror = (float *) calloc(num_channels, sizeof(float));
  last_change     = (DWORD *) calloc(num_channels, sizeof(DWORD));

  if (!last_change)
    {
    cm_msg(MERROR, "init_nitronic", "Not enough memory");
    return FE_ERR_ODB;
    }

  // Adjust number of channels according in ODB
  db_get_key(hDB, hKeyDemand, &key);
  if (key.num_values != num_channels)
    {
    printf("The number of HV channels in the frontend setting is different from the\n");
    printf("number of channels in the HV event. Should the number of channels in the\n");
    printf("HV event be adjusted? ([y]/n) ");
    status = getchar();
    printf("\n");

    if (status == 'n' || status == 'N')
      {
      printf("Nitronic module disabled");
      return FE_ERR_DISABLED;
      }

    // Adjust demand size
    memset(demand, 0, sizeof(float) * num_channels);
    size = sizeof(float) * num_channels;
    db_get_data(hDB, hKeyDemand, demand, &size);
    size = sizeof(float) * num_channels;
    db_set_data(hDB, hKeyDemand, demand, size, num_channels, TID_FLOAT);

    // Adjust measured size
    memset(measured, 0, sizeof(float) * num_channels);
    size = sizeof(float) * num_channels;
    db_get_data(hDB, hKeyMeasured, measured, &size);
    size = sizeof(float) * num_channels;
    db_set_data(hDB, hKeyMeasured, measured, size, num_channels, TID_FLOAT);

    // Adjust names size
    db_get_key(hDB, hKeyNames, &key);
    names = (char *) malloc(key.item_size * num_channels);
    memset(names, 0, key.item_size * num_channels);
    size = key.item_size * num_channels;
    db_get_data(hDB, hKeyNames, names, &size);
    size = key.item_size * num_channels;
    db_set_data(hDB, hKeyNames, names, size, num_channels, TID_STRING);
    free(names);
    }

  voltage_limit = 3000.f;
  size = sizeof(voltage_limit);
  status = db_get_value(hDB, hKeyControl, "Voltage Limit", &voltage_limit, &size, TID_FLOAT);

  update_sensitivity = 3.f;
  size = sizeof(update_sensitivity);
  status = db_get_value(hDB, hKeyControl, "Update Sensitivity", &update_sensitivity, &size, TID_FLOAT);

  current_limit = 3000;
  size = sizeof(current_limit);
  status = db_get_value(hDB, hKeyControl, "Current Limit", &current_limit, &size, TID_WORD);

  address = 1;
  size = sizeof(address);
  status = db_get_value(hDB, hKeyControl, "Address", &address, &size, TID_WORD);

  // Initialize measured_mirror
  size = sizeof(float) * num_channels;
  db_get_data(hDB, hKeyMeasured, measured_mirror, &size);

  // Now initialize RS232 port
  strcpy(com_port, "COM2");
  size = sizeof(com_port);
  status = db_get_value(hDB, hKeyControl, "Com Port", com_port, &size, TID_STRING);

  hDevComm = com_init(com_port, 9600, 'N', 8, 1, 0);

  // check if module is living 
  sprintf(str, "M%02d", address);
  com_puts(hDevComm, str);
  status = com_waitfor(hDevComm, "\r+", str, 80, 2);
  if (!status)
    {
    cm_msg(MERROR, "init_nitronic", "Nitronic doesn't respond. Check power and RS232 connection.");
    return FE_ERR_HW;
    }

  // set current limits
  sprintf(str, "ICH01%04dA", current_limit);
  com_puts(hDevComm, str);
  com_waitfor(hDevComm, "\r+", str, 80, 5);

  /**** open demand record ****/
  db_open_record(hDB, hKeyDemand, demand, num_channels*sizeof(float), MODE_READ, update_demand, NULL);

  /**** open HV on switch ****/
  db_open_record(hDB, hKeyHVon, &HVon, sizeof(HVon), MODE_READ, update_demand, NULL);

  /**** initially read all channels ****/
  update_demand(0, 0);
  read_channel(-1);

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/

void exit_nitronic()
{
  free(demand);
  free(demand_mirror);
  free(measured);
  free(measured_mirror);
  free(last_change);

  com_exit(hDevComm);
}

/*----------------------------------------------------------------------------*/

static void read_channel(int ch)
{
int   value, i, j;
char  str[256];
float max_diff;
DWORD act_time, min_time;

  if (ch == -1)
    {
    sprintf(str, "CH01A");
    com_puts(hDevComm, str);
    com_waitfor(hDevComm, "\r", str, 80, 5);
    for (j=0 ; j < 32 ; j++)
      {
      com_waitfor(hDevComm, "\r", str, 80, 3);
      if (j < num_channels)
        {
        sscanf(str+12, "%d", &value);
        measured[j] = (float) value;
        }
      }
    com_waitfor(hDevComm, "+", str, 80, 3);
    }
  else
    {
    sprintf(str, "CH%02dL", ch+1);
    com_puts(hDevComm, str);
    com_waitfor(hDevComm, "\r+", str, 80, 1);
    sscanf(str+14, "%d", &value);

    measured[ch] = (float) value;
    }

  // check how much channels have changed since last ODB update
  act_time = ss_millitime();
  max_diff = 0.f;
  min_time = 10000;
  
  for (i=0 ; i<num_channels ; i++)
    {
    if ( fabs(measured[i] - measured_mirror[i]) > max_diff)
      max_diff = (float) fabs(measured[i] - measured_mirror[i]);

    if ( act_time - last_change[i] < min_time)
      min_time = act_time - last_change[i];
    }

  // update if maximal change is more or equal update_sensitivity
  if (max_diff >= update_sensitivity ||
      (min_time < 2000 && max_diff > 0))
    {
    for (i=0 ; i<num_channels ; i++)
      measured_mirror[i] = measured[i];

    db_set_data(hDB, hKeyMeasured, measured, sizeof(float)*num_channels, num_channels, TID_FLOAT);

//    UpdateStatistics("HV Nitronic", 0, 1);
    }
}

/*----------------------------------------------------------------------------*/

static void update_demand(INT hDB, INT hKey)
{
INT   i;
char  str[80];
DWORD act_time;

  act_time = ss_millitime();

  if (hKey == hKeyHVon)
    {
    if (HVon)
      {
      com_puts(hDevComm, "ONA\r");
      com_waitfor(hDevComm, "+", str, 80, 5);
      }
    else
      {
      com_puts(hDevComm, "OFFA\r");
      com_waitfor(hDevComm, "+", str, 80, 5);
      }

    }
  else
    for (i=0 ; i<num_channels ; i++)
      {
      // check for allowed voltage
      if (demand[i] > voltage_limit)
        demand[i] = voltage_limit;

      if (demand[i] != demand_mirror[i])
        {
        sprintf(str, "UCH%02d%04.0fL", i+1, demand[i]);
        com_puts(hDevComm, str);
        com_waitfor(hDevComm, "+", str, 80, 3);

        demand_mirror[i] = demand[i];
        last_change[i] = act_time;
        }
      }

  pEquipment->odb_in++;
}

/*----------------------------------------------------------------------------*/

void update_nitronic()
{
int        act;
static int last=0;
DWORD      act_time;

  act_time = ss_millitime();

  act = (last + 1) % num_channels;

  // look for the next channel recently updated
  while (!( act_time - last_change[act] < 5000 ||
          ( act_time - last_change[act] < 20000 
          &&  fabs(measured[act] - demand[act]) > 2*update_sensitivity)))
    {
    act = (act + 1) % num_channels;
    if (act == last)
      {
      act = (last + 1) % num_channels;
      break;
      }
    }

  read_channel(act);

  last = act;
}

/*----------------------------------------------------------------------------*/
