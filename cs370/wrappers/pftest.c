
#include <stdio.h>
#include <stdlib.h>
#define FORCEWRITE "test_sys_forcewrite"

int main(int argc, char **argv )

{
            printf(FORCEWRITE ": Not enough arguments, please supply someone else's file to destroy\n");
			char *buf = "Mary had a little lamb, and two helpings of peas and carrots";
			// this should print
    	  printf(FORCEWRITE ": string: %s\n", buf) ;

}
