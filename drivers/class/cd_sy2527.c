/********************************************************************\

Name:					cd_sy2527.c
Created	by:		Pierre-Andre Amaudruz

Contents:			Slow Device	Generic	Class	Driver
Based	on the generic class driver, implements	a	check	channel
in the idle	task based on	circular/alternate on	minimum	of
n	raised channels	+	one. See doc for further explanation.

$Id: cd_sys2527.c$

\********************************************************************/

#include <stdio.h>
#include "midas.h"
#include "ybos.h"

#define	ODB_UPDATE_TIME	5000

typedef	struct {

	/* ODB keys	*/
	HNDLE	hKeyRoot,	hKeyDemand,	hKeyMeasured,	hKeycurrent;

	/* globals */
	INT	num_channels;
	INT	format;
	DWORD	last_update;
	INT	last_channel;

	/* items in	/Variables record	*/
	float	*demand;
	float	*measured;
	float	*current;
	BOOL *raised;

   /* items in /Settings */
	char *names;
  float *update_threshold;
  float *update_threshold_current;
  float *voltage_limit;
  float *current_limit;
  float *rampup_speed;
  float *rampdown_speed;

	/* mirror	arrays */
	float	*demand_mirror;
	float	*measured_mirror;
	float	*current_mirror;
	float	*rampup_mirror;
	float	*rampdown_mirror;
	float	*voltage_mirror;
	DWORD	*last_change;

	void **driver;
	INT	*channel_offset;
	void **dd_info;

}	SY2527_INFO;

#define	DRIVER(_i) ((INT (*)(INT cmd,	...))(sy2527_info->driver[_i]))

BOOL  odb_priority = FALSE;

#ifndef	abs
#define	abs(a) (((a) < 0)		?	-(a) : (a))
#endif

/*------------------------------------------------------------------*/

static void	free_mem(SY2527_INFO * sy2527_info)
{
	free(sy2527_info->names);
	free(sy2527_info->demand);
  free(sy2527_info->measured);

  free(sy2527_info->update_threshold);
  free(sy2527_info->update_threshold_current);
  free(sy2527_info->voltage_limit);
  free(sy2527_info->current_limit);
  free(sy2527_info->rampup_speed);
  free(sy2527_info->rampdown_speed);

	free(sy2527_info->raised);

	free(sy2527_info->demand_mirror);
	free(sy2527_info->measured_mirror);
	free(sy2527_info->voltage_mirror);
 	free(sy2527_info->rampup_mirror);
	free(sy2527_info->rampdown_mirror);
	free(sy2527_info->last_change);

	free(sy2527_info->dd_info);
	free(sy2527_info->channel_offset);
	free(sy2527_info->driver);

	free(sy2527_info);
}

/*------------------------------------------------------------------*/

INT	sy2527_read(EQUIPMENT	*	pequipment,	int	channel)
{
	int	status;
	SY2527_INFO	*sy2527_info;
	HNDLE	hDB;

	/*---- read	measured value ----*/
	sy2527_info	=	(SY2527_INFO *)	pequipment->cd_info;
	cm_get_experiment_database(&hDB, NULL);

	/* Get channel value */
	status = DRIVER(channel) (CMD_GET, sy2527_info->dd_info[channel],
		channel	-	sy2527_info->channel_offset[channel],
		&sy2527_info->measured[channel]);

	/* Get currrent channel value */
	status = DRIVER(channel) (CMD_GET_CURRENT, sy2527_info->dd_info[channel],
		channel	-	sy2527_info->channel_offset[channel],
		&sy2527_info->current[channel]);

	/*---- read	demand value ----*/
	status = DRIVER(channel) (CMD_GET_DEMAND,	sy2527_info->dd_info[channel],
		channel	-	sy2527_info->channel_offset[channel],
		&sy2527_info->demand[channel]);

	if (sy2527_info->demand[channel] !=	sy2527_info->demand_mirror[channel]) {
		sy2527_info->demand_mirror[channel]	=	sy2527_info->demand[channel];
		db_set_data(hDB, sy2527_info->hKeyDemand,	sy2527_info->demand,
			sizeof(float)	*	sy2527_info->num_channels, sy2527_info->num_channels,
			TID_FLOAT);
	}

	return status;
}

