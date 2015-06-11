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
 * @file server_event_notifier.h
 * 
 * @brief ublx aqapp client application event handler header.
 * 
 * @ingroup
 *
 * @author   Linhu Ying
 * @date     08/06/2015
 *
 ***********************************************************/


#ifndef __SERVER_EVENT_NOTIFIER_H__
#define __SERVER_EVENT_NOTIFIER_H__

/**************************************************************
* Specifies callbacks from a client to its server.
**************************************************************/

/**
* This function is invoked as a callback in case a disconnect on 
* TCP level is detected.
*/
typedef void (*OnClientClosedFunc)(void* server,
                                   void* closedClient);

typedef struct
{
   /* An instance of the server owning the client.
      This instance shall be passed as an argument to the callbacks. */
   void* server;

   /* Specifies a callback to be used by the client to 
      inform its server about a closed connection. */   
   OnClientClosedFunc onClientClosed;
   
} ServerEventNotifier;

#endif
