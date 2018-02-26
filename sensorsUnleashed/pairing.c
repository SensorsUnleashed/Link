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
 * pairing.c
 *
 *  Created on: 12/11/2016
 *      Author: omn
 */
#include <stdlib.h>
#include "../sensorsUnleashed/pairing.h"

#include "contiki.h"
#include "cfs/cfs.h"
#include "cmp.h"
#include "cmp_helpers.h"
#include <string.h>
#include <stdio.h>
#include "net/ipv6/uip.h"
#include "lib/memb.h"
#include "rpl.h"
#include "susensors.h"

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

struct file_s{
	int offset;
	int fd;
};

static bool file_reader(cmp_ctx_t *ctx, void *data, uint32_t limit);
static uint32_t file_writer(cmp_ctx_t* ctx, const void *data, uint32_t count);

#define BUFFERSIZE	300
static uint8_t buffer[BUFFERSIZE] __attribute__ ((aligned (4))) = { 0 };
static uint32_t bufsize = 0;

MEMB(pairings, joinpair_t, 20);


static int addPrefix(uip_ip6addr_t* ip6addr){
	uip_ipaddr_t *prefix = NULL;
	uint8_t pl = 0;

	if(!curr_instance.used && curr_instance.dag.prefix_info.length == 0) {
		return -1;
	}

	prefix = &curr_instance.dag.prefix_info.prefix;
	pl = curr_instance.dag.prefix_info.length;

	int i = 0;
	do{
        if(pl >= 8){
        	ip6addr->u8[i] = prefix->u8[i];
        	pl -= 8;
        }
        else{
        	ip6addr->u8[i] &= (0xff >> pl);		//Clear whats on the prefix bits place (Shouldn't be anything)
        	ip6addr->u8[i] |= prefix->u8[i];	//Add the last part of the prefix
            pl = 0;
        }
        i++;
	}while(pl);

	return 0;
}

//Return 0 if data was stored
//Return 1 if there was no more space
uint8_t pairing_assembleMessage(const uint8_t* data, uint32_t len, uint32_t num){

	if(num == 0) {
		bufsize = 0;	//Clear the buffer
		memset(buffer, 0, BUFFERSIZE);
	}
	if(bufsize + len > BUFFERSIZE) return 1;
	//For now only one can pair at a time. There is a single buffer.
	memcpy(buffer + bufsize, data, len);
	bufsize += len;
	return 0;
}

//Returns the data left to send
//Return 0 if there are no more data to send
//Return -1 if no file found
int16_t pairing_getlist(susensors_sensor_t* s, uint8_t* buffer, uint16_t len, int32_t *offset){
	uint16_t ret = 0;

	char filename[30];
	memset(filename, 0, 30);
	sprintf(filename, "pairs_%s", s->type);

	int fd = cfs_open(filename, CFS_READ);

	if(fd < 0) return -1;

	cfs_seek(fd, *offset, CFS_SEEK_SET);
	ret = cfs_read(fd, buffer, len);
	*offset += ret;

	cfs_close(fd);

	return ret;
}
uint8_t pairing_remove_all(susensors_sensor_t* s){
	char filename[30];
	memset(filename, 0, 30);
	sprintf(filename, "pairs_%s", s->type);

	while(list_head(s->pairs) != 0){
		joinpair_t* p = list_pop(s->pairs);
		mmem_free(&p->dsturl);
		mmem_free(&p->srcurl);
	}

	return cfs_remove(filename);
}

//Used to read from msgpacked buffer
static bool buf_reader(cmp_ctx_t *ctx, void *data, uint32_t limit) {
	for(uint32_t i=0; i<limit; i++){
		*((char*)data++) = *((char*)ctx->buf++);
	}
	return true;
}

