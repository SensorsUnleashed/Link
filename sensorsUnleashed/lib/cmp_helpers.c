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
 * coap_proxy_protocolhandler.c
 *
 *  Created on: 18/09/2016
 *      Author: omn
 */
#include "cmp_helpers.h"
#include "crc16.h"
#include <string.h>
#include <stdio.h>

/*
 *
 * Return:
 * 			 0 for OK
 * 			-1 for CRC error
 * */
int cp_decodemessage(char* source, int len, rx_msg* destination){

	char* plbuf = (char*)destination->payload;

	//Verify the integrity of the data
	char msglen = *(source + len - 3);
	unsigned short crc = *((unsigned short*)(source + len - 2));
	unsigned short crct = crc16_data((unsigned char*)source, len - 2, 0);
	if(crct != crc) return -1;	//UPS - message not ok

	//Finally store the message in a container, for later use
	//Resource strings will be stored in a seperate
	destination->seqno = *source++;
	destination->cmd = *source++;
	destination->len = msglen - 2;
	for(int i=0; i<destination->len; i++){
		*plbuf++ = *source++;
	}

	return 0;
}

/*
 * Encode the requested message
 * return: Buffer length
 * */
uint32_t cp_encodemessage(uint8_t msgid, enum req_cmd cmd, void* payload, char len, uint8_t* buffer){

	char* tmp = payload;
	char* buffer_start = (char*)buffer;
	int count;
	unsigned short crc;
	*buffer++ = msgid;
	*buffer++ = (char)cmd;
	for(int i=0; i<len; i++){
		*buffer++ = *tmp++;
	}
	count = (void*)buffer - (void*)buffer_start;
	*buffer++ = count;
	crc = crc16_data((uint8_t*)buffer_start, count+1, 0);
	*buffer++ = crc & 0xff;
	*buffer++ = crc >> 8;

	return (uint32_t)((void*)buffer - (void*)buffer_start);
}

//Used to read from msgpacked buffer
static bool buf_reader(cmp_ctx_t *ctx, void *data, uint32_t limit) {
	for(uint32_t i=0; i<limit; i++){
		*((char*)data++) = *((char*)ctx->buf++);
	}
	return true;
}


static uint32_t buf_writer(cmp_ctx_t* ctx, const void *data, uint32_t count){
	for(uint32_t i=0; i<count; i++){
		*((uint8_t*)ctx->buf++) = *((char*)data++);
	}
	return count;
}

/*
 * MsgPack Encode the resourceconfiguration message
 * return: Buffer length
 * */
uint32_t cp_encoderesource_conf(struct resourceconf* data, uint8_t* buffer){
	cmp_ctx_t cmp;
	cmp_init(&cmp, buffer, buf_reader, buf_writer);

	cmp_write_u8(&cmp, data->id);
	cmp_write_u32(&cmp, data->resolution);
	cmp_write_u32(&cmp, data->version);
	cmp_write_u8(&cmp, data->flags);
	cmp_write_s32(&cmp, data->max_pollinterval);
	cmp_write_u8(&cmp, data->eventsActive);
	cmp_write_object(&cmp, &data->AboveEventAt);
	cmp_write_object(&cmp, &data->BelowEventAt);
	cmp_write_object(&cmp, &data->ChangeEvent);
	cmp_write_str(&cmp, data->unit, strlen(data->unit)+1);
	cmp_write_str(&cmp, data->spec, strlen(data->spec)+1);
	cmp_write_str(&cmp, data->type, strlen(data->type)+1);
	cmp_write_str(&cmp, data->group, strlen(data->group)+1);
	cmp_write_str(&cmp, data->attr, strlen(data->attr)+1);

	return (uint32_t)(((void*)cmp.buf) - ((void*)buffer));
}

/* Returns:
 *  0 for OK
 *  1 for err
 */
