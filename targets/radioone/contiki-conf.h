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

#ifndef CONTIKI_CONF_H_
#define CONTIKI_CONF_H_

#include <stdint.h>
#include <string.h>
/*---------------------------------------------------------------------------*/
/* Include Project Specific conf */
#ifdef PROJECT_CONF_PATH
#include PROJECT_CONF_PATH
#endif /* PROJECT_CONF_PATH */
/*---------------------------------------------------------------------------*/
#include "cc2538-def.h"
/*---------------------------------------------------------------------------*/
/**
 * \name Serial Boot Loader Backdoor configuration
 *
 * @{
 */
#ifndef FLASH_CCA_CONF_BOOTLDR_BACKDOOR
#define FLASH_CCA_CONF_BOOTLDR_BACKDOOR 1 /**<Enable the boot loader backdoor */
#endif

#ifndef FLASH_CCA_CONF_BOOTLDR_BACKDOOR_PORT_A_PIN
#define FLASH_CCA_CONF_BOOTLDR_BACKDOOR_PORT_A_PIN 6 /**< Pin PA6 (ON/SLEEP on the OpenBase), activates the boot loader */
#endif

#ifndef FLASH_CCA_CONF_BOOTLDR_BACKDOOR_ACTIVE_HIGH
#define FLASH_CCA_CONF_BOOTLDR_BACKDOOR_ACTIVE_HIGH 0 /**< A logic low level activates the boot loader */
#endif
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name CC2538 System Control configuration
 *
 * @{
 */
#ifndef SYS_CTRL_CONF_OSC32K_USE_XTAL
#define SYS_CTRL_CONF_OSC32K_USE_XTAL   1 /**< Use the on-board 32.768-kHz crystal */
#endif

#ifndef LPM_CONF_ENABLE
#define LPM_CONF_ENABLE       0 /**< Set to 0 to disable LPM entirely */
#endif

/** @} */
/*---------------------------------------------------------------------------*/
/* board.h assumes that basic configuration is done */
#include "board.h"
/*---------------------------------------------------------------------------*/
/* Include CPU-related configuration */
#include "cc2538-conf.h"
/*---------------------------------------------------------------------------*/
#endif /* CONTIKI_CONF_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
