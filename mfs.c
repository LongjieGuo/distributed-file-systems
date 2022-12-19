#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "mfs.h"
#include "udp.h"
#include "messages.h"
#include <time.h>
#include <stdlib.h>
#define BUFFER_SIZE (4096)

#define UDP_SIZE (10000)

int MIN_PORT = 20000;
int MAX_PORT = 40000;

char* serv_addr;
int serv_port;
int sd;
int rc;
int i;

struct sockaddr_in addr_send, addr_reci;


int send_request(message_t* request, message_t* response, char* address, int port){
    printf("request type: %d\n", request->request_type);
    rc = UDP_Write(sd, &addr_send,(char*)request, UDP_SIZE);
    if (rc < 0) {
        printf("client:: failed to send\n"); 
        exit(1);
    }
    rc = UDP_Read(sd, &addr_reci,(char*)response, UDP_SIZE);
    if (rc < 0) {
        printf("server:: failed to send\n");
        exit(1);
    }
    return response->rc;
}

int MFS_Init(char *hostname, int port){ 
    srand(time(0));
    int port_num = (rand() % (MAX_PORT - MIN_PORT) + MIN_PORT);
    sd = UDP_Open(port_num);
    rc = UDP_FillSockAddr(&addr_send, hostname, port);
    if (rc < 0) {
        return -1;
    }
    return 0;
}

int MFS_Lookup(int pinum, char* name){
    printf("MFS lookup pinum : %d, name: %s\n", pinum, name);
    message_t request,response;
    request.request_type =2;
    request.inum = pinum;
   
    for (i = 0; i < strlen(name); i++){
        request.name[i] = name[i];
    } 
    request.name[i] = '\0';
    rc = send_request(&request, &response, serv_addr, serv_port);
    if (rc < 0) {
        return -1;
    }
    return response.inum; 
}

int MFS_Stat(int inum, MFS_Stat_t *stat){
    message_t request,response;
    request.request_type = 3;
    request.inum = inum;
    rc = send_request(&request, &response,  serv_addr, serv_port);
    if (rc < 0) {
        return -1;
    }
    stat->size = response.stat.size;
    stat->type = response.stat.type;
    return response.inum;
}

int MFS_Write(int inum, char *buffer, int offset, int nbytes){
    if(nbytes > 4096) return -1;
    message_t request, response;
    request.request_type = 4;
    request.inum = inum;

    for (i = 0; i < strlen(buffer); i++){
        request.buffer[i] = buffer[i];
    } 
    request.buffer[i] = '\0';
    request.offset = offset;
    request.nbytes = nbytes;

    rc = send_request(&request, &response,  serv_addr, serv_port); 
    if (rc < 0) {
        return -1;
    }
    return response.inum;
}        

int MFS_Read(int inum, char *buffer, int offset, int nbytes){
    if(nbytes > 4096) return -1;
    message_t request, response;
    request.request_type = 5;
    request.inum = inum;
    request.offset = offset;
    request.nbytes = nbytes;
    rc = send_request(&request, &response,  serv_addr, serv_port); 
    
    if (rc < 0) {
        return -1;
    }
    for (i = 0; i < strlen(buffer); i++){
        buffer[i] = response.buffer[i];
    } 
    buffer[i] = '\0';
    return response.inum;
}         

int MFS_Creat(int pinum, int type, char *name){
    message_t request,response;
    request.request_type = 6;
    request.inum = pinum;
    request.type = type;

    for (i = 0; i < strlen(name); i++){
        request.name[i] = name[i];
    } 
    request.name[i] = '\0';
    rc = send_request(&request, &response,  serv_addr, serv_port); 
    if (rc < 0) {
        return -1;
    }
    return response.inum;
}                       

int MFS_Unlink(int pinum, char *name){
    message_t request,response;
    request.request_type =7;
    request.inum = pinum;

    for (i = 0; i < strlen(name); i++){
        request.name[i] = name[i];
    } 
    request.name[i] = '\0';
    rc = send_request(&request, &response,  serv_addr, serv_port);
    if (rc < 0) {
        return -1;
    }
    return response.inum;
}                               

int MFS_Shutdown(){
    message_t request,response;
    request.request_type = 8;
    rc = send_request(&request, &response,  serv_addr, serv_port);

    if (rc < 0) {
        return -1;
    }
    return 0;
    
}
