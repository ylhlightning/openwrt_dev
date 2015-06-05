#include "UblxServer.h"
#include "ReactorEventLoop.h"

#include "Error.h"

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
