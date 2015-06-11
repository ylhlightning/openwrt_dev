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
 * @file main.c
 * 
 * @brief ublx aqapp client application server which proxy the ubus request to ubus server.
 * @ingroup
 *
 * @author   Linhu Ying
 * @date     08/06/2015
 *
 ***********************************************************/


#include "ublx_server.h"
#include "reactor_event_loop.h"

#include "error.h"

#include <stddef.h>


int main(void){
   
   /* Create a server and enter an eternal event loop, watching 
      the Reactor do the rest. */
   
   const unsigned int serverPort = 5000;
   DiagnosticsServerPtr server = createServer(serverPort);

   if(NULL == server) {
      error("Failed to create the server");
   }

   /* Enter the eternal reactive event loop. */
   for(;;){
      HandleEvents();
   }
}
