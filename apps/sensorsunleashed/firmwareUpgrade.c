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

#include "firmwareUpgrade.h"
#include "sparrow-flash.h"
#include "dev/flash.h"
#include "dev/rom-util.h"

static uint32_t checkstartaddr = 0;

/*
 * Return 0: flash was cleared
 * Return 1: flash was unable to be cleared
 * */
static int clearFlash(uint32_t start_address, uint32_t len){
	for(uint32_t i=0; i<len; i+=FLASH_PAGE_SIZE){
		if(sparrow_flash_erase_sector(start_address+i) != SPARROW_FLASH_COMPLETE){
			return 1;
		}
	}
	return 0;
}

/*
 * Return 0 on sucess
 * Return >0 on fail
 * 	1: Not able to detect current flash location
 * */
static int prepareFlash(){
	//First find the flash area we're in now.
	volatile uint32_t* vto = ((volatile uint32_t *)(0xe000ed08));

	if(*vto == TOPIMG_START){
		checkstartaddr = BOTIMG_START;
		return clearFlash(BOTIMG_START, IMG_SIZE);
	}
	else if(*vto == BOTIMG_START || *vto == DEBUG_START){
		checkstartaddr = TOPIMG_START;
		return clearFlash(TOPIMG_START, IMG_SIZE);
	}

	return 1;
}


/*
 * Fill a buffer, and when filled write it to EEPROM,
 * in case its not full, just wait for the next chunk
 * In case our payload is 32 bytes, it will only write
 * to EEPROM every second time.
 * Return 0: on success
 * return 1: Not able to prepare flash
 * return 2: Not able to write data to flash
 * */
int fwUpgradeAddChunk(const uint8_t* data, uint32_t len, uint32_t num, uint32_t offset){

	if(num == 0) {
		if(prepareFlash() != 0) return 1;
	}

	uint32_t startaddr = checkstartaddr + offset;

	/* Write to flash */
	for(int i=0; i<len; i+=4){
		if(sparrow_flash_program_word(startaddr, *((uint32_t*)(data+i))) != SPARROW_FLASH_COMPLETE) return 2;
		startaddr += 4;
	}
	return 0;
}

//Return 0 on success
uint8_t check_crc32(unsigned char *buff, int len)
{
	return CRC32_MAGIC_REMAINDER != rom_util_crc32(buff, len);
}

//Return 0 on success
//Return 1 if not
int fwUpgradeDone(){
	return check_crc32((uint8_t*)checkstartaddr, IMG_SIZE);
}

//Return 1 = TOP, 0 = Bot

uint8_t getActiveSlot(){
	volatile uint32_t* vto = ((volatile uint32_t *)(0xe000ed08));

	if(*vto == TOPIMG_START){
		return 1;
	}
	return 0;
}
/*
 * Active is 1 if its the active trail
 * Return the trailaddr
 *  0: If there is none or the one there is is not valid
 * 	Address if found
 * 	If its the active slot that is requested, we already
 * 	know that the trailer is ok - unless of cause its a
 * 	debug image
 */
uint32_t getTrailAddr(int active){
	volatile uint32_t* vto = ((volatile uint32_t *)(0xe000ed08));
	uint32_t addr = 0;
	uint32_t trailaddr = 0;
	if(*vto == TOPIMG_START){
		if(active) return TOPIMG_HEADER;
		addr = BOTIMG_START;
		trailaddr = BOTIMG_HEADER;
	}
	else if(*vto == BOTIMG_START){
		if(active) return BOTIMG_HEADER;
		addr = TOPIMG_START;
		trailaddr = TOPIMG_HEADER;
	}
	else if(*vto == DEBUG_START){
		if(active) return 0;
		addr = TOPIMG_START;
		trailaddr = TOPIMG_HEADER;
	}

	if(check_crc32((uint8_t*)addr, IMG_SIZE) == 0) return trailaddr;

	return 0;
}