int cp_decoderesource_conf(struct resourceconf* data, uint8_t* buffer, char* strings){
	cmp_ctx_t cmp;
	cmp_init(&cmp, buffer, buf_reader, buf_writer);

	cmp_read_u8(&cmp, &data->id);
	cmp_read_u32(&cmp, &data->resolution);
	cmp_read_u32(&cmp, &data->version);
	cmp_read_u8(&cmp, &data->flags);
	cmp_read_s32(&cmp, &data->max_pollinterval);

	cmp_read_u8(&cmp, &data->eventsActive);
	cmp_read_object(&cmp, &data->AboveEventAt);
	cmp_read_object(&cmp, &data->BelowEventAt);
	cmp_read_object(&cmp, &data->ChangeEvent);

	//Put the strings in a mem location and point to them from the data structure
	data->unit = strings;
	uint32_t len = 100;
	cmp_read_str(&cmp, strings, &len);
	strings += len;	//Keep the \0

	data->spec = strings;
	len = 100;	//How long can we allow it to be
	cmp_read_str(&cmp, strings, &len);
	strings += len;	//Keep the \0

	data->type = strings;
	len = 100;	//How long can we allow it to be
	cmp_read_str(&cmp, strings, &len);

	/* The coap resource URL is made up from type/group.
	 * For now we only use these fields for the url, so lets combine those*/
	strings += len-1;
	*strings++ = '/';

	data->group = strings;
	cmp_read_str(&cmp, strings, &len);
	strings += len;	//Keep the \0

	data->attr = strings;
	len = 100;	//How long can we allow it to be
	cmp_read_str(&cmp, strings, &len);
	strings += len;	//Keep the \0

	return 1;
}

//Returns the length of the written data
uint32_t cp_encodeObject(uint8_t* buffer, cmp_object_t *obj){
	cmp_ctx_t cmp;
	cmp_init(&cmp, buffer, 0, buf_writer);
	cmp_write_object(&cmp, obj);
	return (uint32_t)((void*)cmp.buf - (void*)buffer);
}

/*
 * Read a cmp object
 * Param:
 * 	obj = result ID
 * 	len = How far did we read into the buffer
 *
 * */
int cp_decodeObject(uint8_t* buffer, cmp_object_t *obj, uint32_t* len){
	cmp_ctx_t cmp;
	cmp_init(&cmp, buffer, buf_reader, 0);
	if(cmp_read_object(&cmp, obj)){
		*len = (void*)cmp.buf - (void*)buffer;
		return 0;
	}

	return 1;
}

uint32_t cp_encodeU8(uint8_t* buffer, uint8_t val){
	cmp_ctx_t cmp;
	cmp_init(&cmp, buffer, 0, buf_writer);
	cmp_write_u8(&cmp, val);
	return (uint32_t)((void*)cmp.buf - (void*)buffer);
}

uint32_t cp_encodeU8Array(uint8_t* buffer, uint8_t* data, uint32_t size, uint32_t* len){
	cmp_ctx_t cmp;
	cmp_init(&cmp, buffer, 0, buf_writer);

	cmp_write_array(&cmp, size);
	for(int i=0; i<size; i++){
		cmp_write_u8(&cmp, *(data+i));
	}
	*len += (uint8_t*)cmp.buf - buffer;
	return 0;
}

uint32_t cp_encodeU16Array(uint8_t* buffer, uint16_t* data, uint32_t size, uint32_t* len){
	cmp_ctx_t cmp;
	cmp_init(&cmp, buffer, 0, buf_writer);

	cmp_write_array(&cmp, size);
	for(int i=0; i<8; i++){
		cmp_write_u16(&cmp, *(data+i));
	}
	*len += (uint8_t*)cmp.buf - buffer;
	return 0;
}


uint32_t cp_encodeString(uint8_t* buffer, char* str, uint32_t size, uint32_t* len){
	cmp_ctx_t cmp;
	cmp_init(&cmp, buffer, 0, buf_writer);

	cmp_write_str(&cmp, str, size);

	*len += (uint8_t*)cmp.buf - buffer;
	return 0;
}

/*
 * Read the Message ID
 * Param:
 * 	x = result ID
 * 	len = How far did we read into the buffer
 *
 * */
int cp_decodeU8(const uint8_t* buffer, uint8_t* x, uint32_t* len){
	cmp_ctx_t cmp;
	cmp_init(&cmp, (uint8_t*)buffer, buf_reader, 0);

	if(cmp_read_uchar(&cmp, x)){
		*len = (void*)cmp.buf - (void*)buffer;
		return 0;
	}

	return 1;
}

//Return 1 for error
//Return 0 for success
int cp_decodeS8Array(uint8_t* buffer, int8_t* arr, uint32_t* len){
	cmp_ctx_t cmp;
	cmp_init(&cmp, buffer, buf_reader, 0);
	uint32_t size;
	int ret = 0;

	if(cmp_read_array(&cmp, &size)){    //Returns the number of single bytes in the array
		while(size){
			if(cmp_read_s8(&cmp, arr++)){
				size -= 1;
			}
			else{
				ret = 1;
				break;
			}
		}
	}

	*len += (uint8_t*)cmp.buf - buffer;
	return ret;
}


