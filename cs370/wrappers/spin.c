 /***********************************************************
  *
  *   Program:    spin.c
  *   Created:    08/14/2013 9:36AM
  *   Author:     Ken Fox
  *   Requested:
  *   Comments:   load generator
  *
  ************************************************************
  *
  * History:
  *
  * $Log:$
  *
  ************************************************************/

#include <stdio.h>
#include <stdlib.h>
#define INTERVAL 800000000

int main (int argc, char *args[]) {
	int counter = 0;
	while(1) {
		counter++;
		if ( counter % INTERVAL == 0 ) {
			printf ( "PID: %d: counter: %d\n", getpid(), counter);
		}
	}
exit(0);
}

