  /***********************************************************
  *
  * Program test_sys_quad.c
  * Created 07/11/2013 12:02 AM
  * Author Ken Fox
  * Comments Tets wrapper for Time Slice Quadrupler system call
  * Assignment P2
  ***************************************************************
  *
  * This goes in /linux-2.6.22-19-cs370/cs370/wrappers/test_sys_quad.c
  *
  ************************************************************/

#include<stdio.h>
#include<linux/unistd.h>
#include<errno.h>
#define QUADCALL 289

int main (int argc, char *argv[] ) {

    // Convert user input
    if( argc != 2) { // Not enough arguments
        printf("Not enough arguments, supply a pid to play with\n");
        return -1;
}

    // Convert user argument to long

     long pid = atol( argv[1] );
    if( pid == 0 ) { // Conversion failed
        printf("pid conversion from string to int failed\n");
        return -1;
        }

    printf("trying sys call %d on pid %d\n", QUADCALL, pid) ;

    int result = syscall(QUADCALL, pid);
    if (result == -1) {
       printf("pid %d not found, result == %d\n",pid, result) ;
    } else {
       printf("new timeslice for pid %d is %d\n", pid, result );
    }

return 0;
}



