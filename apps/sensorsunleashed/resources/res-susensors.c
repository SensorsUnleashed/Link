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
#include <stdlib.h>     /* strtol */
#include "contiki.h"
#include "rest-engine.h"
#include "dev/leds.h"
#include "coap.h"
#include "board.h"
#include "susensors.h"
#include "pairing.h"
//#include "../../apps/uartsensors/uart_protocolhandler.h"

#define MAX_RESOURCES	20
MEMB(coap_resources, resource_t, MAX_RESOURCES);

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

static void res_susensor_gethandler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_susensor_puthandler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

#define MSG_PACKED	0
#define PLAIN_TEXT	1

static void
res_susensor_gethandler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset){

	const char *url = NULL;
	const char *str = NULL;
	unsigned int ct = -1;
	REST.get_header_content_type(request, &ct);

	//	if(ct != REST.type.APPLICATION_OCTET_STREAM) {
	//		REST.set_response_status(response, REST.status.BAD_REQUEST);
	//		const char *error_msg = "msgPacked, octet-stream only";
	//		REST.set_response_payload(response, error_msg, strlen(error_msg));
	//		return;
	//	}

	//	coap_packet_t *const coap_req = (coap_packet_t *)request;
	//	if(IS_OPTION(coap_req, COAP_OPTION_OBSERVE)) {
	//		if(coap_req->observe == 0) {
	//
	//		}
	//		else if(coap_req->observe == 1){
	//
	//		}
	//	}

	int len = REST.get_url(request, &url);
	struct susensors_sensor *sensor = (struct susensors_sensor *)susensors_find(url, len);
	if(sensor != NULL){
		cmp_object_t obj;
		len = REST.get_query(request, &str);
		if(len > 0){
			if(strncmp(str, "AboveEventAt", len) == 0){
				len = sensor->suconfig(sensor, SUSENSORS_AEVENT_GET, &obj) == 0;
				REST.set_header_max_age(response, 3600);
			}
			else if(strncmp(str, "BelowEventAt", len) == 0){
				len = sensor->suconfig(sensor, SUSENSORS_BEVENT_GET, &obj) == 0;
				REST.set_header_max_age(response, 3600);
			}
			else if(strncmp(str, "ChangeEventAt", len) == 0){
				len = sensor->suconfig(sensor, SUSENSORS_CEVENT_GET, &obj) == 0;
				REST.set_header_max_age(response, 3600);
			}
			else if(strncmp(str, "RangeMin", len) == 0){
				len = sensor->suconfig(sensor, SUSENSORS_RANGEMIN_GET, &obj) == 0;
			}
			else if(strncmp(str, "RangeMax", len) == 0){
				len = sensor->suconfig(sensor, SUSENSORS_RANGEMAX_GET, &obj) == 0;
			}
			else if(strncmp(str, "getEventState", len) == 0){
				len = sensor->suconfig(sensor, SUSENSORS_EVENTSTATE_GET, &obj) == 0;
			}
			else if(strncmp(str, "getEventSetup", len) == 0){
				len = sensor->suconfig(sensor, SUSENSORS_EVENTSETUP_GET, buffer);
				REST.set_response_payload(response, buffer, len);
				return;
			}
			else if(strncmp(str, "pairings", len) == 0){

				int16_t ret = pairing_getlist(sensor, buffer, preferred_size, offset);

				if( ret == -1){
					REST.set_response_status(response, REST.status.BAD_REQUEST);
				}
				else {
					if( ret < preferred_size){	//Finished sending
						*offset = -1;
					}
					REST.set_response_payload(response, buffer, ret);
				}

				len = 0;
			}
			else{
				len = 0;
			}
		}
		else{	//Send the actual value
			len = sensor->status(sensor, ActualValue, &obj) == 0;
			REST.set_header_max_age(response, 30);
		}

		if(len){
			len = cp_encodeObject(buffer, &obj);
			REST.set_response_status(response, REST.status.OK);
			REST.set_response_payload(response, buffer, len);
		}
	}
}

static uint8_t large_update_store[200] = { 0 };

