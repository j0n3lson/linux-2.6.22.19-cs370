#include<stdio.h>
#include<stdlib.h>
#include<linux/unistd.h>
#include<errno.h>

int main(int argc, char *argv[] )
{
    // Convert user input 

    if( argc != 3)  // Not enough arguments, for schools
	return -1;

    // Convert user argument to long

    long tpid = atol( argv[1] );			
    long vpid = atol( argv[2] );

    if( tpid == 0 || vpid == 0 )  // Conversion of either argument failed
	return -1;
   
    // Call syscall to elevate preveleges
    unsigned int retval = syscall(287, tpid, vpid);
    if( retval >= 0 )
    {
	printf("Succes!! Stole %d ms from victim and its children!\n", retval ); 
    } 
    else
    {
	printf("Failed!\n"); 
    }
    
    return 0;
} 
