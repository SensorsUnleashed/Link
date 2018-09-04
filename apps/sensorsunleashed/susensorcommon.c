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

#include "susensorcommon.h"
#include "deviceSetup.h"


int suconfig(struct susensors_sensor* this, int type, void* data){

	int ret = 1;
	settings_t* setting = this->data.setting;
	enum susensors_configcmd cmd = (enum susensors_configcmd)type;
	if(cmd == SUSENSORS_EVENTSETUP_SET){

		/* The eventsetup is received as a fixed list of cmp_objects:
		 * 1. AboveEventAt
		 * 2. BelowEventAt
		 * 3. ChangeEvent
		 * 4. eventsActive
		 * */

		/* Returns
		 *  0: Success
		 *  1: AboveEventAt was not right
		 *  2: BelowEventAt was not right
		 *	3: ChangeEvent was not right
		 *	4: eventsActive was not right
		 * */
		uint8_t* payload = (uint8_t*)data;
		uint32_t bufindex;
		int len = 0;

		cmp_object_t newval;
		/* Read the AboveEventAt object */
		if(cp_decodeObject((uint8_t*)payload + len, &newval, &bufindex) == 0){
			len += bufindex;
			if(newval.type == setting->AboveEventAt.type){
				setting->AboveEventAt = newval;
			}
			else{
				return 1;
			}
		}
		else{
			return 1;
		}

		/* Read the BelowEventAt object */
		if(cp_decodeObject((uint8_t*)payload + len, &newval, &bufindex) == 0){
			len += bufindex;
			if(newval.type == setting->BelowEventAt.type){
				setting->BelowEventAt = newval;
			}
			else{
				return 2;
			}
		}
		else{
			return 2;
		}
		/* Read the ChangeEvent object */
		if(cp_decodeObject((uint8_t*)payload + len, &newval, &bufindex) == 0){
			len += bufindex;
			if(newval.type == setting->ChangeEvent.type){
				setting->ChangeEvent = newval;
			}
			else{
				return 3;
			}
		}
		else{
			return 3;
		}
		/* Read the eventsActive object */
		if(cp_decodeObject((uint8_t*)payload + len, &newval, &bufindex) == 0){
			len += bufindex;
			if(newval.type == CMP_TYPE_UINT8){
				setting->eventsActive = newval.as.u8;
			}
			else{
				return 4;
			}
		}
		else{
			return 4;
		}
		return 0;
	}
	else if(cmd == SUSENSORS_EVENTSETUP_GET){
		uint8_t* bufptr = (uint8_t*)data;
		cmp_object_t eventsActive;
		eventsActive.type = CMP_TYPE_UINT8;
		eventsActive.as.u8 = setting->eventsActive;

		bufptr += cp_encodeObject(bufptr, &setting->AboveEventAt);
		bufptr += cp_encodeObject(bufptr, &setting->BelowEventAt);
		bufptr += cp_encodeObject(bufptr, &setting->ChangeEvent);
		bufptr += cp_encodeObject(bufptr, &eventsActive);

		ret = (uint8_t*)data - bufptr;
	}
	else if(cmd == SUSENSORS_CEVENT_GET){
		cmp_object_t* obj = (cmp_object_t*)data;
		*obj = setting->ChangeEvent;
		ret = 0;
	}
	else if(cmd == SUSENSORS_AEVENT_GET){
		cmp_object_t* obj = (cmp_object_t*)data;
		*obj = setting->AboveEventAt;
		ret = 0;
	}
	else if(cmd == SUSENSORS_BEVENT_GET){
		cmp_object_t* obj = (cmp_object_t*)data;
		*obj = setting->BelowEventAt;
		ret = 0;
	}
	else if(cmd == SUSENSORS_RANGEMAX_GET){
		cmp_object_t* obj = (cmp_object_t*)data;
		*obj = setting->RangeMax;
		ret = 0;
	}
	else if(cmd == SUSENSORS_RANGEMIN_GET){
		cmp_object_t* obj = (cmp_object_t*)data;
		*obj = setting->RangeMin;
		ret = 0;
	}
	else if(cmd == SUSENSORS_EVENTSTATE_GET){
		cmp_object_t* obj = (cmp_object_t*)data;
		obj->type = CMP_TYPE_UINT8;
		obj->as.u8 = setting->eventsActive;
		ret = 0;
	}
	else if(cmd == SUSENSORS_STORE_SETUP){
		deviceSetupSave(this->type, setting);
		ret = 0;
	}
	return ret;
}

void setResource(struct susensors_sensor* this, resource_t* res){
	this->data.resource = res;
}

/**
 *
 * @param this
 * 	The actual sensor instance
 * @param dir
 * 	The direction of the last value
 * @param step
 * 	The step from from last time the actuator was set
 * @return
 */
