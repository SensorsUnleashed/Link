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
#include <string.h>

#include "contiki.h"

#include "lib/susensors.h"
#include "lib/memb.h"
#include "../pairing.h"
#include "rest-engine.h"
#include "coap-observe.h"
#include "lib/cmp.h"
#include "reverseNotify.h"

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

const char* strAbove = "/above";
const char* strBelow = "/below";
const char* strChange = "/change";

process_event_t susensors_event;
process_event_t susensors_pair;
#ifdef USE_BR_DETECT
process_event_t susensors_service;
#endif
process_event_t susensors_presence;
process_event_t susensors_new_observer;

PROCESS(susensors_process, "Sensors");

LIST(sudevices);
#ifdef USE_BR_DETECT
LIST(missing);
#endif
MEMB(sudevices_memb, susensors_sensor_t, DEVICES_MAX);

//A node has requested to observe one of our resources
void new_observer(coap_observer_t *obs){
	process_post(&susensors_process, susensors_new_observer, obs);
}

#ifdef USE_BR_DETECT
static bool buf_reader(cmp_ctx_t *ctx, void *data, uint32_t limit) {
	for(uint32_t i=0; i<limit; i++){
		*((char*)data++) = *((char*)ctx->buf++);
	}
	return true;
}
#endif

void initSUSensors(){
	list_init(sudevices);
	memb_init(&sudevices_memb);
#ifdef USE_BR_DETECT
	list_init(missing);
#endif
}

susensors_sensor_t* addSUDevices(susensors_sensor_t* device){
	susensors_sensor_t* d;
	d = memb_alloc(&sudevices_memb);
	if(d == 0) return NULL;

	memcpy(d, device, sizeof(susensors_sensor_t));
	LIST_STRUCT_INIT(d, pairs);
	list_add(sudevices, d);

	return d;
}

/*---------------------------------------------------------------------------*/
susensors_sensor_t *
susensors_first(void)
{
	return list_head(sudevices);
}
/*---------------------------------------------------------------------------*/
susensors_sensor_t *
susensors_next(susensors_sensor_t* s)
{
	return list_item_next(s);
}
/*---------------------------------------------------------------------------*/
void
susensors_changed(susensors_sensor_t* s, uint8_t event)
{
	s->event_flag |= event;
	process_poll(&susensors_process);
}
/*---------------------------------------------------------------------------*/
susensors_sensor_t*
susensors_find(const char *prefix, unsigned short len)
{
	susensors_sensor_t* i;

	/* Search through all processes and search for the specified process
     name. */
	if(!len)
		len = strlen(prefix);

	for(i = susensors_first(); i; i = susensors_next(i)) {
		uint8_t su_url_len = strlen(i->type);

		if((su_url_len == len
				|| (len > su_url_len
						&& (((struct resourceconf*)(i->data.config))->flags & HAS_SUB_RESOURCES)
						&& prefix[su_url_len] == '/'))
				&& strncmp(prefix, i->type, su_url_len-1) == 0) {

			return i;

		}
	}
	return NULL;
}

//A node just called in - letting us know it was just powered on.
int missingJustCalled(uip_ip6addr_t* srcip){

	int interested = 0;
	//Go through all device pairs and trigger a new observe request
	for(susensors_sensor_t *d = susensors_first(); d; d = susensors_next(d)) {
		for(joinpair_t* i = list_head(d->pairs); i; i = list_item_next(i)){
			int found = 1;
			for(int j=0; j<8; j++){
				if(srcip->u16[j] != i->destip.u16[j]){
					found = 0;
					break;
				}
			}
			if(found){
				interested = 1;
				i->triggerindex = aboveEvent;
				process_post(&susensors_process, susensors_pair, i);
			}
		}
	}
	return interested;
}
#ifdef USE_BR_DETECT
/* Add the pair to the list of missing nodes - the list module will make sure only identical pairs is added
 * There can be more pairs to the same node, but we only want the detect
 * mechanism to detect for node id's not resources.
 * Returns:
 * 	1 if we need to start the detect mechanism
 * 	0 if we are already looking for it.
 * */
static int addMissing(joinpair_t* pair){
	int ret = 0;

	//If there is nothing in the list, we always need to detect
	ret = list_length(missing) == 0;

	for(joinpair_t* i = list_head(missing); i && !ret; i = list_item_next(i)){
		for(int j=0; j<8; j++){
			if(pair->destip.u16[j] != i->destip.u16[j]){
				list_add(missing, pair);
				ret = 1;
				break;
			}
		}
	}

	list_add(missing, pair);

	return ret;
}

