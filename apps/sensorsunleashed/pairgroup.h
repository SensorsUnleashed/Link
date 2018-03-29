/*
 * pairgroup.h
 *
 *  Created on: 27. mar. 2018
 *      Author: omn
 */

#ifndef APPS_SENSORSUNLEASHED_PAIRGROUP_H_
#define APPS_SENSORSUNLEASHED_PAIRGROUP_H_

#include "contiki.h"
#include "pairing.h"

/*
 * To avoid having to parse through urls, group all
 *	sensors, that have the same pairs. This will consume
 *	little more memory, but will speed up the process
 *	of finding the right sensor pairs.
 *
 *	pairgroups_t contains a list of pairgroups
 *	a pairgroup all use the sanme pair from the same node
 */

struct pairgroup_s{
	struct pairgroup_s *next;
	uint8_t triggerindex;
	uint8_t paired;
	LIST_STRUCT(pairgroup);
};

struct pairgroupItem_s{
	struct pairgroupItem_s *next;
	joinpair_t* pair;
};
typedef struct pairgroup_s pairgroup_t;
typedef struct pairgroupItem_s pairgroupItem_t;


void pairGroupInit();
pairgroup_t* pairGroupAdd(joinpair_t* pair, enum su_basic_events event);
int pairGroupRemove(joinpair_t* pair, enum su_basic_events event);

#endif /* APPS_SENSORSUNLEASHED_PAIRGROUP_H_ */
