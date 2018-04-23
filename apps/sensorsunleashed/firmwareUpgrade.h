/*
 * firmwareUpgrade.h
 *
 *  Created on: 7. apr. 2018
 *      Author: omn
 */

#ifndef APPS_SENSORSUNLEASHED_FIRMWAREUPGRADE_H_
#define APPS_SENSORSUNLEASHED_FIRMWAREUPGRADE_H_

#include "contiki.h"

#define TOPIMG_START	0x21D000
#define BOTIMG_START	0x204000
#define DEBUG_START		0x202000
#define IMG_SIZE		0x19000	//100kB

#define TOPIMG_HEADER 	0x235FE4
#define BOTIMG_HEADER	0x21CFE4

#define CRC32_MAGIC_REMAINDER   0x2144DF1C
/*
 * headerversion:
 * 	How should this header be parsed.
 * 	Currently its version 1, but if it changes in the future
 * 	we can.
 * product_type:
 * 	For now we have:
 * 	1 = debugdevice
 * 	2 = mainsdetector
 * 	3 = relayboard
 * radiotarget
 * 	1 = su-cc2538-1
 * 	2 = radioone
 * version_major, minor, dev
 * 	The software version for this device
 * startaddress:
 * 	This is where in flash this image is meant to be put
 * 	used for the setup tool to determine if the image is
 * 	correct before appling it.
 * creation_date_epoch
 * 	The time for creating this image. Measured in ms since 1970-01-01 00:00:00
 * valid:
 * 	Set by the bootloader, if it detects the image is wrong.
 * 	0xFF = Never checked.
 * 	0x7F =
 * CRC = the CRC of all data from start to end - CRC
 * */

struct firmwareInf{
	uint8_t headerversion;			//0
	uint8_t reserved1;
	uint16_t product_type;
	uint8_t creation_date_epoch[8]; 	//1+2 Needs to be 8 byte aligned
	uint8_t radio_target;			//3
	uint8_t version_major;
	uint8_t version_minor;
	uint8_t version_dev;
	uint32_t startaddress;			//4
	uint32_t reserved2;				//5
	uint32_t CRC;					//6
};

int fwUpgradeAddChunk(const uint8_t* data, uint32_t len, uint32_t num, uint32_t offset);
int fwUpgradeDone();
uint32_t getTrailAddr(int active);
uint8_t getActiveSlot();

#endif /* APPS_SENSORSUNLEASHED_FIRMWAREUPGRADE_H_ */
