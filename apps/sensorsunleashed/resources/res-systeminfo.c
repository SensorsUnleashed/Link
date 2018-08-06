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

#include <string.h>
#include "contiki.h"
#include "rest-engine.h"
#include "dev/leds.h"
#include "dev/sys-ctrl.h"
#include "sys/ctimer.h"
#include "board.h"
#include "coap-observe.h"
#include "coap-observe-client.h"
#include "susensors.h"
#include "cmp_helpers.h"
#include "project-conf.h"
#include "firmwareUpgrade.h"
extern process_event_t systemchange;
static void res_sysinfo_gethandler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_sysinfo_puthandler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
/* A simple actuator example. Toggles the red led */
RESOURCE(res_sysinfo,
		"title=\"Nodeinfo\"",
		res_sysinfo_gethandler,
		NULL,
		res_sysinfo_puthandler,
		NULL);

static struct ctimer callbacktimer;

//Return 0 on success
static int encodeTrailer(uint32_t trailaddr, uint8_t* buffer){
	volatile struct firmwareInf *trailer = (volatile struct firmwareInf*)(trailaddr);
	uint32_t len = 0;

	uint8_t arr[5];
	arr[0] = trailer->version_major;
	arr[1] = trailer->version_minor;
	arr[2] = trailer->version_dev;
	arr[3] = trailer->radio_target;
	arr[4] = trailer->startaddress == TOPIMG_START;	//location: 1 = top, 0 = bot

	cmp_object_t pt;
	pt.type = CMP_TYPE_UINT16;
	pt.as.u16 = trailer->product_type;

	cmp_object_t epoch;
	epoch.type = CMP_TYPE_UINT64;
	epoch.as.u64 = *((uint64_t*)&trailer->creation_date_epoch[0]);

	cp_encodeU8Array(buffer + len, arr, sizeof(arr), &len);
	len += cp_encodeObject(buffer + len, &pt);
	len += cp_encodeObject(buffer + len, &epoch);

	return len;
}

static void
res_sysinfo_gethandler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset){
	//REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
	//REST.set_header_max_age(response, 0);	//3 minutes

	const char *str = NULL;
	const char* slotnfostr = NULL;

	int len = 0;
	//Pay attention to the max payload length
	len = REST.get_query(request, &str);
	if(len > 0){

		if(strncmp(str, "Versions", len) == 0){
			len = 0;
			uint8_t arr[3];
			arr[0] = SU_VER_MAJOR;
			arr[1] = SU_VER_MINOR;
			arr[2] = SU_VER_DEV;

			cp_encodeString(buffer+len, BOARD_STRING, strlen(BOARD_STRING), (uint32_t*)&len);
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
		else if(REST.get_query_variable(request, "SlotNfo", &slotnfostr) > 0 && slotnfostr != NULL){
			if(*slotnfostr == '1'){
				uint32_t taddr = getTrailAddr(1);
				if(taddr != 0){
					//Request for active slot
					len = encodeTrailer(taddr, buffer);
				}
				else{
					REST.set_response_status(response, REST.status.NOT_FOUND);
					return;
				}
			}
			else if(*slotnfostr == '0'){
				//Request for other slot
				uint32_t taddr = getTrailAddr(0);
				if(taddr != 0){
					//Request for active slot
					len = encodeTrailer(taddr, buffer);
				}
				else{
					REST.set_response_status(response, REST.status.NOT_FOUND);
					return;
				}
			}
		}
		else if(strncmp(str, "activeSlot", len) == 0){
			cmp_object_t actslot;
			actslot.type = CMP_TYPE_UINT8;
			actslot.as.u8 = getActiveSlot();

			len = cp_encodeObject(buffer, &actslot);
		}
		REST.set_response_payload(response, buffer, len);
	}
	else{
		//Ping just respond with ok.
		REST.set_response_status(response, REST.status.OK);
	}
}

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF("[%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x]", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7], ((uint8_t *)addr)[8], ((uint8_t *)addr)[9], ((uint8_t *)addr)[10], ((uint8_t *)addr)[11], ((uint8_t *)addr)[12], ((uint8_t *)addr)[13], ((uint8_t *)addr)[14], ((uint8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF("[%02x:%02x:%02x:%02x:%02x:%02x]", (lladdr)->addr[0], (lladdr)->addr[1], (lladdr)->addr[2], (lladdr)->addr[3], (lladdr)->addr[4], (lladdr)->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(addr)
#endif

static void res_sysinfo_puthandler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset){

	const char *str = NULL;
	unsigned int ct = -1;
	const uint8_t *payload = NULL;
	REST.get_header_content_type(request, &ct);

	if(ct != REST.type.APPLICATION_OCTET_STREAM) {
		REST.set_response_status(response, REST.status.BAD_REQUEST);
		const char *error_msg = "msgPacked, octet-stream only";
		REST.set_response_payload(response, error_msg, strlen(error_msg));
		return;
	}

	coap_packet_t *const coap_req = (coap_packet_t *)request;
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
		else if(strncmp(str, "upg", len) == 0){ //Firmware upgrade
			if((len = REST.get_request_payload(request, (const uint8_t **)&payload))) {

				if(fwUpgradeAddChunk(payload, len, coap_req->block1_num, coap_req->block1_offset) == 0){
					coap_set_header_block1(response, coap_req->block1_num, 0, coap_req->block1_size);
					REST.set_response_status(response, REST.status.OK);

					    PRINTF("Blockwise: block 1 request: Num: %u, More: %u, Size: %u, Offset: %u\n",
					           (unsigned int)coap_req->block1_num,
					           (unsigned int)coap_req->block1_more,
					           (unsigned int)coap_req->block1_size,
					           (unsigned int)coap_req->block1_offset);
				}
				else {
					REST.set_response_status(response, REST.status.REQUEST_ENTITY_TOO_LARGE);
					return;
				}

				if(coap_req->block1_more == 0){
					if(fwUpgradeDone() == 0){
						REST.set_response_status(response, REST.status.CREATED);
					}
					else{
						REST.set_response_status(response, REST.status.NOT_MODIFIED);
					}
				}
			}

		}
#if 0	//Needs reimplementing!
		else if(strncmp(str, "obsretry", len) == 0){
			process_post(&susensors_process, susensors_service, NULL);
			REST.set_response_status(response, REST.status.OK);
		}
#endif
		else if(strncmp(str, "swreset", len) == 0){
			ctimer_set(&callbacktimer, CLOCK_SECOND, sys_ctrl_reset, NULL);
			REST.set_response_status(response, REST.status.OK);
		}
		else{
			REST.set_response_status(response, REST.status.BAD_REQUEST);
		}
	}
}

