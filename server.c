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
int port_number;

void *fs_img;
super_t *super_block;
unsigned int *inode_bitmap;
unsigned int *data_bitmap;
inode_t *inodes;
void *data;


/**********************
    HELPER FUNCTIONS
***********************/

void int_handler(int dummy) {
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
    for(int i = 0; i<super_block->num_inodes; i++) {
        int bit = get_bit((unsigned int *)inodes, i);
        if(bit==0) {
            return i;
        }
    }
    return -1;
}

int get_free_data_block() {
    for(int i = 0; i<super_block->num_data; i++) {
        int bit = get_bit((unsigned int *)data, i);
        if(bit == 0) {
            return i;
        }
    }
    return -1;
}


/**********************
      FS FUNCTIONS
***********************/

int fs_lookup(int pinum, char *name) {
    if (get_bit(inode_bitmap, pinum) == 0) {
        return -1;
    }
    inode_t *inode = inodes + pinum * sizeof(inode_t);
    if (inode->type != MFS_DIRECTORY) {
        return -1;
    }
    int size = inode->size;
    int bytes_read = 0;
    for (int i = 0; inode->direct[i] != -1; i++) {
        dir_ent_t *dir = fs_img + inode->direct[i] * UFS_BLOCK_SIZE; // direct[i] indexes into the entire disk
        for (int j = 0; j < UFS_BLOCK_SIZE; j += sizeof(dir_ent_t)) {
            if (strcmp(dir->name, name) == 0) {
                return dir->inum;
            }
            dir++;
            bytes_read += 32;
            if (bytes_read == size) {
                break;
            }
        }
    }
    return -1;
}

int fs_stat(int inum, MFS_Stat_t *m){
    if (get_bit(inode_bitmap, inum) == 0) {
        return -1;
    }
    inode_t *inode = inodes + inum * sizeof(inode_t);
    m->type = inode->type;
    m->size = inode->size;
    return 0;
    /*
    message_t reply;
    int inode_block = super_block->inode_region_addr + (inum * sizeof(inode_t)) / UFS_BLOCK_SIZE; 

    //check inode bitmap
    if(get_bit(inode_bitmap, inum) == 0){
        reply.inum = -1;
        UDP_Write(sd, &addr, (char*)&reply, sizeof(message_t));
        printf("bit map invalid\n");
        return -1;
    }else{
        inode_t inode;
        read(fd, &inode, sizeof(inode_t));
        reply.inum = 0;
        reply.stat.size = inode.size;
        reply.stat.type = inode.type;
    }
    int rc = UDP_Write(sd, &addr, (char*)&reply, sizeof(message_t));
    return rc;*/
}

int fs_write(int inum, char *buffer, int offset, int nbytes){
    message_t reply;
    UDP_Write(sd, &addr, (char*)&reply, sizeof(message_t));
    return -1;
}

int fs_read(int inum, char *buffer, int offset, int nbytes) {
    return 0;
}

int fs_creat(int inum, int type, char *name) {
    return 0;
}

int fs_unlink(int inum, char *name) {
    return 0;
}

int fs_shutdown(){
    message_t reply;
    fsync(fd);
	close(fd);
    UDP_Write(sd, &addr, (char*)&reply, sizeof(message_t));
	UDP_Close(port_number);
    printf("shutdown\n");
	exit(0);
	return 0;
}


/**********************
      SETTING UP
***********************/

int server_start(int port, char* img_path){
    sd = UDP_Open(port);
    if (sd <= 0) {
        printf("failed to bind UDP socket\n");
        return -1;
    }

    // read disk image, process meta data
    int file_fd = open(img_path, O_RDWR | O_APPEND);
    struct stat finfo;
    if (fstat(file_fd, &finfo) != 0) {
        perror("Fstat failed");
    }

    fs_img = mmap(NULL, finfo.st_size, MAP_SHARED, PROT_READ | PROT_WRITE, file_fd, 0);
    super_block = (super_t *) fs_img;
    inode_bitmap = fs_img + super_block->inode_bitmap_addr * UFS_BLOCK_SIZE;
    data_bitmap  = fs_img + super_block->data_bitmap_addr * UFS_BLOCK_SIZE;
    inodes = fs_img + super_block->inode_region_addr * UFS_BLOCK_SIZE;
    data = fs_img + super_block->data_region_addr * UFS_BLOCK_SIZE;
    // int max_inodes = super_block->inode_bitmap_len * sizeof(unsigned int) * 8;
    
    /* testing lookup */
    printf("result for pinum=0 .: %d\n", fs_lookup(0, "."));
    printf("result for pinum=0 ..: %d\n", fs_lookup(0, ".."));

    /* testing stat */


    /*
    while (1) {
        message_t *request = malloc(sizeof(message_t));
        message_t *response = malloc(sizeof(message_t));
        int fs_rc;

        printf("server:: waiting...\n");
        int rc = UDP_Read(sd, &addr, (char*) &request, sizeof(message_t));

        //int responseRet;
        printf("server:: read message [size:%d contents:(%s)]\n", rc, (char*)request);
        if (rc > 0) {
            if(rc ==2){
                fs_rc = fs_lookup(request->inum, request->name);
            }
            else if (rc ==3){
                fs_rc = fs_stat(request->inum);
            }
            else if(rc ==4){
                fs_rc = fs_write(request->inum,request->buffer,request->offset,request -> nbytes);
            }
            else if (rc ==5){
                fs_rc = fs_read(request->inum,request->buffer,request->offset,request -> nbytes);
            }
            else if (rc ==6){
                fs_rc = fs_creat(request ->inum,request->type,request->name);
            }
            else if (rc ==7){
                fs_rc = fs_unlink(request ->inum, request ->name);
            }
            else if(rc == 8){
                fs_rc = fs_shutdown();
            }
            else{
                printf("ERROR NO RC\n");
            }
        } else{
            printf("invalid RC\n");
        }
        response->rc = fs_rc;

    }
    */
    return 0;
}

int main(int argc, char *argv[]){
    signal(SIGINT, int_handler);
    assert(argc == 3);
	server_start(atoi(argv[1]), argv[2]);
    return 0;
}
