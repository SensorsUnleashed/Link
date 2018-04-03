/*
 * defaultDeviceSetup.h
 *
 *  Created on: 2. apr. 2018
 *      Author: omn
 */

#ifndef BOARDS_DEV_DEFAULTDEVICESETUP_H_
#define BOARDS_DEV_DEFAULTDEVICESETUP_H_

#include "susensors.h"

int deviceSetupGet(const char* devicename, settings_t* setup, const settings_t* defaultsetting);
int deviceSetupSave(const char* devicename, settings_t* setup);

#endif /* BOARDS_DEV_DEVICESETUP_H_ */
