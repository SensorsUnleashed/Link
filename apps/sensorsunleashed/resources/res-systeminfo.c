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
 * res-systeminfo.c
 *
 *  Created on: 07/10/2016
 *      Author: omn
 */
#include <string.h>
#include "contiki.h"
#include "rest-engine.h"
#include "dev/leds.h"
#include "board.h"
#include "coap-observe.h"
#include "coap-observe-client.h"
#include "susensors.h"
#include "cmp_helpers.h"
#include "project-conf.h"

extern process_event_t systemchange;
extern process_event_t susensors_service;
static void res_sysinfo_gethandler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_sysinfo_puthandler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
/* A simple actuator example. Toggles the red led */
RESOURCE(res_sysinfo,
		"title=\"Nodeinfo\"",
		res_sysinfo_gethandler,
		NULL,
		res_sysinfo_puthandler,
		NULL);

static void
res_sysinfo_gethandler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset){
	//REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
	//REST.set_header_max_age(response, 0);	//3 minutes

	const char *str = NULL;
	int len = 0;
	//Pay attention to the max payload length
	len = REST.get_query(request, &str);
	if(len > 0){

		if(strncmp(str, "Versions", len) == 0){
			len = 0;
			cp_encodeString(buffer+len, BOARD_STRING, strlen(BOARD_STRING), (uint32_t*)&len);

			uint8_t arr[3];
			arr[0] = SU_VER_MAJOR;
			arr[1] = SU_VER_MINOR;
			arr[2] = SU_VER_DEV;
			cp_encodeU8Array(buffer+len, arr, sizeof(arr), (uint32_t*)&len);
		}
		else if(strncmp(str, "CoapStatus", len) == 0){
			len = 0;
			uint8_t arr[3];
			int pairtot = 0;

			//Count the Total number of pairs
			for(susensors_sensor_t* d = susensors_first(); d; d = susensors_next(d)) {
				list_t pairs = d->pairs;
				pairtot += list_length(pairs);
			}

			arr[0] = 0xFF; //TODO: list_length(coap_get_observers());
			arr[1] = 0xFF;// TODO: list_length(coap_get_observees());
			arr[2] = pairtot;

			cp_encodeU8Array(buffer, arr, sizeof(arr), (uint32_t*)&len);
		}
		else if(strncmp(str, "RPLStatus", len) == 0){

		}
		REST.set_response_payload(response, buffer, len);
	}
	else{
		//Ping just respond with ok.
		REST.set_response_status(response, REST.status.OK);
	}
}


static void res_sysinfo_puthandler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset){

	const char *str = NULL;
	unsigned int ct = -1;
	REST.get_header_content_type(request, &ct);

	if(ct != REST.type.APPLICATION_OCTET_STREAM) {
		REST.set_response_status(response, REST.status.BAD_REQUEST);
		const char *error_msg = "msgPacked, octet-stream only";
		REST.set_response_payload(response, error_msg, strlen(error_msg));
		return;
	}

	int len = REST.get_query(request, &str);
	if(len > 0){
		if(strncmp(str, "cfsformat", len) == 0){
			process_post(PROCESS_BROADCAST, systemchange, NULL);
			REST.set_response_status(response, REST.status.OK);
		}
		else if(strncmp(str, "obs", len) == 0){
			if(missingJustCalled(&UIP_IP_BUF->srcipaddr)){
				REST.set_response_status(response, REST.status.OK);
			}
			else{
				REST.set_response_status(response, REST.status.DELETED);
			}
		}
#if 0	//Needs reimplementing!
		else if(strncmp(str, "obsretry", len) == 0){
			process_post(&susensors_process, susensors_service, NULL);
			REST.set_response_status(response, REST.status.OK);
		}
#endif
		else{
			REST.set_response_status(response, REST.status.BAD_REQUEST);
		}
	}
}

