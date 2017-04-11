#include "disk_emu.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define RDIR_BLOCK_ADDRESS          17
#define RDIR_BLOCK_SIZE             4
#define INODE_BLOCK_ADDRESS         3
#define INODE_BLOCK_SIZE            13
#define NUM_INODES                  200
#define NUM_INODES_PER_BLOCK        16
#define NUM_INODE_DIRECT            14
#define NUM_BLOCKS                  1024
#define BLOCK_SIZE                  1024
#define NUM_OF_FILES                64     //per Data block
#define MAX_FILESIZE                10
#define MAX_FILEDESCRIPTABLE_SIZE   200
#define FILE_NAME "Zahra"

typedef struct _inode_t { //1 inode 64 bytes
    int size;
    int direct[14];
    int indirect;
} inode_t;

typedef struct _superBlock_t {
    unsigned int magic;
    int blockSize;
    inode_t root; // root is j node;
} superBlock_t;

typedef struct _rootDirEntry_t { //root directory with 16 inodes/block                                
    int inodeNum;
    char filename[10];
} rootDir_t;

typedef struct _rootDirTable_t { // block of file entries,each file takes 16 bytes, hence 1024/16=64
    rootDir_t fileEntries[NUM_OF_FILES];
} rootDirTable_t;

typedef struct _fileDescriptorEntry_t { //organizes the files
    int fileopen;
    int inodeNum;
    int readPointer;
    int writePointer;
    inode_t inode;
} fileDescriptorEntry_t;

typedef struct _fileDescriptorTable_t {
    fileDescriptorEntry_t table[MAX_FILEDESCRIPTABLE_SIZE];
} fileDescriptorTable_t;

typedef struct _inodeBlock_t {
    inode_t inodes[16];
} inodeBlock_t;

typedef struct _DataBlock_t {
    unsigned char data[1024]; // for FBM and WM
} DataBlock_t;


superBlock_t sb;
rootDirTable_t rDT; //root directory table
fileDescriptorTable_t fDT; //file descriptor table
inodeBlock_t iNodeBlock;
DataBlock_t FBM; // Free Bit MAP
DataBlock_t WM; // Write Mask
inode_t currInode;
DataBlock_t cache;
char bufferin[1024];


//Functions you should implement. 
//Return -1 for error besides mkssfs
void mkssfs(int fresh);
int ssfs_fopen(char *name);
int ssfs_fclose(int fileID);
int ssfs_frseek(int fileID, int loc);
int ssfs_fwseek(int fileID, int loc);
int ssfs_fwrite(int fileID, char *buf, int length);
int ssfs_fread(int fileID, char *buf, int length);
int ssfs_remove(char *file);
int ssfs_commit();
int ssfs_restore(int cnum);
