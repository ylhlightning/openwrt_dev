/*********************************************************** 
 * 
 * Copyright (C) u-blox Italy S.p.A.
 * 
 * u-blox Italy S.p.A.
 * Via Stazione di Prosecco 15
 * 34010 Sgonico - TRIESTE, ITALY
 * 
 * All rights reserved.
 * 
 * This source file is the sole property of
 * u-blox Italy S.p.A. Reproduction or utilization of
 * this source in whole or part is forbidden
 * without the written consent of u-blox Italy S.p.A.
 * 
 ******************************************************************************/
/** 
 * 
 * @file reactor.h
 * 
 * @brief ublx aqapp client application server reactor haader.
 * 
 * @ingroup
 *
 * @author   Linhu Ying
 * @date     08/06/2015
 *
 ***********************************************************/



#ifndef __REACTOR_H__
#define __REACTOR_H__

#include "event_handler.h"
#include "thpool.h"

extern threadpool thpool;
void Register(EventHandler* handler);
void Unregister(EventHandler* handler);

#endif