//Return not needed pairing - Indexes shall be ordered - lowest first e.g, [0,3,4]
//Return 0 on success
//Return 1 if there are no pairingfile
//Return 2 Its not possible to create a temperary file
//Return 3 There pairings file is empty.
//Return 4 index array malformed
uint8_t pairing_remove(susensors_sensor_t* s, uint32_t len, uint8_t* indexbuffer){

	uint8_t* index;			//Current line index to be removed
	uint32_t indexlen = 0;	//Received length of indexs
	int currentindex = 0;	//Current pairindex
	struct file_s orig, temp;

	char filename[30];
	memset(filename, 0, 30);
	sprintf(filename, "pairs_%s", s->type);

	orig.fd = cfs_open(filename, CFS_READ);
	orig.offset = 0;
	if(orig.fd < 0) return 1;

	temp.fd = cfs_open("temp", CFS_READ | CFS_WRITE);
	temp.offset = 0;
	if(temp.fd < 0) {
		cfs_close(orig.fd);
		return 2;
	}

	cmp_ctx_t cmp;
	cmp_init(&cmp, &orig, file_reader, file_writer);

	cmp_ctx_t cmptmp;
	cmp_init(&cmptmp, &temp, file_reader, file_writer);

	cmp_ctx_t cmpindex;
	cmp_init(&cmpindex, indexbuffer, buf_reader, 0);

	if(!cmp_read_array(&cmpindex, &indexlen)) {
		cfs_close(orig.fd);
		cfs_close(temp.fd);
		return 4;
	}

	uint8_t arr[indexlen];
	for(int i=0; i<indexlen; i++){
		if(!cmp_read_u8(&cmpindex, &arr[i])){
			cfs_close(orig.fd);
			cfs_close(temp.fd);
			return 4;
		}
	}
	index = &arr[0];

	/* Copy all the wanted pairings to a temp file */
	bufsize = BUFFERSIZE;
	while(cmp_read_bin(&cmp, buffer, &bufsize)){
		if(currentindex == *index){
			if(--indexlen > 0){
				index++;
			}
		}
		else{
			cmp_write_bin(&cmptmp, buffer, bufsize);
		}
		currentindex++;
		bufsize = BUFFERSIZE;
	}

	cfs_close(orig.fd);

	if(orig.offset == 0){	//File was empty, just leave
		return 3;
	}

	cfs_remove(filename);
	orig.fd = cfs_open(filename, CFS_WRITE);	//Truncate the file to the new content
	orig.offset = 0;
	if(orig.fd < 0) return 1;

	//	//Start writing from the end
	//	cfs_seek(temp.fd, 0, CFS_SEEK_SET);

	//Next copy the temp data back to the original file
	while(cmp_read_bin(&cmptmp, buffer, &bufsize)){
		cmp_write_bin(&cmp, buffer, bufsize);
		bufsize = BUFFERSIZE;
	}

	bufsize = 0;

	cfs_close(orig.fd);
	cfs_close(temp.fd);
	cfs_remove("temp");

	return 0;
}


//Return 0 if success
//Return >0 if error:
// 1 = IP address can not be parsed
// 2 = dst_uri could not be parsed
// 3 = Unable to parse the triggers
// 4 = src_uri could not be parsed
// 5 = Unable to allocate enough dynamic memory
// 6 = Unable to get the prefix, so not possible to pair

uint8_t parseMessage(joinpair_t* pair){

	uint32_t stringlen;
	char stringbuf[100];
	uint8_t* payload = &buffer[0];
	uint32_t bufindex = 0;

	memset(stringbuf, 0, 100);

	/*
	 * Decode the IP address
	 * 	To save space, and allow sensors to be moved into other address spaces
	 * 	the configurator can chose to send only the suffix, meaning the prefix
	 * 	has to be added.
	 * 	If the array was 16 bytes its an entire ip address, otherwise its suffix
	 * 	only
	 * */
	stringlen = cp_decodeU16Array((uint8_t*) payload + bufindex, (uint16_t*)&pair->destip, &bufindex);
	stringlen *= 2;	//We work as 8bit
	if(stringlen < 0){
		return 1;
	}
	else if(stringlen < 16){
		//Move the suffix to the other end of the address.
		//TODO: This should be made smarter.
		memcpy(&pair->destip.u8[16-stringlen], &pair->destip.u8[0], stringlen);
		addPrefix(&pair->destip);
	}

	//Decode the URL of the device
	stringlen = 100;
	if(cp_decode_string((uint8_t*) payload + bufindex, &stringbuf[0], &stringlen, &bufindex) != 0){
		return 2;
	}
	//stringlen++;	//We need the /0 also

	if(mmem_alloc(&pair->dsturl, stringlen+1) == 0){
		return 5;
	}
	memcpy((char*)MMEM_PTR(&pair->dsturl), (char*)stringbuf, stringlen+1);

	//Event triggers
	if(cp_decodeS8Array((uint8_t*) payload + bufindex, pair->triggers, &bufindex) != 0){
		return 3;
	}

	//Generate all the connection handles (FIXME: Even though they might not be used)
	if(pair->dsturlAbove == 0){
		pair->dsturlAbove = malloc(stringlen + strlen(strAbove)+1);
		memcpy(pair->dsturlAbove, &stringbuf, strlen(stringbuf));
		memcpy(pair->dsturlAbove + strlen(stringbuf), strAbove, strlen(strAbove)+1);
	}
	if(pair->dsturlBelow == 0){
		pair->dsturlBelow = malloc(stringlen + strlen(strBelow)+1);
		memcpy(pair->dsturlBelow, &stringbuf, strlen(stringbuf));
		memcpy(pair->dsturlBelow + strlen(stringbuf), strBelow, strlen(strBelow)+1);
	}
	if(pair->dsturlChange == 0){
		pair->dsturlChange = malloc(stringlen + strlen(strChange)+1);
		memcpy(pair->dsturlChange, &stringbuf, strlen(stringbuf));
		memcpy(pair->dsturlChange + strlen(stringbuf), strChange, strlen(strChange)+1);
	}

	stringlen = 100;
	if(cp_decode_string((uint8_t*) payload + bufindex, &stringbuf[0], &stringlen, &bufindex) != 0){
		return 4;
	}
	stringlen++;	//We need the /0 also
	if(mmem_alloc(&pair->srcurl, stringlen) == 0){
		return 3;
	}
	memcpy((char*)MMEM_PTR(&pair->srcurl), (char*)stringbuf, stringlen);

	pair->deviceptr = 0;
	pair->triggerindex = aboveEvent;

	return 0;
}

