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

#define MAX_MESSAGE_SIZE 1024

/************************************************************
* Function declarations.
************************************************************/

static Handle acceptClientConnection(int serverHandle);

static Handle getClientSocket(void* instance);

static void handleReadEvent(void* instance);

static int ublx_msg_dispatcher(int src_socket_fd, ublx_client_msg_t *msg, int ublx_client_msg_len)
{
  int dst_socket_fd = 0, n = 0;
  char *ip_addr = "192.168.1.1";
  int port = 5001;
  struct sockaddr_in serv_addr;
    
  if((dst_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    printf("Error : Could not create socket\n");
    return 1;
  }
     
  memset(&serv_addr, '0', sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
   
  if(inet_pton(AF_INET, ip_addr, &serv_addr.sin_addr)<=0)
  {
    printf("inet_pton error occured\n");
    return 1;
  }

  if(connect(dst_socket_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    printf("Error : Connect Failed\n");
    return 1;
  }
    
  if(send(dst_socket_fd, msg, ublx_client_msg_len, 0) < 0)
  {
    printf("Error: Send messge error\n");
    return 1;
  }
  else
  {
    printf("Message sent to Server\n");
  }
    
  while((n = recv(dst_socket_fd, msg, ublx_client_msg_len,0)) > 0)
  { 
    printf("Received reply [%s] from Server\n", msg->ublx_client_reply_msg);
  }
    
  if(n < 0)
  {
    printf("Read error!\n");
    return 1;
  }

  close(dst_socket_fd);

  return 0;
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

      if(ublx_msg_dispatcher(client->clientSocket, &clientMessage, client_message_len) == 1)
      {
        printf("Server: Proxy client %d message fail\n", client->clientSocket);
        client->eventNotifier.onClientClosed(client->eventNotifier.server, client);
      }
      else
      {
        ssize_t sendResult = send(client->clientSocket, &clientMessage, client_message_len, 0);

        if(0 < sendResult)
        {
          printf("Server: Proxy client %d with reply message [%s]\n", client->clientSocket, clientMessage.ublx_client_reply_msg);
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

