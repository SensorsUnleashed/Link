/*
 * timerdevice.h
 *
 *  Created on: 26. mar. 2018
 *      Author: omn
 */

#ifndef BOARDS_DEV_TIMERDEVICE_H_
#define BOARDS_DEV_TIMERDEVICE_H_

#include "susensors.h"
#include "cmp_helpers.h"

extern struct resourceconf timerconfig;

susensors_sensor_t* addASUTimerDevice(const char* name, struct resourceconf* config);

#define TIMER_DEVICE "su/timer"

#endif /* BOARDS_DEV_TIMERDEVICE_H_ */
