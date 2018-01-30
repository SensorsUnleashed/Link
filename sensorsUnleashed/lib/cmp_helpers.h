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
 * coap_proxy_protocolhandler.h
 *
 *  Created on: 18/09/2016
 *      Author: omn
 */

#ifndef COAP_UART_DEVICE_HANDLER_COAP_PROXY_PROTOCOLHANDLER_H_
#define COAP_UART_DEVICE_HANDLER_COAP_PROXY_PROTOCOLHANDLER_H_

#include "cmp.h"

//TODO: When implementation is done, simplify these commands
enum req_cmd {
	resource_count = 1,		//Get the total no of resources
	resource_config,
	resource_value_update,	//A resource autonomously postes its value
	resource_req_updateAll,	//Request that the device updates us with the last values from all resources (Happens automatically after init/wake)
	resource_event,			//Post an event, whether its above or below can be verified by the subscribers.
	resource_reset,			//Ask the device to reset
	resource_setval,		//Set the data for a specific resource
	resource_updateval,		//Update a data value for a specific resource
	debugstring,
};

enum up_parameter{
	/* Sensor specifics reading */
	ActualValue,
	EventStatus,	//Get the event fired

	/* Common config */
	ChangeEventConfigValue,
	AboveEventConfigValue,
	BelowEventConfigValue,

	RangeMinValue,
	RangeMaxValue,

	EventState,
};

//This struct contains the raw messages.
typedef struct  {
	unsigned char seqno;
	enum req_cmd cmd;
	char len;
	void* payload;
}rx_msg;

enum eventstate{
	NoEventActive =     (1 << 0),	//1
	AboveEventActive =  (1 << 1),	//2
	BelowEventActive =  (1 << 2),	//4
	ChangeEventActive = (1 << 3),	//8
};

//void* lastval;			//MsgPacked value. Actual SI value is calculated as: Value * 1/resolution = X [Unit].
struct resourceconf{
	uint8_t id;
	uint32_t resolution;	//1/resoltion of a unit. eg. if resolution is 1000 and unit is C, then each LSB equals 0.001C
	uint32_t version;		//Q22.10
	uint8_t flags;			//Flags - which handle do we serve: Get/Post/PUT/Delete
	int32_t max_pollinterval;	//How often can you ask for a new reading

	/* Pseudocode:
	 *
	 * Eg. We want a toggle button to signal when its on, but not off
	 * AboveEventAt = 1
	 * AboveEventAt = -1	//Off because its outside button spec
	 * changeEvent = 0		//0 = off
	 *
	 * We want a temperature sensor to feed us with updates with every 0.5C
	 * AboveEvent = 9999	//Off because its outside button spec
	 * BelowEvent = 9999	//Off because its outside button spec
	 * changeEvent = 0.5	//For every 0.5C low/high from last event, a new will be emitted
	 *
	 * It is possible to have all event types enabled together, but its the subscriber that
	 * will need to detect which event was fired. They are all alike.
	 * */
	uint8_t eventsActive;		///Generation of events on or Off (Determined by eventstate)
	cmp_object_t AboveEventAt;	///When resource crosses this line from low to high give an event (>=)
	cmp_object_t BelowEventAt;	///When resource crosses this line from high to low give an event (<=)
    cmp_object_t ChangeEvent;	///When value has changed more than changeEvent + lastevent value <>= value

    cmp_object_t RangeMin;		//What is the minimum value this device can read
    cmp_object_t RangeMax;		//What is the maximum value this device can read


	char* unit;				//SI unit - can be preceeded with mC for 1/1000 of a C. In that case the resolution should be 1;
	char* spec;				//Human readable spec of the sensor
	char* type;				//Can be button, sensor, motor, relay etc.
	char* group;			//Can be actuator, sensor, kitchen etc
	char* attr;				//Attributes served to the coap clients

	//uint8_t notation;		//Qm.f => MMMM.FFFF	eg. Q1.29 = int32_t with 1 integer bit and 29 fraction bits, Q32 = uint32_t = 32 positive integer bits. Q31 is a int32_t
};

int cp_decodemessage(char* source, int len, rx_msg* destination);
uint32_t cp_encodemessage(uint8_t msgid, enum req_cmd cmd, void* payload, char len, uint8_t* buffer);

uint32_t cp_encoderesource_conf(struct resourceconf* data, uint8_t* buffer);
uint32_t cp_encodeObject(uint8_t* buffer, cmp_object_t *obj);
uint32_t cp_encodeU8(uint8_t* buffer, uint8_t val);
uint32_t cp_encodeU8Array(uint8_t* buffer, uint8_t* data, uint32_t size, uint32_t* len);
uint32_t cp_encodeU16Array(uint8_t* buffer, uint16_t* data, uint32_t size, uint32_t* len);
uint32_t cp_encodeString(uint8_t* buffer, char* str, uint32_t size, uint32_t* len);

int cp_decoderesource_conf(struct resourceconf* data, uint8_t* buffer, char* strings);
int cp_decodeU8(const uint8_t* buffer, uint8_t* x, uint32_t* len);
int cp_decodeS8Array(uint8_t* buffer, int8_t* arr, uint32_t* len);
int cp_decodeU16Array(uint8_t* buffer, uint16_t* arr, uint32_t* len);
int cp_decode_string(uint8_t* buffer, char* string, uint32_t* stringlen, uint32_t* len);
int cp_decodeObject(uint8_t* buffer, cmp_object_t *obj, uint32_t* len);

int cp_cmp_to_string(cmp_object_t* obj, uint8_t* result, uint32_t* len);
int cp_convMsgPackToString(uint8_t* buffer, uint8_t* conv, uint32_t* len);



#endif /* COAP_UART_DEVICE_HANDLER_COAP_PROXY_PROTOCOLHANDLER_H_ */
