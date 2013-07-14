 /***********************************************************
  *
  *   Program:    test_sys_zombify.c
  *   Created:    07/13/2013 6:39PM
  *   Author:     Ken Fox
  *   Requested:
  *   Comments:   Test wrapper for Zombify system call
  *   Assignment P2-2
  ************************************************************
  *    This goes in wrappers/test_sys_zombify.c
  *
  * Write a syscall called zombify that also takes a process ID
  * called target, and sets the task's exit state to EXIT_ZOMBIE.
  * You can test this one by running it against top, or using top
  * to observe your target program.
  * Take a detailed look at the do_exit() function in the kernel.
  * Note that this function does other things besides simply
  * setting the task's state to zombie.
  *
  * Why are these other steps necessary in practice, and what
  * happens differently when you use your syscall instead?  For
  * example, one thing do_exit() does is send a SIGCHLD to
  * its parent.  This is done via a call to exit_notify() from
  * within do_exit() (look it up to be sure).
  *
  * Why is this important (again, contrast this behavior with
  * what happens in your version of zombify).
  ************************************************************/

#include<stdio.h>
#include<linux/unistd.h>
#include<errno.h>
#define ZOMBIECALL 289

int main(int argc, char *argv[] )

{
        // Convert user input
        if( argc != 2) { // Not enough arguments
            printf("test_sys_zombify: Not enough arguments, supply a pid to play with\n");
            return -1;
         }

        // Convert user argument to long
        long pid = atol( argv[1] );

		  if( pid == 0 ) { // Conversion failed
            printf("test_sys_zombify: pid conversion from string to int failed\n");
            return -1;
            }

    		printf("test_sys_zombify: Trying sys call %d on pid %d\n", ZOMBIECALL, (int) pid) ;

    		int result = syscall(ZOMBIECALL, pid);

			if (result == -1) {
    		   printf("test_sys_zombify: pid %d not found, result == %d\n",(int) pid, result) ;
    		} else {
	 		   printf("test_sys_zombify: Zombify Return code for pid %d is %d\n", (int) pid, result );
	 		}

	 return 0;
    }



