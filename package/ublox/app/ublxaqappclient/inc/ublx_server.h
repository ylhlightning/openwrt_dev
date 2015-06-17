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
 * @file ublx_server.h
 * 
 * @brief ublx aqapp client application server.
 * 
 * @ingroup
 *
 * @author   Linhu Ying
 * @date     08/06/2015
 *
 ***********************************************************/


#ifndef __UBLX_SERVER_H__
#define __UBLX_SERVER_H__

#include "thpool.h"

/* An opaque, incomplete type for the FIRST-CLASS ADT. */
typedef struct DiagnosticsServer* DiagnosticsServerPtr;

threadpool thpool;

/**
* Creates a server listening for connect requests on the given port.
* The server registers itself at the Reactor upon creation.
*/
DiagnosticsServerPtr createServer(unsigned int tcpPort);

/**
* Unregisters at the Reactor and deletes all connected clients 
* before the server itself is disposed.
* After completion of this function, the server-handle is invalid.
*/
void destroyServer(DiagnosticsServerPtr server);

#endif
