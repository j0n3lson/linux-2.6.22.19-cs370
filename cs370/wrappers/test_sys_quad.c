 /***********************************************************
  *
  *   Program:    test_sys_quad.c
  *   Created:    07/11/2013 12:02AM
  *   Author:     Ken Fox
  *   Requested:
  *   Comments:   Test wrapper for Time SLice quadrupler system call
  *   Assignment P2-2
  ************************************************************
  *    This goes in wrappers/test_sys_quad.c
  * History:
  *
  * $Log:$
  *
  ************************************************************/

#include<stdio.h>
#include<linux/unistd.h>
#include<errno.h>
#define QUADCALL 287

int main(int argc, char *argv[] )

{
    // Convert user input

    if( argc != 2)  // Not enough arguments
	return -1;

    // Convert user argument to long

    long pid = atol( argv[1] );
    if( pid == 0 )  // Conversion failed
	return -1;

//	printf("Process ID: %d Timeslice before: %d", pid, getTimeSLice(pid) );
	syscall(QUADCALL);
//	printf("Process ID: %d Timeslice after: %d", pid, getTimeSLice(pid) );

	return 0;

}




