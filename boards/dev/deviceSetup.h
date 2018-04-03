/*
 * defaultDeviceSetup.h
 *
 *  Created on: 2. apr. 2018
 *      Author: omn
 */

#ifndef BOARDS_DEV_DEFAULTDEVICESETUP_H_
#define BOARDS_DEV_DEFAULTDEVICESETUP_H_

#include "susensors.h"

extern settings_t default_relaysetting;
extern settings_t default_yellow_led_setting;
extern settings_t default_pulseCounter_settings;
extern settings_t default_mainsDetector_settings;
extern settings_t default_pushbutton_settings;
extern settings_t default_timer_settings;

int deviceSetupGet(const char* devicename, settings_t* setup, settings_t* defaultsetting);
int deviceSetupSave(const char* devicename, settings_t* setup);
#endif /* BOARDS_DEV_DEVICESETUP_H_ */
