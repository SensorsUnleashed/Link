/*
#include <dev/deviceSetup.h>

 * defaultDeviceSetup.c
 *
 *  Created on: 2. apr. 2018
 *      Author: omn
 */
#include "cmp_helpers.h"
#include "cfs.h"
#include "cfs-coffee-arch.h"
#include "deviceSetup.h"

/* Store in flash */
settings_t default_relaysetting = {
		.eventsActive = AboveEventActive | BelowEventActive | ChangeEventActive,
		.AboveEventAt = {
				.type = CMP_TYPE_UINT8,
				.as.u8 = 1
		},
		.BelowEventAt = {
				.type = CMP_TYPE_UINT8,
				.as.u8 = 0
		},
		.ChangeEvent = {
				.type = CMP_TYPE_UINT8,
				.as.u8 = 1
		},
		.RangeMin = {
				.type = CMP_TYPE_UINT8,
				.as.u8 = 0
		},
		.RangeMax = {
				.type = CMP_TYPE_UINT8,
				.as.u8 = 1
		},
};

settings_t default_yellow_led_setting = {
		.eventsActive = ChangeEventActive,
		.AboveEventAt = {
				.type = CMP_TYPE_UINT8,
				.as.u8 = 1
		},
		.BelowEventAt = {
				.type = CMP_TYPE_UINT8,
				.as.u8 = 0
		},
		.ChangeEvent = {
				.type = CMP_TYPE_UINT8,
				.as.u8 = 1
		},
		.RangeMin = {
				.type = CMP_TYPE_UINT16,
				.as.u8 = 0
		},
		.RangeMax = {
				.type = CMP_TYPE_UINT16,
				.as.u8 = 1
		},
};

settings_t default_pulseCounter_settings = {
		.eventsActive = ChangeEventActive,
		.AboveEventAt = {
				.type = CMP_TYPE_UINT16,
				.as.u16 = 1000
		},
		.BelowEventAt = {
				.type = CMP_TYPE_UINT16,
				.as.u16 = 0
		},
		.ChangeEvent = {
				.type = CMP_TYPE_UINT16,
				.as.u16 = 500
		},
		.RangeMin = {
				.type = CMP_TYPE_UINT16,
				.as.u16 = 0
		},
		.RangeMax = {
				.type = CMP_TYPE_UINT16,
				.as.u16 = 60000
		},
};

settings_t default_mainsDetector_settings = {
		.eventsActive = AboveEventActive | BelowEventActive | ChangeEventActive,
		.AboveEventAt = {
				.type = CMP_TYPE_UINT8,
				.as.u8 = 1
		},
		.BelowEventAt = {
				.type = CMP_TYPE_UINT8,
				.as.u8 = 0
		},
		.ChangeEvent = {
				.type = CMP_TYPE_UINT8,
				.as.u8 = 1
		},
		.RangeMin = {
				.type = CMP_TYPE_UINT8,
				.as.u8 = 0
		},
		.RangeMax = {
				.type = CMP_TYPE_UINT8,
				.as.u8 = 1
		},
};

settings_t default_pushbutton_settings = {
		.eventsActive = BelowEventActive,
		.AboveEventAt = {
				.type = CMP_TYPE_UINT8,
				.as.u8 = 1
		},
		.BelowEventAt = {
				.type = CMP_TYPE_UINT8,
				.as.u8 = 0
		},
		.ChangeEvent = {
				.type = CMP_TYPE_UINT8,
				.as.u8 = 1
		},
		.RangeMin = {
				.type = CMP_TYPE_UINT8,
				.as.u8 = 0
		},
		.RangeMax = {
				.type = CMP_TYPE_UINT8,
				.as.u8 = 1
		},
};

