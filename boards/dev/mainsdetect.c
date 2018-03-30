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
#include "sys/timer.h"
#include "mainsdetect.h"

#include "dev/sys-ctrl.h"
#include "dev/ioc.h"
#include "dev/gptimer.h"
#include "dev/gpio.h"
#include "dev/ioc.h"
#include "dev/leds.h"

#define MAINSDETECT_PORT_BASE          GPIO_PORT_TO_BASE(MAINSDETECT_PORT)
#define MAINSDETECT_PIN_MASK           GPIO_PIN_MASK(MAINSDETECT_PIN)
#define MAINSDETECT_VECTOR			   GPIO_A_IRQn //NVIC_INT_GPIO_PORT_A

//Timeout = 1/16Mhz * 28 * 60000 = 105ms	//5 periods + prell margin
#define PRESCALER	28-1
#define TICKTIMEOUT	60000

#include "rest-engine.h"
#include "susensorcommon.h"

static struct relayRuntime mainsdetectruntime[1];

static susensors_sensor_t* mainsdetect;

struct resourceconf mainsdetectconfig = {
		.resolution = 1,
		.version = 1,
		.flags = METHOD_GET | METHOD_PUT | IS_OBSERVABLE | HAS_SUB_RESOURCES,
		.max_pollinterval = 2,
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
				.type = CMP_TYPE_UINT8,
				.as.u8 = 0
		},
		.RangeMax = {
				.type = CMP_TYPE_UINT8,
				.as.u8 = 1
		},

		.unit = "",
		.spec = "Detect if mains is present or not",
		.type = MAINSDETECT_ACTUATOR,
		.attr = "title=\"Mainsdetect\" ;rt=\"Monitor\"",
};


static int get(struct susensors_sensor* this, int type, void* data)
{
	int ret = 1;
	cmp_object_t* obj = (cmp_object_t*)data;
	if((enum up_parameter) type == ActualValue){
		struct relayRuntime* r = (struct relayRuntime*)this->data.runtime;
		*obj = r->LastValue;
		ret = 0;
	}
	return ret;
}

static int set(struct susensors_sensor* this, int type, void* data)
{
	int ret = 1;
	return ret;
}

static int configure(struct susensors_sensor* this, int type, int value)
{
	switch(type) {
	case SUSENSORS_HW_INIT:
		/** The first timer, counts pulses */
		/*
		 * Remove the clock gate to enable GPT2 and then initialise it
		 */
		REG(SYS_CTRL_RCGCGPT) |= SYS_CTRL_RCGCGPT_GPT2;

		/* Set PA2 to be pulse counter input */
		REG(IOC_GPT2OCP1) = (MAINSDETECT_PORT << 3) + MAINSDETECT_PIN;

		/* Set pin PA2 (DIO0) to peripheral mode */
		GPIO_PERIPHERAL_CONTROL(MAINSDETECT_PORT_BASE,MAINSDETECT_PIN_MASK);

		/* From user manual p. 329 */
		/* 1. Ensure the timer is disabled (the TAEN bit is cleared) before making any changes. */
		REG(GPT_2_BASE + GPTIMER_CTL) = 0;

		/* 2. Write the GPTM Configuration (GPTIMER_CFG) register with a value of 0x0000.0004. */
		/* 16-bit timer configuration */
		REG(GPT_2_BASE + GPTIMER_CFG) = 0x04;

		/* 3. In the GPTM Timer Mode (GPTIMER_TnMR) register, write the TnCMR field to 0x0 and the TnMR field to 0x3.*/
		/* Capture mode, Edge-count mode*/
		REG(GPT_2_BASE + GPTIMER_TAMR) |= GPTIMER_TAMR_TAMR;		//Capture mode
		REG(GPT_2_BASE + GPTIMER_TAMR) &= ~(GPTIMER_TAMR_TACMR);	//Edge count mode
		REG(GPT_2_BASE + GPTIMER_TAMR) |= GPTIMER_TAMR_TACDIR;		//Count up

		/* 4. Configure the type of event(s) that the timer captures by writing the TnEVENT field of the G Control (GPTIMER_CTL) register. */
		/* Positive edges */
		REG(GPT_2_BASE + GPTIMER_CTL) &= ~GPTIMER_CTL_TAEVENT;	//0=Postive Edge, 1=Negative Edge, 3=Both

		/* 5. If a prescaler is to be used, write the prescale value to the GPTM Timer n Prescale Regist (GPTIMER_TnPR). */
		/* No prescaler */
		REG(GPT_2_BASE + GPTIMER_TAPR) = 0;

		/* 6. Load the timer start value into the GPTM Timer n Interval Load (GPTIMER_TnILR) registe */
		REG(GPT_2_BASE + GPTIMER_TAILR) = 4;	//When reached, its starts over from 0

		/* 7. Load the event count into the GPTM Timer n Match (GPTIMER_TnMATCHR) register. */
		REG(GPT_2_BASE + GPTIMER_TAMATCHR) = 4;	//To generate event on start?

		/* 8. If interrupts are required, set the CnMIM bit in the GPTM Interrupt Mask (GPTIMER_IMR) register. */
		REG(GPT_2_BASE + GPTIMER_IMR) |= GPTIMER_IMR_CAMIM;

		/** Now setup the second timer - when this one times out, we know the mains has gone */
		/* 1. Ensure the timer is disabled (the TnEN bit in the GPTIMER_CTL register is cleared) before making any changes. */
		REG(GPT_2_BASE + GPTIMER_CTL) &= ~GPTIMER_CTL_TBEN;

		/* 2. Write the GPTM Configuration Register (GPTIMER_CFG) with a value of 0x0000 0000. */

		/* 3. Configure the TnMR field in the GPTM Timer n Mode Register (GPTIMER_TnMR):
		 * (a) Write a value of 0x1 for one-shot mode.
		 * (b) Write a value of 0x2 for periodic mode.
		 * */
		REG(GPT_2_BASE + GPTIMER_TBMR) |= GPTIMER_TBMR_TBMR_ONE_SHOT;

		/* 4. Optionally, configure the TnSNAPS, TnWOT, TnMTE, and TnCDIR bits in the GPTIMER_TnMR register to select whether to
		 * capture the value of the free-running timer at time-out, use an external trigger to start counting, configure an additional
		 * trigger or interrupt, and count up or down.
		*/

		/* Prescale - TICK is 16MHz - we need a timeout of 100ms, and register of 16bit */
		REG(GPT_2_BASE + GPTIMER_TBPR) = PRESCALER;

		/* 5. Load the end value into the GPTM Timer n Interval Load Register (GPTIMER_TnILR). */
		REG(GPT_2_BASE + GPTIMER_TBILR) = TICKTIMEOUT;

		/* 6. If interrupts are required, set the appropriate bits in the GPTM Interrupt Mask Register (GPTIMER_IMR). */
		REG(GPT_2_BASE + GPTIMER_IMR) |= GPTIMER_IMR_TBTOIM;	//Timeout

		mainsdetect = this;
		break;
	case SUSENSORS_ACTIVE:
		if(value) {
			leds_on(LEDS_GREEN);
			if(!REG(GPT_2_BASE + GPTIMER_CTL) & GPTIMER_CTL_TAEN){
				/* Enable interrupts */
				NVIC_ClearPendingIRQ(GPT2A_IRQn);
				NVIC_ClearPendingIRQ(GPT2B_IRQn);
				NVIC_EnableIRQ(GPT2A_IRQn);
				NVIC_EnableIRQ(GPT2B_IRQn);
				REG(GPT_2_BASE + GPTIMER_CTL) |= GPTIMER_CTL_TBEN;	//Start with the timeout
			}
		} else {
			NVIC_DisableIRQ(GPT2A_IRQn);
			NVIC_DisableIRQ(GPT2B_IRQn);
		}
		return value;
	default:
		break;
	}
	return 0;
}

