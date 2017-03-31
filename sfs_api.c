#include "sfs_api.h"
#define NUM_INODES   200
#define NUM_INODES_PER_BLOCK 16
#define NUM_BLOCKS   1024
#define BLOCK_SIZE   1024
#define NUM_OF_FILES 64     //per Data block
#define MAX_FILESIZE 10
#define MAX_FILEDESCRIPTABLE_SIZE 64
#define FILE_NAME "Zahra"

typedef struct _inode_t { //1 inode 64 bytes
    int size;
    int direct[14];
    int indirect;
} inode_t;

typedef struct _superBlock_t {
    unsigned int magic;
    int blockSize;
    int fileSysSize; //#blocks
    int numOfInodes;
    inode_t root; // root is j node;
} superBlock_t;

typedef struct _rootDirEntry_t { //root directory with 16 inodes/block                                
    int inodeNum;
    char filenames[10];
} rootDir_t;

typedef struct _rootDirTable_t {                    // block of file entries,each file takes 16 bytes, hence 1024/16=64
    rootDir_t fileEntries[NUM_OF_FILES];
} rootDirTable_t;

typedef struct _fileDescriptorEntry_t { //organizes the files
    int inodeIndex;
    inode_t inode;
    int readPointer;
    int writePointer;
} fileDescriptorEntry_t;

typedef struct _fileDescriptorTable_t {
    fileDescriptorEntry_t table[MAX_FILEDESCRIPTABLE_SIZE];

} fileDescriptorTable_t;

typedef struct _inodeBlock_t {
    inode_t inodes[16];
} inodeBlock_t;

typedef struct _DataBlock_t {
    unsigned char blocks[1024]; // for FBM and WM
} DataBlock_t;

int inodeTracker[NUM_INODES];

superBlock_t sb;
rootDirTable_t rDT; //root directory table
fileDescriptorTable_t fDT; //file descriptor table
inodeBlock_t iNodeBlock;
DataBlock_t FBM; // Free Bit MAP
DataBlock_t WM; // Write Mask
inode_t iNodeTracker[200];