static joinpair_t* findFromMissing(uip_ip6addr_t* destip){
	joinpair_t* pair = NULL;
	for(joinpair_t* i = list_head(missing); i; i = list_item_next(i)){
		for(int j=0; j<8; j++){
			if(i->destip.u16[j] != destip->u16[j]) {
				pair = NULL;
				break;
			}
			else
				pair = i;
		}

		if(pair){
			return pair;
		}
	}

	return pair;
}

static void nodedetected_callback(coap_observee_t *obs, void *notification,
		coap_notification_flag_t flag){

	joinpair_t* pair;
	cmp_ctx_t cmp;
	int len = 0;
	const uint8_t *payload = NULL;

	printf("Notification handler\n");
	printf("Observee URI: %s\n", obs->url);
	if(notification) {
		len = REST.get_request_payload(notification, &payload);
		//len = coap_get_payload(notification, &payload);
	}
	switch(flag) {
	case NOTIFICATION_OK:
		printf("Payload len = %d\n", len);
		cmp_init(&cmp, (uint8_t*)payload, buf_reader, 0);
		uint32_t size = 0;
		uip_ip6addr_t destip;

		//First unpack the payload (If its ok, that is)
		if(cmp_read_array(&cmp, &size) == false){
			coap_obs_remove_observee_by_token(&obs->addr, UIP_HTONS(COAP_DEFAULT_PORT), obs->token, obs->token_len);
			return;
		}
		for(int j=0; j<size; j++){
			cmp_read_u16(&cmp, &destip.u16[j]);
		}

		//Next find the pairs needing this IP
		do{
			pair = findFromMissing(&destip);
			if(pair){
				//Signal to the process that it can try to pair again.
				process_post(&susensors_process, susensors_pair, pair);
				list_remove(missing, pair);
			}
		}while(pair != NULL);

		if(list_length(missing) == 0){
			//We're missing noone, remove observee
			coap_obs_remove_observee_by_token(&obs->addr, UIP_HTONS(COAP_DEFAULT_PORT), obs->token, obs->token_len);
		}

		break;
	case OBSERVE_OK: /* server accepeted observation request */
		printf("OBSERVE_OK: %*s\n", len, (char *)payload);
		break;
	case OBSERVE_NOT_SUPPORTED:
		printf("OBSERVE_NOT_SUPPORTED: %*s\n", len, (char *)payload);
		obs = NULL;
		break;
	case ERROR_RESPONSE_CODE:
		printf("ERROR_RESPONSE_CODE: %*s\n", len, (char *)payload);
		obs = NULL;
		break;
	case NO_REPLY_FROM_SERVER:
		printf("NO_REPLY_FROM_SERVER: "
				"removing observe registration with token %x%x\n",
				obs->token[0], obs->token[1]);
		obs = NULL;
		break;
	}
}
#endif

static void notification_callback(coap_observee_t *obs, void *notification,
		coap_notification_flag_t flag){

	int len = 0;
	const uint8_t *payload = NULL;
	joinpair_t* pair = (joinpair_t*) obs->data;

	printf("Notification handler\n");
	printf("Observee URI: %s\n", obs->url);
	if(notification) {
		len = coap_get_payload(notification, &payload);
	}
	switch(flag) {
	case NOTIFICATION_OK:
		printf("NOTIFICATION OK: %*s\n", len, (char *)payload);
		break;
	case OBSERVE_OK: /* server accepeted observation request */
		printf("OBSERVE_OK: %*s\n", len, (char *)payload);
		break;
	case OBSERVE_NOT_SUPPORTED:
		printf("OBSERVE_NOT_SUPPORTED: %*s\n", len, (char *)payload);
		break;
	case ERROR_RESPONSE_CODE:
		printf("ERROR_RESPONSE_CODE: %*s\n", len, (char *)payload);
		break;
	case NO_REPLY_FROM_SERVER:
		printf("NO_REPLY_FROM_SERVER: "
				"removing observe registration with token %x%x\n",
				obs->token[0], obs->token[1]);
#ifdef USE_BR_DETECT
		joinpair_t* pair = (joinpair_t*) obs->data;

		//Signal to the susensor process, that a pair/binding failed.
		//If the node is already watched, then just bail.
		if(addMissing(pair)){
			//process_post(&susensors_process, susensors_service, pair);
		}
#endif
		break;
	}

	//Ready to request next pair
	process_post(&susensors_process, susensors_pair, pair);
}

