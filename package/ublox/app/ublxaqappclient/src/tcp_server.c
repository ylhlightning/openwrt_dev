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
 * @file tcp_server.c
 * 
 * @brief ublx aqapp client application tcp server function.
 * 
 * @ingroup
 *
 * @author   Linhu Ying
 * @date     08/06/2015
 *
 ***********************************************************/


#include "tcp_server.h"

#include "error.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int createServerSocket(unsigned int tcpPort)
{
   struct sockaddr_in address = {0};
   
   const int serverHandle = socket(PF_INET, SOCK_STREAM, 0);
   
   if(serverHandle < 0) {
      error("Failed to create server socket");
   }
   
   address.sin_family = AF_INET;
   address.sin_port = htons(tcpPort);
   address.sin_addr.s_addr = htonl(INADDR_ANY);
   
   if(bind(serverHandle, (struct sockaddr*) &address, sizeof address) != 0) {
      error("bind() failed");
   }

   if(listen(serverHandle, SOMAXCONN) != 0) {
      error("listen() failed");
   }
   
   return serverHandle;
}

void disposeServerSocket(int serverSocket)
{
   (void) close(serverSocket);
}

