/*
 * reverseNotify.h
 *
 *  Created on: 28. dec. 2017
 *      Author: omn
 */

#ifndef SENSORSUNLEASHED_REVERSENOTIFY_H_
#define SENSORSUNLEASHED_REVERSENOTIFY_H_

#include "net/ipv6/uiplib.h"

struct __attribute__ ((__packed__)) revlookup_s{
	struct revlookup_s *next;
	uip_ip6addr_t srcip;
};

typedef struct revlookup_s revlookup_t;

list_t revNotifyInit();
void revNotifyAdd(uip_ip6addr_t srcaddr);
void revNotifyRmAddr(uip_ip6addr_t srcip);
void revNotifyRmItem(revlookup_t* item);

#endif /* SENSORSUNLEASHED_REVERSENOTIFY_H_ */