static void above_notificationcb(coap_observee_t *obs, void *notification,
		coap_notification_flag_t flag){

	int len = 0;
	const uint8_t *payload = NULL;
	joinpair_t* pair = (joinpair_t*) obs->data;

	if(flag == NOTIFICATION_OK){

		if(notification) {
			len = coap_get_payload(notification, &payload);
		}


		susensors_sensor_t* this = (susensors_sensor_t*) pair->deviceptr;
		if(pair->aboveEventhandler != 0){
			pair->aboveEventhandler(this, len, payload);
		}

	}
	else{
		notification_callback(obs, notification, flag);
	}
}

static void below_notificationcb(coap_observee_t *obs, void *notification,
		coap_notification_flag_t flag){
	int len = 0;
	const uint8_t *payload = NULL;
	joinpair_t* pair = (joinpair_t*) obs->data;

	if(flag == NOTIFICATION_OK){

		if(notification) {
			len = coap_get_payload(notification, &payload);
		}

		susensors_sensor_t* this = (susensors_sensor_t*) pair->deviceptr;
		if(pair->belowEventhandler != 0){
			pair->belowEventhandler(this, len, payload);
		}

	}
	else{
		notification_callback(obs, notification, flag);
	}
}

static void change_notificationcb(coap_observee_t *obs, void *notification,
		coap_notification_flag_t flag){
	int len = 0;
	const uint8_t *payload = NULL;
	joinpair_t* pair = (joinpair_t*) obs->data;

	if(flag == NOTIFICATION_OK){

		if(notification) {
			len = coap_get_payload(notification, &payload);
		}

		susensors_sensor_t* this = (susensors_sensor_t*) pair->deviceptr;
		if(pair->changeEventhandler != 0){
			pair->changeEventhandler(this, len, payload);
		}

	}
	else{
		notification_callback(obs, notification, flag);
	}
}

static void txPresence_cb(void *data, void *response){
	revlookup_t* rl = (revlookup_t*) data;
	revlookup_t* nrl;
	coap_packet_t *const coap_res = (coap_packet_t *)response;

	nrl = list_item_next(rl);
	if(nrl){
		process_post(&susensors_process, susensors_presence, nrl);
	}

	if(response == NULL){	//Timeout
		return;
	}

	if(coap_res->code == DELETED_2_02){
		//We are not known to the node we posted our presence to, remove it from our list
		revNotifyRmItem(rl);
	}
}

static void txPresence(revlookup_t* rl){
	coap_packet_t request[1];
	coap_transaction_t *t;

	coap_init_message(request, COAP_TYPE_CON, COAP_PUT, coap_get_mid());
	coap_set_header_uri_path(request, "su/nodeinfo");	//Any message will do, we only need to see it awake
	coap_set_header_uri_query(request, "obs");
	coap_set_header_content_format(request, REST.type.APPLICATION_OCTET_STREAM);
	t = coap_new_transaction(request->mid, &rl->srcip, UIP_HTONS(COAP_DEFAULT_PORT));
	if(t) {
		t->callback = txPresence_cb;
		t->callback_data = rl;
		t->packet_len = coap_serialize_message(request, t->packet);
		coap_send_transaction(t);
	}
}

/* Go through all the devices and all the pairs
 * Return 0 on finished all pairs else 1
 * */
