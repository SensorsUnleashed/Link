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
#undef PROJECT_CONF_H_	//TODO: Workaround to make eclipse show the defines right
#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

#define SU_VER_MAJOR	0	//Is increased when there has been changes to the protocol, which is not backwards compatible
#define SU_VER_MINOR	0	//Is increased if the protocol is changed, but still backwards compatible
#define SU_VER_DEV		1	//Is increased for every minor fix

#define LOG_CONF_LEVEL_MAC  		LOG_LEVEL_DBG
//#define LOG_CONF_LEVEL_TCPIP		LOG_LEVEL_INFO
#define LOG_CONF_LEVEL_IPV6			LOG_LEVEL_INFO
#define LOG_CONF_LEVEL_FRAMER		LOG_LEVEL_INFO
//#define LOG_CONF_LEVEL_6LOWPAN		LOG_LEVEL_INFO

#define WATCHDOG_CONF_ENABLE	0

//#define UART_CONF_ENABLE	0

//Enable uart receive
#define UART0_CONF_WITH_INPUT	1

/* Pre-allocated memory for loadable modules heap space (in bytes)*/
#define MMEM_CONF_SIZE 2048

/* Enable client-side support for COAP observe */
#define COAP_OBSERVE_CLIENT 1

/* The number of concurrent messages that can be stored for retransmission in the transaction layer. */
#define COAP_MAX_OPEN_TRANSACTIONS     10

/* Maximum number of failed request attempts before action */
#define COAP_MAX_ATTEMPTS              3

#define DBG_CONF_USB 1 /** All debugging over UART by default */


#define HAS_MAINSDETECT		1
#define HAS_PULSECOUNTER	1

#endif /* __PROJECT_ERBIUM_CONF_H__ */