/**
 * ISR handler
 */
void mainsdetect_pulsecnt_isr(){

	leds_on(LEDS_GREEN);

	struct relayRuntime* r = (struct relayRuntime*)mainsdetect->data.runtime;
	if(r->LastValue.as.u8 == 0){
		r->LastValue.as.u8 = 1;
		leds_on(LEDS_GREEN);
		setEventU8(mainsdetect, 1, 1);
	}

	REG(GPT_2_BASE + GPTIMER_TBV) = TICKTIMEOUT;

	//Clear the interrupts
	REG(GPT_2_BASE + GPTIMER_ICR) |= GPTIMER_ICR_CAMCINT;

	//Enable TimerB (For mains gone detection)
	REG(GPT_2_BASE + GPTIMER_CTL) |= GPTIMER_CTL_TBEN;
}

void mainsdetect_mainsgone_isr(){
	leds_off(LEDS_GREEN);

	/* To avoid prell use disable the input timer for one oneshot more */
	if(REG(GPT_2_BASE + GPTIMER_CTL) & GPTIMER_CTL_TAEN){
		REG(GPT_2_BASE + GPTIMER_CTL) &= ~GPTIMER_CTL_TAEN;
		REG(GPT_2_BASE + GPTIMER_CTL) |= GPTIMER_CTL_TBEN;
	}
	else{
		REG(GPT_2_BASE + GPTIMER_CTL) |= GPTIMER_CTL_TAEN;
	}

	struct relayRuntime* r = (struct relayRuntime*)mainsdetect->data.runtime;
	if(r->LastValue.as.u8 == 1){
		leds_off(LEDS_GREEN);
		r->LastValue.as.u8 = 0;
		setEventU8(mainsdetect, -1, 1);
	}

	//Clear the interrupts
	REG(GPT_2_BASE + GPTIMER_ICR) |= GPTIMER_ICR_TBTOCINT;
	REG(GPT_2_BASE + GPTIMER_ICR) |= GPTIMER_ICR_CAMCINT;
	REG(GPT_2_BASE + GPTIMER_TAV) = 0;
}


/* An event was received from another device - now act on it */
static int eventHandler(struct susensors_sensor* this, int len, uint8_t* payload){
	return 0;
}

susensors_sensor_t* addASUMainsDetector(const char* name, struct resourceconf* config){
	susensors_sensor_t d;
	d.type = (char*)name;
	d.status = get;
	d.value = set;
	d.configure = configure;
	d.eventhandler = eventHandler;
	d.suconfig = suconfig;
	d.data.config = config;

	d.setEventhandlers = NULL;	//TODO: Implement this

	mainsdetectruntime[0].enabled = 0;
	mainsdetectruntime[0].hasEvent = 0,
			mainsdetectruntime[0].LastEventValue.type = CMP_TYPE_UINT8;
	mainsdetectruntime[0].LastEventValue.as.u8 = 0;
	mainsdetectruntime[0].ChangeEventAcc.as.u8 = 0;
	d.data.runtime = (void*) &mainsdetectruntime[0];

	return addSUDevices(&d);
}