static void
res_susensor_puthandler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset){

	const char *url = NULL;
	const char *str = NULL;
	const uint8_t *payload = NULL;
	unsigned int ct = -1;
	REST.get_header_content_type(request, &ct);

	if(ct != REST.type.APPLICATION_OCTET_STREAM) {
		REST.set_response_status(response, REST.status.BAD_REQUEST);
		const char *error_msg = "msgPacked, octet-stream only";
		REST.set_response_payload(response, error_msg, strlen(error_msg));
		return;
	}

	int len = REST.get_url(request, &url);
	coap_packet_t *const coap_req = (coap_packet_t *)request;
	struct susensors_sensor *sensor = (struct susensors_sensor *)susensors_find(url, len);
	if(sensor != NULL){
		len = REST.get_query(request, &str);
		if(len > 0){
			/* Issue a command. The command is one of the enum suactions values*/
			const char *commandstr = NULL;
			char *pEnd;
			if(strncmp(str, "postEvent", len) == 0){
				len = REST.get_request_payload(request, &payload);
				if((len = sensor->eventhandler(sensor, len, (uint8_t*)payload)) == 0){
					REST.set_response_status(response, REST.status.OK);
				}
				else{
					REST.set_response_status(response, REST.status.BAD_REQUEST);
				}
			}
			else if(REST.get_query_variable(request, "setCommand", &commandstr) > 0 && commandstr != NULL) {
				if(sensor->value(sensor, strtol(commandstr, &pEnd, 10), 0) == 0){	//For now, no payload - might be necessary in the furture
					cmp_object_t obj;
					len = sensor->status(sensor, ActualValue, &obj) == 0;
					len = cp_encodeObject(buffer, &obj);
					REST.set_response_payload(response, buffer, len);
					REST.set_response_status(response, REST.status.OK);
				}
				else{
					REST.set_response_status(response, REST.status.BAD_REQUEST);
				}
			}
			else if(strncmp(str, "eventsetup", len) == 0){
				len = REST.get_request_payload(request, &payload);
				if(len <= 0){
					REST.set_response_status(response, REST.status.BAD_REQUEST);
					const char *error_msg = "no payload in query";
					REST.set_response_payload(response, error_msg, strlen(error_msg));
					return;
				}

				//TODO: Generate human readable error messages, if parsing fails (see pairing)
				if(sensor->suconfig(sensor, SUSENSORS_EVENTSETUP_SET, (void*)payload) == 0){
					REST.set_response_status(response, REST.status.CHANGED);
				}
				else{
					REST.set_response_status(response, REST.status.BAD_REQUEST);
				}
			}
			else if(strncmp(str, "pairRemoveIndex", len) == 0){
				len = REST.get_request_payload(request, &payload);
				int ret = pairing_remove(sensor, len, (uint8_t*) payload);
				if(ret == 0){
					REST.set_response_status(response, REST.status.CHANGED);
				}
				else if(ret == 2){
					REST.set_response_status(response, REST.status.INTERNAL_SERVER_ERROR);
				}
				else if(ret == 3){
					REST.set_response_status(response, REST.status.NOT_MODIFIED);
				}
				else if(ret == 4){
					REST.set_response_status(response, REST.status.BAD_REQUEST);
				}
			}
			else if(strncmp(str, "pairRemoveAll", len) == 0){
				if(pairing_remove_all(sensor) == 0){
					REST.set_response_status(response, REST.status.CHANGED);
				}
				else{
					REST.set_response_status(response, REST.status.BAD_REQUEST);
				}
			}
			else if(strncmp(str, "join", len) == 0){
				if((len = REST.get_request_payload(request, (const uint8_t **)&payload))) {
					if(coap_req->block1_num * coap_req->block1_size + len <= sizeof(large_update_store)) {

						if(pairing_assembleMessage(payload, len, coap_req->block1_num) == 0){
							REST.set_response_status(response, REST.status.CHANGED);
							coap_set_header_block1(response, coap_req->block1_num, 0, coap_req->block1_size);
						}

						if(coap_req->block1_more == 0){
							//We're finished receiving the payload, now parse it.
							int res = pairing_handle(sensor);
							if(res > 0){
								//All is good, return the id of the created pair
								uint32_t l = 0;
								cp_encodeU8(buffer, res, &l);
								REST.set_response_status(response, REST.status.CREATED);
								REST.set_response_payload(response, buffer, len);
							}
							else{
								switch(res){
								case 0:
									REST.set_response_status(response, REST.status.INTERNAL_SERVER_ERROR);
									break;
								case -1:
									REST.set_response_status(response, REST.status.BAD_REQUEST);
									const char *error_msg1 = "IPAdress wrong";
									REST.set_response_payload(response, error_msg1, strlen(error_msg1));
									break;
								case -2:
									REST.set_response_status(response, REST.status.BAD_REQUEST);
									const char *error_msg2 = "Destination URI wrong";
									REST.set_response_payload(response, error_msg2, strlen(error_msg2));
									break;
								case -3:
									REST.set_response_status(response, REST.status.INTERNAL_SERVER_ERROR);
									break;
								case -4:
									REST.set_response_status(response, REST.status.BAD_REQUEST);
									const char *error_msg3 = "Source URI wrong";
									REST.set_response_payload(response, error_msg3, strlen(error_msg3));
									break;
								case -5:
									REST.set_response_status(response, REST.status.NOT_MODIFIED);
									break;
								}
							}
						}
					}
					else {
						REST.set_response_status(response, REST.status.REQUEST_ENTITY_TOO_LARGE);
						return;
					}
				}
			}/* join */
		}/* len > 0 */
	}/* sensor != 0 */
}

#define CONFIG_SENSORS	1
//Return 0 if the sensor was added as a coap resource
//Return 1 if the sensor does not contain the necassery coap config
//Return 2 if we can't allocate any more sensors
//Return 3 the sensor was NULL
resource_t* res_susensor_activate(const struct susensors_sensor* sensor){

	if(sensor == NULL) return NULL;
	const struct extras* extra = &sensor->data;
	struct resourceconf* config;

	config = (struct resourceconf*)extra->config;
	//Create the resource for the coap engine
	resource_t* r = (resource_t*)memb_alloc(&coap_resources);
	if(r == 0)
		return NULL;

	r->url = config->type;
	r->attributes = config->attr;
	r->flags = config->flags;

	if(r->flags & METHOD_GET){
		r->get_handler = res_susensor_gethandler;
	}else r->get_handler = NULL;
	if(r->flags & METHOD_POST){
		//r->post_handler = res_proxy_post_handler;
	}else r->post_handler = NULL;
	if(r->flags & METHOD_PUT){
		r->put_handler = res_susensor_puthandler;
	}else r->put_handler = NULL;
	if(r->flags & METHOD_DELETE){
		//r->delete_handler = res_proxy_delete_handler;
	}else r->delete_handler = NULL;

	//Finally activate the resource with the rest coap
	rest_activate_resource(r, (char*)r->url);
	PRINTF("Activated resource: %s Attributes: %s - Spec: %s, Unit: %s\n", r->url, r->attributes, config->spec, config->unit );

	return r;
}