/*------------------------------------------------------------------*/
/** Write demand Voltage to device (SET)
*/
void sy2527_demand(INT hDB,	INT	hKey,	void *info)
{
	// Write to Device demand values
	INT	i, status;
	SY2527_INFO	*sy2527_info;
	EQUIPMENT	*pequipment;

	pequipment = (EQUIPMENT	*) info;
	sy2527_info	=	(SY2527_INFO *)	pequipment->cd_info;

	/* set individual	channels only	if demand	value	differs from mirror	*/
	for	(i = 0;	i	<	sy2527_info->num_channels; i++)
		if (sy2527_info->demand[i] !=	sy2527_info->demand_mirror[i]) {
			status = DRIVER(i) (CMD_SET, sy2527_info->dd_info[i],
				i	-	sy2527_info->channel_offset[i],	sy2527_info->demand[i]);
			sy2527_info->demand_mirror[i]	=	sy2527_info->demand[i];
			sy2527_info->last_change[i]	=	ss_millitime();
		}

		pequipment->odb_in++;
}


/*------------------------------------------------------------------*/
void hv_current_limit(INT hDB, INT hKey, void *info)
{
  INT i;
  SY2527_INFO	*sy2527_info;

  EQUIPMENT *pequipment;

  pequipment = (EQUIPMENT *) info;
  sy2527_info = (SY2527_INFO *) pequipment->cd_info;

  sy2527_info = ((EQUIPMENT *) info)->cd_info;

  /* check for voltage limit */
  for (i = 0; i < sy2527_info->num_channels; i++)
    DRIVER(i) (CMD_SET_CURRENT_LIMIT, sy2527_info->dd_info[i],
    i - sy2527_info->channel_offset[i], sy2527_info->current_limit[i]);

  pequipment->odb_in++;
}

/*------------------------------------------------------------------*/
void hv_rampup_speed(INT hDB, INT hKey, void *info)
{
  INT i;
  SY2527_INFO	*sy2527_info;

  EQUIPMENT *pequipment;

  pequipment = (EQUIPMENT *) info;
  sy2527_info = (SY2527_INFO *) pequipment->cd_info;

  sy2527_info = ((EQUIPMENT *) info)->cd_info;

  /* check for voltage limit */
  for (i = 0; i < sy2527_info->num_channels; i++)
    DRIVER(i) (CMD_SET_RAMPUP, sy2527_info->dd_info[i],
    i - sy2527_info->channel_offset[i], &sy2527_info->rampup_speed[i]);

  pequipment->odb_in++;
}

/*------------------------------------------------------------------*/
void hv_rampdown_speed(INT hDB, INT hKey, void *info)
{
  INT i;
  SY2527_INFO	*sy2527_info;

  EQUIPMENT *pequipment;

  pequipment = (EQUIPMENT *) info;
  sy2527_info = (SY2527_INFO *) pequipment->cd_info;

  sy2527_info = ((EQUIPMENT *) info)->cd_info;

  /* check for voltage limit */
  for (i = 0; i < sy2527_info->num_channels; i++)
    DRIVER(i) (CMD_SET_RAMPDOWN, sy2527_info->dd_info[i],
    i - sy2527_info->channel_offset[i], &sy2527_info->rampdown_speed[i]);

  pequipment->odb_in++;
}

/*------------------------------------------------------------------*/
void hv_voltage_limit(INT hDB, INT hKey, void *info)
{
  INT i;
  SY2527_INFO	*sy2527_info;

  EQUIPMENT *pequipment;

  pequipment = (EQUIPMENT *) info;
  sy2527_info = (SY2527_INFO *) pequipment->cd_info;

  sy2527_info = ((EQUIPMENT *) info)->cd_info;

  /* check for voltage limit */
  for (i = 0; i < sy2527_info->num_channels; i++)
    DRIVER(i) (CMD_SET_RAMPUP, sy2527_info->dd_info[i],
    i - sy2527_info->channel_offset[i], &sy2527_info->voltage_limit[i]);

  pequipment->odb_in++;
}

/*------------------------------------------------------------------*/
void sy2527_update_label(INT hDB,	INT	hKey,	void *info)
{
	INT	i, status;
	SY2527_INFO	*sy2527_info;
	EQUIPMENT	*pequipment;

	pequipment = (EQUIPMENT	*) info;
	sy2527_info	=	(SY2527_INFO *)	pequipment->cd_info;

	/* update	channel	labels based on	the	midas	channel	names	*/
	for	(i = 0;	i	<	sy2527_info->num_channels; i++)
		status = DRIVER(i) (CMD_SET_LABEL, sy2527_info->dd_info[i],
		i	-	sy2527_info->channel_offset[i],
		sy2527_info->names + NAME_LENGTH * i);
}

