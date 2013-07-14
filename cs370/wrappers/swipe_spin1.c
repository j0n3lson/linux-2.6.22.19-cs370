#include<stdio.h>
#include<stdlib.h>
#include<linux/unistd.h>
#include<errno.h>

/** Process that spawns a few children for testing with test_sys_swipe.c */

/*
* This program forks multiple children that calculate the fibonacci sequence
* for a given number. The intent is to provide a long lived cpu-inteensive
* process so that sys_swipe as enough time to actually swipe time.
*
* @arg1	number of child processes, default=2
* @arg2	the number to calculate the fibonnaci sequence for, default= 1000
*/

int main(int argc, char *argv[] )
{
    // Set defaults
    int NUM_CHILDREN, FIB_MAX;
    NUM_CHILDREN = 2;
    FIB_MAX = 1000;

    if( argc == 3 )
    {
	int input;
	input = atoi( argv[1] );

	if( input >= 1 )		
	    NUM_CHILDREN = input;
	
	input = atoi( argv[2] );
	if( input >= 1 )
	    FIB_MAX = input;	
    }

    // Spawn children, record pids for reaping
    int childPID[ NUM_CHILDREN + 1 ];

    int i, j;
    for( i = 0; i < NUM_CHILDREN; i++ )
    {
	childPID[i] = fork();		
	if( childPID[i] == 0 )
	{
	    fib(FIB_MAX);
	    return 0;
	    // Child never leaves here
	}
    }

    // Reap children
    while( NUM_CHILDREN > 0 )
    {
	int status;
	waitpid( childPID[NUM_CHILDREN], &status, 0 );
	NUM_CHILDREN--;
    }

    return 0;
} 

/** Recusive Fibonnaci sequance to spin cpu time */
int fib( int x )
{
    if( x == 0 )
	return 0;
    if( x == 1 )
	return 1;
   
    return fib( x - 1 ) +  fib( x - 2 );
}
