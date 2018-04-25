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

//static void
//busywait(uint32_t ms)
//{
//  uint32_t i;
//  /*  uint32_t n; */
//
//  while(ms--) {
//    for(i = 0; i < 3037; i++) {
//      __asm ("nop");
//    }
//  }
//}
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
		//butval = GPIO_READ_PIN(gpio_base, gpio_mask);
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
				//boot_image(botimg);
				active = botimg;
			}
			else{
				//boot_image(topimg);
				active = topimg;
			}
		}
		else if(botimg->version_minor != topimg->version_minor){
			if(botimg->version_minor > topimg->version_minor){
				//boot_image(botimg);
				active = botimg;
			}
			else{
				//boot_image(topimg);
				active = topimg;
			}
		}
		else if(botimg->version_dev != topimg->version_dev){
			if(botimg->version_dev > topimg->version_dev){
				//boot_image(botimg);
				active = botimg;
			}
			else{
				//boot_image(topimg);
				active = topimg;
			}
		}
		else if(*botdate != *topdate){
			if(*botdate > *topdate){
				//boot_image(botimg);
				active = botimg;
			}
			else{
				//boot_image(topimg);
				active = topimg;
			}
		}
		else{	//The images in both sectors are identical - use the bot one
			//boot_image(botimg);
			active = botimg;
		}
	}
	else if(botok){
		//boot_image(botimg);
		active = botimg;
	}
	else if(topok){
		//boot_image(topimg);
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
