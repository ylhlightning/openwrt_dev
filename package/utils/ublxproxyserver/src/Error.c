#include "Error.h"

#include <stdlib.h>
#include <stdio.h>

/**
* This function defines the error handling strategy.
* In this sample code, it simply prints the given error string and 
* terminates the program.
*/
void error(const char* reason)
{
   (void) printf("Fatal error: %s\n", reason);
   
   exit(EXIT_FAILURE);
}
