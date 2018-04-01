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

#include "susensors.h"
#include "lib/memb.h"
#include "pairing.h"
#include "rest-engine.h"
#include "coap-observe.h"
#include "cmp.h"
#include "reverseNotify.h"
#include "coap-engine.h"
#include "pairgroup.h"

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

process_event_t susensors_pair;
process_event_t susensors_pair_fail;
process_event_t susensors_presence;
process_event_t susensors_presence_success;
process_event_t susensors_presence_fail;
process_event_t susensors_new_observer;
process_event_t susensors_txhandler;
process_event_t susensors_event_handle;

enum transaction_Priority_e{
	Priority_Urgent,
	Priority_High,
	Priority_Medium,
	Priority_Low,
};
typedef enum transaction_Priority_e transaction_Priority_t;
/*
 * Highest priority is processed first
 * */
struct __attribute__ ((__packed__)) transaction_s{
	struct tx_package_s *next;
	transaction_Priority_t priority;
	process_event_t ev;
	process_data_t data;
	uip_ip6addr_t addr;
};
typedef struct transaction_s transaction_t;


#define MAX_ACTIVE_TRANSACTIONS	1
static uint8_t activeTransactions = 0;


PROCESS(susensors_process, "Sensors");

LIST(sudevices);
LIST(transactions);

MEMB(sudevices_memb, susensors_sensor_t, DEVICES_MAX);
MEMB(transactions_memb, transaction_t, 30);

void transactionAdd(process_event_t ev, process_data_t data, transaction_Priority_t priority, uip_ip6addr_t addr);


//A node has requested to observe one of our resources
void new_observer(coap_observer_t *obs){
	process_post(&susensors_process, susensors_new_observer, obs);
}

/* Called when a pair is added outside boot-up*/
void pair_added(joinpair_t* pair){

	transactionAdd(susensors_pair, pair, Priority_High, pair->destip);
	process_post(&susensors_process, susensors_txhandler, NULL);
}

void pair_removed(joinpair_t* pair){

	if(pair->triggers[0] != -1){
		if(pairGroupRemove(pair, aboveEvent) == 0){
			coap_remove_observer_by_uri(&pair->destip, UIP_HTONS(COAP_DEFAULT_PORT), pair->dsturlAbove);
		}
	}
	if(pair->triggers[1] != -1){
		if(pairGroupRemove(pair, belowEvent) == 0){
			coap_remove_observer_by_uri(&pair->destip, UIP_HTONS(COAP_DEFAULT_PORT), pair->dsturlBelow);
		}
	}
	if(pair->triggers[2] != -1){
		if(pairGroupRemove(pair, changeEvent) == 0){
			coap_remove_observer_by_uri(&pair->destip, UIP_HTONS(COAP_DEFAULT_PORT), pair->dsturlChange);
		}
	}
}

void initSUSensors(){
	list_init(sudevices);
	memb_init(&sudevices_memb);

	/* Initialize the REST engine. */
	rest_init_engine();
	coap_init_engine();

	pairing_init();
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
	process_post(&susensors_process, susensors_event_handle, s);
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
				pairGroupRemove(i, aboveEvent);
				pairGroupRemove(i, belowEvent);
				pairGroupRemove(i, changeEvent);
				process_post(&susensors_process, susensors_pair, i);
			}
		}
	}
	return interested;
}

static void notification_callback(coap_observee_t *obs, void *notification,
		coap_notification_flag_t flag){

	pairgroup_t* g = (pairgroup_t*) obs->data;

	for(pairgroupItem_t* item = list_head(g->pairgroup); item; item = list_item_next(item)){
		joinpair_t* pair = item->pair;
		switch(flag) {
		case NOTIFICATION_OK:
			break;
		case OBSERVE_OK: /* server accepeted observation request */
			process_post(&susensors_process, susensors_pair, pair);
			break;
		case OBSERVE_NOT_SUPPORTED:
		case ERROR_RESPONSE_CODE:
			//TODO: Handle response!
		case NO_REPLY_FROM_SERVER:
			process_post(&susensors_process, susensors_pair_fail, pair);
			g->paired = 0;
			break;
		}
	}
}

