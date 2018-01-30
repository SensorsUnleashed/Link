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
#ifndef RELAY_H_
#define RELAY_H_
/* -------------------------------------------------------------------------- */

#include "lib/susensors.h"

extern struct resourceconf relayconfigs;

susensors_sensor_t* addASURelay(const char* name, struct resourceconf* config);

#ifdef RELAY_CONF_PIN
#define RELAY_PIN        RELAY_CONF_PIN
#else
#define RELAY_PIN        4
#endif
#ifdef RELAY_CONF_PORT
#define RELAY_PORT       RELAY_CONF_PORT
#else
#define RELAY_PORT       GPIO_A_NUM
#endif

#define RELAY_ERROR             (1)
#define RELAY_SUCCESS           0x00

#define RELAY_ACTUATOR "su/powerrelay"
extern struct susensors_sensor relay;

#endif /* RELAY_H_ */
