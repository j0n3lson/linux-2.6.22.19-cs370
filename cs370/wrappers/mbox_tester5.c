/***********************************************************
 *
 *   Program:    mbox_tester.c
 *   Created:
 *   Author:
 *   Assignment: CS370 P3 tester
 *   Comments:   Drives mailbox
 *
 ************************************************************
 *
 *
 *  create a list of pids that get shared among all of the pids
 *                                                          */

/* Includes */

#include <unistd.h>    /* Symbolic Constants */
#include <sys/types.h> /* Primitive System Data Types */
#include <errno.h>     /* Errors */
#include <stdio.h>     /* Input/Output */
#include <sys/wait.h>  /* Wait for Process Termination */
#include <stdlib.h>    /* General Utilities */
#include <sys/shm.h>
#include <strings.h>    /* General String manipulation Utilities */


// system call numbers
#define MSGSEND  292
#define MSGRCV   293

// default receive message size --
#define MSGSIZE = 100
#define EXITMSG = "All Done"

#define PROCESS_COUNT  5
// default number of children to spawn


/** common structure to hold pid and status */
struct child_info {
pid_t child_pid;
int done;
int sent;
};

//forward  function declarations
void do_child( int pid, int n );

/*-----------------7/28/2013 10:27PM----------------
 * DUMMY sys calls for distributed development
 * --------------------------------------------------*/
int syscall_mysend( int syscall_num, pid_t child_pid, int count, char* buf );
int syscall_myreceive( int syscall_num, pid_t child_pid, int count, char* buf );


/** ---------------  MAIN ------------------------ **/

int main( int argc, char* argv [] ){

  int n;
  n = PROCESS_COUNT; // number of child processes


  /** when instantiaing need to tell the compiler that
  * we're dealing with a struct, then what type of struct
  * it seems to be ok with the array declaration without a malloc()
  */

  struct child_info children[n]; // object to keep child process informaiton in
  int i;

  for ( i = 0; i < n; i++ ){

    // init children
    children[i].done = 0;
    children[i].sent = 0;

    //child pid is so that the child process can check the value
    pid_t childpid = fork();

    // reference the contents of the struct using dot notation
    children[i].child_pid = childpid;

    if( childpid >= 0 ){   /* fork succeeded */
      if( childpid == 0 ){ /* fork() returns 0 to the child process */
      do_child( i, n );
      }                    // child pid is 0
    } else{
      /** childpid >=0 ::: fork failed */
      perror( "Child fork failed" ); /* display error message */
      exit( 0 );
    }
  } // end child pid creation loop


  // send children array of child_info structs data to children in a message
  // I decided to message them individually rather than with a broad cast

  // convert the struct to an space delimited list of pids

  char * mybuff = malloc ( ( n * sizeof(long) )+ n );

  mybuff = mybuff + sprintf("%ld",(long) children[0].child_pid);
  for ( i = 1 ; i < n ; i++ ) {
	 mybuff = mybuff + sprintf(" %ld", (long) children[i].child_pid );
  }



  int count = sizeof( mybuff );                                            /* need help here */
  int result = 0 ;
  for ( i = 0; i < n; i++ ){
  	result = syscall( MSGSEND, children[i].child_pid, count, (char*) mybuff ); /* need help here */
  	if ( result == -1) {
  		printf("error sending pid list to child %d\n",children[i].child_pid );
  	}
  }

  // reap the little buggers
  int status;
  int j;

  for ( j = 0; j < n; j++ ){
  		waitpid( children[j].child_pid, &status, 0 );
  }

  printf( "%s: testing completed for %d children. \n", argv[0], n );

  exit( 0 );
} // Main


/** --------------- do_child ------------------------*/

