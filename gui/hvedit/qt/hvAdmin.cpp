/*******************************************************************************************

  hvAdmin.cpp

  Administration class to encapsulate midas online data base (ODB) settings.

  The constructor tries to read the file '.hvEdit' in which all the relevant
  ODB pathes can be given. If '.hvEdit' does not exist, some default settings
  will be tried.

*********************************************************************************************

    begin                : Andreas Suter, 2002/11/22
    modfied:             :
    copyright            : (C) 2003 by
    email                : andreas.suter@psi.ch

  $Log$
  Revision 1.1  2003/05/09 10:08:09  midas
  Initial revision

  Revision 1.1.1.2  2003/03/03 09:33:50  nemu
  add documentation path info to help facility.

  Revision 1.1.1.1  2003/02/27 15:26:12  nemu
  added log info


********************************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <stdio.h>
#include <string.h>

#include <qstring.h>

#include "hvAdmin.h"

//**********************************************************************
// hvAdmin constructor
//
// Tries to read '.hvEdit' file in order to get the correct ODB pathes.
// If '.hvEdit' doesn't exist, default pathes are tried.
//**********************************************************************
hvAdmin::hvAdmin()
{
  FILE *fp;
  char str[80], info[80];

  strcpy(info, ""); // initialize string

  // initialize flags for ODB pathes
  b_hvA_default_dir_hv_settings = FALSE;
  b_hvA_default_dir_docu = FALSE;
  b_hvA_midas_odb_hv_root = FALSE;
  b_hvA_midas_odb_hv_names = FALSE;
  b_hvA_midas_odb_hv_demand = FALSE;
  b_hvA_midas_odb_hv_measured = FALSE;
  b_hvA_midas_odb_hv_current = FALSE;
  b_hvA_midas_odb_hv_current_limit = FALSE;

  // read .hvEdit file
  if ((fp = fopen(".hvEdit", "r")) != NULL) {  // '.hvEdit' exists
    while (!feof(fp)) {
      if (fgets(str, 80, fp) == NULL) break;
      // default directory were the hv settings files should be found.
      if (strstr(str, "DEFAULT_SETTING_DIR=") != NULL) {
        hvA_default_dir_hv_settings = str+strlen("DEFAULT_SETTING_DIR=");
	//hvA_default_dir_hv_settings.remove("\n"); // Qt3.1
	hvA_default_dir_hv_settings.truncate(hvA_default_dir_hv_settings.length()-1); // Qt3.0.3
	b_hvA_default_dir_hv_settings = TRUE;
      }
      // default directory were to find documentation.
      if (strstr(str, "DEFAULT_DOC_DIR=") != NULL) {
        hvA_default_dir_docu = str+strlen("DEFAULT_DOC_DIR=");
	//hvA_default_dir_docu.remove("\n"); // Qt3.1
	hvA_default_dir_docu.truncate(hvA_default_dir_docu.length()-1); // Qt3.0.3
	b_hvA_default_dir_docu = TRUE;
      }
      // midas root key for the hv equipment.
      if (strstr(str, "MIDAS_KEY_HV_ROOT=") != NULL) {
        hvA_midas_odb_hv_root = str+strlen("MIDAS_KEY_HV_ROOT=");
        //hvA_midas_odb_hv_root.remove("\n"); // Qt3.1
	hvA_midas_odb_hv_root.truncate(hvA_midas_odb_hv_root.length()-1); // Qt3.0.3
	b_hvA_midas_odb_hv_root = TRUE;
      }
      // midas names key for the hv equipment.
      if (strstr(str, "MIDAS_KEY_HV_NAMES=") != NULL) {
        hvA_midas_odb_hv_names = str+strlen("MIDAS_KEY_HV_NAMES=");
	//hvA_midas_odb_hv_names.remove("\n"); // Qt3.1
	hvA_midas_odb_hv_names.truncate(hvA_midas_odb_hv_names.length()-1); // Qt3.0.3
	b_hvA_midas_odb_hv_names = TRUE;
      }
      // midas demand hv key for the hv equipment.
      if (strstr(str, "MIDAS_KEY_HV_DEMAND=") != NULL) {
        hvA_midas_odb_hv_demand = str+strlen("MIDAS_KEY_HV_DEMAND=");
	//hvA_midas_odb_hv_demand.remove("\n"); // Qt3.1
	hvA_midas_odb_hv_demand.truncate(hvA_midas_odb_hv_demand.length()-1); // Qt3.0.3
	b_hvA_midas_odb_hv_demand = TRUE;
      }
      // midas measured hv key for the hv equipment.
      if (strstr(str, "MIDAS_KEY_HV_MEASURED=") != NULL) {
        hvA_midas_odb_hv_measured = str+strlen("MIDAS_KEY_HV_MEASURED=");
	//hvA_midas_odb_hv_measured.remove("\n"); // Qt3.1
	hvA_midas_odb_hv_measured.truncate(hvA_midas_odb_hv_measured.length()-1); // Qt3.0.3
	b_hvA_midas_odb_hv_measured = TRUE;
      }
      // midas current key for the hv equipment.
      if (strstr(str, "MIDAS_KEY_HV_CURRENT=") != NULL) {
        hvA_midas_odb_hv_current = str+strlen("MIDAS_KEY_HV_CURRENT=");
	//hvA_midas_odb_hv_current.remove("\n"); // Qt3.1
	hvA_midas_odb_hv_current.truncate(hvA_midas_odb_hv_current.length()-1); // Qt3.0.3
	b_hvA_midas_odb_hv_current = TRUE;
      }
      // midas current limit key for the hv equipment.
      if (strstr(str, "MIDAS_KEY_HV_CURRENT_LIMIT=") != NULL) {
        hvA_midas_odb_hv_current_limit = str+strlen("MIDAS_KEY_HV_CURRENT_LIMIT=");
	//hvA_midas_odb_hv_current_limit.remove("\n"); // Qt3.1
	hvA_midas_odb_hv_current_limit.truncate(hvA_midas_odb_hv_current_limit.length()-1); // Qt3.0.3
	b_hvA_midas_odb_hv_current_limit = TRUE;
      }
    }
    fclose(fp);
  } else { // '.hvEdit' doesn't exist
    strcpy(info," '.hvEdit' does not exist.");
  }

  // check if all key could been read
  if (!b_hvA_default_dir_hv_settings) { // default path not defined
    hvA_default_dir_hv_settings = "./";
  }
  if (!b_hvA_default_dir_docu) { // default docu path not defined
    hvA_default_dir_docu = "~/";
  }
  if (!b_hvA_midas_odb_hv_root) { // root key
    hvA_midas_odb_hv_root = "/Equipment/HV";
    printf("WARNING: HVEdit - Couldn't read MIDAS_KEY_HV_ROOT, will try default. %s", info);
  }
  if (!b_hvA_midas_odb_hv_names) { // hv names key
    hvA_midas_odb_hv_names = "Settings/Names";
    printf("WARNING: HVEdit - Couldn't read MIDAS_KEY_HV_NAMES, will try default. %s", info);
  }
  if (!b_hvA_midas_odb_hv_demand) { // demand hv key
    hvA_midas_odb_hv_demand = "Variables/Demand";
    printf("WARNING: HVEdit - Couldn't read MIDAS_KEY_HV_DEMAND, will try default. %s", info);
  }
  if (!b_hvA_midas_odb_hv_measured) { // measured hv key
    hvA_midas_odb_hv_measured = "Variables/Measured";
    printf("WARNING: HVEdit - Couldn't read MIDAS_KEY_HV_MEASURED, will try default. %s", info);
  }
  if (!b_hvA_midas_odb_hv_current) { // current hv key
    hvA_midas_odb_hv_current = "Variables/Current";
    printf("WARNING: HVEdit - Couldn't read MIDAS_KEY_HV_CURRENT, will try default. %s", info);
  }
  if (!b_hvA_midas_odb_hv_current_limit) { // current limit hv key
    hvA_midas_odb_hv_current_limit = "Settings/Current Limit";
    printf("WARNING: HVEdit - Couldn't read MIDAS_KEY_HV_CURRENT_LIMIT, will try default. %s", info);
  }
}

//**********************************************************************
// hvAdmin destructor
//**********************************************************************
hvAdmin::~hvAdmin()
{
}
