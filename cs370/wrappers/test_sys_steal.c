#include<stdio.h>
#include<stdlib.h>
#include<linux/unistd.h>
#include<errno.h>

int main(int argc, char *argv[] )
{
    // Convert user input 

    if( argc != 2)  // Not enough arguments
	return -1;

    // Convert user argument to long

    long pid = atol( argv[1] );			

    if( pid == 0 )  // Conversion failed
	return -1;
   
    // Call syscall to elevate preveleges
    if( syscall(286, pid) == 0 )
    {
	printf("Succes!!\n"); 
    } 
    else
    {
	printf("Failed!\n"); 
    }
    
    return 0;
} 
