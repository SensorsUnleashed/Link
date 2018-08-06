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

#include "bootloader.h"
#include "dev/rom-util.h"
#include "dev/gpio.h"
#include "dev/ioc.h"
#include "dev/sys-ctrl.h"
#include "../apps/sensorsunleashed/firmwareUpgrade.h"

void __set_MSP(uint32_t topOfMainStack) __attribute__((naked));
void __set_MSP(uint32_t topOfMainStack)
{
	__asm volatile ("MSR msp, %0\n\t"
			"BX  lr     \n\t" : : "r" (topOfMainStack));
}

/**
 * Return non-zero if button is congured and pressed.
 */
static int
is_button_pressed(uint8_t radioid)
{
	volatile uint32_t gpio_base = 0;
	volatile uint32_t gpio_mask;
	volatile int hasbutton = 0;
	volatile int butval = 0;
	volatile uint8_t port, pin;
	static uint8_t init = 0;

	if(radioid == 1){
		port = GPIO_C_NUM;
		pin = 1;
		hasbutton = 1;
	}
	else if(radioid == 2){
		port = GPIO_D_NUM;
		pin = 0;
		hasbutton = 1;
	}

	if(hasbutton){
		gpio_base = GPIO_PORT_TO_BASE(port);
		gpio_mask = (1 << pin);

		if(!init){
			ioc_init();
			GPIO_SOFTWARE_CONTROL(gpio_base, gpio_mask);
			GPIO_SET_INPUT(gpio_base, gpio_mask);
			/* Disable pull-ups */
			ioc_set_over(port, pin, IOC_OVERRIDE_DIS);
			init = 1;
		}
		butval = GPIO_READ_PIN(gpio_base, gpio_mask);
	}

	return (butval & gpio_mask) == 0;
}

static void boot_image(volatile struct firmwareInf *img)
{
	typedef void (*entry_point_t)(void);

	entry_point_t entry_point =
			(entry_point_t)(*(uint32_t *)(img->startaddress + 4));
	uint32_t stack = *(uint32_t *)(img->startaddress);

	//Set Vector Table offset
	*((volatile uint32_t *)(0xe000ed08)) = img->startaddress;
	__set_MSP(stack);

	entry_point();
	//  for(;;) {
	//    flash_led(20);
	//    busywait(800);
	//  }
}

//Return 0 on success
static uint8_t
check_crc32(unsigned char *buff, int len)
{
	return CRC32_MAGIC_REMAINDER != rom_util_crc32(buff, len);
}

int main(void){

	//First read the header of both images
	volatile struct firmwareInf *topimg, *botimg, *active;
	topimg = (volatile struct firmwareInf*)(TOPIMG_HEADER);
	botimg = (volatile struct firmwareInf*)(BOTIMG_HEADER);
	active = 0;
	uint64_t* topdate = (uint64_t*)&topimg->creation_date_epoch[0];
	uint64_t* botdate = (uint64_t*)&botimg->creation_date_epoch[0];

	int topok = 0;
	int botok = 0;

	if(topimg->startaddress == TOPIMG_START){
		if(check_crc32((uint8_t*)topimg->startaddress, IMG_SIZE) == 0){
			topok = 1;
		}
	}

	if(botimg->startaddress == BOTIMG_START){
		if(check_crc32((uint8_t*)botimg->startaddress, IMG_SIZE) == 0){
			botok = 1;
		}
	}

	/* Start the most recent firmware.
	 * Priority:
	 * 	1. The highest version is preferred.
	 * 	2. The newest is preferred
	 * 	*/
	if(botok && topok){
		//Both is present, boot the newest one
		if(botimg->version_major != topimg->version_major){
			if(botimg->version_major > topimg->version_major){
				active = botimg;
			}
			else{
				active = topimg;
			}
		}
		else if(botimg->version_minor != topimg->version_minor){
			if(botimg->version_minor > topimg->version_minor){
				active = botimg;
			}
			else{
				active = topimg;
			}
		}
		else if(botimg->version_dev != topimg->version_dev){
			if(botimg->version_dev > topimg->version_dev){
				active = botimg;
			}
			else{
				active = topimg;
			}
		}
		else if(*botdate != *topdate){
			if(*botdate > *topdate){
				active = botimg;
			}
			else{
				active = topimg;
			}
		}
		else{	//The images in both sectors are identical - use the bot one
			active = botimg;
		}
	}
	else if(botok){
		active = botimg;
	}
	else if(topok){
		active = topimg;
	}

	if(is_button_pressed(active->radio_target) && topok && botok){
		active = active == topimg ? botimg : topimg;
	}
	boot_image(active);

	//Perhaps we should flash a few times to indicate failure
	while(1);

	return 0;
}
