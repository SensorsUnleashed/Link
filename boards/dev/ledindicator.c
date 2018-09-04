/*******************************************************************************
 * Copyright (c) 2018, Ole Nissen.
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

#include "ledindicator.h"
#include "contiki.h"
#include "dev/leds.h"
#include "deviceSetup.h"
#include "susensorcommon.h"
#include "board.h"

typedef enum su_basic_actions su_led_actions;

struct resourceconf ledindicatorconfig = {
		.resolution = 1,
		.version = 1,
		.flags = METHOD_GET | METHOD_PUT | IS_OBSERVABLE | HAS_SUB_RESOURCES,
		.max_pollinterval = 2,
		.unit = "",
		.spec = "LED indicator",
		//.type = LED_INDICATOR,
		.attr = "title=\"LED indicator\" ;rt=\"Indicator\"",
};

static const settings_t default_led_setting = {
		.eventsActive = ChangeEventActive,
		.AboveEventAt = {
				.type = CMP_TYPE_UINT8,
				.as.u8 = 1
		},
		.BelowEventAt = {
				.type = CMP_TYPE_UINT8,
				.as.u8 = 0
		},
		.ChangeEvent = {
				.type = CMP_TYPE_UINT8,
				.as.u8 = 1
		},
		.RangeMin = {
				.type = CMP_TYPE_UINT16,
				.as.u8 = 0
		},
		.RangeMax = {
				.type = CMP_TYPE_UINT16,
				.as.u8 = 1
		},
};


static int set(struct susensors_sensor* this, int type, void* data){
	int ret = 1;

	if(type == setToggle){
		leds_toggle(((struct ledRuntime*)(this->data.runtime))->mask);
		ret = 0;
	}
	else if(type == setOn){
		leds_on(((struct ledRuntime*)(this->data.runtime))->mask);
		ret = 0;
	}
	else if(type == setOff){
		leds_off(((struct ledRuntime*)(this->data.runtime))->mask);
		ret = 0;
	}

	return ret;
}

static int configure(struct susensors_sensor* this, int type, int value){
	return 0;
}

int get(struct susensors_sensor* this, int type, void* data){
	int ret = 1;
	cmp_object_t* obj = (cmp_object_t*)data;

	if((enum up_parameter) type == ActualValue){
		obj->type = CMP_TYPE_UINT8;
		obj->as.u8 = (leds_get() & ((struct ledRuntime*)(this->data.runtime))->mask) > 0;
		ret = 0;
	}
	return ret;
}

static int  setOnhandler(struct susensors_sensor* this, int len, const uint8_t* payload){
	this->value(this, setOn, NULL);
	return 0;
}
static int  setOffhandler(struct susensors_sensor* this, int len, const uint8_t* payload){
	this->value(this, setOff, NULL);
	return 0;
}
static int  setChangehandler(struct susensors_sensor* this, int len, const uint8_t* payload){
	this->value(this, setToggle, NULL);
	return 0;
}


/* Return the function to call when a specified trigger is in use */
static void* getFunctionPtr(su_led_actions trig){

	if(trig >= setOn && trig <= setToggle){
		switch(trig){
		case setOn:
			return setOnhandler;
			break;
		case setOff:
			return setOffhandler;
			break;
		case setToggle:
			return setChangehandler;
			break;
		default:
			return NULL;
		}
	}

	return NULL;
}

static eventhandler_ptr setEventhandlers(struct susensors_sensor* this, int8_t triggers){

	return getFunctionPtr(triggers);
}

susensors_sensor_t* addASULedIndicator(const char* name, settings_t* settings, struct ledRuntime* extra){

	if(deviceSetupGet(name, settings, &default_led_setting) < 0) return 0;

	susensors_sensor_t d;
	d.type = (char*)name;
	d.status = get;
	d.value = set;
	d.configure = configure;
	d.eventhandler = testevent;
	d.suconfig = suconfig;
	d.data.setting = settings;

	d.setEventhandlers = setEventhandlers;

	d.data.config = &ledindicatorconfig;
	d.data.runtime = extra;
	d.data.runtime = (void*) extra;

	ledindicatorconfig.type = (char*)name;

	return addSUDevices(&d);
}
