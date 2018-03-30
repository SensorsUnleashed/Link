/*
 * pairgroup.c
 *
 *  Created on: 27. mar. 2018
 *      Author: omn
 */


#include <malloc.h>
#include "pairgroup.h"

LIST(groupslist);

/* 10 groups which each can contain 20 pairs */
MEMB(groupslist_memb, pairgroup_t, 10);
MEMB(pairgroupitem_memb, pairgroupItem_t, 20);

static pairgroup_t* pairGroupNew(enum su_basic_events triggerindex){
	pairgroup_t* g = memb_alloc(&groupslist_memb);
	if(g != 0){
		g->paired = 0;
		g->triggerindex = triggerindex;
		LIST_STRUCT_INIT(g, pairgroup);
		list_add(groupslist, g);
	}

	return g;
}

//Return 0 on match
static int comparePairLink(joinpair_t* p1, joinpair_t* p2, enum su_basic_events event){

	if(memcmp(p1->destip.u8, p2->destip.u8, 16) != 0) return 1;

	if(event == aboveEvent){
		if(strcmp(p1->dsturlAbove, p2->dsturlAbove) != 0) return 1;
	}
	else if(event == belowEvent){
		if(strcmp(p1->dsturlBelow, p2->dsturlBelow) != 0) return 1;
	}
	else if(event == changeEvent){
		if(strcmp(p1->dsturlChange, p2->dsturlChange) != 0) return 1;
	}

	return 0;
}

void pairGroupInit(){
	list_init(groupslist);
	memb_init(&groupslist_memb);
}

/*
 * Returns NULL if we have no link
 * Returns ptr to the group in should be included in
 * */
static pairgroup_t* pairGroupContainsLink(joinpair_t* pair, enum su_basic_events event){

	for(pairgroup_t* g = list_head(groupslist); g; g = list_item_next(g)){
		if(g->triggerindex == event){
			pairgroupItem_t* item = list_head(g->pairgroup);
			if(comparePairLink(item->pair, pair, event) == 0){
				return g;
			}
		}
	}

	return 0;
}

static int pairGroupHasPair(pairgroup_t* g, joinpair_t* pair){
	for(pairgroupItem_t* i = list_head(g->pairgroup); i; i = list_item_next(i)){
		if(i->pair == pair){
			return 1;
		}
	}
	return 0;
}

pairgroup_t* pairGroupAdd(joinpair_t* pair, enum su_basic_events event){

	pairgroup_t* g = pairGroupContainsLink(pair, event);
	if(g != 0){
		if(pairGroupHasPair(g, pair)) return g;
	}

	pairgroupItem_t* item = memb_alloc(&pairgroupitem_memb);
	item->pair = pair;

	if(g == 0){
		g = pairGroupNew(event);
		if(g == 0) return 0;
	}

	list_add(g->pairgroup, item);

	return g;
}


/*
 * Indications wether a group is gone is done by callbacks
 * */
int pairGroupRemove(joinpair_t* pair, enum su_basic_events event){

	pairgroup_t* g = pairGroupContainsLink(pair, event);
	if(g == 0) return 0;

	int len = 0;
	for(pairgroupItem_t* pgi = list_head(g->pairgroup); pgi; pgi = list_item_next(pgi)){
		if(pgi->pair == pair){
			list_remove(g->pairgroup, pgi);
			memb_free(&pairgroupitem_memb, pgi);

			len = list_length(g->pairgroup);
			if(len == 0){
				list_remove(groupslist, g);
				memb_free(&groupslist_memb, g);
			}
			break;
		}
	}
	return len;
}
