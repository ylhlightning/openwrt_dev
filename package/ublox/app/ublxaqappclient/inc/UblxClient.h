#ifndef DIAGNOSTICS_CLIENT_H
#define DIAGNOSTICS_CLIENT_H

#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "Handle.h"
#include "ServerEventNotifier.h"

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