/*------------------------------------------------------------------*/
INT	sy2527_init(EQUIPMENT	*	pequipment)
{
	int	status,	size,	i, j,	index, offset;
	char str[256];
	HNDLE	hDB, hKey;
	SY2527_INFO	*sy2527_info;

	/* allocate	private	data */
	pequipment->cd_info	=	calloc(1,	sizeof(SY2527_INFO));
	sy2527_info	=	(SY2527_INFO *)	pequipment->cd_info;

	/* get class driver	root key */
	cm_get_experiment_database(&hDB, NULL);
	sprintf(str, "/Equipment/%s",	pequipment->name);
	db_create_key(hDB, 0,	str, TID_KEY);
	db_find_key(hDB, 0,	str, &sy2527_info->hKeyRoot);

	/* save	event	format */
	size = sizeof(str);
	db_get_value(hDB,	sy2527_info->hKeyRoot, "Common/Format",	str, &size,	TID_STRING,	TRUE);

	if (equal_ustring(str, "Fixed"))
		sy2527_info->format	=	FORMAT_FIXED;
	else if	(equal_ustring(str,	"MIDAS"))
		sy2527_info->format	=	FORMAT_MIDAS;
	else if	(equal_ustring(str,	"YBOS"))
		sy2527_info->format	=	FORMAT_YBOS;

	/* count total number	of channels	*/
	db_create_key(hDB, sy2527_info->hKeyRoot,	"Settings/Channels", TID_KEY);
	db_find_key(hDB, sy2527_info->hKeyRoot,	"Settings/Channels", &hKey);

  for	(i = 0,	sy2527_info->num_channels	=	0; pequipment->driver[i].name[0];	i++) {
    /* ODB value has priority	over driver	list in	channel	number */
    size = sizeof(INT);
    db_get_value(hDB,	hKey,	pequipment->driver[i].name,
      &pequipment->driver[i].channels, &size,	TID_INT, TRUE);

    if (pequipment->driver[i].channels ==	0)
      pequipment->driver[i].channels = 1;

    sy2527_info->num_channels	+= pequipment->driver[i].channels;
  }

  if (sy2527_info->num_channels	== 0)	{
		cm_msg(MERROR, "hv_init",	"No	channels found in	device driver	list");
		return FE_ERR_ODB;
	}

	/* Allocate	memory for buffers */
	sy2527_info->names = (char *)	calloc(sy2527_info->num_channels,	NAME_LENGTH);

	sy2527_info->demand	=	(float *)	calloc(sy2527_info->num_channels,	sizeof(float));
	sy2527_info->measured	=	(float *)	calloc(sy2527_info->num_channels,	sizeof(float));
	sy2527_info->current	=	(float *)	calloc(sy2527_info->num_channels,	sizeof(float));
	sy2527_info->raised	=	(BOOL	*) calloc(sy2527_info->num_channels, sizeof(BOOL));

	sy2527_info->update_threshold	=	(float *)	calloc(sy2527_info->num_channels,	sizeof(float));

	sy2527_info->update_threshold_current	=	(float *)	calloc(sy2527_info->num_channels,	sizeof(float));

	sy2527_info->demand_mirror = (float	*) calloc(sy2527_info->num_channels, sizeof(float));
	sy2527_info->measured_mirror = (float	*) calloc(sy2527_info->num_channels, sizeof(float));
	sy2527_info->current_mirror = (float	*) calloc(sy2527_info->num_channels, sizeof(float));
	sy2527_info->voltage_mirror = (float	*) calloc(sy2527_info->num_channels, sizeof(float));
	sy2527_info->rampup_mirror = (float	*) calloc(sy2527_info->num_channels, sizeof(float));
	sy2527_info->rampdown_mirror = (float	*) calloc(sy2527_info->num_channels, sizeof(float));
	sy2527_info->last_change = (DWORD	*) calloc(sy2527_info->num_channels, sizeof(DWORD));

  sy2527_info->voltage_limit = (float *) calloc(sy2527_info->num_channels, sizeof(float));
  sy2527_info->current_limit = (float *) calloc(sy2527_info->num_channels, sizeof(float));
  sy2527_info->rampup_speed = (float *) calloc(sy2527_info->num_channels, sizeof(float));
  sy2527_info->rampdown_speed = (float *) calloc(sy2527_info->num_channels, sizeof(float));

  sy2527_info->dd_info = (void *)	calloc(sy2527_info->num_channels,	sizeof(void	*));
	sy2527_info->channel_offset	=	(INT *)	calloc(sy2527_info->num_channels,	sizeof(INT));
	sy2527_info->driver	=	(void	*) calloc(sy2527_info->num_channels, sizeof(void *));

	if (!sy2527_info->driver)	{
		cm_msg(MERROR, "hv_init",	"Not enough	memory");
		return FE_ERR_ODB;
	}

	/*---- Initialize	device drivers ----*/

	/* call	init method	*/
	for	(i = 0;	pequipment->driver[i].name[0]; i++)	{
		sprintf(str, "Settings/Devices/%s",	pequipment->driver[i].name);
		status = db_find_key(hDB,	sy2527_info->hKeyRoot, str,	&hKey);
		if (status !=	DB_SUCCESS)	{
			db_create_key(hDB, sy2527_info->hKeyRoot,	str, TID_KEY);
			status = db_find_key(hDB,	sy2527_info->hKeyRoot, str,	&hKey);
			if (status !=	DB_SUCCESS)	{
				cm_msg(MERROR, "hv_init",	"Cannot	create %s	entry	in online	database", str);
				free_mem(sy2527_info);
				return FE_ERR_ODB;
			}
		}

		status = pequipment->driver[i].dd(CMD_INIT,	hKey,	&pequipment->driver[i].dd_info,
			pequipment->driver[i].channels,
			pequipment->driver[i].flags,
			pequipment->driver[i].bd);
		if (status !=	FE_SUCCESS)	{
			free_mem(sy2527_info);
			return status;
		}
	}

	/* compose device	driver channel assignment	*/
	for	(i = 0,	j	=	0, index = 0,	offset = 0;	i	<	sy2527_info->num_channels; i++,	j++) {
		while	(j >=	pequipment->driver[index].channels &&	pequipment->driver[index].name[0]) {
			offset +=	j;
			index++;
			j	=	0;
		}

		sy2527_info->driver[i] = pequipment->driver[index].dd;
		sy2527_info->dd_info[i]	=	pequipment->driver[index].dd_info;
		sy2527_info->channel_offset[i] = offset;
	}

  /*---- Create Read/Write Settings 
  based on Current Device Settings ----*/
  /** Under Settings:
      Update Threshold Measured
      Update Threshold Current (measured)
      Voltage Limit
      Current Limit
      Ramp Speed Up 
      Ramp Speed Down
  */
  /*-------------------------------------------------------------*/
	/*---- get default names from	device driver	----*/
	for	(i = 0;	i	<	sy2527_info->num_channels; i++)	{
		sprintf(sy2527_info->names + NAME_LENGTH * i,	"Dft%%CH_%d",	i);
		DRIVER(i)	(CMD_GET_DEFAULT_NAME, sy2527_info->dd_info[i],
			i	-	sy2527_info->channel_offset[i],	sy2527_info->names + NAME_LENGTH * i);
	}
  if (db_find_key(hDB, sy2527_info->hKeyRoot, "Settings/Names", &hKey) != DB_SUCCESS) {
	db_merge_data(hDB, sy2527_info->hKeyRoot,	"Settings/Names",
		sy2527_info->names,	NAME_LENGTH	*	sy2527_info->num_channels,
		sy2527_info->num_channels, TID_STRING);
  }
  db_find_key(hDB, sy2527_info->hKeyRoot, "Settings/Names", &hKey);
  if (pequipment->driver[0].flags && DF_PRIO_DEVICE) {
  status = db_set_data(hDB, hKey,	sy2527_info->names,
			NAME_LENGTH	*	sy2527_info->num_channels, sy2527_info->num_channels,
			TID_STRING);
  } else {
	db_merge_data(hDB, sy2527_info->hKeyRoot,	"Settings/Names",
		sy2527_info->names,	NAME_LENGTH	*	sy2527_info->num_channels,
		sy2527_info->num_channels, TID_STRING);
  }
  db_open_record(hDB, hKey, sy2527_info->names,
    NAME_LENGTH * sy2527_info->num_channels, MODE_READ, sy2527_update_label, pequipment);

  /*-------------------------------------------------------------*/
  /* Update threshold (ODB defined) */
  for (i = 0; i < sy2527_info->num_channels; i++)
    sy2527_info->update_threshold[i] = 2.f;       /* default 2V */
    db_merge_data(hDB, sy2527_info->hKeyRoot, "Settings/Update Threshold Measured",
    sy2527_info->update_threshold, sizeof(float) * sy2527_info->num_channels,
    sy2527_info->num_channels, TID_FLOAT);

/*-------------------------------------------------------------*/
  /* Voltage limit (device read) */
	for	(i = 0;	i	<	sy2527_info->num_channels; i++)	{
		DRIVER(i)	(CMD_GET_VOLTAGE_LIMIT, sy2527_info->dd_info[i],
			i	-	sy2527_info->channel_offset[i],	&sy2527_info->voltage_limit[i]);
		sy2527_info->voltage_mirror[i]	=	-1234.f;		 /*	invalid	value	*/
	}
  if (db_find_key(hDB, sy2527_info->hKeyRoot, "Settings/Voltage Limit", &hKey) != DB_SUCCESS) {
  db_merge_data(hDB, sy2527_info->hKeyRoot, "Settings/Voltage Limit",
    sy2527_info->voltage_limit, sizeof(float) * sy2527_info->num_channels,
    sy2527_info->num_channels, TID_FLOAT);
  }
  db_find_key(hDB, sy2527_info->hKeyRoot, "Settings/Voltage Limit", &hKey);
  if (pequipment->driver[0].flags && DF_PRIO_DEVICE) {
  status = db_set_data(hDB, hKey,	sy2527_info->voltage_limit,
			sizeof(float)	*	sy2527_info->num_channels, sy2527_info->num_channels,
			TID_FLOAT);
  } else {
  db_merge_data(hDB, sy2527_info->hKeyRoot, "Settings/Voltage Limit",
    sy2527_info->voltage_limit, sizeof(float) * sy2527_info->num_channels,
    sy2527_info->num_channels, TID_FLOAT);
  }
  db_open_record(hDB, hKey, sy2527_info->voltage_limit
    , sizeof(float) * sy2527_info->num_channels
    , MODE_READ, hv_voltage_limit, pequipment);

/*-------------------------------------------------------------*/
  /* Current limit (device read) */
	for	(i = 0;	i	<	sy2527_info->num_channels; i++)	{
		DRIVER(i)	(CMD_GET_CURRENT_LIMIT, sy2527_info->dd_info[i],
			i	-	sy2527_info->channel_offset[i],	&sy2527_info->current_limit[i]);
		sy2527_info->current_mirror[i]	=	-1234.f;		 /*	invalid	value	*/
	}
  /* Set ODB with device values */
  if (db_find_key(hDB, sy2527_info->hKeyRoot, "Settings/Current Limit", &hKey) != DB_SUCCESS) {
  db_merge_data(hDB, sy2527_info->hKeyRoot, "Settings/Current Limit",
    sy2527_info->current_limit, sizeof(float) * sy2527_info->num_channels,
    sy2527_info->num_channels, TID_FLOAT);
  }
  db_find_key(hDB, sy2527_info->hKeyRoot, "Settings/Current Limit", &hKey);
  if (pequipment->driver[0].flags && DF_PRIO_DEVICE) {
  status = db_set_data(hDB, hKey,	sy2527_info->current_limit,
			sizeof(float)	*	sy2527_info->num_channels, sy2527_info->num_channels,
			TID_FLOAT);
  } else {
  db_merge_data(hDB, sy2527_info->hKeyRoot, "Settings/Current Limit",
    sy2527_info->current_limit, sizeof(float) * sy2527_info->num_channels,
    sy2527_info->num_channels, TID_FLOAT);
  }
  db_open_record(hDB, hKey, sy2527_info->current_limit
    , sizeof(float) * sy2527_info->num_channels
    , MODE_READ, hv_current_limit, pequipment);

/*-------------------------------------------------------------*/
  /* Ramp Up speed (device read) */
	for	(i = 0;	i	<	sy2527_info->num_channels; i++)	{
		DRIVER(i)	(CMD_GET_RAMPUP, sy2527_info->dd_info[i],
			i	-	sy2527_info->channel_offset[i],	&sy2527_info->rampup_speed[i]);
		sy2527_info->rampup_mirror[i]	=	-1234.f;		 /*	invalid	value	*/
	}
  if (db_find_key(hDB, sy2527_info->hKeyRoot, "Settings/Ramp Up Speed", &hKey) != DB_SUCCESS) {
  db_merge_data(hDB, sy2527_info->hKeyRoot, "Settings/Ramp Up Speed",
    sy2527_info->rampup_speed, sizeof(float) * sy2527_info->num_channels,
    sy2527_info->num_channels, TID_FLOAT);
  }
  db_find_key(hDB, sy2527_info->hKeyRoot, "Settings/Ramp Up Speed", &hKey);
  if (pequipment->driver[0].flags && DF_PRIO_DEVICE) {
  status = db_set_data(hDB, hKey,	sy2527_info->rampup_speed,
			sizeof(float)	*	sy2527_info->num_channels, sy2527_info->num_channels,
			TID_FLOAT);
  } else {
  db_merge_data(hDB, sy2527_info->hKeyRoot, "Settings/Ramp Up Speed",
    sy2527_info->rampup_speed, sizeof(float) * sy2527_info->num_channels,
    sy2527_info->num_channels, TID_FLOAT);
  }
  db_open_record(hDB, hKey, sy2527_info->rampup_speed
    , sizeof(float) * sy2527_info->num_channels
    , MODE_READ, hv_rampup_speed, pequipment);

/*-------------------------------------------------------------*/
  /* Ramp Down speed (device read) */
  for	(i = 0;	i	<	sy2527_info->num_channels; i++)	{
		DRIVER(i)	(CMD_GET_RAMPDOWN, sy2527_info->dd_info[i],
			i	-	sy2527_info->channel_offset[i],	&sy2527_info->rampdown_speed[i]);
		sy2527_info->rampdown_mirror[i]	=	-1234.f;		 /*	invalid	value	*/
	}
  if (db_find_key(hDB, sy2527_info->hKeyRoot, "Settings/Ramp Down Speed", &hKey) != DB_SUCCESS) {
  db_merge_data(hDB, sy2527_info->hKeyRoot, "Settings/Ramp Down Speed",
    sy2527_info->rampdown_speed, sizeof(float) * sy2527_info->num_channels,
    sy2527_info->num_channels, TID_FLOAT);
  }
  db_find_key(hDB, sy2527_info->hKeyRoot, "Settings/Ramp Down Speed", &hKey);
  if (pequipment->driver[0].flags && DF_PRIO_DEVICE) {
  status = db_set_data(hDB, hKey,	sy2527_info->rampdown_speed,
			sizeof(float)	*	sy2527_info->num_channels, sy2527_info->num_channels,
			TID_FLOAT);
  } else {
  db_merge_data(hDB, sy2527_info->hKeyRoot, "Settings/Ramp Down Speed",
    sy2527_info->rampdown_speed, sizeof(float) * sy2527_info->num_channels,
    sy2527_info->num_channels, TID_FLOAT);
  }
  db_open_record(hDB, hKey, sy2527_info->rampdown_speed
    , sizeof(float) * sy2527_info->num_channels
    , MODE_READ, hv_rampdown_speed, pequipment);
/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/
  /*---- create	demand variables ----*/
  /** Under Variables:
  Demand (voltage)  (device read/write)
  Measured (voltage) (device read)

  */
	for	(i = 0;	i	<	sy2527_info->num_channels; i++)	{
		DRIVER(i)	(CMD_GET_DEMAND, sy2527_info->dd_info[i],
			i	-	sy2527_info->channel_offset[i],	&sy2527_info->demand[i]);
		sy2527_info->demand_mirror[i]	=	-1234.f;		 /*	invalid	value	*/
	}
	/* write back	demand values	*/
	status = db_find_key(hDB, sy2527_info->hKeyRoot, "Variables/Demand"
    , &sy2527_info->hKeyDemand);
	if (status !=	DB_SUCCESS)	{
		db_create_key(hDB, sy2527_info->hKeyRoot,	"Variables/Demand",	TID_FLOAT);
		db_find_key(hDB, sy2527_info->hKeyRoot,	"Variables/Demand",	&sy2527_info->hKeyDemand);
	}
	size = sizeof(float) * sy2527_info->num_channels;
	db_set_data(hDB, sy2527_info->hKeyDemand,	sy2527_info->demand, size,
		sy2527_info->num_channels, TID_FLOAT);
	db_open_record(hDB,	sy2527_info->hKeyDemand, sy2527_info->demand,
		sy2527_info->num_channels	*	sizeof(float), MODE_READ,	sy2527_demand,
		pequipment);

/*-------------------------------------------------------------*/
	/*---- create	measured variables ----*/
	db_merge_data(hDB, sy2527_info->hKeyRoot,	"Variables/Measured",
		sy2527_info->measured, sizeof(float) * sy2527_info->num_channels,
		sy2527_info->num_channels, TID_FLOAT);
	db_find_key(hDB, sy2527_info->hKeyRoot,	"Variables/Measured",	&sy2527_info->hKeyMeasured);
	memcpy(sy2527_info->measured_mirror, sy2527_info->measured,
		sy2527_info->num_channels	*	sizeof(float));

	/*---- create	measured current	variables	----*/
	db_merge_data(hDB, sy2527_info->hKeyRoot,	"Variables/current",
		sy2527_info->current, sizeof(float) * sy2527_info->num_channels,
		sy2527_info->num_channels, TID_FLOAT);
	db_find_key(hDB, sy2527_info->hKeyRoot,	"Variables/current",	&sy2527_info->hKeycurrent);
	memcpy(sy2527_info->current_mirror, sy2527_info->current,
		sy2527_info->num_channels	*	sizeof(float));

/*-------------------------------------------------------------*/
	/*---- set initial demand	values (write to device) ----*/
	sy2527_demand(hDB, sy2527_info->hKeyDemand,	pequipment);

	/* initially read	all	channels */
	for	(i = 0;	i	<	sy2527_info->num_channels; i++)
		sy2527_read(pequipment,	i);

	return FE_SUCCESS;
}

