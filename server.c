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

int file_fd;
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
}

int fs_write(int inum, char *buffer, int offset, int nbytes){
    if (get_bit(inode_bitmap, inum) == 0) {
        return -1;
    }
    if (nbytes < 0 || nbytes > UFS_BLOCK_SIZE) {
        return -1;
    }
    inode_t *inode = inodes + inum * sizeof(inode_t);
    if (inode->type == MFS_DIRECTORY) {
        return -1;
    }

    // make sure we have enough data blocks, otherwise fail
    int total_size = offset + nbytes;
    int block_needed = total_size / UFS_BLOCK_SIZE + 1;
    for (int i = 0; i < block_needed ; i++) {
        if (inode->direct[i] == -1) {
            int free_block = get_free_data_block();
            if (free_block == -1) {
                return -1;
            }
            inode->direct[i] = free_block;
        }
    }

    // only need to consider writing/reading 1/2 block(s)
    int start_block = offset / UFS_BLOCK_SIZE;
    int start_byte = offset - start_block * UFS_BLOCK_SIZE;
    if (start_byte + nbytes >= UFS_BLOCK_SIZE) {
        if (inode->direct[start_block] == -1) {
            return -1;
        }
        void *data_addr = fs_img + inode->direct[start_block] * UFS_BLOCK_SIZE + start_byte;
        memcpy(data_addr, buffer, UFS_BLOCK_SIZE - start_byte);
        if (inode->direct[start_block+1] == -1) {
            return -1;
        }
        data_addr = fs_img + inode->direct[start_block+1] * UFS_BLOCK_SIZE;
        memcpy(data_addr, buffer + (UFS_BLOCK_SIZE - start_byte), nbytes-(UFS_BLOCK_SIZE - start_byte));
    } else {
        if (inode->direct[start_block] == -1) {
            return -1;
        }
        void *data_addr = fs_img + inode->direct[start_block] * UFS_BLOCK_SIZE + start_byte;
        memcpy(data_addr, buffer, nbytes);
    }
    if (inode->size < total_size) {
        inode->size = total_size;
    }
    return 0;
}

int fs_read(int inum, char *buffer, int offset, int nbytes) {
    if (get_bit(inode_bitmap, inum) == 0) {
        return -1;
    }
    inode_t *inode = inodes + inum * sizeof(inode_t);
    if (offset + nbytes > inode->size) {
        return -1;
    }

    int start_block = offset / UFS_BLOCK_SIZE;
    int start_byte = offset - start_block * UFS_BLOCK_SIZE;
    if (start_byte + nbytes >= UFS_BLOCK_SIZE) {
        // when spanning across two blocks
        // this code needs further testing
        if (inode->direct[start_block] == -1) {
            return -1;
        }
        void *data_addr = fs_img + inode->direct[start_block] * UFS_BLOCK_SIZE + start_byte;
        memcpy(buffer, data_addr, UFS_BLOCK_SIZE - start_byte);
        if (inode->direct[start_block+1] == -1) {
            return -1;
        }
        data_addr = fs_img + inode->direct[start_block+1] * UFS_BLOCK_SIZE;
        memcpy(buffer + (UFS_BLOCK_SIZE - start_byte), data_addr, nbytes-(UFS_BLOCK_SIZE - start_byte));
    } else {
        if (inode->direct[start_block] == -1) {
            return -1;
        }
        void *data_addr = fs_img + inode->direct[start_block] * UFS_BLOCK_SIZE + start_byte;
        memcpy(buffer, data_addr, nbytes);
    }
    return 0;
}

int fs_creat(int pinum, int type, char *name) {
    if (get_bit(inode_bitmap, pinum) == 0) {
        return -1;
    }
    return 0;
}

int fs_unlink(int inum, char *name) {
    return 0;
}

int fs_shutdown(){
    // message_t reply;
    // fsync(fd);
	// close(fd);
    // UDP_Write(sd, &addr, (char*)&reply, sizeof(message_t));
	// UDP_Close(port_number);
    // printf("shutdown\n");
	// exit(0);
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
    file_fd = open(img_path, O_RDWR | O_APPEND);
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
    
    // testing lookup
    // printf("result for pinum=0 .: %d\n", fs_lookup(0, "."));
    // printf("result for pinum=0 ..: %d\n", fs_lookup(0, ".."));
    // printf("result for pinum=0 ..: %d\n", fs_lookup(0, "a.jpg"));

    // testing stat
    // MFS_Stat_t stat;
    // int rc = fs_stat(0, &stat);
    // printf("rc = %d, type = %d, size = %d\n", rc, stat.type, stat.size);

    // test read (sanity)
    // char str[100];
    // int rc = fs_read(0, str, 32, 32);
    // printf("rc = %d, %s\n", rc, str);

    // uncomment for actual use
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