void init_superBlock_t() {
    sb.magic = 0xABCD005;
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

void init_fileDescriptorTable_t() {
    for (int i = 0; i < NUM_OF_FILES; i++) {
        fDT.table[i].inode.size = -1;
        fDT.table[i].inodeIndex = -1;
        fDT.table[i].readPointer = -1;
        fDT.table[i].writePointer = -1;

    }
}

void init_FirstInodeBlock_t() {
    for (int i = 0; i < 16; i++) {
        iNodeBlock.inodes[i].size = -1; // means inode is not used.
        for (int j = 0; j < 14; j++) {
            iNodeBlock.inodes[i].direct[j] = -1;
        }

        iNodeBlock.inodes[i].indirect = -1;
    }
}

void init_freeBitMap_t() {
    for (int i = 0; i < NUM_BLOCKS; i++) {
        FBM.blocks[i] = 1; // means inode is not used.
    }
}

void init_iNode_tracker_t() {
    for (int i = 1; i < 200; i++) {
        iNodeTracker[i].size = -1; // means inode is not used.
    }
}

void mkssfs(int fresh) {


    // INITIALIZING
    init_superBlock_t();
    init_rootDirTable_t();
    init_fileDescriptorTable_t();
    init_FirstInodeBlock_t();
    init_freeBitMap_t();
    init_iNode_tracker_t();
    
    printf("initialized\n");
    FBM.blocks[0] = 0; // superblock
    FBM.blocks[1] = 0; // FBM
    FBM.blocks[2] = 0; // WM
    FBM.blocks[3] = 0; // first inode block
    FBM.blocks[4] = 0; // first root directory block

    sb.root.direct[0] = 3;
    iNodeBlock.inodes[0].direct[0] = 4;

    //OPEN A NEW DISK
    if (!fresh) {

        if (init_disk(FILE_NAME, BLOCK_SIZE, NUM_BLOCKS) == -1)
            printf("CANNOT OPEN the file");

    } else {
        if (init_fresh_disk(FILE_NAME, BLOCK_SIZE, NUM_BLOCKS) == -1)
            printf("CANNOT OPEN");
        else {
            // writing to the disk
            printf("I am writing\n");
            write_blocks(0, 1, &sb);
            write_blocks(1, 1, &FBM);
            write_blocks(2, 1, &WM);
            write_blocks(3, 1, &iNodeBlock);
            write_blocks(4, 1, &rDT);
            printf("end\n");
            

        }
    }

}

int ssfs_fopen(char *name) {
    
    int i;
    int fdindex;
    int nodeIndex;
    int tempinodenum;
    int blocknum;
    inode_t ninode;
    inodeBlock_t *nblock;
    fileDescriptorEntry_t newFileEntry;
   
    // Lookup for file if it currently resides in our table
    for (i = 0; i < NUM_OF_FILES; i++) {
        printf("Looking for the file\n");
        if (strcmp(name, rDT.fileEntries[i].filenames) == 0) {
            tempinodenum = rDT.fileEntries[i].inodeNum;                         // find the corresponding inode number
            blocknum= sb.root.direct[tempinodenum/ NUM_INODES_PER_BLOCK];      // find the block number that corresponds to the inode
            read_blocks(blocknum, 1, &ninode);                                   // read the block get the inode details                                                                    // want to use the current ninode.
            newFileEntry.inodeIndex= tempinodenum;
            newFileEntry.inode= ninode;
            newFileEntry.readPointer= 0;
            newFileEntry.writePointer= ninode.size;
            for (int j = 0; j < MAX_FILEDESCRIPTABLE_SIZE; j++) {
                if (fDT.table[j].inodeIndex==-1) {
                    fDT.table[j] = newFileEntry;
                    fdindex=j;
                    return fdindex;
                }
            }
            
        }
            // if it doesnt then create a file
        else {
            for (i = 0; i < NUM_OF_FILES; i++) {
                printf("Not Found!! Creating\n");
                if (strcmp(rDT.fileEntries[i].filenames,"\0") == 0) {                       // search for an available entry in the root directory table
                    rootDir_t new;                                                  // create a new entry space
                    rDT.fileEntries[i] = new;                                      // allocate this space to the new entry
                    strcpy(new.filenames, name);                                  // name the file
                    //need to allocate a data block for the inode;
                    //need to find an inode for this new file.
                    printf("here1\n");
                    for (int j = 1; j < NUM_INODES; j++) {
                        if (iNodeTracker[j].size == -1) {
                            iNodeTracker[j].size = 0; //is used now
                            nodeIndex = j;
                            break;
                        }
                    }
                    printf("here2\n");
                    new.inodeNum = nodeIndex;                                          // assign  the inode to the new entry
                    blocknum = sb.root.direct[nodeIndex/ NUM_INODES_PER_BLOCK]; 
                    nblock = malloc(sizeof (inodeBlock_t));                            // read the block we are gonna allocate to
                    read_blocks(blocknum, 1, nblock);
                    inode_t *currInode = &(nblock->inodes[nodeIndex % 16]);
                    currInode->size=0;
                    printf("here3\n");
                    write_blocks(blocknum, 1, nblock);                                // creates a file descriptor for the new entry and updates the content.
                    printf("Block Num%i\n",blocknum);
                    newFileEntry.inode = *currInode;
                    newFileEntry.inodeIndex = nodeIndex;
                    newFileEntry.readPointer = 0;
                    newFileEntry.writePointer = 0;
                     printf("Nodeindex %i\n",nodeIndex);
                    for (int j = 0; j < MAX_FILEDESCRIPTABLE_SIZE; j++) {
                        if (fDT.table[j].inodeIndex==-1) {
                        fDT.table[j] = newFileEntry;
                        fdindex=j;
                        printf("FDindex %i\n",fdindex);
                        printf("Done\n");
                        return fdindex;
                       
                    }

                }

            }
        }


        return i;
    }
}
}
int ssfs_fclose(int fileID) {
    
    if(fileID<0){
        printf("INDEX NOT DEFINED");
        return-1;
    }
    if(fileID>64){
        printf("Too Many files, cannot handle");
        return -1;
    }
    else{
    
    
    
    }
    
   
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


int main(){
    mkssfs(1);
    ssfs_fopen("zahra");
    ssfs_fopen("fatuu");
    ssfs_fopen("hassu");
    ssfs_fopen("zahra");
    return 0;
}