/*------------------------------------------------------------------*/

INT	sy2527_exit(EQUIPMENT	*	pequipment)
{
	INT	i;

	free_mem((SY2527_INFO	*) pequipment->cd_info);

	/* call	exit method	of device	drivers	*/
	for	(i = 0;	pequipment->driver[i].dd !=	NULL;	i++)
		pequipment->driver[i].dd(CMD_EXIT, pequipment->driver[i].dd_info);

	return FE_SUCCESS;
}

/*------------------------------------------------------------------*/
INT	sy2527_idle(EQUIPMENT	*	pequipment)
{
	static BOOL	odb_update_flag	=	FALSE;
	static INT last_raised_channel = -1;
	static INT number_raised = 0;
	INT	act, i,	next_raised, status, loop;
	DWORD	act_time;
	SY2527_INFO	*sy2527_info;
	HNDLE	hDB;

	/* Class info	shortcut */
	sy2527_info	=	(SY2527_INFO *)	pequipment->cd_info;

	/* get current time	*/
	act_time = ss_millitime();

	/* Do	loop N channel per idle	*/
	for	(loop	=	0; loop	<	number_raised	+	1; loop++) {

		/* Select	channel	to read	*/
		/* k = last_raised_channel;	*/

		if (last_raised_channel	<	0) {
			/* Normal	channel	*/
			act	=	(sy2527_info->last_channel + 1)	%	sy2527_info->num_channels;
			//			printf("N			:%d	", act);
		}	else {
			/* Raised	channel	*/
			act	=	last_raised_channel;
			//			printf("		R	:%d	", act);
		}

		/* Read	channel	*/
		status = sy2527_read(pequipment, act);

		/* Check read	value	against	previous value (mirror)	*/
		if (abs(sy2527_info->measured[act] - sy2527_info->measured_mirror[act])	>
			sy2527_info->update_threshold[act])	{
				/* Greater than	Delta	=> raise channel for close monitoring	*/
				sy2527_info->raised[act] = TRUE;
				/* Force an	ODB	update to	reflect	change */
				odb_update_flag	=	TRUE;
			}	else {
				/* Within	the	limits =>	lower	channel	if it	was	raised */
				sy2527_info->raised[act] = FALSE;
			}

//			printf("F:%d mes:%f	mir:%f th:%f\n", sy2527_info->raised[act]
//			,	sy2527_info->measured[act]
//			,sy2527_info->measured_mirror[act],	sy2527_info->update_threshold[act]);

			/* hold	last value */
			sy2527_info->measured_mirror[act]	=	sy2527_info->measured[act];

			/* Get next	raised channel if	any	(-1:none)	*/
			if (act	== sy2527_info->num_channels)	{
				next_raised	=	-1;
			}	else {
				for	(next_raised = act + 1;	next_raised	<	sy2527_info->num_channels; next_raised++)	{
					if (sy2527_info->raised[next_raised])
						break;
				}
				if (next_raised	== sy2527_info->num_channels)	{
					next_raised	=	-1;
				}
			}

			/* update	last raised	channel	for	next go	*/
			if (next_raised	<	0) {
				/* no	more raised	channel	set	normal channel (increment	channel) */
				sy2527_info->last_channel	=	(sy2527_info->last_channel + 1)	%	sy2527_info->num_channels;
				last_raised_channel	=	-1;
			}	else {
				/* More	raised channel then	set	for	next round */
				last_raised_channel	=	next_raised;
			}

			/* count the raised	channels
			to be	used for possible	multiple read	per	idle
			see	for	loop at	begining of	function.
			*/
			for	(i = 0,	number_raised	=	0; i < sy2527_info->num_channels;	i++) {
				if (sy2527_info->raised[i])
					number_raised++;
			}
	}

	/* sends new values	to ODB if:
	ODB_UPDATE_TIME	timeout	exceeded
	or
	At least one channel has been	raised */
	if (odb_update_flag	|| ((act_time	-	sy2527_info->last_update)	>	ODB_UPDATE_TIME))	{
		cm_get_experiment_database(&hDB, NULL);
		db_set_data(hDB, sy2527_info->hKeyMeasured,	sy2527_info->measured,
			sizeof(float)	*	sy2527_info->num_channels, sy2527_info->num_channels,
			TID_FLOAT);
		db_set_data(hDB, sy2527_info->hKeycurrent,	sy2527_info->current,
			sizeof(float)	*	sy2527_info->num_channels, sy2527_info->num_channels,
			TID_FLOAT);
		sy2527_info->last_update = act_time;
		odb_update_flag	=	FALSE;

		/* Keep	odb	count	*/
		pequipment->odb_out++;
	}
	return status;
}