static int setupPairsConnections(joinpair_t* pair){
	susensors_sensor_t* this = pair->deviceptr;

	do{
		//There is no more triggers, now proceed to the next device if any
		if(pair->triggerindex >= noEvent){
			for(this = susensors_next(this); this; this = susensors_next(this)) {
				pair = list_head(this->pairs);
				if(pair != NULL) break;
			}

			//Check if we're done
			if(this == NULL || pair == NULL) {
				return 0;
			}
		}

		if(pair->triggers[0] != -1 && pair->triggerindex < aboveEvent){	//Above
			pair->triggerindex = 0;
			pair->aboveEventhandler = this->setEventhandlers(this, pair->triggers[aboveEvent]);
			if(pair->aboveEventhandler != NULL){
				coap_obs_request_registration(&pair->destip, UIP_HTONS(COAP_DEFAULT_PORT), pair->dsturlAbove,
						above_notificationcb, pair);
			}
		}
		else if(pair->triggers[1] != -1 && pair->triggerindex < belowEvent){	//Below
			pair->triggerindex = belowEvent;
			pair->belowEventhandler = this->setEventhandlers(this, pair->triggers[belowEvent]);
			if(pair->belowEventhandler != NULL){
				coap_obs_request_registration(&pair->destip, UIP_HTONS(COAP_DEFAULT_PORT), pair->dsturlBelow,
						below_notificationcb, pair);
			}
		}
		else if(pair->triggers[2] != -1 && pair->triggerindex < changeEvent){	//Change
			pair->triggerindex = changeEvent;
			pair->changeEventhandler = this->setEventhandlers(this, pair->triggers[changeEvent]);
			if(pair->changeEventhandler != NULL){
				coap_obs_request_registration(&pair->destip, UIP_HTONS(COAP_DEFAULT_PORT), pair->dsturlChange,
						change_notificationcb, pair);
			}
		}
		else{
			pair->triggerindex = noEvent;
		}
	} while(pair->triggerindex == noEvent);

	return 1;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(susensors_process, ev, data)
{
	static int events;
	static susensors_sensor_t* d;
	static int revlookupdone = 0;

	PROCESS_BEGIN();

	susensors_event = process_alloc_event();
	susensors_pair = process_alloc_event();
#ifdef USE_BR_DETECT
	susensors_service = process_alloc_event();
#endif
	susensors_presence = process_alloc_event();
	susensors_new_observer= process_alloc_event();

	register_new_observer_notify_callback(new_observer);

	revNotifyInit();

	for(d = susensors_first(); d; d = susensors_next(d)) {
		d->event_flag = 0;
		d->configure(d, SUSENSORS_HW_INIT, 0);
		d->configure(d, SUSENSORS_ACTIVE, 1);

		//Restore sensor pairs stored in flash
		restore_SensorPairs(d);
	}


	//Begin the pair connection
	joinpair_t* pair = NULL;
	for(d = susensors_first(); d; d = susensors_next(d)) {
		pair = list_head(d->pairs);
		if(pair != NULL){
			process_post(&susensors_process, susensors_pair, pair);
			break;
		}
	}

	while(1) {
		PROCESS_WAIT_EVENT();

		if(ev == susensors_pair){
			if(data != NULL){
				//revNotifyRmAddr(((joinpair_t*)data)->destip);	//No need to notify the device more than once
				if(setupPairsConnections(data) == 0){
					//Done pairing our own devices - now pass handle reverse Notify
					process_post(&susensors_process, susensors_presence, NULL);
				}
			}
		}
#ifdef USE_BR_DETECT
		else if(ev == susensors_service){
			joinpair_t* pair = (joinpair_t*) data;
			uip_ip6addr_t root;

			//susensors_sensor_t* this = (susensors_sensor_t*) pair->deviceptr;
			uint8_t* addr = pair->destip.u8;

			if(uiplib_ip6addrconv("fd81:3daa:fb4a:f7ae:212:4b00:60d:9aa4", &root)){
				//64bit prefix only
				sprintf(pair->nodediscuri, "%02x%02x:%02x%02x:%02x%02x:%02x%02x", \
						((uint8_t *)addr)[8], ((uint8_t *)addr)[9], \
						((uint8_t *)addr)[10], ((uint8_t *)addr)[11], \
						((uint8_t *)addr)[12], ((uint8_t *)addr)[13], \
						((uint8_t *)addr)[14], ((uint8_t *)addr)[15]);

				/* Request The root RPL DAG to look for it, and give us a feedback, when its back online*/
				coap_obs_request_registration_query(&root, UIP_HTONS(COAP_DEFAULT_PORT), "su/detect", pair->nodediscuri,
						nodedetected_callback, NULL);
			}
		}
#endif

		/*
		 * Send a presence message to the ones we know wants our service (Only used for power up)
		 * */
		else if(ev == susensors_presence){
			if(revlookupdone) break;
			revlookupdone = 1;
			revlookup_t* rl;

			if(data == NULL){	//This is the first time we get called. Initiate..
				rl = list_head(revNotifyGetList());
			}
			else{
				rl = (revlookup_t*) data;
			}

			if(rl != NULL){
				txPresence(rl);
			}
		}
		else if(ev == susensors_new_observer){
			coap_observer_t *obs =  (coap_observer_t*) data;
			revNotifyAdd(obs->addr);
		}
		else {
			do {
				printf("notify!\n");
				events = 0;
				for(d = susensors_first(); d; d = susensors_next(d)) {
					resource_t* resource = d->data.resource;
					if(resource != NULL){
						if(d->event_flag & SUSENSORS_CHANGE_EVENT){
							d->event_flag &= ~SUSENSORS_CHANGE_EVENT;
							coap_notify_observers_sub(resource, strChange);
						}
						if(d->event_flag & SUSENSORS_BELOW_EVENT){
							d->event_flag &= ~SUSENSORS_BELOW_EVENT;
							coap_notify_observers_sub(resource, strBelow);
						}
						if(d->event_flag & SUSENSORS_ABOVE_EVENT){
							d->event_flag &= ~SUSENSORS_ABOVE_EVENT;
							coap_notify_observers_sub(resource, strAbove);
						}
					}
					d->event_flag = SUSENSORS_NO_EVENT;
				}
			} while(events);
		}
	}

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/

