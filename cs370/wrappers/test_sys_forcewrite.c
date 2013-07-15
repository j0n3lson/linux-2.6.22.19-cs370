 /***********************************************************
  *
  *   Program:    test_sys_forcewrite.c
  *   Created:    07/14/2013 6:59PM
  *   Author:
  *   Requested:
  *   Comments:   Wrapper function to test forced writing of file
  * using the sys_forcewrite call
  *
  ************************************************************
  * This goes in cs370/wrappers/test_sys_forcewrite.c
  * History:
  *
  * $Log:$
  *
  ************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h> 

#define FORCEWRITECALL 291
#define FORCEWRITE "test_sys_forcewrite"

int main(int argc, char **argv )

{
        // Convert user input
        if( argc != 2) { // Not enough arguments
            printf(FORCEWRITE ": Not enough arguments, please supply someone elses file to destroy\n");
            return -1;
         }


			int fd;   /* file descriptor */

			/*- open file in read only mode then  bork it   -*/
			if ((fd = open( argv[1], O_RDONLY )) < 0) {
			//if ((fd = open( argv[1], O_CREAT|O_TRUNC|O_RDWR)) < 0) {
			   perror("open");
			   exit(1);
			}
			char *buf = "Mary had a little lamb, and two helpings of peas and carrots";
			// this should print
	   	  	printf(FORCEWRITE ": string: %s\n",buf) ;

			int count = 64 ; // strlen(buf);

    		printf(FORCEWRITE ": Trying sys call %d on file: %s, bytes %d\n", FORCEWRITECALL, argv[1], count ) ;

    		int result = syscall(FORCEWRITECALL, fd,  buf, count);
			//sys_forcewrite(unsigned int fd, const char __user * buf, size_t count)


			if (result == -1) {
    		   printf(FORCEWRITE ": failed for file: %s, result %d\n", argv[1], result) ;
    		} else {
	 		   printf(FORCEWRITE ": Bytes written %d\n",  result );
	 		}


			/* close file */
			if ((result = close(fd)) < 0) {
			   perror("close");
			   exit(1);
			}

	 return 0;
    }



