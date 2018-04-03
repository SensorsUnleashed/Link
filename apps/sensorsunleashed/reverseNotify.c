/*
 * reverseNotify.c
 *
 *  Created on: 28. dec. 2017
 *      Author: omn
 */
#include "contiki.h"
#include "net/ipv6/uiplib.h"
#include "mmem.h"
#include "lib/memb.h"
#include "lib/list.h"
#include "cfs/cfs.h"
#include "cmp.h"
#include "susensors.h"
#include "reverseNotify.h"
#include "cmp_helpers.h"

LIST(revlookup);
MEMB(revlookup_memb, revlookup_t, 20);

static void writeFile();

static const char* filename = "revnotify";

list_t revNotifyInit(){
	list_init(revlookup);
	memb_init(&revlookup_memb);

	struct file_s read;
	read.fd = cfs_open(filename, CFS_READ);
	read.offset = 0;

	if(read.fd < 0) {
		return revlookup;
	}

	cmp_ctx_t cmp;
	cmp_init(&cmp, &read, file_reader, file_writer);
	uint32_t size;

	while(cmp_read_array(&cmp, &size) == true){
		if(size != 8) return NULL;
		revlookup_t* addr = (revlookup_t*)memb_alloc(&revlookup_memb);
		if(addr == NULL) return NULL;

		for(int j=0; j<size; j++){
			cmp_read_u16(&cmp, &addr->srcip.u16[j]);
		}
		list_add(revlookup, addr);
	}
	cfs_close(read.fd);
	return revlookup;
}

static revlookup_t* revNotifyFind(uip_ip6addr_t srcaddr){

	int found = 0;
	//Find out if the addr is already in our list
	for(revlookup_t* i = list_head(revlookup); i; i = list_item_next(i)){
		found = 1;
		for(int j=0; j<8; j++){
			if(srcaddr.u16[j] != i->srcip.u16[j]){
				found = 0;
				break;
			}
		}
		if(found){
			return i;
		}
	}

	return NULL;
}

void revNotifyAdd(uip_ip6addr_t srcaddr){

	int add = revNotifyFind(srcaddr) == NULL;

	//Add to the list and write the entire list to flash.
	//This will seldom happen, so no need to append to the
	//list. It only adds complexity
	if(add){
		revlookup_t* addr = (revlookup_t*)memb_alloc(&revlookup_memb);

		if(addr){
			memcpy(&addr->srcip, &srcaddr, 16);
			list_add(revlookup, addr);
			writeFile();
		}
	}
}

void revNotifyRmAddr(uip_ip6addr_t srcip){
	revlookup_t* item = revNotifyFind(srcip);
	if(item != NULL){
		list_remove(revlookup, item);
		memb_free(&revlookup_memb, item);
	}
}

void revNotifyRmItem(revlookup_t* item){
	list_remove(revlookup, item);
	memb_free(&revlookup_memb, item);
	writeFile();
}

static void writeFile(){
	cmp_ctx_t cmp;
	struct file_s write;
	write.fd = cfs_open(filename, CFS_READ | CFS_WRITE );
	write.offset = 0;

	if(write.fd < 0) {
		return;
	}

	if(list_length(revlookup) == 0){
		cfs_remove(filename);
	}
	else{
		cmp_init(&cmp, &write, 0, file_writer);

		for(revlookup_t* i = list_head(revlookup); i; i = list_item_next(i)){
			cmp_write_array(&cmp, 8);
			for(int j=0; j<8; j++){
				cmp_write_u16(&cmp, i->srcip.u16[j]);
			}
		}
	}

	cfs_close(write.fd);
}

