#include "sfs_api.h"
#define NUM_INODES   200
#define NUM_BLOCKS   1024
#define NUM_OF_FILES 64     //per Data block
#define MAX_FILESIZE 11
#define FILE_NAME "Zahra"

typedef struct _superBlock_t {
    unsigned char magic [4];
    int blockSize;
    int fileSysSize; //#blocks
    int numOfInodes;
    inode_t* root; // root is j node;
} superBlock_t;

typedef struct _rootDirTable_t { // block of file entries,each file takes 16 bytes, hence 1024/16=64
    rootDir_t fileEntries[NUM_OF_FILES];
} rootDirTable_t;

typedef struct _rootDirEntry_t { //root directory with 16 inodes/block                                
    int inodeNum;
    char filenames[10];
} rootDir_t;

typedef struct _fileDescriptors_t { //organizes the files
    inode_t* inode;
    int inodeIndex;
    int readPointer;
    int writePointer;
} filesDescriptors_t;

typedef struct _inodeBlock_t {
    inode_t inodes[16];
} inodeBlock_t;

typedef struct _inode_t { //1 inode 64 bytes
    int size;
    int direct[14];
    int indirect;
} inode_t;

typedef struct _DataBlock_t {
    unsigned char blocks[1024]; // for FBM and WM
} DataBlock_t;

int inodeTracker[NUM_INODES];

superBlock_t sb;
rootDirTable_t rDT;
inodeBlock_t iNodeBlock;
DataBlock_t FBM; // Free Bit MAP
DataBlock_t WM; // Write Mask

void init_superBlock_t() {
    sb.magic = 4;
    sb.fileSysSize = NUM_BLOCKS * BLOCK_SIZE; //#blocks
    sb.blockSize = 1024;
    sb.numOfInodes = 200;
    // root is j node in the super block;
    for (int i = 0; i < 14; i++) {
        sb.root.direct[i] = -1;
        sb.root.size = -1;
    }
}

void init_rootDirTable_t() {
    for (int i = 0; i < NUM_OF_FILES; i++) {
        rDT.fileEntries[i].inodeNum = -1;
        rDT.fileEntries[i].filenames[0] = '\0'; // means inode is not used.
    }
}

void init_FirstInodeBlock_t() {
    for (int i = 0; i < 14; i++) {
        iNodeBlock.inodes[i].size = -1; // means inode is not used.
        iNodeBlock.inodes[i].direct = -1;
        iNodeBlock.inodes[i].indirect = -1;
    }
}

void init_freeBitMap_t() {
    for (int i = 0; i < NUM_BLOCKS; i++) {
        FBM.blocks[i] = 1; // means inode is not used.
    }
}

void mkssfs(int fresh) {


    // INITIALIZING
    init_superBlock_t();
    init_rootDirTable_t();
    init_FirstInodeBlock_t();
    init_freeBitMap_t();

    FBM.blocks[0] = 0; // superblock
    FBM.blocks[1] = 0; // FBM
    FBM.blocks[2] = 0; // WM
    FBM.blocks[3] = 0; // first inode block
    FBM.blocks[4] = 0; // first root directory block

    sb.root->direct[0] = iNodeBlock;             
    iNodeBlock.inodes[0].direct[0] = 4;

    //OPEN A NEW DISK
    if (fresh) {

        if (init_disk(FILE_NAME, BLOCK_SIZE, NUM_BLOCKS) == -1)
            printf("CANNOT OPEN the file");

    } else {
        if (init_fresh_disk(FILE_NAME, BLOCK_SIZE, NUM_BLOCKS) == -1)
            printf("CANNOT OPEN");
        else {
            // writing to the disk
            write_blocks(0, 1, &sb);
            write_blocks(1, 1, &FBM);
            write_blocks(2, 1, &WM);
            write_blocks(3, 1, &iNodeBlock);
            write_blocks(4, 1, &rDT);

        }
    }

}

int ssfs_fopen(char *name) {
    
    int i;
    // Lookup for file if it currently resides in our table
    for(i=0;i<NUM_OF_FILES;i++){
        if(rDT.fileEntries[i]= '\0'){
            continue;
        }
        if(strncmp(name, rDT.fileEntries[i].filenames, NUM_OF_FILES)==0){
            return i;
        }
    }
    printf("Err!! Not Found!! Creating");
    // if it doesnt then create a file
    for(i=0;i<NUM_OF_FILES;i++){
        if(rDT.fileEntries[i]= '\0'){
            filesDescriptors_t *desc = malloc(sizeof(filesDescriptors_t));
            strncpy(desc->*name, i, MAX_FILESIZE);
            desc->inodeIndex =i;
            desc->readPointer =i;
            desc->writePointer =i % NUM_BLOCKS;
            rDT.fileEntries[i]= desc;
            return i;
        }
    }
 
    
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
