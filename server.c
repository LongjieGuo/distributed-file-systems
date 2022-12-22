#include <stdio.h>
#include <unistd.h>
#include "udp.h"
#include "ufs.h"
#include "messages.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define BUFFER_SIZE (4096) //?
#define UDP_SIZE (10000)

int sd;
struct sockaddr_in addr;
int port_number;

int file_fd;
struct stat finfo;
void *fs_img;
super_t *super_block;
unsigned int *inode_bitmap;
unsigned int *data_bitmap;
void *inodes;
void *data;
int max_inodes;


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

void clear_bit(unsigned int *bitmap, int position) {
    int index = position / 32;
    int offset = 31 - (position % 32);
    bitmap[index] &= ~(0x1 << offset);
}

int get_free_inode() {
    for(int i = 0; i<super_block->num_inodes; i++) {
        int bit = get_bit(inode_bitmap, i);
        if(bit==0) {
            return i;
        }
    }
    return -1;
}

int get_free_data_block() {
    for(int i = 0; i<super_block->num_data; i++) {
        int bit = get_bit(data_bitmap, i);
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
    if (inode->type != UFS_DIRECTORY) {
        return -1;
    }

    for (int i = 0; inode->direct[i] != -1; i++) {
        dir_ent_t *dir_ent = fs_img + inode->direct[i] * UFS_BLOCK_SIZE;
        for (int j = 0; j < UFS_BLOCK_SIZE / sizeof(dir_ent_t); j++) {
            if (dir_ent->inum == -1) {
                break;
            }
            if (strcmp(dir_ent->name, name) == 0) {
                return dir_ent->inum;
            }
            dir_ent++;
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
    if (inum < 0 || inum > max_inodes) {
        return -1;
    }
    if (get_bit(inode_bitmap, inum) == 0) {
        return -1;
    }
    if (nbytes < 0 || nbytes > UFS_BLOCK_SIZE) {
        return -1;
    }
    inode_t *inode = inodes + inum * sizeof(inode_t);
    if (inode->type == UFS_DIRECTORY) {
        return -1;
    }

    // make sure we have enough data blocks, otherwise fail
    int total_size = offset + nbytes;
    int block_needed = total_size / UFS_BLOCK_SIZE;
    if (block_needed > DIRECT_PTRS) {
        return -1;
    }
    for (int i = 0; i < block_needed ; i++) {
        if (inode->direct[i] == -1) {
            int free_block = get_free_data_block();
            if (free_block == -1) {
                return -1;
            }
            set_bit(data_bitmap, free_block);
            inode->direct[i] = free_block + super_block->data_region_addr;
        }
    }

    // only need to consider writing/reading 1/2 block(s)
    int start_block = offset / UFS_BLOCK_SIZE;
    int start_byte = offset - start_block * UFS_BLOCK_SIZE;
    if (start_byte + nbytes > UFS_BLOCK_SIZE) {
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
    if (start_byte + nbytes > UFS_BLOCK_SIZE) {
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
    if (get_bit(inode_bitmap, pinum) == 0 || strlen(name) >= 28) {
        return -1;
    }
    inode_t *pinode = inodes + pinum * sizeof(inode_t);
    if (pinode->type != UFS_DIRECTORY) {
        return -1;
    }
    // does not distinguish betwwen file and directory
    if (fs_lookup(pinum, name) != -1) {
        return 0;
    }

    int inum = get_free_inode();
    if (inum == -1) {
        return -1;
    }

    // create the new inode
    inode_t *inode = inodes + inum * sizeof(inode_t);
    set_bit(inode_bitmap, inum);
    inode->type = type;
    inode->size = 0;
    for (int i = 0; i < DIRECT_PTRS; i++) {
        inode->direct[i] = -1;
    }
    // for file
    if (type == UFS_DIRECTORY) {
        inode->size = 2 * sizeof(dir_ent_t);
        int data_block = get_free_data_block();
        if (data_block == -1) {
            return -1;
        }
        inode->direct[0] = data_block + super_block->data_region_addr;
        dir_ent_t *dir_ent = fs_img + inode->direct[0] * UFS_BLOCK_SIZE;
        for (int i = 0; i < UFS_BLOCK_SIZE / sizeof(dir_ent_t); i++) {
            dir_ent->inum = -1;
            dir_ent++;
        }
        // create two default directory entries: . and ..
        dir_ent = fs_img + inode->direct[0] * UFS_BLOCK_SIZE;
        dir_ent->inum = inum;
        strcpy(dir_ent->name, ".");
        dir_ent++;
        dir_ent->inum = pinum;
        strcpy(dir_ent->name, "..");
    }

    // update parent directory entry
    // be careful when blocks are not enough
    int i;
    dir_ent_t *dir_ent;
    
    for (i = 0; pinode->direct[i] != - 1; i++) {
        dir_ent = fs_img + pinode->direct[i] * UFS_BLOCK_SIZE;
        for (int j = 0; j < UFS_BLOCK_SIZE / sizeof(dir_ent_t); j++) {
            if (dir_ent->inum == -1) {
                strcpy(dir_ent->name, name);
                dir_ent->inum = inum;
                pinode->size += sizeof(dir_ent_t);
                return 0;
            }
            dir_ent++;
        }
    }

    // no space left, allocate more
    if (i >= DIRECT_PTRS) {
        return -1;
    }
    int free_block = get_free_data_block();
    if (free_block == -1) {
        return -1;
    }
    set_bit(data_bitmap, free_block);
    pinode->direct[i] = free_block + super_block->data_region_addr;
    dir_ent = fs_img + pinode->direct[i] * UFS_BLOCK_SIZE;
    for (int j = 0; j < UFS_BLOCK_SIZE / sizeof(dir_ent_t); j++) {
        dir_ent->inum = -1;
        dir_ent++;
    }
    dir_ent = fs_img + pinode->direct[i] * UFS_BLOCK_SIZE;
    strcpy(dir_ent->name, name);
    dir_ent->inum = inum;
    pinode->size += sizeof(dir_ent_t);
    return 0;
}

int fs_unlink(int pinum, char *name) {
    if (get_bit(inode_bitmap, pinum) == 0) {
        return -1;
    }
    inode_t *pinode = inodes + pinum * sizeof(inode_t);
    for (int i = 0; pinode->direct[i] != -1; i++) {
        dir_ent_t *dir_ent = fs_img + pinode->direct[i] * UFS_BLOCK_SIZE;
        for (int j = 0; j < UFS_BLOCK_SIZE / sizeof(dir_ent_t); j++) {
            if (strcmp(dir_ent->name, name) == 0) {
                inode_t *target_inode = inodes + dir_ent->inum * sizeof(inode_t);
                if (target_inode->type == UFS_REGULAR_FILE) {
                    // clear data bitmap
                    // for (int k = 0; target_inode->direct[k] != -1; k++) {
                    //     clear_bit(data_bitmap, target_inode->direct[k] - 4);
                    // }
                    // clear inode bitmap
                    clear_bit(inode_bitmap, dir_ent->inum);
                } else {
                    // when it's a directory
                    if (target_inode->size > 2 * sizeof(dir_ent_t)) {
                        return -1;
                    }
                    // TODO: (not tested) clear bitmaps
                }
                dir_ent->inum = -1;
                pinode->size -= sizeof(dir_ent_t);
                return 0;
            }
            dir_ent++;
        }
    }
    return 0;
}

int fs_shutdown(){
    msync(fs_img, finfo.st_size, MS_SYNC);
	close(file_fd);
	exit(0);
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
    
    if (fstat(file_fd, &finfo) != 0) {
        perror("Fstat failed");
    }
    fs_img = mmap(NULL, finfo.st_size, MAP_PRIVATE, PROT_READ | PROT_WRITE, file_fd, 0);
    super_block = (super_t *) fs_img;
    inode_bitmap = fs_img + super_block->inode_bitmap_addr * UFS_BLOCK_SIZE;
    data_bitmap  = fs_img + super_block->data_bitmap_addr * UFS_BLOCK_SIZE;
    inodes = fs_img + super_block->inode_region_addr * UFS_BLOCK_SIZE;
    data = fs_img + super_block->data_region_addr * UFS_BLOCK_SIZE;
    max_inodes = super_block->inode_bitmap_len * sizeof(unsigned int) * 8;

    fs_creat(0, MFS_DIRECTORY, "test");
    int pinum = fs_lookup(0, "test");
    fs_creat(pinum, MFS_DIRECTORY, "0");
    printf("rc=%d\n", fs_lookup(pinum, "0"));
    
    /*
    while (1) {
        message_t *request = malloc(sizeof(message_t));
        message_t *response = malloc(sizeof(message_t));
        
        int fs_rc;

        if (UDP_Read(sd, &addr, (char*) request, sizeof(message_t)) < 0) {
            printf("server:: failed to receive\n");
            exit(1);
        }

        if(request->request_type ==2){
            fs_rc = fs_lookup(request->inum, request->name);
        }
        else if (request->request_type ==3){
            MFS_Stat_t *stat = malloc(sizeof(MFS_Stat_t));
            fs_rc = fs_stat(request->inum, stat);
            // copy stat
            memcpy(&response->stat, stat, sizeof(MFS_Stat_t));
        }
        else if(request->request_type ==4){
            fs_rc = fs_write(request->inum, request->buffer, request->offset, request -> nbytes);
        }
        else if (request->request_type ==5){
            char buffer[BUFFER_SIZE];
            fs_rc = fs_read(request->inum, buffer, request->offset, request -> nbytes);
            // copy buffer
            memcpy(response->buffer, buffer, request->nbytes);
        }
        else if (request->request_type ==6){
            fs_rc = fs_creat(request->inum, request->type, request->name);
        }
        else if (request->request_type ==7){
            fs_rc = fs_unlink(request->inum, request->name);
        }
        else if(request->request_type == 8){
            fs_rc = fs_shutdown();
        }
        else{
            printf("ERROR NO RC\n");
        }
        msync(fs_img, finfo.st_size, MS_SYNC);
        response->rc = fs_rc;

        int udp_rc = UDP_Write(sd, &addr, (char*)response, sizeof(message_t));
        if (udp_rc < 0) {
            printf("server:: failed to send, rc=%d, addr=%p, response=%p\n", udp_rc, &addr, response);
            printf("%s", strerror(errno));
            exit(1);
        }
    }*/
    return 0;
}

int main(int argc, char *argv[]){
    signal(SIGINT, int_handler);
    assert(argc == 3);
	server_start(atoi(argv[1]), argv[2]);
    return 0;
}
