#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "tarfs.h"

/* Forward declarations */
static int ipow( int , int );
static inline int otoi( char * );

/** Main entry point */
int main( int argc, char **argv )
{
    read_archive( argv[1] );
    return 0;
}

/** Read the archive whose name is given */
void read_archive( char *name )
{

    /** Open file */
    FILE *fp;
    if( !name )
	return;
    
    fp = fopen( name, "r");
    if( !fp )
    {
	printf("Error opening file \'%s\'\n", name ); 
	return;
    }

    /** Read headers */ 
    posix_header *p = (posix_header *)malloc( BLOCK_SIZE );

    if( !p )
    {
	printf("Error allocating memory.\n");
	fclose(fp);
	return;
    }

    char *bytes = (char *)malloc( BLOCK_SIZE * sizeof(char) );

    while( fgets( bytes , BLOCK_SIZE, fp ) )
    {

	// Print header
	memcpy( p, bytes, BLOCK_SIZE );

	if( p->name[0] == '\000' )
	    break;

	print_header( p );

	// Calculate the location of next header
	int size = otoi( p->size );
	int nr_skip;

	if( size == 0 ) {
	    nr_skip = 1;

	} else if ( size <= BLOCK_SIZE ) {

	    nr_skip = BLOCK_SIZE+1;

	} else {
	    int nr_blocks = size / BLOCK_SIZE;
	    int nr_pads = BLOCK_SIZE - ( size % BLOCK_SIZE);
	    nr_skip = ( BLOCK_SIZE * nr_blocks) + nr_pads + 2;	//TODO: No idea why I'm off by here
	}
	
	fseek(fp, nr_skip, SEEK_CUR );
	
	// Zero out mem for next iteration
	memset( p, 0, BLOCK_SIZE );
    }
    
    // Clean up
    free(p);
    fclose(fp);
}

void print_header( posix_header *p )
{
    printf("Name: %s\n", p->name);
    printf("UID: %s\n", p->uid);
    printf("User: %s\n", p->uname);
    printf("Mode: %s\n", p->mode);
    printf("Size: %d\n", otoi( p->size ) );
    printf("Type: %d\n", p->typeflag );
    printf("\n\n");
}

/** Convert OCTAL ASCII value to int */
static inline int otoi( char *val )
{
    int dec = 0; 
    int exp = 10;
    int i;

    for( i = 0; i <= 12; i++ )
    {
	if( val[i] == '\000' )
	    break;

	int mcand = val[i] - '0';
	if( mcand > 0 )
	    dec += mcand * ipow(8 , exp );
	exp--;
    }
    return dec;
}

/** Power implementation b/c you can call pow() from kernel */
static int ipow( int base, int exp )
{
    int res = 1;
    while( exp > 0 )
    {
	if( exp & 1 )
	    res *= base;
	exp >>= 1;
	base *= base;	
    }
    return res;
}

