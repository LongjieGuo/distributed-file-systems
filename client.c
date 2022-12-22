#include <stdio.h>
#include "udp.h"
#include "mfs.c"
#include <time.h>
#include <stdlib.h>
#define BUFFER_SIZE (4096)


//#define MFS_INIT (1)
//#define MFS_LOOKUP (2)
//#define MFS_STAT (3)
//#define MFS_WRITE (4)
//#define MFS_READ (5)       DO NOT NEED THIS IN CODE JUST REFERECE TO WHAT IS NEEDED
//#define MFS_CRET (6)
//#define MFS_UNLINK (7)
//#define MFS_SHUTDOWN (8)


int main(int argc, char *argv[]) {
   //char name[5];
    MFS_Init("localhost", 2000);
    //MFS_Creat(0, MFS_DIRECTORY, "test");
	
    MFS_Shutdown();

    return 0;
}
  
