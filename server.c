// #include <stdio.h>
// #include "udp.h"

// #define BUFFER_SIZE (1000)

// // server code
// int main(int argc, char *argv[]) {
//     int sd = UDP_Open(10000);
//     assert(sd > -1);
//     while (1) {
// 	struct sockaddr_in addr;
// 	char message[BUFFER_SIZE];
// 	printf("server:: waiting...\n");
// 	int rc = UDP_Read(sd, &addr, message, BUFFER_SIZE);
// 	printf("server:: read message [size:%d contents:(%s)]\n", rc, message);
// 	if (rc > 0) {
//             char reply[BUFFER_SIZE];
//             sprintf(reply, "goodbye world");
//             rc = UDP_Write(sd, &addr, reply, BUFFER_SIZE);
// 	    printf("server:: reply\n");
// 	} 
//     }
//     return 0; 
// }

#include "udp.h"
#include "signal.h"
#include "messages.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE (1000)

int sd;
int fd;
struct sockaddr_in addr;
int portNumber;

super_t superBlock;
int inode_map;
int data_map;
int inode_start_pos;
int data_start_pos;
char* free_inode_addr;
char* free_data_addr;


void intHandler(int dummy) {
    UDP_Close(sd);
    exit(130);
}

unsigned int get_bit(unsigned int *bitmap, int position) {
   int index = position / 32;
   int offset = 31 - (position % 32);
   return (bitmap[index] >> offset) & 0x1;
}


void set_bit(unsigned int *bitmap, int position) {
   int index = position / 32;
   int offset = 31 - (position % 32);
   bitmap[index] |= 0x1 << offset;
}


int get_free_inode() {
    for(int i = 0; i<superBlock.num_inodes; i++) {
        int bit = get_bit((unsigned int *)free_inode_addr, i);
        if(bit==0) {
            return i;
        }
    }
    return -1;
}

int get_free_data_block() {
    for(int i = 0; i<superBlock.num_data; i++) {
        int bit = get_bit((unsigned int *)free_data_addr, i);
        if(bit == 0) {
            return i;
        }
    }
    return -1;
}


int fs_stat(int inode, MFS_Stat_t *message){
    message_t reply;
    int inode_block = superBlock.inode_region_addr + (inode * sizeof(inode_t)) / UFS_BLOCK_SIZE; 
    int inode_position = inode_start_pos + inode * sizeof(inode_t);

    //check inode bitmap
    if(get_bit((unsigned int *)free_inode_addr, inode) == 0){
        reply.inum = -1;
        UDP_Write(sd, &addr, (char*)&reply, sizeof(message));
        printf("bit map invalid\n");
        return -1;
    } 
    else{
        inode_t inode;
        //move read pointer to the position, SEEK_SET  means starting from the beginning
        lseek(fd, inode_position, SEEK_SET);
        read(fd, &inode, sizeof(inode_t));
        reply.inum = 0;
        reply.stat.size = inode.size;
        reply.stat.type = inode.type;
    }
    int rc = UDP_Write(sd, &addr, (char*)&reply, sizeof(message));
    return rc;
}



int fs_write(int inum, char *buffer, int offset, int nbytes){
    message_t reply;
    UDP_Write(sd, &addr, (char*)&reply, sizeof(message_t));
    return -1;
}



int fs_shutdown(){
    message_t reply;
    fsync(fd);
	close(fd);
    UDP_Write(sd, &addr, (char*)&reply, sizeof(message_t));
	UDP_Close(portNumber);
    printf("shutdown\n");
	exit(0);
	return 0;
}




int server_start(int port, char* img){
    
    sd = UDP_Open(port);


    while (1) {
        message_t* request = malloc(sizeof(message_t));
        printf("server:: waiting...\n");
        int rc = UDP_Read(sd, &addr, (char*)request, sizeof(message_t));

        //int responseRet;
        printf("server:: read message [size:%d contents:(%s)]\n", rc, (char*)request);
        if (rc > 0) {
            if(rc ==2){
                fs_lookup(request->inum, request->name);
                break;
            }
            else if (rc ==3){
                fs_stat(request->inum, &request->stat);
            }
            else if(rc ==4){
                fs_write(request->inum,request->buffer,request->offset,request -> nbytes);
            }
            else if (rc ==5){
                fs_read(request->inum,request->buffer,request->offset,request -> nbytes);
            }
            else if (rc ==6){
                fs_creat(request ->inum,request->type,request->name);
            }
            else if (rc ==7){
                fs_unlink(request ->inum, request ->name);
            }
            else if(rc == 8){
                fs_shutdown();
            }
            else{
                printf("ERROR NO RC\n");
            }
        }
        else{
            printf("invalid RC\n");
        }
    
    return 0; 
}

int main(int argc, char *argv[]){
    signal(SIGINT, intHandler);
    if (argc != 3){
		printf("Error\n");
    }
	server_start(atoi(argv[1]), argv[2]);
    return 0;
}
