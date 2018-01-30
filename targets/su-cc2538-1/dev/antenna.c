/*
 * Copyright (c) 2014, Thingsquare, http://www.thingsquare.com/.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */
/*---------------------------------------------------------------------------*/
/**
 * \addtogroup openmote-antenna
 * @{
 *
 * Driver for the OpenMote-CC2538 RF switch.
 * INT is the internal antenna (chip) configured through ANT1_SEL (V1)
 * EXT is the external antenna (connector) configured through ANT2_SEL (V2)
 * @{
 *
 * \file
 * Driver implementation for the OpenMote-CC2538 antenna switch
 */
/*---------------------------------------------------------------------------*/
#include "contiki-conf.h"
#include "dev/gpio.h"
#include "dev/antenna.h"

#include "dev/rfcore-xreg.h"
#include "dev/cctest.h"
/*---------------------------------------------------------------------------*/
#define BSP_RADIO_BASE              GPIO_PORT_TO_BASE(GPIO_C_NUM)
#define BSP_RADIO_PA_EN				GPIO_PIN_MASK(3)
#define BSP_RADIO_LNA_EN			GPIO_PIN_MASK(2)

#define BSP_RADIO_GAIN_BASE			GPIO_PORT_TO_BASE(GPIO_D_NUM)
#define BSP_RADIO_HGM				GPIO_PIN_MASK(2)


/* RF observable control register value to output PA signal */
#define RFC_OBS_CTRL_PA_PD_INV        0x6A

/* RF observable control register value to output LNA signal */
#define RFC_OBS_CTRL_LNAMIX_PD_INV    0x68

/* RF observable control register value to output LNA signal
 *  * for CC2591 compression workaround.
 */
#define RFC_OBS_CTRL_DEMOD_CCA        0x0D

/* OBSSELn register value to select RF observable 0 */
#define OBSSEL_OBS_CTRL0             0x81

/* OBSSELn register value to select RF observable 1 */
#define OBSSEL_OBS_CTRL1             0x80


#define RFC_OBS_CTRL0	REG(RFCORE_XREG_RFC_OBS_CTRL0)
#define RFC_OBS_CTRL1	REG(RFCORE_XREG_RFC_OBS_CTRL1)
#define OBSSEL3		REG(CCTEST_OBSSEL3)
#define OBSSEL2		REG(CCTEST_OBSSEL2)

/*---------------------------------------------------------------------------*/
void
antenna_init(void)
{

	GPIO_SET_OUTPUT(BSP_RADIO_BASE, BSP_RADIO_PA_EN);
	GPIO_SET_OUTPUT(BSP_RADIO_BASE, BSP_RADIO_LNA_EN);
	GPIO_SET_OUTPUT(BSP_RADIO_GAIN_BASE, BSP_RADIO_HGM);

	/* Set PA, LNA and set High Gain Mode */
	GPIO_WRITE_PIN(BSP_RADIO_BASE, BSP_RADIO_PA_EN, 1);
	GPIO_WRITE_PIN(BSP_RADIO_BASE, BSP_RADIO_LNA_EN, 1);
	GPIO_WRITE_PIN(BSP_RADIO_GAIN_BASE, BSP_RADIO_HGM, REC_HIGH_GAIN_MODE_EN == 0 ? 0 : 1);

	RFC_OBS_CTRL0 = RFC_OBS_CTRL_PA_PD_INV;
	OBSSEL3 = OBSSEL_OBS_CTRL0;

	// PC2 -> EN (LNA control)
	RFC_OBS_CTRL1 = RFC_OBS_CTRL_LNAMIX_PD_INV;
	OBSSEL2 = OBSSEL_OBS_CTRL1;
}

/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
