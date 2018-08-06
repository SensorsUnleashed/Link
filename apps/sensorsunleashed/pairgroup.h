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

#ifndef APPS_SENSORSUNLEASHED_PAIRGROUP_H_
#define APPS_SENSORSUNLEASHED_PAIRGROUP_H_

#include "contiki.h"
#include "pairing.h"

/*
 * To avoid having to parse through urls, group all
 *	sensors, that have the same pairs. This will consume
 *	little more memory, but will speed up the process
 *	of finding the right sensor pairs.
 *
 *	pairgroups_t contains a list of pairgroups
 *	a pairgroup all use the sanme pair from the same node
 */

struct pairgroup_s{
	struct pairgroup_s *next;
	uint8_t triggerindex;
	uint8_t paired;
	LIST_STRUCT(pairgroup);
};

struct pairgroupItem_s{
	struct pairgroupItem_s *next;
	joinpair_t* pair;
};
typedef struct pairgroup_s pairgroup_t;
typedef struct pairgroupItem_s pairgroupItem_t;


void pairGroupInit();
pairgroup_t* pairGroupAdd(joinpair_t* pair, enum su_basic_events event);
int pairGroupRemove(joinpair_t* pair, enum su_basic_events event);

#endif /* APPS_SENSORSUNLEASHED_PAIRGROUP_H_ */
