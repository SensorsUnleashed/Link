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
#include "contiki.h"
#include "relay.h"
#include "dev/gpio.h"
#include "dev/ioc.h"
#include "dev/leds.h"
#include "board.h"
#include "deviceSetup.h"

/*---------------------------------------------------------------------------*/
#define RELAY_PORT_BASE          GPIO_PORT_TO_BASE(RELAY_PORT)
#define RELAY_PIN_MASK           GPIO_PIN_MASK(RELAY_PIN)

#include "susensorcommon.h"

struct susensors_sensor relay;
/*---------------------------------------------------------------------------*/

static struct relayRuntime relayruntime[2];
int noofrelays = 0;

typedef enum su_basic_actions su_relay_actions;

struct resourceconf relayconfig = {
		.resolution = 1,
		.version = 1,
		.flags = METHOD_GET | METHOD_PUT | IS_OBSERVABLE | HAS_SUB_RESOURCES,
		.max_pollinterval = 2,
		.unit = "",
		.spec = "Relay output control OFF=0, ON=1, TOGGLE=2",
		.type = RELAY_ACTUATOR,
		.attr = "title=\"Relay output\" ;rt=\"Control\"",
};


/*---------------------------------------------------------------------------*/

/**
 * Set the relay (this) to ON
 * @param this
 * 	Actual relay
 * @return
 * 	Returns the stepsize and direction
 * 	- is down, + is up and value is step size
 */
static int relay_on(struct susensors_sensor* this)
{
	struct relayRuntime* r = (struct relayRuntime*)this->data.runtime;
	if(r->LastValue.as.u8 == 0){
		r->LastValue.as.u8 = 1;
		GPIO_SET_PIN(RELAY_PORT_BASE, RELAY_PIN_MASK);
		return 0;
	}

	return 1;
}

/**
 * Set the relay (this) to OFF
 * @param this Actual relay
 * @return
 * 	0 if the output of the relay was changed
 * 	1 if the output of the relay was NOT changed
 */
static int relay_off(struct susensors_sensor* this)
{
	struct relayRuntime* r = (struct relayRuntime*)this->data.runtime;
	if(r->LastValue.as.u8 == 1){
		r->LastValue.as.u8 = 0;
		GPIO_CLR_PIN(RELAY_PORT_BASE, RELAY_PIN_MASK);
		return 0;
	}

	return 1;
}


static int get(struct susensors_sensor* this, int type, void* data)
{
	int ret = 1;
	cmp_object_t* obj = (cmp_object_t*)data;
	if((enum up_parameter) type == ActualValue){
		obj->type = CMP_TYPE_UINT8;
		obj->as.u8 = (uint8_t) GPIO_READ_PIN(RELAY_PORT_BASE, RELAY_PIN_MASK) > 0;
		ret = 0;
	}
	return ret;
}

static int set(struct susensors_sensor* this, int type, void* data)
{
	int enabled = ((struct relayRuntime*)this->data.runtime)->enabled;
	int ret = 1;
	if((su_relay_actions)type == setOff && enabled){
		if((ret = relay_off(this)) == 0){	//The relay changed from 1 -> 0
			setEventU8(this, -1, 1);
		}
	}
	else if((su_relay_actions)type == setOn && enabled){
		if((ret = relay_on(this)) == 0){	//The relay changed from 0 -> 1
			setEventU8(this, 1, 1);
		}
	}
	else if((su_relay_actions)type == setToggle && enabled){
		if(GPIO_READ_PIN(RELAY_PORT_BASE, RELAY_PIN_MASK) > 0){
			if((ret = relay_off(this)) == 0){
				setEventU8(this, -1, 1);
			}
		}
		else{
			if((ret = relay_on(this)) == 0){
				setEventU8(this, 1, 1);
			}
		}
	}
	return ret;
}

static int configure(struct susensors_sensor* this, int type, int value)
{
	switch(type) {
	case SUSENSORS_HW_INIT:
		break;
	case SUSENSORS_ACTIVE:
		if(value){	//Activate
			GPIO_SOFTWARE_CONTROL(RELAY_PORT_BASE, RELAY_PIN_MASK);
			GPIO_SET_OUTPUT(RELAY_PORT_BASE, RELAY_PIN_MASK);
			ioc_set_over(RELAY_PORT, RELAY_PIN, IOC_OVERRIDE_OE);
			GPIO_CLR_PIN(RELAY_PORT_BASE, RELAY_PIN_MASK);
			((struct relayRuntime*)this->data.runtime)->enabled = 1;
		}
		else{
			GPIO_SET_INPUT(RELAY_PORT_BASE, RELAY_PIN_MASK);
			((struct relayRuntime*)this->data.runtime)->enabled = 0;
		}
		break;
	}

	return RELAY_SUCCESS;
}

/* An event was received from another device - now act on it */
static int eventHandler(struct susensors_sensor* this, int len, uint8_t* payload){
	uint8_t event;
	uint32_t parselen = len;
	cmp_object_t eventval;
	if(cp_decodeU8(payload, &event, &parselen) != 0) return 1;
	payload += parselen;
	parselen = len - parselen;
	if(cp_decodeObject(payload, &eventval, &parselen) != 0) return 2;

	if(event & AboveEventActive){
		this->value(this, setOn, NULL);
	}
	else if(event & BelowEventActive){
		this->value(this, setOff, NULL);
	}
	else if(event & ChangeEventActive){
		this->value(this, setToggle, NULL);
	}

	return 0;
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
static void* getFunctionPtr(su_relay_actions trig){

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

static eventhandler_ptr setEventhandlers(struct susensors_sensor* this, int8_t trigger){

	return getFunctionPtr(trigger);
}

susensors_sensor_t* addASURelay(const char* name, settings_t* settings){

	if(deviceSetupGet(name, settings, &default_relaysetting) != 0) return 0;

	susensors_sensor_t d;
	d.type = (char*)name;
	d.status = get;
	d.value = set;
	d.configure = configure;
	d.eventhandler = eventHandler;
	d.suconfig = suconfig;
	d.data.config = &relayconfig;
	d.data.setting = settings;

	d.setEventhandlers = setEventhandlers;

	relayruntime[noofrelays].enabled = 0;
	relayruntime[noofrelays].hasEvent = 0,
	relayruntime[noofrelays].LastEventValue.type = CMP_TYPE_UINT8;
	relayruntime[noofrelays].LastEventValue.as.u8 = 0;
	relayruntime[noofrelays].ChangeEventAcc.as.u8 = 0;
	d.data.runtime = (void*) &relayruntime[noofrelays++];

	return addSUDevices(&d);
}
