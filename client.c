#include <stdio.h>
#include "udp.h"
#include "mfs.c"
#include <time.h>
#include <stdlib.h>
#define BUFFER_SIZE (1000)


//#define MFS_INIT (1)
//#define MFS_LOOKUP (2)
//#define MFS_STAT (3)
//#define MFS_WRITE (4)
//#define MFS_READ (5)       DO NOT NEED THIS IN CODE JUST REFERECE TO WHAT IS NEEDED
//#define MFS_CRET (6)
//#define MFS_UNLINK (7)
//#define MFS_SHUTDOWN (8)


int main(int argc, char *argv[]) {
    struct sockaddr_in addrSnd, addrRcv;
    // char message[BUFFER_SIZE];

    char* request = argv[1];
    int sd = MFS_Init("localhost", 2133);

    if(strcmp(request, "1") == 0){
            MFS_Init("localhost", 2133);
    }
    else if(strcmp(request, "2") == 0){
            int rc = MFS_Lookup(0, "random");
            printf("rc: %d\n", rc);
    }
    else if(strcmp(request, "3") == 0){
            
        }//need  elseif .... READ, WRITE , CREAT , UNLINK ALSO 
    else if(strcmp(request, "6") == 0){
          MFS_Creat(0, 1, "test");
          int rc = MFS_Lookup(0, "test");
          printf("return code  %d\n", rc);
    }
    else if(strcmp(request, "8") == 0){
            MFS_Shutdown();
        }
    else{
         printf("ERROR: failed to find request value"); 
    }

       
     //   sprintf(message, "hello world");

   // printf("client:: send message [%s]\n", message);
   // rc = UDP_Write(sd, &addrSnd, message, BUFFER_SIZE);
 //   if (rc < 0) {
//	printf("client:: failed to send\n");
//	exit(1);
 //   }

 //   printf("client:: wait for reply...\n");
 //   rc = UDP_Read(sd, &addrRcv, message, BUFFER_SIZE);
  //  printf("client:: got reply [size:%d contents:(%s)\n", rc, message);
    return 0;
}



  
   
   
