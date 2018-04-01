#ifndef SUSYSTEM_CONF_H_
#define SUSYSTEM_CONF_H_

//The default amount of bytes allocated when a new file is created
//Every device creates 1 file for its pairs
#define COFFEE_CONF_DYN_SIZE	400

#define WATCHDOG_CONF_ENABLE	0

//#define UART_CONF_ENABLE	0

//Enable uart receive
#define UART0_CONF_WITH_INPUT	1

/* Pre-allocated memory for loadable modules heap space (in bytes)*/
#define MMEM_CONF_SIZE 512

/* Enable client-side support for COAP observe */
#define COAP_OBSERVE_CLIENT 1

/* The number of concurrent messages that can be stored for retransmission in the transaction layer. */
#define COAP_MAX_OPEN_TRANSACTIONS     10

#define COAP_CONF_MAX_OBSERVEES			6

/* Maximum number of failed request attempts before action */
#define COAP_MAX_ATTEMPTS              3

#define DBG_CONF_USB 1 /** All debugging over UART by default */

#endif
