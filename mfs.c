#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "messages.h"
#include "mfs.h"
#include "udp.h"
#include <time.h>
#include <stdlib.h>
#define BUFFER_SIZE (1000)
int sd;
int rc;
char* serverAddress;
int serverPort;
struct sockaddr_in addrSnd, addrRcv;



int send_request(message_t request, message_t* response, char* address, int port){
    
    rc = UDP_Write(sd, &addrSnd,(char*) &request, BUFFER_SIZE);

    if (rc < 0) {
        printf("client:: failed to send\n");
        exit(1);
    }
    printf("waiting for response\n");
    rc = UDP_Read(sd, &addrRcv,(char*) response, BUFFER_SIZE);
    printf("client:: got reply [size:%d contents:(%s)\n", rc, (char*)response);
    return response->inum;
}



int MFS_Init(char *hostname, int port){  
    int port_client = 1234;
    sd = UDP_Open(port_client);
    rc = UDP_FillSockAddr(&addrSnd, hostname, port);
    
    return 0;
}


int MFS_Lookup(int pinum, char* name){
    message_t request,response;
    
    request.requestType =2;
    request.inum = pinum;
    strncpy(request.name,name,28);
    //request.name = name;
    int rc = send_request(request, &response,  serverAddress, serverPort);
    return rc;
}

int MFS_Stat(int inum, MFS_Stat_t *stats){
    message_t request,response;
    request.requestType = 3;
    request.inum = inum;
    int rc = send_request(request, &response,  serverAddress, serverPort);
    return rc;
}

int MFS_Write(int inum, char *buffer, int offset, int nbytes){
    message_t request,response;
    
    request.requestType= 4;
    //request.buffer = buffer;
    strncpy(request.buffer,buffer,sizeof(request.buffer));
    request.inum = inum;
    request.offset = offset;
    request.nbytes = nbytes;
    int rc = send_request(request, &response,  serverAddress, serverPort);
    return rc;
}        

int MFS_Read(int inum, char *buffer, int offset, int nbytes){
    message_t request,response;
    
    request.requestType= 5;
    //request.buffer = buffer;
    strncpy(request.buffer,buffer,sizeof(request.buffer));
    request.inum = inum;
    request.offset = offset;
    request.nbytes = nbytes;
    int rc = send_request(request, &response,  serverAddress, 
    serverPort);
    return rc;
}      

int MFS_Creat(int pinum, int type, char *name){
    message_t request,response;
    
    request.requestType = 6;
    request.inum = pinum;
    request.type = type;
    //request.name =name;
    strncpy(request.name,name,28);
    int rc = send_request(request, &response,  serverAddress, serverPort); 
    return rc;
}                       

int MFS_Unlink(int pinum, char *name){
    message_t request,response;
    
    request.inum = pinum;
    request.requestType = 7;
    int rc = send_request(request, &response,  serverAddress, serverPort);
    return rc;
}                                

int MFS_Shutdown(){
    message_t request;
    message_t response;
    request.requestType = 8;
    int rc = send_request(request, &response,  serverAddress, serverPort);    
}