//Return 0 if success
//Return >0 if error:
// 1 = IP address can not be parsed
// 2 = dst_uri could not be parsed
// 3 = Unable to allocate enough dynamic memory
// 4 = src_uri could not be parsed
// 5 = device already paired
uint8_t pairing_handle(susensors_sensor_t* s){

	uint8_t* payload = &buffer[0];
	list_t pairings_list = s->pairs;

	if(strlen(s->type) + bufsize > BUFFERSIZE) return 3;
	cp_encodeString((uint8_t*) payload + bufsize, s->type, strlen(s->type), &bufsize);

	joinpair_t* p = (joinpair_t*)memb_alloc(&pairings);
	int ret = parseMessage(p);

	if(ret != 0){
		memb_free(&pairings, p);
		return ret;
	}

	p->deviceptr = s;


	//p now contains all pairing details.
	//To avoid having redundant paring, check if we
	//already has this pairing in store

	//Check if the pairing is already done
	joinpair_t* pair = NULL;

	for(pair = (joinpair_t *)list_head(pairings_list); pair; pair = pair->next) {
		if(pair->dsturl.size == p->dsturl.size){
			if(strncmp((char*)MMEM_PTR(&pair->dsturl),(char*)MMEM_PTR(&p->dsturl), pair->dsturl.size) == 0){
				memb_free(&pairings, p);
				return 5;
			}
		}
	}

	//Add pair to the list of pairs
	list_add(pairings_list, p);

	//Finally store pairing info into flash
	store_SensorPair(s, payload, bufsize);

	PRINTF("Pair dst: %s, triggers: 0x%X\n", (char*)MMEM_PTR(&p->dsturl), (unsigned int)p->triggers);

	//Signal to the susensor class, that a new pair/binding is ready.
	process_post(&susensors_process, susensors_pair, p);

	return 0;
}

void store_SensorPair(susensors_sensor_t* s, uint8_t* data, uint32_t len){
	char filename[30];
	memset(filename, 0, 30);
	sprintf(filename, "pairs_%s", s->type);
	struct file_s write;
	write.fd = cfs_open(filename, CFS_READ | CFS_WRITE | CFS_APPEND);
	write.offset = 0;

	if(write.fd < 0) {
		return;
	}

	cmp_ctx_t cmp;
	cmp_init(&cmp, &write, file_reader, file_writer);

	cmp_write_bin(&cmp, data, len);
	cfs_close(write.fd);
}

void restore_SensorPairs(susensors_sensor_t* s){
	struct file_s read;
	list_t pairings_list = s->pairs;
	char filename[40];
	memset(filename, 0, 40);
	sprintf(filename, "pairs_%s", s->type);

	read.fd = cfs_open(filename, CFS_READ);
	read.offset = 0;

	if(read.fd < 0) {
		return;
	}

	cmp_ctx_t cmp;
	cmp_init(&cmp, &read, file_reader, file_writer);
	bufsize = BUFFERSIZE;
	while(cmp_read_bin(&cmp, buffer, &bufsize)){
		joinpair_t* pair = (joinpair_t*)memb_alloc(&pairings);
		if(parseMessage(pair) == 0){
			PRINTF("SrcUri: %s -> DstUri: %s\n", (char*)MMEM_PTR(&pair->srcurl), (char*)MMEM_PTR(&pair->dsturl));
			pair->deviceptr = s;
			list_add(pairings_list, pair);

//			if(first){
//				//Signal to the susensor class, that a new pair/binding is ready.
//				process_post(&susensors_process, susensors_pair, pair);
//				first = 0;
//			}
		}
		else{
			memb_free(&pairings, pair);
		}
		bufsize = BUFFERSIZE;
	}
	bufsize = 0;
	cfs_close(read.fd);
}

static bool file_reader(cmp_ctx_t *ctx, void *data, uint32_t len) {

	struct file_s* file = (struct file_s*)ctx->buf;
	if(file->fd >= 0) {
		cfs_seek(file->fd, file->offset, CFS_SEEK_SET);
		file->offset += cfs_read(file->fd, data, len);
	}

	return true;
}

static uint32_t file_writer(cmp_ctx_t* ctx, const void *data, uint32_t len){

	struct file_s* file = (struct file_s*)ctx->buf;

	if(file->fd >= 0) {
		len = cfs_write(file->fd, data, len);
	}

	return len;
}
