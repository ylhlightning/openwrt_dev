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
 * @file ublx_client.h
 * 
 * @brief ublx aqapp client application server client functions.
 * 
 * @ingroup
 *
 * @author   Linhu Ying
 * @date     08/06/2015
 *
 ***********************************************************/


#ifndef __UBLX_CLIENT_H__
#define __UBLX_CLIENT_H__

#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "handle.h"
#include "server_event_notifier.h"

#define UBLX_OPEN_CONNECTION 1
#define UBLX_CLIENT_MSG_LEN 1024

#define MSG_OK "OK"
#define MSG_ERROR "ERROR"

/* An opaque, incomplete type for the FIRST-CLASS ADT. */
typedef struct DiagnosticsClient* DiagnosticsClientPtr;

typedef struct ublx_client_msg
{
  unsigned int ublx_client_func;
  unsigned char ublx_client_data[UBLX_CLIENT_MSG_LEN];
  char ublx_client_reply_msg[UBLX_CLIENT_MSG_LEN];
} ublx_client_msg_t;

/**
* Creates a representation of the client used to send diagnostic messages.
* The given handle must refer to a valid socket signalled for a pending connect request.
*/
DiagnosticsClientPtr createClient(Handle serverHandle, 
                                  const ServerEventNotifier* eventNotifier);

/**
* Unregisters the given client at the Reactor and releases all associated resources.
* After completion of this function, the client-handle is invalid.
*/
void destroyClient(DiagnosticsClientPtr client);

#endif
