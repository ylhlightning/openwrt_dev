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
 * @file event_handler.h
 * 
 * @brief ublx aqapp client application event handler header.
 * 
 * @ingroup
 *
 * @author   Linhu Ying
 * @date     08/06/2015
 *
 ***********************************************************/


#ifndef __EVENT_HANDLER_H__
#define __EVENT_HANDLER_H__

#include "handle.h"

/* All interaction from Reactor to an event handler goes 
through function pointers with the following signatures: */
typedef Handle (*getHandleFunc)(void* instance);
typedef void (*handleEventFunc)(void* instance);

typedef struct
{
  void* instance;
  getHandleFunc getHandle;
  handleEventFunc handleEvent;
} EventHandler;

#endif
