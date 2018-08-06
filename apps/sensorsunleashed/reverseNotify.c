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

