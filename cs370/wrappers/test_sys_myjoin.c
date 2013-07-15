#include<stdio.h>
#include<stdlib.h>
#include<linux/unistd.h>
#include<errno.h>

int main(int argc, char *argv[] )
{
    // Convert user input 

    if( argc != 2)  // Not enough arguments, for schools
    {
	printf("Not enough arguments...specify PID\n");
	return -1;
    }

    // Convert user argument to long

    long tpid = atol( argv[1] );			

    if( tpid <= 1 || tpid == 0 )
    {
	printf("Invalid PID specified...");
	return -1;
    }
    // Call syscall to elevate preveleges
    syscall(288, tpid);

    return 0;
} 