/*------------------------------------------------------------------*/
INT	cd_sy2527_read(char	*pevent, int offset)
{
	float	*pdata;
	DWORD	*pdw;
	SY2527_INFO	*sy2527_info;
	EQUIPMENT	*pequipment;

	pequipment = *((EQUIPMENT	**)	pevent);
	sy2527_info	=	(SY2527_INFO *)	pequipment->cd_info;

	if (sy2527_info->format	== FORMAT_FIXED) {
		memcpy(pevent, sy2527_info->demand,	sizeof(float)	*	sy2527_info->num_channels);
		pevent +=	sizeof(float)	*	sy2527_info->num_channels;

		memcpy(pevent, sy2527_info->measured,	sizeof(float)	*	sy2527_info->num_channels);
		pevent +=	sizeof(float)	*	sy2527_info->num_channels;

		return 2 * sizeof(float) * sy2527_info->num_channels;
	}	else if	(sy2527_info->format ==	FORMAT_MIDAS)	{
		bk_init(pevent);

		/* create	DMND bank	*/
		bk_create(pevent,	"VDMN",	TID_FLOAT, &pdata);
		memcpy(pdata,	sy2527_info->demand, sizeof(float) * sy2527_info->num_channels);
		pdata	+= sy2527_info->num_channels;
		bk_close(pevent, pdata);

		/* create	MSRD bank	*/
		bk_create(pevent,	"VMSR",	TID_FLOAT, &pdata);
		memcpy(pdata,	sy2527_info->measured, sizeof(float) * sy2527_info->num_channels);
		pdata	+= sy2527_info->num_channels;
		bk_close(pevent, pdata);

		/* create	MSRD bank	*/
		bk_create(pevent,	"IMSR",	TID_FLOAT, &pdata);
		memcpy(pdata,	sy2527_info->current, sizeof(float) * sy2527_info->num_channels);
		pdata	+= sy2527_info->num_channels;
		bk_close(pevent, pdata);

		return bk_size(pevent);
	}	else if	(sy2527_info->format ==	FORMAT_YBOS) {
		ybk_init((DWORD	*) pevent);

		/* create	EVID bank	*/
		ybk_create((DWORD	*) pevent, "EVID", I4_BKTYPE,	(DWORD *)	(&pdw));
		*(pdw)++ = EVENT_ID(pevent);			/* Event_ID	+	Mask */
		*(pdw)++ = SERIAL_NUMBER(pevent);	/* Serial	number */
		*(pdw)++ = TIME_STAMP(pevent);		/* Time	Stamp	*/
		ybk_close((DWORD *)	pevent,	pdw);

		/* create	DMND bank	*/
		ybk_create((DWORD	*) pevent, "DMND", F4_BKTYPE,	(DWORD *)	&	pdata);
		memcpy(pdata,	sy2527_info->demand, sizeof(float) * sy2527_info->num_channels);
		pdata	+= sy2527_info->num_channels;
		ybk_close((DWORD *)	pevent,	pdata);

		/* create	MSRD bank	*/
		ybk_create((DWORD	*) pevent, "MSRD", F4_BKTYPE,	(DWORD *)	&	pdata);
		memcpy(pdata,	sy2527_info->measured, sizeof(float) * sy2527_info->num_channels);
		pdata	+= sy2527_info->num_channels;
		ybk_close((DWORD *)	pevent,	pdata);

		return ybk_size((DWORD *)	pevent);
	}	else
		return 0;
}

/*------------------------------------------------------------------*/

INT	cd_sy2527(INT	cmd, EQUIPMENT * pequipment)
{
	INT	status;

	switch (cmd) {
	 case	CMD_INIT:
		 status	=	sy2527_init(pequipment);
		 break;

	 case	CMD_EXIT:
		 status	=	sy2527_exit(pequipment);
		 break;

	 case	CMD_IDLE:
		 status	=	sy2527_idle(pequipment);
		 break;

	 default:
		 cm_msg(MERROR,	"Generic class driver",	"Received	unknown	command	%d", cmd);
		 status	=	FE_ERR_DRIVER;
		 break;
	}

	return status;
}
