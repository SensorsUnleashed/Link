/*******************************************************************************
 * Copyright (c) 2017, Ole Nissen.
 *  All rights reserved. 
 *  
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions 
 *  are met: 
 *  1. Redistributions of source code must retain the above copyright 
 *  notice, this list of conditions and the following disclaimer. 
 *  2. Redistributions in binary form must reproduce the above
 *  copyright notice, this list of conditions and the following
 *  disclaimer in the documentation and/or other materials provided
 *  with the distribution. 
 *  3. The name of the author may not be used to endorse or promote
 *  products derived from this software without specific prior
 *  written permission.  
 *  
 *  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 *  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 *  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  
 *
 * This file is part of the Sensors Unleashed project
 *******************************************************************************/
/*
 * pulsesensor.h
 *
 *  Created on: 08/12/2016
 *      Author: omn
 */

#ifndef SENSORSUNLEASHED_DEV_PULSESENSOR_H_
#define SENSORSUNLEASHED_DEV_PULSESENSOR_H_

#include "susensors.h"

#define PULSE_SENSOR "su/pulsecounter"

extern struct resourceconf pulseconfig;
susensors_sensor_t* addASUPulseInputRelay(const char* name, struct resourceconf* config);


/**
 * NOTE:
 * The pulsecounter will generate interrupts for every changeEvent value. This means, that
 * the resolution will be this value. Below event, will never get fired, because, for now,
 * the pulse counter cant count down (Could be an option later on, to allow that). The above
 * event will be fired when the counting value goes beyond the set value, but still, the
 * resolution will be defined by the changeEvent value.
 */

#endif /* SENSORSUNLEASHED_DEV_PULSESENSOR_H_ */
