#include<stdio.h>
#include<linux/unistd.h>
#include<errno.h>

int main()
{
    printf("Process ID: %d\n", syscall(285) );
    return 0;
}
