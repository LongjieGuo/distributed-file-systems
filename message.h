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

#define BUFFER_SIZE (1000)

typedef struct {
    int mtype; // message type from above
    int rc;    // return code
    int inum;
    int pinum;
    char name[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];
    /* write and read */
    int offset;
    int nbytes;
    int crtype; // for create
} message_t;

#endif // __message_h__
