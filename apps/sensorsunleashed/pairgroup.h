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

struct pairgroups_s{
	struct pairgroups_s *next;
	LIST_STRUCT(pairgroup);
};

struct pairgroup_s{
	struct pairgroup_s *next;
	joinpair_t* pair;
};
typedef struct pairgroups_s pairgroups_t;
typedef struct pairgroup_s pairgroup_t;

void pairGroup_add(joinpair_t* pair);

#endif /* APPS_SENSORSUNLEASHED_PAIRGROUP_H_ */
