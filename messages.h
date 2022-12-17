#ifndef __message_h__
#define __message_h__
#define MFS_INIT (1)
#define MFS_LOOKUP (2)
#define MFS_STAT (3)
#define MFS_WRITE (4)
#define MFS_READ (5)
#define MFS_CRET (6)
#define MFS_UNLINK (7)
#define MFS_SHUTDOWN (8)



#include "mfs.h"
#include "ufs.h"


typedef struct {
    MFS_Stat_t stat;
    int requestType;
    int inum;
    int type;       
    char name[28];
    char buffer[MFS_BLOCK_SIZE];
    int offset;
    int nbytes;
} message_t;

#endif // __message_h__