/*
 * timerdevice.c
 *
 *  Created on: 26. mar. 2018
 *      Author: omn
 */
#include "timerdevice.h"
#include "susensorcommon.h"
#include "ctimer.h"
#include "deviceSetup.h"
/*
 * The timer device can be used to trigger another device - delayed
 *
 * */
static void timer_cb(void* ptr);
static int setNextTimeout(susensors_sensor_t* this);

struct resourceconf timerconfig = {
		.resolution = 1,
		.version = 1,
		.flags = METHOD_GET | METHOD_PUT | IS_OBSERVABLE | HAS_SUB_RESOURCES,
		.max_pollinterval = 2,
		.unit = "",
		.spec = "Timer device; STOP=0, START=1, RESETSTART=2",
		.type = TIMER_DEVICE,
		.attr = "title=\"Timer Device\" ;rt=\"Control\"",
};

static const settings_t default_timer_settings = {
		.eventsActive = AboveEventActive | BelowEventActive,
		.AboveEventAt = {
				.type = CMP_TYPE_UINT32,
				.as.u32 = 60
		},
		.BelowEventAt = {
				.type = CMP_TYPE_UINT32,
				.as.u32 = 2
		},
		.ChangeEvent = {
				.type = CMP_TYPE_UINT32,
				.as.u32 = 2
		},
		.RangeMin = {
				.type = CMP_TYPE_UINT32,
				.as.u32 = 1
		},
		.RangeMax = {
				.type = CMP_TYPE_UINT32,
				.as.u32 = 30000000,	//Seconds (347,2 days)
		},
};


struct timerRuntime {
	uint8_t enabled;
	uint8_t hasEvent;
	cmp_object_t LastEventValue;
	cmp_object_t LastValue;
	cmp_object_t ChangeEventAcc;	///Accumulated steps; for determining if event should be fired

	/* Extra specific to device */
	clock_time_t step;
	struct ctimer timer;

};
static struct timerRuntime runtimeData;

//Actual value is the expiration time
static int get (susensors_sensor_t* this, int type, void* data){
	struct timerRuntime* tr = this->data.runtime;
	int ret = 1;
	cmp_object_t* obj = (cmp_object_t*)data;
	if((enum up_parameter) type == ActualValue){
		obj->type = CMP_TYPE_UINT32;
		if(ctimer_expired(&tr->timer)){
			obj->as.u32 = 0;
		}
		else{
			obj->as.u32 = etimer_expiration_time(&tr->timer.etimer) / CLOCK_SECOND;
		}

		ret = 0;
	}
	return ret;
}

static int set (susensors_sensor_t* this, int type, void* data){

	struct timerRuntime* tr = this->data.runtime;
	settings_t* r = this->data.setting;
	int enabled = tr->enabled;
	int ret = 1;

	if(!enabled) return ret;

	if((enum su_timer_actions)type == timerStop){
		ctimer_stop(&tr->timer);
		ret = 0;
	}
	else if((enum su_timer_actions)type == timerStart){
		if(ctimer_expired(&tr->timer)){
			setNextTimeout(this);
		}
		ret = 0;
	}
	else if((enum su_timer_actions)type == timerRestart){
		ctimer_stop(&tr->timer);
		tr->LastEventValue.as.u32 = r->BelowEventAt.as.u32 + 1;
		int step = tr->LastValue.as.u32;
		tr->LastValue.as.u32 = 0;
		setEventU32(this, -1, step);
		setNextTimeout(this);
		ret = 0;
	}

	return ret;
}

static int setNextTimeout(susensors_sensor_t* this){

	settings_t* r = this->data.setting;
	struct timerRuntime* tr = this->data.runtime;

	clock_time_t interval = 0;
	tr->step = 0;
	if(tr->ChangeEventAcc.as.u32 <= r->ChangeEvent.as.u32 && (r->eventsActive & ChangeEventActive)){
		interval = (clock_time_t)(r->ChangeEvent.as.u32 - tr->ChangeEventAcc.as.u32);
		tr->step = interval == 0 ? r->ChangeEvent.as.u32 : interval;
	}

	if(tr->LastValue.as.u32 < r->BelowEventAt.as.u32 && (r->eventsActive & BelowEventActive)){
		interval = (clock_time_t)(r->BelowEventAt.as.u32 + 1 - tr->LastValue.as.u32);
		tr->step = (tr->step == 0 || interval < tr->step) ? interval : tr->step;
	}

	if(tr->LastValue.as.u32 < r->AboveEventAt.as.u32 && (r->eventsActive & AboveEventActive)){
		interval = (clock_time_t)(r->AboveEventAt.as.u32 - tr->LastValue.as.u32);
		tr->step = (tr->step == 0 || interval < tr->step) ? interval : tr->step;
	}

	if(tr->step){
		ctimer_set(&tr->timer, tr->step * CLOCK_SECOND, timer_cb, (void*)this);
	}
	return 0;
}


static void timer_cb(void* ptr){
	susensors_sensor_t* this = ptr;
	struct timerRuntime* tr = this->data.runtime;

	tr->LastValue.as.u32 += tr->step;
	setEventU32(this, 1, tr->step);

	/* Set events */
	setNextTimeout(this);
}

static int configure(susensors_sensor_t* this, int type, int value){
	struct timerRuntime* tr = this->data.runtime;

	switch(type) {
	case SUSENSORS_HW_INIT:
		break;
	case SUSENSORS_ACTIVE:
		if(value){	//Activate
			tr->LastValue.type = CMP_TYPE_UINT32;
			tr->ChangeEventAcc.type = CMP_TYPE_UINT32;
			tr->enabled = 1;
			set(this, timerRestart, NULL);
		}
		else{
			set(this, timerStop, NULL);
			tr->enabled = 0;
		}
		break;
	}

	return 0;
}

static int  setStophandler(struct susensors_sensor* this, int len, const uint8_t* payload){
	this->value(this, timerStop, NULL);
	return 0;
}
static int  setStarthandler(struct susensors_sensor* this, int len, const uint8_t* payload){
	this->value(this, timerStart, NULL);
	return 0;
}
static int  setRestarthandler(struct susensors_sensor* this, int len, const uint8_t* payload){
	this->value(this, timerRestart, NULL);
	return 0;
}


/* Return the function to call when a specified trigger is in use */
static void* getFunctionPtr(enum su_timer_actions trig){

	if(trig >= timerStop && trig <= timerRestart){
		switch(trig){
		case timerStop:
			return setStophandler;
			break;
		case timerStart:
			return setStarthandler;
			break;
		case timerRestart:
			return setRestarthandler;
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


susensors_sensor_t* addASUTimerDevice(const char* name, settings_t* settings){

	if(deviceSetupGet(name, settings, &default_timer_settings) < 0) return 0;

	susensors_sensor_t d;
	d.type = (char*)name;
	d.status = get;
	d.value = set;
	d.configure = configure;
	d.eventhandler = NULL;
	d.suconfig = suconfig;
	d.data.config = &timerconfig;
	d.data.setting = settings;
	d.data.runtime = &runtimeData;

	d.setEventhandlers = setEventhandlers;

	return addSUDevices(&d);
}
