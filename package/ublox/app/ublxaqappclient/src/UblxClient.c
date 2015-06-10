/*********************************************************************************
*
* The server side representation of a client sending diagnostic messages to the log.
*
* An instance of this client ADT is created as the server detects a connect request.
* Upon creation, the instance registers itself at the Reactor. The Reactor will notify 
* this client representation as an incoming diagnostics message is pending. 
* This client representation simply reads the message and prints it to stdout.
**********************************************************************************/

#include "UblxClient.h"
#include "UblxServer.h"
#include "EventHandler.h"
#include "Reactor.h"
#include "Error.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

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

/************************************************************
* Function declarations.
************************************************************/

static Handle acceptClientConnection(int serverHandle);

static Handle getClientSocket(void* instance);

static void handleReadEvent(void* instance);

static int wwan_connection_open(void);


static struct ubus_context *ctx;
static struct blob_buf b;
static bool simple_output = false;

static ubus_proc_handler_t ubus_proc_handler_table[] = 
{
  {UBLX_OPEN_CONNECTION, wwan_connection_open}
};

static int ubus_proc_handler_size = sizeof(ubus_proc_handler_table) / sizeof(struct ubus_proc_handler);

/************************************************************
* Function implementations.
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
  return 0;
}

static void test_client_subscribe_cb(struct ubus_context *ctx, struct ubus_object *obj)
{
  printf("Subscribers active: %d\n", obj->has_subscribers);
}

static struct ubus_object test_client_object = {
  .subscribe_cb = test_client_subscribe_cb,
};

static void receive_call_result_data(struct ubus_request *req, int type, struct blob_attr *msg)
{
  char *str;
  if (!msg)
  {
    return;
  }
  str = blobmsg_format_json_with_cb(msg, true, NULL, NULL, simple_output ? -1 : 0);
  
  printf("%s\n", str);
  free(str);
}

static int client_ubus_process(char *ubus_act)
{
  static struct ubus_request req;
  uint32_t id;
  int ret;
  const char *ubus_socket = NULL;

  ctx = ubus_connect(ubus_socket);
  if (!ctx) {
    printf("Failed to connect to ubus\n");
    return FALSE;
  }

  ret = ubus_add_object(ctx, &test_client_object);
  if (ret) {
    printf("Failed to add_object object: %s\n", ubus_strerror(ret));
    return FALSE;
  }

  if (ubus_lookup_id(ctx, ubus_act, &id)) {
    printf("Failed to look up test object\n");
    return FALSE;
  }

  blob_buf_init(&b, 0);
  blobmsg_add_u32(&b, "id", test_client_object.id);

  if(ubus_invoke(ctx, id, "enable", b.head, receive_call_result_data, 0, 3000) == 0)
  {
    ret = TRUE;
  }
  else
  {
    ret = FALSE;
  }

  ubus_free(ctx);
  return ret;
}

static int wwan_connection_open(void)
{
  int ret;
  ret = client_ubus_process("enable");
  if(ret == FALSE)
    return FALSE;
  
  ret = client_ubus_process("connect");
  if(ret == FALSE)
    return FALSE;

  return TRUE;
}

/************************************************************
* Implementation of the EventHandler interface.
************************************************************/

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

   const ssize_t receiveResult = recv(client->clientSocket, &clientMessage, client_message_len, 0);
   
   if(0 < receiveResult) {
      /* In the real world, we would probably put a protocol on top of TCP/IP in 
      order to be able to restore the message out of the byte stream (it is no 
      guarantee that the complete message is received in one recv(). */
      
      printf("Client: received message from client: %d, request:%d\n", client->clientSocket, clientMessage.ublx_client_func);

      if(ubus_client_process_find(clientMessage.ublx_client_func) == FALSE)
      {
        printf("Server: can not find client %d handler function\n", client->clientSocket);
        client->eventNotifier.onClientClosed(client->eventNotifier.server, client);
      }
      else
      {
        ssize_t sendResult = send(client->clientSocket, &clientMessage, client_message_len, 0);

        if(0 < sendResult)
        {
          strncpy(clientMessage.ublx_client_reply_msg, MSG_OK, strlen(MSG_OK));
          printf("Server: Reply client %d with reply message [%s]\n", client->clientSocket, clientMessage.ublx_client_reply_msg);
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