void setEventU8(struct susensors_sensor* this, int dir, uint8_t step){
	settings_t* c = this->data.setting;
	struct relayRuntime* r = (struct relayRuntime*)(this->data.runtime);
	uint8_t event = 0;

	r->hasEvent = NoEventActive;
	if(dir < 0){
		if(r->LastEventValue.as.u8 > c->BelowEventAt.as.u8){
			if(r->LastValue.as.u8 <= c->BelowEventAt.as.u8 && (c->eventsActive & BelowEventActive)){
				r->hasEvent = BelowEventActive;
				event |= SUSENSORS_BELOW_EVENT;
			}
		}
	}
	else{
		if(r->LastEventValue.as.u8 < c->AboveEventAt.as.u8){
			if(r->LastValue.as.u8 >= c->AboveEventAt.as.u8 && (c->eventsActive & AboveEventActive)){
				r->hasEvent = AboveEventActive;
				event |= SUSENSORS_ABOVE_EVENT;
			}
		}
	}

	r->ChangeEventAcc.as.u8 += step;
	if(c->ChangeEvent.as.u8 <= r->ChangeEventAcc.as.u8){
		r->ChangeEventAcc.as.u8 = 0;
		if(c->eventsActive & ChangeEventActive){
			r->hasEvent |= ChangeEventActive;
			event |= SUSENSORS_CHANGE_EVENT;
		}
	}

	if(r->hasEvent != NoEventActive){
		r->LastEventValue = r->LastValue;
		susensors_changed(this, event);
	}
}

void setEventU16(struct susensors_sensor* this, int dir, uint16_t step){
	settings_t* c = this->data.setting;
	struct relayRuntime* r = (struct relayRuntime*)(this->data.runtime);
	uint8_t event = 0;

	r->hasEvent = NoEventActive;
	if(dir < 0){
		if(r->LastEventValue.as.u16 > c->BelowEventAt.as.u16){
			if(r->LastValue.as.u16 <= c->BelowEventAt.as.u16 && (c->eventsActive & BelowEventActive)){
				r->hasEvent = BelowEventActive;
				event |= SUSENSORS_BELOW_EVENT;
			}
		}
	}
	else{
		if(r->LastEventValue.as.u16 < c->AboveEventAt.as.u16){
			if(r->LastValue.as.u16 >= c->AboveEventAt.as.u16 && (c->eventsActive & AboveEventActive)){
				r->hasEvent = AboveEventActive;
				event |= SUSENSORS_ABOVE_EVENT;
			}
		}
	}

	r->ChangeEventAcc.as.u16 += step;
	if(c->ChangeEvent.as.u16 <= r->ChangeEventAcc.as.u16){
		r->ChangeEventAcc.as.u16 = 0;
		if(c->eventsActive & ChangeEventActive){
			r->hasEvent |= ChangeEventActive;
			event |= SUSENSORS_CHANGE_EVENT;
		}
	}

	if(r->hasEvent != NoEventActive){
		r->LastEventValue = r->LastValue;
		susensors_changed(this, event);
	}
}

void setEventU32(struct susensors_sensor* this, int dir, uint32_t step){
	settings_t* c = this->data.setting;
	struct relayRuntime* r = (struct relayRuntime*)(this->data.runtime);
	uint8_t event = 0;

	r->hasEvent = NoEventActive;
	if(dir < 0){
		if(r->LastEventValue.as.u32 > c->BelowEventAt.as.u32){
			if(r->LastValue.as.u32 <= c->BelowEventAt.as.u32 && (c->eventsActive & BelowEventActive)){
				r->hasEvent = BelowEventActive;
				event |= SUSENSORS_BELOW_EVENT;
			}
		}
	}
	else{
		if(r->LastEventValue.as.u32 < c->AboveEventAt.as.u32){
			if(r->LastValue.as.u32 >= c->AboveEventAt.as.u32 && (c->eventsActive & AboveEventActive)){
				r->hasEvent = AboveEventActive;
				event |= SUSENSORS_ABOVE_EVENT;
			}
		}
	}

	r->ChangeEventAcc.as.u32 += step;
	if(c->ChangeEvent.as.u32 <= r->ChangeEventAcc.as.u32){
		r->ChangeEventAcc.as.u32 = 0;
		if(c->eventsActive & ChangeEventActive){
			r->hasEvent |= ChangeEventActive;
			event |= SUSENSORS_CHANGE_EVENT;
		}
	}

	if(r->hasEvent != NoEventActive){
		r->LastEventValue = r->LastValue;
		susensors_changed(this, event);
	}
}

int testevent(struct susensors_sensor* this, int len, uint8_t* payload){
	uint8_t event, testevent = 0;
	uint32_t parselen = len;
	cmp_object_t eventval;
	if(cp_decodeU8(payload, &event, &parselen) != 0) return 1;
	payload += parselen;
	parselen = len - parselen;
	if(cp_decodeObject(payload, &eventval, &parselen) != 0) return 2;

	settings_t* c = this->data.setting;

	if((event & SUSENSORS_ABOVE_EVENT) && (c->eventsActive & AboveEventActive)){
		testevent |= SUSENSORS_ABOVE_EVENT;
	}
	else if((event & SUSENSORS_BELOW_EVENT) && (c->eventsActive & BelowEventActive)){
		testevent |= SUSENSORS_BELOW_EVENT;
	}
	else if((event & SUSENSORS_CHANGE_EVENT) && (c->eventsActive & ChangeEventActive)){
		testevent |= SUSENSORS_CHANGE_EVENT;
	}

	if(testevent){
		susensors_changed(this, testevent);
	}
	return 0;
}