settings_t default_timer_settings = {
		.eventsActive = AboveEventActive | BelowEventActive,
		.AboveEventAt = {
				.type = CMP_TYPE_UINT32,
				.as.u32 = 60
		},
		.BelowEventAt = {
				.type = CMP_TYPE_UINT32,
				.as.u32 = 2
		},
		.ChangeEvent = {
				.type = CMP_TYPE_UINT32,
				.as.u32 = 2
		},
		.RangeMin = {
				.type = CMP_TYPE_UINT32,
				.as.u32 = 1
		},
		.RangeMax = {
				.type = CMP_TYPE_UINT32,
				.as.u32 = 30000000,	//Seconds (347,2 days)
		},
};

/*
 * return
 * 		 0: Found a file in flash
 * 		 1: No file in flash
 * */
static int readSetup(cmp_ctx_t* cmp, settings_t* setup){
	int ret = 1;
	do{
		if(!cmp_read_u8(cmp, &setup->cfs_file_id)) break;
		if(!cmp_read_u8(cmp, &setup->eventsActive)) break;
		if(!cmp_read_object(cmp, &setup->AboveEventAt)) break;
		if(!cmp_read_object(cmp, &setup->BelowEventAt)) break;
		if(!cmp_read_object(cmp, &setup->ChangeEvent)) break;
		if(!cmp_read_object(cmp, &setup->RangeMax)) break;
		if(!cmp_read_object(cmp, &setup->RangeMin)) break;
		ret = 0;
	}while(0);
	return ret;
}

/*
 * return
 * 		 0: File written to flash
 * 		 1: Nothing written in flash
 * */
static int writeSetup(cmp_ctx_t* cmp, settings_t* setup, uint8_t newid){
	int ret = 1;
	do{
		if(!cmp_write_u8(cmp, newid)) break;
		if(!cmp_write_u8(cmp, setup->eventsActive)) break;
		if(!cmp_write_object(cmp, &setup->AboveEventAt)) break;
		if(!cmp_write_object(cmp, &setup->BelowEventAt)) break;
		if(!cmp_write_object(cmp, &setup->ChangeEvent)) break;
		if(!cmp_write_object(cmp, &setup->RangeMax)) break;
		if(!cmp_write_object(cmp, &setup->RangeMin)) break;
		ret = 0;
	}while(0);

	return ret;

}
/*
 * Get the setup from flash
 * If there has never been anything stored in flash,
 * use the default one.
 * The default can be change in the structs above
 *
 * There is always 2 versions of the file. If newest one
 * is used, but if it fails, it is deleted and we fail back
 * to the oldest one.
 *
 * FileA_ID	FileB_ID
 *	0		1					Use FileB
 *	5		4					Use FileA
 *	X		X					Use Default
 *
 *	When last id is 255 it will restart at 0. In that case, we now that the least number is largest
 * return
 * 		-1: Device name to long - bailing
 * 		 0: Found a file in flash and uses it
 * 		 1: No file in flash, use the default
 * */