//Return 1 for error
//Return 0 for success
int cp_decodeU16Array(uint8_t* buffer, uint16_t* arr, uint32_t* len){
	cmp_ctx_t cmp;
	cmp_init(&cmp, buffer, buf_reader, 0);
	uint32_t size;
	int ret = 0;

	if(cmp_read_array(&cmp, &size)){    //Returns the number of single bytes in the array
		while(size){
			if(cmp_read_u16(&cmp, arr++)){
				size -= 2;
			}
			else{
				ret = 1;
				break;
			}
		}
	}

	*len += (uint8_t*)cmp.buf - buffer;
	return ret;
}


//stringlen should contain the maximum available bytes in the buffer "string"
//The stringlen will contain the actual stringlen if parsing is a success
//len will be the index of where we read to
//Returns 0 if success
//Returns 1 if error
int cp_decode_string(uint8_t* buffer, char* string, uint32_t* stringlen, uint32_t* len){
	cmp_ctx_t cmp;
	cmp_init(&cmp, buffer, buf_reader, 0);
	int ret = 1;
	if(cmp_read_str(&cmp, string, stringlen)){
		ret = 0;
	}
	*len += (uint8_t*)cmp.buf - buffer;
	return ret;
}

/*
 * Convert an messagepack object to a char string
 * Parameter:
 * 	obj:	The object to convert
 * 	result: The resulting char buffer
 * 	len: 	String length without trailing '\0' (Note its not initialized - has to be done from the caller)
 *
 *  Returns:
 *  0 for OK
 *  1 for err
 *
 * */
int cp_cmp_to_string(cmp_object_t* obj, uint8_t* result, uint32_t* len){

	switch(obj->type){
	case CMP_TYPE_POSITIVE_FIXNUM:
	case CMP_TYPE_NIL:	// NULL = 0
	case CMP_TYPE_UINT8:
		*len += sprintf((char*)result, "%u", obj->as.u8);
		break;
	case CMP_TYPE_BOOLEAN:
		*len += sprintf((char*)result, "%u", obj->as.boolean);
		break;
	case CMP_TYPE_FLOAT:
	case CMP_TYPE_DOUBLE:
		*len += sprintf((char*)result, "%f", obj->as.dbl);
		break;
	case CMP_TYPE_UINT16:
		*len += sprintf((char*)result, "%u", obj->as.u16);
		break;
	case CMP_TYPE_UINT32:
		*len += sprintf((char*)result, "%lu", obj->as.u32);
		break;
	case CMP_TYPE_UINT64:
		*len += sprintf((char*)result, "%Lu", obj->as.u64);
		break;
	case CMP_TYPE_SINT8:
	case CMP_TYPE_NEGATIVE_FIXNUM:
		*len += sprintf((char*)result, "%d", obj->as.s8);
		break;
	case CMP_TYPE_SINT16:
		*len += sprintf((char*)result, "%d", obj->as.s16);
		break;
	case CMP_TYPE_SINT32:
		*len = sprintf((char*)result, "%ld", obj->as.s32);
		break;
	case CMP_TYPE_SINT64:
		*len += sprintf((char*)result, "%Ld", obj->as.s64);
		break;
	case CMP_TYPE_FIXMAP:
	case CMP_TYPE_FIXARRAY:
	case CMP_TYPE_FIXSTR:
	case CMP_TYPE_BIN8:
	case CMP_TYPE_BIN16:
	case CMP_TYPE_BIN32:
	case CMP_TYPE_EXT8:
	case CMP_TYPE_EXT16:
	case CMP_TYPE_EXT32:
	case CMP_TYPE_FIXEXT1:
	case CMP_TYPE_FIXEXT2:
	case CMP_TYPE_FIXEXT4:
	case CMP_TYPE_FIXEXT8:
	case CMP_TYPE_FIXEXT16:
	case CMP_TYPE_STR8:
	case CMP_TYPE_STR16:
	case CMP_TYPE_STR32:
	case CMP_TYPE_ARRAY16:
	case CMP_TYPE_ARRAY32:
	case CMP_TYPE_MAP16:
	case CMP_TYPE_MAP32:
		return 1;
	}
	return 0;
}

/* Convert the device readingspayload to a string.
 * Parameter:
 * 	buffer: Raw messagepacked buffer
 * 	conv: String result
 * 	len: String length without trailing '\0' (Note its not initialized - has to be done from the caller)
 * 	TODO: Consider changing the len, to how far we read the buffer instead.
 *
 * Returns:
 *  0 for OK
 *  1 for err
 */
int cp_convMsgPackToString(uint8_t* buffer, uint8_t* conv, uint32_t* len){
	cmp_ctx_t cmp;
	cmp_init(&cmp, buffer, buf_reader, 0);

	cmp_object_t obj;
	if(!cmp_read_object(&cmp, &obj)){
		return 1;
	}

	return cp_cmp_to_string(&obj, conv, len);
}

