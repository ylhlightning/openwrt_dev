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
 * @file ublx_client.c
 * 
 * @brief ublx aqapp client application server which proxy the client application request to ubus server.
 * 
 * @ingroup
 *
 * @author   Linhu Ying
 * @date     08/06/2015
 *
 ***********************************************************/


#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "ublx_client.h"
#include "ublx_server.h"
#include "event_handler.h"
#include "reactor.h"
#include "error.h"
#include "ublx_client_handler.h"

#include "libubus.h"
#include "lib320u.h"

struct DiagnosticsClient
{
   Handle clientSocket;
   EventHandler eventHandler;
   ServerEventNotifier eventNotifier;
}; 

typedef struct ubus_proc_handler
{
   int client_func;
   int (*proc_handler)(void);
} ubus_proc_handler_t;

#define MAX_MESSAGE_SIZE 1024

ubus_proc_handler_t ubus_proc_handler_table[] =
{
  {UBLX_WWAN_OPEN_CONNECTION, wwan_connection_open},
  {UBLX_WWAN_GET_ADDR,   wwan_get_addr}
};

int ubus_proc_handler_size = sizeof(ubus_proc_handler_table) / sizeof(struct ubus_proc_handler);

/************************************************************
* Function declarations.
************************************************************/
static int ubus_client_process_find(int client_func);

static Handle acceptClientConnection(int serverHandle);

static Handle getClientSocket(void* instance);

static void handleReadEvent(void* instance);


/************************************************************
* Implementation of the EventHandler interface.
************************************************************/
static int ubus_client_process_find(int client_func)
{
  int idx;
  for(idx = 0; idx < ubus_proc_handler_size; idx++)
  {
    if(ubus_proc_handler_table[idx].client_func == client_func)
    {
      return idx;
    }
  }

  printf("No ubus proc handler found.\n");
  return FALSE;
}


static Handle getClientSocket(void* instance)
{
   const DiagnosticsClientPtr client = instance;
   return client->clientSocket;
}

static void handleReadEvent(void* instance)
{
   const DiagnosticsClientPtr client = instance;
  
   ublx_client_msg_t clientMessage;

   int client_message_len = sizeof(clientMessage);

   int client_handler_idx;

   const ssize_t receiveResult = recv(client->clientSocket, &clientMessage, client_message_len, 0);
   
   if(0 < receiveResult) {
      /* In the real world, we would probably put a protocol on top of TCP/IP in 
      order to be able to restore the message out of the byte stream (it is no 
      guarantee that the complete message is received in one recv(). */
      
      printf("Client: received message from client: %d, request:%d\n", client->clientSocket, clientMessage.ublx_client_func);

      client_handler_idx = ubus_client_process_find(clientMessage.ublx_client_func) == FALSE;

      if(client_handler_idx == FALSE)
      {
        printf("Server: can not find client %d handler function\n", client->clientSocket);
        client->eventNotifier.onClientClosed(client->eventNotifier.server, client);
      }
      else
      {
        if(ubus_proc_handler_table[client_handler_idx].proc_handler() == FALSE)
        {
           printf("Server: Client handler execution failed.\n");
           strncpy(clientMessage.ublx_client_reply_msg, MSG_ERROR, strlen(MSG_ERROR));
        }
        else
        {
           printf("Server: Client handler execution success.\n");
           strncpy(clientMessage.ublx_client_reply_msg, MSG_OK, strlen(MSG_OK));
        }

        ssize_t sendResult = send(client->clientSocket, &clientMessage, client_message_len, 0);

        if(0 < sendResult)
        {
          printf("Server: Reply client %d with message success\n", client->clientSocket);
        }
        else
        {
          printf("Server: Reply client %d with message fail\n", client->clientSocket);
        }
      }

      client->eventNotifier.onClientClosed(client->eventNotifier.server, client);
   }
   
}

/************************************************************
* Implementation of the ADT functions of the client.
************************************************************/

/**
* Creates a representation of the client used to send diagnostic messages.
* The given handle must refer to a valid socket signalled for a pending connect request.
* The created client representation registers itself as the Reactor.
*/
DiagnosticsClientPtr createClient(Handle serverHandle, 
                                  const ServerEventNotifier* eventNotifier)
{
   DiagnosticsClientPtr client = malloc(sizeof *client);
   
   if(NULL != client) {
      client->clientSocket = acceptClientConnection(serverHandle);
      
      /* Successfully created -> register the client at the Reactor. */
      client->eventHandler.instance = client;
      client->eventHandler.getHandle = getClientSocket;
      client->eventHandler.handleEvent = handleReadEvent;

      Register(&client->eventHandler);
      
      assert(NULL != eventNotifier);
      client->eventNotifier = *eventNotifier;
   }
   
   return client;
}

/**
* Unregisters the given client at the Reactor and releases all associated resources.
* After completion of this function, the client-handle is invalid.
*/
void destroyClient(DiagnosticsClientPtr client)
{
   /* Before deleting the client we have to unregister at the Reactor. */
   Unregister(&client->eventHandler);
   
   (void) close(client->clientSocket);
   free(client);
}

/************************************************************
* Definition of the local utility function.
************************************************************/

static Handle acceptClientConnection(int serverHandle)
{
   struct sockaddr_in clientAddress = {0};
   
   socklen_t addressSize = sizeof clientAddress;

   const Handle clientHandle = accept(serverHandle, (struct sockaddr*) &clientAddress, &addressSize);
   
   if(0 > clientHandle) {
      /* NOTE: In the real world, this function should be more forgiving.
      For example, the client should be allowed to abort the connection request. */
      error("Failed to accept client connection");
   }
   
   (void) printf("Client: New connection created on IP-address %X\n", ntohl(clientAddress.sin_addr.s_addr));
   
   return clientHandle;
}



