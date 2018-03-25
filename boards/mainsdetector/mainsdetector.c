/*
 * relayboard.c
 *
 *  Created on: 30. jan. 2018
 *      Author: omn
 */
#include "contiki.h"
#include "dev/leds.h"
#include "susensors.h"
#include "susensorcommon.h"
#include "mainsdetect.h"
#include "resources/res-susensors.h"
#include "cfs-coffee-arch.h"
#include "rpl.h"

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

PROCESS(device_process, "SU devicehandler");
AUTOSTART_PROCESSES(&device_process);

struct ledRuntime led_yellow = { LEDS_YELLOW };
extern  resource_t  res_sysinfo;

process_event_t systemchange;

PROCESS_THREAD(device_process, ev, data)
{
	static struct etimer et;

	PROCESS_BEGIN();

	initSUSensors();

	susensors_sensor_t* d;
	d = addASUMainsDetector(MAINSDETECT_ACTUATOR, &mainsdetectconfig);
	if(d != NULL){
		setResource(d, res_susensor_activate(d));
	}

	rest_activate_resource(&res_sysinfo, "su/nodeinfo");
	systemchange = process_alloc_event();

	//Wait for the routing table to be ready
	leds_on(LEDS_RED);
	while(1){
		etimer_set(&et, CLOCK_SECOND);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
		leds_toggle(LEDS_RED);

		if(rpl_is_reachable()){
			break;
		}
	}

	process_start(&susensors_process, NULL);

	leds_on(LEDS_RED);

	while(1) {
		PROCESS_YIELD();

		if(ev == systemchange){
			cfs_coffee_format();
		}
	}
	PROCESS_END();
}

