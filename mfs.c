#include <sys/select.h>
#include <time.h>
#include <stdio.h>
#include "messages.h"
#include "mfs.h"
#include "udp.h"

struct sockaddr_in arr_send,addr_rcv;
int sd;
int server_up = 0;

int MFS_Init(char *hostname, int port){
    
    server_up = 1;
    
    int MIN_PORT = 20000;
    int MAX_PORT = 40000;

    srand(time(0));
    int port_num = (rand() % (MAX_PORT - MIN_PORT) + MIN_PORT);

    sd  = UDP_Open(port_num);
    int rc = UDP_FillSockAddr(&arr_send, hostname, port);
    assert(rc>-1);
    
    return 0;
}

int MFS_Lookup(int pinum, char *name){


    if(server_up ==0){
        return -1;
    }

    if(pinum < 0 || strlen(name) == 0){
        return -1;
    }
  

    message_t message;
    message.request_type = MFS_LOOKUP;
    message.inum = pinum;
    strcpy(message.name,name);

    int rc = UDP_Write(sd,&arr_send,(char*)(&message),sizeof(message_t));
    if(rc<0){
        return -1;
    }
    rc = UDP_Read(sd, &addr_rcv, (char *)&message, sizeof(message_t));

    return message.rc;
}

int MFS_Stat(int inum, MFS_Stat_t *stat_msg){

    if(server_up == 0){
        return -1;
    }

    if(inum < 0 || stat_msg == NULL){
        return -1;
    }

    message_t message;
    message.request_type = MFS_STAT;
    message.inum = inum;

    int rc = UDP_Write(sd,&arr_send,(char*)(&message),sizeof(message_t));
    if(rc<0){
        return -1;
    }

    rc = UDP_Read(sd, &addr_rcv, (char *)&message, sizeof(message_t));
    if(message.rc!=0) {
        return -1;
    }

    stat_msg->type = message.stat.type;
    stat_msg->size = message.stat.size;
    fprintf(stderr,"Stat returned type %d size %d\n",stat_msg->type,stat_msg->size);

    return 0;
}

int MFS_Write(int inum, char *buffer, int offset, int nbytes){

    if(server_up == 0){
        return -1;
    }


    if(offset < 0 || nbytes > 4096){
        return -1;
    }

    if(inum < 0 || strlen(buffer) == 0){
        return -1;
    }

    message_t message;
    message.request_type = MFS_WRITE;
    message.inum = inum;
    memcpy(message.buffer,buffer,nbytes);
    message.offset = offset;
    message.nbytes = nbytes;

    int rc = UDP_Write(sd,&arr_send,(char*)(&message),sizeof(message_t));
    if(rc<0){
        return -1;
    }

    rc = UDP_Read(sd, &addr_rcv, (char *)&message, sizeof(message_t));
    if(message.rc!=0) {
        return -1;
    }
    return 0;
}

int MFS_Read(int inum, char *buffer, int offset, int nbytes){

    if(server_up == 0){
        return -1;
    } 


    if(inum < 0 || offset < 0 || nbytes < 0){
        return -1;
    }


    message_t message;
    message.request_type = MFS_READ;
    message.inum = inum;
    message.offset = offset;
    message.nbytes = nbytes;

    int rc = UDP_Write(sd,&arr_send,(char*)(&message),sizeof(message_t));
    if(rc<0){
        return -1;
    }

    rc = UDP_Read(sd, &addr_rcv, (char *) &message, sizeof(message_t));
    if(message.rc!=0) {
        return -1;
    }
    memcpy(buffer,message.buffer,nbytes);

    return 0;
}

int MFS_Creat(int pinum, int type, char *name){

    if(server_up ==0){
        return -1;
    }

    if( type < 0){
        return -1;
    }



    if(pinum < 0 || strlen(name) < 0 ){
        return -1;
    }

    if(strlen(name)>=28) {
        return -1;
    }

    message_t message;
    message.request_type = MFS_CRET;
    message.inum = pinum;
    message.type = type;
    strcpy(message.name,name);

    int rc = UDP_Write(sd,&arr_send,(char*)(&message),sizeof(message_t));
    if(rc<0){
        return -1;
    }

    rc = UDP_Read(sd, &addr_rcv, (char *) &message, sizeof(message_t));
    if(message.rc!=0) {
        return -1;
    }

    return 0;
}

int MFS_Unlink(int pinum, char *name){

    if(server_up ==0){
        return -1;
    }

    if(pinum < 0 || strlen(name) < 0){
        return -1;
    }


    message_t message;
    message.request_type = MFS_UNLINK;
    message.inum = pinum;
    strcpy(message.name,name);

    int rc = UDP_Write(sd,&arr_send,(char*)(&message),sizeof(message_t));
    if(rc<0){
        return -1;
    }

    rc = UDP_Read(sd, &addr_rcv, (char *) &message, sizeof(message_t));
    if(message.rc!=0){
        return -1;
    }

    return 0;
}

int MFS_Shutdown(){

    if(server_up ==0){
        return -1;
    } 

    message_t message;
    message.request_type = MFS_SHUTDOWN;

    int rc = UDP_Write(sd, &arr_send, (char *)(&message), sizeof(message_t));
    if(rc<0){
        return -1;
    }
    return 0;
}
