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
#include "contiki.h"
#include "rest-engine.h"
#include "coap-engine.h"
#include "dev/leds.h"
#include "dev/pulsesensor.h"
#include <stdlib.h>

#include "dev/button-sensor.h"
#include "dev/relay.h"
#include "dev/ledindicator.h"
#include "dev/mainsdetect.h"
#include "../sensorsUnleashed/pairing.h"
#include "resources/res-susensors.h"
#include "dev/susensorcommon.h"

#include "lib/sensors.h"
#include "cfs-coffee-arch.h"
#include "rpl.h"
#include "board.h"

extern process_event_t sensors_event;
process_event_t systemchange;


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

struct ledRuntime led_red = { LEDS_RED };
struct ledRuntime led_green = { LEDS_GREEN };
struct ledRuntime led_orange = { LEDS_ORANGE };
struct ledRuntime led_yellow = { LEDS_YELLOW };

/*
 * Resources to be activated need to be imported through the extern keyword.
 * The build system automatically compiles the resources in the corresponding sub-directory.
 */
extern  resource_t  res_sysinfo;

PROCESS(er_uart_server, "Erbium Uart Server");
AUTOSTART_PROCESSES(&er_uart_server);

void
collect_common_net_print(void)
{
  rpl_dag_t *dag;
  uip_ds6_route_t *r;

  /* Let's suppose we have only one instance */
  dag = rpl_get_any_dag();
  if(dag->preferred_parent != NULL) {
    PRINTF("Preferred parent: ");
    //PRINT6ADDR(rpl_get_parent_ipaddr(dag->preferred_parent));
    PRINTF("\n");
  }
  for(r = uip_ds6_route_head();
      r != NULL;
      r = uip_ds6_route_next(r)) {
    PRINT6ADDR(&r->ipaddr);
  }
  PRINTF("---\n");
}

PROCESS_THREAD(er_uart_server, ev, data)
{
	static struct etimer et;

	PROCESS_BEGIN();

	//Init dynamic memory	(Default 4096Kb)
	mmem_init();

	initSUSensors();

	/* Initialize the REST engine. */
	rest_init_engine();
	coap_init_engine();

	susensors_sensor_t* d;
	d = addASURelay(RELAY_ACTUATOR, &relayconfigs);
	if(d != NULL) {
		setResource(d, res_susensor_activate(d));
	}
//	d = addASULedIndicator("su/led_red", &ledindicatorconfig, &led_red);
//	if(d != NULL){
//		setResource(d, res_susensor_activate(d));
//	}
//	d = addASULedIndicator("su/led_green", &ledindicatorconfig, &led_green);
//	if(d != NULL){
//		setResource(d, res_susensor_activate(d));
//	}
//	d = addASULedIndicator("su/led_orange", &ledindicatorconfig, &led_orange);
//	if(d != NULL){
//		setResource(d, res_susensor_activate(d));
//	}
	d = addASULedIndicator("su/led_yellow", &ledindicatorconfig, &led_yellow);
	if(d != NULL){
		setResource(d, res_susensor_activate(d));
	}
	d = addASUPulseInputRelay(PULSE_SENSOR, &pulseconfig);
	if(d != NULL){
		setResource(d, res_susensor_activate(d));
	}
	d = addASUMainsDetector(MAINSDETECT_ACTUATOR, &mainsdetectconfig);
	if(d != NULL){
		setResource(d, res_susensor_activate(d));
	}
	d = addASUButtonSensor(BUTTON_SENSOR, &pushbuttonconfig);
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

		if(ev == sensors_event){	//Button was pressed
			PRINTF("Button press\n");
		}
		else if(ev == button_press_duration_exceeded){
			if(*((uint8_t*)data) == 1){
				PRINTF("Button long-press - 1 sec \n");

				collect_common_net_print();

			}
			else if(*((uint8_t*)data) == 5){
				PRINTF("Button long-press - 5 sec \n");
			}

		}
		else if(ev == systemchange){
			cfs_coffee_format();
		}
	}
	PROCESS_END();
}
