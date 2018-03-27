/*
 * pairgroup.c
 *
 *  Created on: 27. mar. 2018
 *      Author: omn
 */


#include "pairgroup.h"

LIST(groupslist);

/* 10 groups which each can contain 20 pairs */
MEMB(groupslist_memb, pairgroups_t, 10);
MEMB(pairgroup_memb, pairgroup_t, 20);

static int initialized = 0;

static pairgroups_t* pairGroupNew(){
	pairgroups_t* g = memb_alloc(&groupslist_memb);
	if(g != 0){
		LIST_STRUCT_INIT(g, pairgroup);
	}
	list_add(groupslist, g);
	return g;
}

/* Return 0 on equal else > 0*/
static int pairGroupCmp(joinpair_t* p1, joinpair_t* p2){
	if(memcmp(p1->destip.u8, p2->destip.u8, 16) != 0){
		return 1;
	}

	if(p1->triggers[aboveEvent] == p2->triggers[aboveEvent] || p1->triggers[aboveEvent] != -1){
		if(strcmp(p1->dsturlAbove, p2->dsturlAbove) != 0){
			return 1;
		}
	}
	if(p1->triggers[belowEvent] != p2->triggers[belowEvent] || p1->triggers[belowEvent] == -1){
		if(strcmp(p1->dsturlBelow, p2->dsturlBelow) != 0){
			return 1;
		}
	}
	if(p1->triggers[changeEvent] != p2->triggers[changeEvent] || p1->triggers[changeEvent] == -1){
		if(strcmp(p1->dsturlChange, p2->dsturlChange) != 0){
			return 1;
		}
	}

	return 0;
}

static void pairGroupInit(){
	list_init(groupslist);
	memb_init(&groupslist_memb);
	initialized = 1;
}

void pairGroup_add(joinpair_t* pair){

	if(!initialized) pairGroupInit();

	pairgroup_t* item = memb_alloc(&pairgroup_memb);
	item->pair = pair;

	/* Now add the pair to the right group */
	for(pairgroups_t* g = list_head(groupslist); g; g = list_item_next(groupslist)){
		pairgroup_t* p = list_head(g->pairgroup);
		if(pairGroupCmp(p->pair, pair) == 0){
			list_add(g->pairgroup, item);
			return;
		}
	}

	pairgroups_t* g = pairGroupNew();
	if(g != 0){
		list_add(g->pairgroup, item);
		return;
	}
}