int deviceSetupGet(const char* devicename, settings_t* setupA, settings_t* defaultsetting){

	settings_t setupB;
	struct file_s read;
	char filenameA[COFFEE_NAME_LENGTH];
	char filenameB[COFFEE_NAME_LENGTH];
	memset(filenameA, 0, COFFEE_NAME_LENGTH);
	memset(filenameB, 0, COFFEE_NAME_LENGTH);
	int retA = 1;
	int retB = 1;

	if(strlen(devicename) + 6 > COFFEE_NAME_LENGTH) return -1;
	sprintf(filenameA, "setupA_%s", devicename);
	sprintf(filenameB, "setupB_%s", devicename);

	//Read fileA if any
	read.fd = cfs_open(filenameA, CFS_READ);
	read.offset = 0;

	do{
		if(read.fd < 0) break;

		cmp_ctx_t cmp;
		cmp_init(&cmp, &read, file_reader, file_writer);

		retA = readSetup(&cmp, setupA);

		cfs_close(read.fd);
	}while(0);

	read.fd = cfs_open(filenameB, CFS_READ);
	read.offset = 0;

	//Read fileB if any
	do{
		if(read.fd < 0) break;

		cmp_ctx_t cmp;
		cmp_init(&cmp, &read, file_reader, file_writer);

		retB = readSetup(&cmp, &setupB);

		cfs_close(read.fd);
	}while(0);


	if(retA == 0 && retB == 0){
		if(setupA->cfs_file_id > setupB.cfs_file_id){
			if((setupA->cfs_file_id - setupB.cfs_file_id) > 100){	//Handle roll around (100 is just any number large enough)
				//In this case, B is actually the newest one
				memcpy(setupA, &setupB, sizeof(settings_t));
			}
			return 0;
		}
		else if(setupA->cfs_file_id < setupB.cfs_file_id){
			if((setupB.cfs_file_id - setupA->cfs_file_id) > 100){	//Handle roll around (100 is just any number large enough)
				//In this case, A is actually the newest one
				return 0;
			}
			memcpy(setupA, &setupB, sizeof(settings_t));
			return 0;
		}
	}
	else if(retA == 0){
		return 0;
	}
	else if(retB == 0){
		memcpy(setupA, &setupB, sizeof(settings_t));
		return 0;
	}
	else{	//Use default setting
		memcpy(setupA, defaultsetting, sizeof(settings_t));
	}

	return 1;
}

/*
 * Store the current setup. Delete the oldest
 * version of the setup file, so that there is
 * always 2 versions available
 * Return:
 * 		 0: Setting saved
 * 		-1: Unable to get a file descriptor - maybe out of space
 * 		-2: Unable to allocate enough space
 * 		-3: Unable to write to file
 * */
int deviceSetupSave(const char* devicename, settings_t* setup){
	settings_t setupX;
	struct file_s write;
	cmp_ctx_t cmp;
	char filenameA[COFFEE_NAME_LENGTH];
	char filenameB[COFFEE_NAME_LENGTH];
	memset(filenameA, 0, COFFEE_NAME_LENGTH);
	memset(filenameB, 0, COFFEE_NAME_LENGTH);
	int retA = 1;

	if(strlen(devicename) + 6 > COFFEE_NAME_LENGTH) return -1;
	sprintf(filenameA, "setupA_%s", devicename);
	sprintf(filenameB, "setupB_%s", devicename);

	write.fd = cfs_open(filenameA, CFS_READ | CFS_WRITE);
	write.offset = 0;

	do{
		if(write.fd < 0) break;

		cmp_init(&cmp, &write, file_reader, file_writer);

		retA = readSetup(&cmp, &setupX);

		cfs_close(write.fd);
	}while(0);

	if(retA == 0 && setupX.cfs_file_id == setup->cfs_file_id){
		//The current Active file is filenameA, erase and re-write filenameB
		cfs_remove(filenameB);

		write.fd = cfs_open(filenameB, CFS_READ | CFS_WRITE);
		write.offset = 0;
		if(cfs_coffee_reserve(filenameB, 30) != 0) return -1;	//The maximum length is 29 bytes, including overhead

		cmp_init(&cmp, &write, NULL, file_writer);
		if(writeSetup(&cmp, setup, ++setup->cfs_file_id) == 0){
			setup->cfs_file_id += 1;
			cfs_close(write.fd);
			return 0;
		}
		cfs_close(write.fd);
		return -3;
	}

	//The current Active file is filenameB, erase and re-write filenameA
	cfs_remove(filenameA);

	write.fd = cfs_open(filenameA, CFS_READ | CFS_WRITE);
	write.offset = 0;
	if(cfs_coffee_reserve(filenameA, 30) != 0) return -1;	//The maximum length is 29 bytes, including overhead

	cmp_init(&cmp, &write, NULL, file_writer);
	if(writeSetup(&cmp, setup, ++setup->cfs_file_id) == 0){
		setup->cfs_file_id += 1;
		cfs_close(write.fd);
		return 0;
	}
	cfs_close(write.fd);

	return 0;
}