static void above_notificationcb(coap_observee_t *obs, void *notification,
		coap_notification_flag_t flag){

	int len = 0;
	const uint8_t *payload = NULL;
	pairgroup_t* g = (pairgroup_t*) obs->data;

	if(flag == NOTIFICATION_OK){

		if(notification) {
			len = coap_get_payload(notification, &payload);
		}

		for(pairgroupItem_t* i = list_head(g->pairgroup); i; i = list_item_next(i)){
			joinpair_t* pair = i->pair;

			susensors_sensor_t* this = (susensors_sensor_t*) pair->deviceptr;
			if(pair->aboveEventhandler != 0){
				pair->aboveEventhandler(this, len, payload);
			}
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
	pairgroup_t* g = (pairgroup_t*) obs->data;

	if(flag == NOTIFICATION_OK){

		if(notification) {
			len = coap_get_payload(notification, &payload);
		}

		for(pairgroupItem_t* i = list_head(g->pairgroup); i; i = list_item_next(i)){
			joinpair_t* pair = i->pair;

			susensors_sensor_t* this = (susensors_sensor_t*) pair->deviceptr;
			if(pair->belowEventhandler != 0){
				pair->belowEventhandler(this, len, payload);
			}
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
	pairgroup_t* g = (pairgroup_t*) obs->data;

	if(flag == NOTIFICATION_OK){

		if(notification) {
			len = coap_get_payload(notification, &payload);
		}

		for(pairgroupItem_t* i = list_head(g->pairgroup); i; i = list_item_next(i)){
			joinpair_t* pair = i->pair;

			susensors_sensor_t* this = (susensors_sensor_t*) pair->deviceptr;
			if(pair->changeEventhandler != 0){
				pair->changeEventhandler(this, len, payload);
			}
		}
	}
	else{
		notification_callback(obs, notification, flag);
	}
}

static void txPresence_cb(void *data, void *response){
	revlookup_t* rl = (revlookup_t*) data;
	coap_packet_t *const coap_res = (coap_packet_t *)response;

	if(response == NULL){	//Timeout
		process_post(&susensors_process, susensors_presence_fail, rl);
		return;
	}
	else if(coap_res->code == DELETED_2_02){
		//We are not known to the node we posted our presence to, remove it from our list
		process_post(&susensors_process, susensors_presence_success, rl);
	}
	else {
		process_post(&susensors_process, susensors_presence_success, rl);
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

void localPairConnect(joinpair_t* pair){
	if(pair->localdeviceptr != 0) return;
	for(susensors_sensor_t* d = susensors_first(); d; d = susensors_next(d)){
		if(strcmp((char*)MMEM_PTR(&pair->dsturl), d->type) == 0){
			pair->localdeviceptr = d;
		}
	}
}
/* Go through all the devices and all the pairs
 * Return 0 on finished all pairs else 1
 * */
static int setupPairsConnections(joinpair_t* pair){
	susensors_sensor_t* this = pair->deviceptr;

	if(pair->triggers[0] != -1 && pair->triggerindex < aboveEvent){	//Above
		pair->triggerindex = aboveEvent;
		if(this->setEventhandlers != 0)
			pair->aboveEventhandler = this->setEventhandlers(this, pair->triggers[aboveEvent]);
		if(pair->aboveEventhandler != NULL){
			if(pair->localhost){
				localPairConnect(pair);
				process_post(&susensors_process, susensors_pair, pair);
			}
			else{
				pairgroup_t* g = pairGroupAdd(pair, aboveEvent);
				if(g->paired == 0){
					g->paired = 1;
					coap_obs_request_registration(&pair->destip, UIP_HTONS(COAP_DEFAULT_PORT), pair->dsturlAbove,
							above_notificationcb, g);
				}
				else{
					process_post(&susensors_process, susensors_pair, pair);
				}

			}
		}
	}
	else if(pair->triggers[1] != -1 && pair->triggerindex < belowEvent){	//Below
		pair->triggerindex = belowEvent;
		if(this->setEventhandlers != 0)
			pair->belowEventhandler = this->setEventhandlers(this, pair->triggers[belowEvent]);
		if(pair->belowEventhandler != NULL){
			if(pair->localhost){
				localPairConnect(pair);
				process_post(&susensors_process, susensors_pair, pair);
			}
			else{
				pairgroup_t* g = pairGroupAdd(pair, belowEvent);
				if(g->paired == 0){
					g->paired = 1;
					coap_obs_request_registration(&pair->destip, UIP_HTONS(COAP_DEFAULT_PORT), pair->dsturlBelow,
						below_notificationcb, g);
				}
				else{
					process_post(&susensors_process, susensors_pair, pair);
				}
			}
		}
	}
	else if(pair->triggers[2] != -1 && pair->triggerindex < changeEvent){	//Change
		pair->triggerindex = changeEvent;
		if(this->setEventhandlers != 0)
			pair->changeEventhandler = this->setEventhandlers(this, pair->triggers[changeEvent]);
		if(pair->changeEventhandler != NULL){
			if(pair->localhost){
				localPairConnect(pair);
				process_post(&susensors_process, susensors_pair, pair);
			}
			else{
				pairgroup_t* g = pairGroupAdd(pair, changeEvent);
				if(g->paired == 0){
					g->paired = 1;
					coap_obs_request_registration(&pair->destip, UIP_HTONS(COAP_DEFAULT_PORT), pair->dsturlChange,
						change_notificationcb, g);
				}
				else{
					process_post(&susensors_process, susensors_pair, pair);
				}
			}
		}
	}
	else{
		pair->triggerindex = noEvent;
	}


	return pair->triggerindex != noEvent;
}

void transactionAdd(process_event_t ev, process_data_t data, transaction_Priority_t priority, uip_ip6addr_t addr){
	transaction_t* t = (transaction_t*)memb_alloc(&transactions_memb);
	if(t == NULL) return;

	t->ev = ev;
	t->data = data;
	t->priority = priority;
	t->addr = addr;

	transaction_t* next = NULL;
	transaction_t* last = NULL;
	if(list_length(transactions) > 0 && priority != Priority_Low){
		for(transaction_t* i = list_head(transactions); i; i = list_item_next(i)){
			next=list_item_next(i);
			if(next == NULL){
				if(i->priority > priority){
					list_insert(transactions, last, t);
					return;
				}
			}
			else if(i->priority < priority && next->priority != priority){
				list_insert(transactions, last, t);
				return;
			}
			last = i;
		}
	}

	list_add(transactions, t);
}

int transactionStartNext(){
	if(activeTransactions < MAX_ACTIVE_TRANSACTIONS){
		transaction_t* t = list_head(transactions);
		if(t != NULL){
			process_post(&susensors_process, t->ev, t->data);
			list_remove(transactions, t);
			memb_free(&transactions_memb, t);
			activeTransactions++;
		}
		else{
			return 0;	//No more transactions in que
		}
	}

	return MAX_ACTIVE_TRANSACTIONS - activeTransactions;
}

void transactionRemove(){
	if(activeTransactions > 0){
		activeTransactions--;
	}
}

void transactionRemoveNode(uip_ip6addr_t addr){
	transactionRemove();
	//Remove all other transactions for that node
	for(transaction_t* i = list_head(transactions); i; i = list_item_next(i)){
		if(memcmp(&addr, &i->addr, 16) == 0){
			list_remove(transactions, i);
			memb_free(&transactions_memb, i);
			i = list_head(transactions);
		}
	}
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(susensors_process, ev, data)
{
	static susensors_sensor_t* d;

	PROCESS_BEGIN();

	susensors_pair = process_alloc_event();
	susensors_pair_fail = process_alloc_event();

	susensors_presence = process_alloc_event();
	susensors_presence_success = process_alloc_event();
	susensors_presence_fail = process_alloc_event();

	susensors_new_observer= process_alloc_event();
	susensors_txhandler = process_alloc_event();

	susensors_event_handle = process_alloc_event();

	//Register callbacks
	register_new_observer_notify_callback(new_observer);
	pair_register_add_callback(pair_added);
	pair_register_rem_callback(pair_removed);

	pairGroupInit();
	list_t rlist = revNotifyInit();
	if(list_length(rlist) > 0){
		for(revlookup_t* a = list_head(rlist); a; a = list_item_next(a)){
			transactionAdd(susensors_presence, a, Priority_Medium, a->srcip);
		}
	}

	for(d = susensors_first(); d; d = susensors_next(d)) {
		d->event_flag = 0;
		d->configure(d, SUSENSORS_HW_INIT, 0);
		d->configure(d, SUSENSORS_ACTIVE, 1);

		//Restore sensor pairs stored in flash
		restore_SensorPairs(d);

		//Add the pairs to the transaction buffer
		list_t plist = d->pairs;
		for(joinpair_t* p = list_head(plist); p; p = list_item_next(p)){
			transactionAdd(susensors_pair, p, Priority_High, p->destip);
		}
	}

	//Begin the transactions
	process_post(&susensors_process, susensors_txhandler, NULL);

	while(1) {
		PROCESS_WAIT_EVENT();

		if(ev == susensors_txhandler){
			printf("susensors_txhandler!\n");
			while(transactionStartNext());
		}

		/*
		 * Do the pairing
		 * */
		else if(ev == susensors_pair){
			if(data != NULL){
				printf("susensors_pair!\n");
				if(setupPairsConnections(data) == 0){
					transactionRemove();
					process_post(&susensors_process, susensors_txhandler, NULL);
				}
			}
		}
		else if(ev == susensors_pair_fail){
			printf("susensors_pair_fail!\n");
			joinpair_t* pair = (joinpair_t*)data;
			transactionRemoveNode(pair->destip);
			process_post(&susensors_process, susensors_txhandler, NULL);
		}

		/*
		 * Send a presence message to the ones we know wants our service (Only used for power up)
		 * */
		else if(ev == susensors_presence){
			printf("susensors_presence!\n");
			revlookup_t* rl = (revlookup_t*) data;
			txPresence(rl);
		}
		else if(ev == susensors_presence_success){
			printf("susensors_presence_success!\n");
			transactionRemove();
			process_post(&susensors_process, susensors_txhandler, NULL);
		}
		else if(ev == susensors_presence_fail){
			printf("susensors_presence_fail!\n");
			revlookup_t* rl = (revlookup_t*) data;
			transactionRemoveNode(rl->srcip);
			process_post(&susensors_process, susensors_txhandler, NULL);
		}

		/* Called whenever a new node has joined our CoAP server */
		else if(ev == susensors_new_observer){
			coap_observer_t *obs =  (coap_observer_t*) data;
			revNotifyAdd(obs->addr);
		}
		else if(ev == susensors_event_handle){
			d = (susensors_sensor_t*) data;
			resource_t* resource = d->data.resource;

			//Handle all remote pairs
			if(resource != NULL){
				if(d->event_flag & SUSENSORS_CHANGE_EVENT){
					coap_notify_observers_sub(resource, strChange);
				}
				if(d->event_flag & SUSENSORS_BELOW_EVENT){
					coap_notify_observers_sub(resource, strBelow);
				}
				if(d->event_flag & SUSENSORS_ABOVE_EVENT){
					coap_notify_observers_sub(resource, strAbove);
				}
			}

			//Handle all local pairs - no need to use CoAP for this
			for(susensors_sensor_t* dd = susensors_first(); dd; dd = susensors_next(dd)){
				for(joinpair_t* p = list_head(dd->pairs); p; p = list_item_next(p)){
					cmp_object_t obj;
					uint8_t payload[10];
					int len;

					if(p->localhost && p->localdeviceptr == d){

						d->status(d, ActualValue, &obj);
						len = cp_encodeObject(payload, &obj);

						if(d->event_flag & SUSENSORS_CHANGE_EVENT){
							if(p->triggers[changeEvent] != -1){
								p->changeEventhandler(dd, len, payload);
							}
						}
						if(d->event_flag & SUSENSORS_BELOW_EVENT){
							if(p->triggers[belowEvent] != -1){
								p->belowEventhandler(dd, len, payload);
							}
						}
						if(d->event_flag & SUSENSORS_ABOVE_EVENT){
							if(p->triggers[aboveEvent] != -1){
								p->aboveEventhandler(dd, len, payload);
							}
						}
					}
				}
			}
			d->event_flag = SUSENSORS_NO_EVENT;
		}
	}
	PROCESS_END();
}
/*---------------------------------------------------------------------------*/