void
do_child(int i, int n){


	int me = getpid();

  	struct child_info  siblings[n] ;     // object to keep sibling contact information in

 	printf( "Child process number %d PID %d created\n", i, me );
  	sleep(n); /* once for each child spawned */


	// I want the parent to send the list of children in the child_info struct
	// to me so that I can iterate over it and send messages
	// to everyone.

	int count = n * sizeof(struct child_info); /* need help here */
	void *buf = malloc(count);                 /* need help here */
	int result;                                /* need help here */

	result  = syscall( MSGRCV, getpid(), count , buf) ;  /* need help here */

	while  ( result != count ) { /** wait for message from parent */
  		result  = syscall( MSGRCV, getpid(), count ,buf) ;  /* need help here */
	}

	/**
	 when I get here I want to have a child_info struct with all the other PIDS
	 in it with each done element initialized to 0
	*/
	  int h = 0;
	  char * tok;
	  tok = strtok (buf, " ");
	  siblings[h].child_pid = atoi(tok);
	  while ( tok != NULL) {
			h++;
	  		if (h < n) {
				tok = strtok (NULL, " ");
			  	siblings[h].child_pid = atoi(tok);
			}
	  }

		int x = 0;
  		for ( x = 0 ; x < n ; x++) {
			siblings[x].done = 0;	// init done array
			siblings[x].sent = 0;  // init sent array
		}

  exit(0);

	/* first do a read of all the pids in case there are pending messages since the
	 * whole bloody mess is asynchronous -- this is the first time through so it it not possible that
	 * the done message has been sent -- we just want to clear out any possible blocking calls
	 * waiting on us before we start
	 * */

	char *mbuf = malloc(64); /* generic 64 byte buffer -- only going to hold "hello" and "release" */

	for ( x = 0 ; x < n ; x++) {
		if ( siblings[x].child_pid != me ) {
			result  = syscall( MSGRCV, siblings[x].child_pid, count ,mbuf) ;
			if ( result > 0 ) {
				/** this is a toy program so we don't care about the message, only that we got it */
				siblings[x].done = 1;
			}
		}
	}


		// cycle through the array of pids
		// check for a message first from that pid since receive is non blocking
	int done = 0;
	for ( x = 0 ; x < n ; x++) {
		if ( siblings[x].child_pid != me ) {
			result  = syscall( MSGRCV, siblings[x].child_pid, count ,mbuf) ;
			// do something with the result or the message
			//is the message the end message?
			if ( mbuf == "release" ) {
				// if yes then we are basically done but need to do one last
				// read check from everyone before terminating

				int k = 0;
				for ( k = 0 ; k < n ; k++) {
					if (siblings[k].child_pid != me && siblings[k].done != 0)
						result  = syscall( MSGRCV, siblings[k].child_pid, count ,mbuf) ;
				}
				done = 1;
				break;

			} else {
				// the message isn't "Done" but we got something so update the
				// done[] elementr for this pid -
				siblings[x].done = 1;

				// check whether we have received a message from everyone else, and if so
				// send out the done message
				int j = 0;
				int finished = 1;
				for ( j = 0 ; j < n ; j++) if ( siblings[j].done == 0 ) finished = siblings[j].done;
				if ( finished ) {
					// sedn out the final message
					for ( j = 0 ; j < n ; j++) {
						int clear = 0;
						while ( ! clear) {
							int retval = 0;
							retval = syscall(MSGSEND, siblings[j].child_pid, count , "release");

							if ( retval == -1 ) {
								result  = syscall( MSGRCV, siblings[j].child_pid, count ,mbuf) ;
							} else {
								clear = 1 ;
							}
						} // while not clear
					} // for j
					done = 1;
					break;
				} // if finished
			} // receive a regular message

			/** time to send one message to a the current sibling **/
			int clear = 0;
			mbuf = "hello";

			while ( ! clear) {
				int retval = 0;
				retval = syscall(MSGSEND, siblings[x].child_pid, count , mbuf);

				if ( retval == -1 ) {
					result  = syscall( MSGRCV, siblings[x].child_pid, count ,mbuf) ;
				} else {
					clear = 1 ;
				}
			} // while not clear

			siblings[x].sent = 1;

		} // not me
	} // for loop

// clean up any pending send requests to this PID
  int k = 0;
  for ( k = 0 ; k < n ; k++) {
		for ( x = 0 ; x < n ; x++) {
			if ( siblings[x].child_pid != me ) {
				result  = syscall( MSGRCV, siblings[x].child_pid, count ,mbuf) ;
				if ( result > 0 ) {
					/** this is a toy program so we don't care about the message, only that we got it */
					siblings[x].done = 1;
				}
			}
		}
  }


  printf( "CHild Process %d exiting\n", getpid() );
  exit( 0 );



 } // do_child()


/*-----------------7/28/2013 10:28PM----------------
 * Dummy functions to emulate send and receive
 * --------------------------------------------------*/
int syscall_mysend( int syscall_num, pid_t child_pid, int count, char* buf ){

  printf( "PID (%d)Syetem call number: %d , sent to child pid %d, length %d bytes.\n", getpid(), syscall_num, child_pid,
    count );

  printf( "PID (%d) Message sent: %s\n", getpid(), buf );
  return(0);
}

int syscall_myreceive( int syscall_num, pid_t child_pid, int count, char* buf ){

  printf( "PID (%d)Syetem call number: %d , received from pid %d, length %d bytes.\n", getpid(), syscall_num, child_pid,
    count );

  printf( "PID (%d) Message received: %s\n", getpid(), buf );

  return ( 0 );
}
