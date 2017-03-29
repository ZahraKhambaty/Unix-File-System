#include "sfs_api.h"

#define BLOCK_SIZE 1024
#define NUM_BLOCKS 1024
#define FILE_NAME "Zahra"

typedef struct _superBlock_t {
    unsigned char magic [4];
    int fileSysSize; //#blocks
    block_t blocks[1024];
    inode_t root; // root is j node
    inode_t shadow[4];
    inode_t lastShadow;
} superBlock_t;

typedef struct _inodeBlock_t {
    inode_t inodes[16];
} inodeBlock_t;

typedef struct _inode_t { //1 inode 64 bytes
    int size;
    int direct[14];
    int indirect;
} inode_t;

typedef struct _block_t {
    unsigned char mapBlocks[1024]; // for FBM and WM
} block_t;

typedef struct _rootDir_t { //root directory with 16 inodes/block                                
    int inodeNum;
    char filenames[200];
} rootDir_t;

typedef struct _files_t{
    int readPointer;
    int writePointer;
}files_t;

// initialize free bit map(FBM) and WriteMap(WM)

const int SUPERBLOCK_NUM= 0;          // diskBlock where the super block is stored
const int INODESTORE_BLOCK_NUM = 1;   // the disk block where the i nodes are stored
const int  ROOTDIR_INODE_NUMBER = 1;  // hared coded inode number for the root directory

block_t FBM;
block_t WM;



void mkssfs(int fresh) {
    //OPEN A NEW DISK
    if (fresh) {
        
        if (init_fresh_disk(FILE_NAME, BLOCK_SIZE, NUM_BLOCKS) == -1)
            printf("CANNOT OPEN");

    }
    else {
        if (init_fresh_disk(FILE_NAME, BLOCK_SIZE, NUM_BLOCKS) == -1)
            printf("CANNOT OPEN");
    }

}

int ssfs_fopen(char *name) {
    return 0;
}

int ssfs_fclose(int fileID) {
    return 0;
}

int ssfs_frseek(int fileID, int loc) {
    return 0;
}

int ssfs_fwseek(int fileID, int loc) {
    return 0;
}

int ssfs_fwrite(int fileID, char *buf, int length) {
    return 0;
}

int ssfs_fread(int fileID, char *buf, int length) {
    return 0;
}

int ssfs_remove(char *file) {
    return 0;
}
