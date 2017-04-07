#include "sfs_api.h"
#include "disk_emu.h"
#include <string.h>
#include <stdlib.h>
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

typedef struct _rootDirTable_t { // block of file entries,each file takes 16 bytes, hence 1024/16=64
    rootDir_t fileEntries[NUM_OF_FILES];
} rootDirTable_t;

typedef struct _fileDescriptorEntry_t { //organizes the files
    int fdindex;
    int inodeIndex;
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
    unsigned char blocks[1024]; // for FBM and WM
} DataBlock_t;

int inodeTracker[NUM_INODES];

superBlock_t sb;
rootDirTable_t rDT; //root directory table
fileDescriptorTable_t fDT; //file descriptor table
inodeBlock_t iNodeBlock;
DataBlock_t FBM; // Free Bit MAP
DataBlock_t WM; // Write Mask
inode_t ninode;
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
        rDT.fileEntries[i].inodeNum = -1; // means inode is not used.
        for (int k = 0; k < 10; k++) {
            rDT.fileEntries[i].filenames[k] = '\0';
        }
    }
}

void init_fileDescriptorTable_t() {
    for (int i = 0; i < NUM_OF_FILES; i++) {

        fDT.table[i].fdindex = -1;
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

void init_Inodes_t() {

    ninode.size = -1; // means inode is not used.
    for (int j = 0; j < 14; j++) {
        ninode.direct[j] = -1;
    }

    ninode.indirect = -1;
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
    init_Inodes_t();

    printf("initialized\n");
    FBM.blocks[0] = 0; // superblock
    FBM.blocks[1] = 0; // FBM
    FBM.blocks[2] = 0; // WM
    FBM.blocks[3] = 0; // first inode block
    FBM.blocks[4] = 0; // first root directory block

    sb.root.direct[0] = 3; // points to 1st inode block 
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
            write_blocks(3, 13, &iNodeBlock);
            write_blocks(14, 4, &rDT);
            printf("end\n");


        }
    }

}

int ssfs_fopen(char *name) {

    int i;
    int nodeIndex;
    int tempinodenum;
    int blocknum;
    // inode_t ninode;
    inodeBlock_t* iNodeBlock;
    fileDescriptorEntry_t newFileEntry;
    int found = 0;


    // Lookup for file if it currently resides in our table
    // printf("Looking for the file\n");
    for (i = 0; i < 200; i++) {
        //    printf("Comparing to %s\n", rDT.fileEntries[i].filenames);
        if (strcmp(name, rDT.fileEntries[i].filenames) == 0) {
            found = 1;
            // find the corresponding inode number
            tempinodenum = rDT.fileEntries[i].inodeNum;
            printf("tempinodenum%i\n", tempinodenum);
            blocknum = sb.root.direct[tempinodenum / NUM_INODES_PER_BLOCK]; // find the block number that corresponds to the inode
            printf("In if\n");
            iNodeBlock = malloc(sizeof (inodeBlock_t));
            read_blocks(blocknum, 1, iNodeBlock);
            // read the block get the inode details
            // want to use the current ninode.
            //      printf("In if\n");
            newFileEntry.inodeIndex = tempinodenum;
            newFileEntry.inode = ninode;
            //      printf("In if\n");
            newFileEntry.readPointer = 0;
            newFileEntry.writePointer = ninode.size;
            //     printf("In if\n");
            for (int j = 0; j < MAX_FILEDESCRIPTABLE_SIZE; j++) {
                //        printf("In for\n");
                if (fDT.table[j].fdindex == -1) {
                    //           printf("in if in for\n");
                    fDT.table[j] = newFileEntry;
                    fDT.table[j].fdindex = j;
                    //         printf("changed index = %d\n", fDT.table[j].fdindex);
                    return fDT.table[j].fdindex;
                }
            }
        }
    }
    // if it doesnt then create a file c
    if (!found) {
        //  printf("Not Found!! Creating\n");
        for (i = 0; i < NUM_OF_FILES; i++) {
            // search for an available entry in the root directory table
            if (strcmp(rDT.fileEntries[i].filenames, "\0") == 0) {
                // create a new entry space
                rootDir_t *new = &rDT.fileEntries[i];
                // allocate this space to the new entry
                strcpy(new->filenames, name); // name the file
                //need to allocate a data block for the inode;
                //need to find an inode for this new file.
                //         printf("here1\n");
                for (int j = 1; j < NUM_INODES; j++) {
                    if (iNodeTracker[j].size == -1) {
                        iNodeTracker[j].size = 0; //is used now
                        nodeIndex = j;
                        break;
                    }
                }
                //         printf("here2\n");
                new->inodeNum = nodeIndex;
                // assign  the inode to the new entry
                // access root directory block

                blocknum = sb.root.direct[nodeIndex / NUM_INODES_PER_BLOCK];
                iNodeBlock = malloc(sizeof (inodeBlock_t)); // read the block we are gonna allocate to
                read_blocks(blocknum, 1, iNodeBlock);
                inode_t *currInode = &(iNodeBlock->inodes[nodeIndex % 16]);
                currInode->size = 0;
                //         printf("here3\n");
                write_blocks(blocknum, 1, iNodeBlock); // creates a file descriptor for the new entry and updates the content.
                //        printf("Block Num%i\n", blocknum);
                FBM.blocks[blocknum] = 0; // updating the fbm to indicate the data block is used
                newFileEntry.inode = *currInode;
                newFileEntry.inodeIndex = nodeIndex;
                newFileEntry.readPointer = 0;
                newFileEntry.writePointer = 0;
                write_blocks(4, 1, &rDT);
                //       printf("Nodeindex %i\n", nodeIndex);
                for (int j = 0; j < MAX_FILEDESCRIPTABLE_SIZE; j++) {
                    if (fDT.table[j].fdindex == -1) {

                        newFileEntry.fdindex = j;
                        fDT.table[j] = newFileEntry;

                        //             printf("FDindex %i\n", fDT.table[j].fdindex);
                        //             printf("Done\n");
                        return fDT.table[j].fdindex;

                    }
                }

            }
        }
    }
    return 0;
}

int ssfs_fclose(int fileID) {

    if (fileID < 0) {
        printf("INDEX NOT DEFINED\n");
        return-1;
    }
    if (fileID >= 200) {
        printf("Too Many files, cannot handle\n");
        return -1;
    }
    //  printf("Finding\n");
    //  printf("Closing\n");
    //  printf("fDT.index %i\n", fDT.table[fileID].fdindex);
    //  printf("fDT.inode.size %i\n", fDT.table[fileID].inode.size);
    //  printf("fDT.inodeIndex %i\n", fDT.table[fileID].inodeIndex);
    if (fDT.table[fileID].fdindex != -1) {
        fDT.table[fileID].fdindex = -1;
        fDT.table[fileID].inode.size = -1;
        fDT.table[fileID].readPointer = -1;
        fDT.table[fileID].writePointer = -1;
        fDT.table[fileID].inodeIndex = -1;
        return 0;
    } else {

        return -1;
    }
}


//   printf("fDT.index %i\n", fDT.table[fileID].fdindex);

int ssfs_frseek(int fileID, int loc) {

    if (fileID < 0) {
        printf("INDEX NOT DEFINED");
        return-1;
    }
    if (fileID > 200) {
        printf("Too Many files, cannot handle");
        return -1;
    } else {
        fDT.table[fileID].readPointer = loc;
        return 0;
    }


}

int ssfs_fwseek(int fileID, int loc) {


    if (fileID < 0) {
        printf("INDEX NOT DEFINED");
        return-1;
    }
    if (fileID > 64) {
        printf("Too Many files, cannot handle");
        return -1;
    } else {
        fDT.table[fileID].writePointer = loc;
        return 0;
    }

}

int ssfs_fwrite(int fileID, char *buf, int length) {

    DataBlock_t *cache;
    inodeBlock_t *iNodeBlock;
    int blocknum;
    int byteswritten;
    int bitsoverwritten;
    char *bufferin;

    if (fileID < 0) {
        printf("INDEX NOT DEFINED");
        return -1;
    }
    if (fileID > 200) {
        printf("Too Many files, cannot handle");
        return -1;
    } else {

        if (fDT.table[fileID].inode.size == 0) {
            printf("Writing to a newly created file");
            iNodeBlock = malloc(sizeof (inodeBlock_t));
            blocknum = sb.root.direct[fDT.table[fileID].inodeIndex / NUM_INODES_PER_BLOCK];
            read_blocks(blocknum, 1, iNodeBlock);
            inode_t *currInode = &(iNodeBlock->inodes[fDT.table[fileID].inodeIndex % 16]);
            if (length > 1023) {
                for (int j = 0; j < length / 1024; j++) {
                    write_blocks(currInode->direct[j], 1, buf);
                    buf += 1024;
                }
            } else {
                write_blocks(currInode->direct[0], 1, buf);
            }
            fDT.table[fileID].writePointer = length;

        } else {
            printf("Writing to an existing file\n");
            iNodeBlock = malloc(sizeof (inodeBlock_t));
            blocknum = sb.root.direct[fDT.table[fileID].inodeIndex / NUM_INODES_PER_BLOCK];
            read_blocks(blocknum, 1, iNodeBlock);
            inode_t *currInode = &(iNodeBlock->inodes[fDT.table[fileID].inodeIndex % 16]);
            cache = malloc(sizeof (DataBlock_t));
            bufferin = malloc(1024 * sizeof (char));
            int num_direct = (fDT.table[fileID].writePointer) / 1024;
            //TODO need loop for other cases
            //there is space in the current data block
            if (length <= (1024 - (fDT.table[fileID].writePointer))) {
                //read the data block from disk
                read_blocks(currInode->direct[num_direct], 1, cache);
                //memcopy the buffer starting write pointer
                memcpy(cache + fDT.table[fileID].writePointer, buf, length);
                //push back to disk
                write_blocks(currInode->direct[num_direct], 1, cache);
                //record the number of bytes written
                bitsoverwritten += length;
                //move the write pointer and increment inode size
                fDT.table[fileID].inode.size += length;
                fDT.table[fileID].writePointer += length;

                //adjust 
                iNodeBlock->inodes[fDT.table[fileID].inodeIndex % 16].size += length;
                write_blocks(blocknum, 1, iNodeBlock);
                return length;
            } else { // we need another data block
                //TODO implement case 2 & 3
                return 0;
            }
        }
    }
    return 0;
}

int ssfs_fread(int fileID, char *buf, int length) {

    //  DataBlock_t *cache;
    inodeBlock_t *iNodeBlock;
    int blocknum;
    //int datablocknum;
    if (fDT.table[fileID].inode.size == 0) {
        return -1;
    }

    if (fDT.table[fileID].fdindex == -1) {
        return -1;
    }

    if (fileID < 0) {
        printf("INDEX NOT DEFINED");
        return-1;
    }
    if (fileID >= 200) {
        printf("Too Many files, cannot handle");
        return -1;
    } else {
        iNodeBlock = malloc(sizeof (inodeBlock_t));
        blocknum = sb.root.direct[fDT.table[fileID].inodeIndex / NUM_INODES_PER_BLOCK];
        read_blocks(blocknum, 1, iNodeBlock);
        inode_t *currInode = &(iNodeBlock->inodes[fDT.table[fileID].inodeIndex % 16]);
        int num_direct= length + fDT.table[fileID].readPointer;
        if (length > 1024) {
            for (int j = fDT.table[fileID].readPointer/1024 ; j < num_direct ; j++) {
                read_blocks(currInode->direct[j], 1, buf);
                buf += 1024;
            }
        } else {
            read_blocks(currInode->direct[0], 1, buf);
        }
        fDT.table[fileID].readPointer = length;
        return length;
    }
    return -1;
}

int ssfs_remove(char *file) {

    int getInodenum;
    int blocknum;
    int inodeIndex;
    inodeBlock_t *iNodeBlock;
    // access the root directory
    printf("Starting\n");
    for (int j = 0; j < NUM_OF_FILES; j++) {
        if (strcmp(rDT.fileEntries[j].filenames, file) == 0) {
            printf("enterig loop\n");
            getInodenum = rDT.fileEntries[j].inodeNum; // get inodenum corresponding to the file
            // Clear the tracker
            iNodeTracker[getInodenum].size = -1;
            iNodeTracker[getInodenum].indirect = -1;
            for (int k = 0; k < 14; k++) {
                iNodeTracker[getInodenum].direct[k] = -1;

            }
            // release entry from RDT
            for (int k = 0; k < 10; k++) {
                rDT.fileEntries[j].filenames[k] = '\0';

            }
            rDT.fileEntries[j].inodeNum = -1;
            iNodeBlock = malloc(sizeof (inodeBlock_t));
            blocknum = sb.root.direct[getInodenum / NUM_INODES_PER_BLOCK]; // find the block number that corresponds to the inode
            // read the block get the inode details
            inodeIndex = getInodenum % NUM_INODES_PER_BLOCK;
            read_blocks(blocknum, 1, iNodeBlock);
            inode_t *currInode = &(iNodeBlock->inodes[inodeIndex]); // get the inode
            // want to release the current ninode.
            currInode->size = -1;
            for (int k = 0; k < 14; k++) {
                if (currInode->direct[k] != -1) {
                    FBM.blocks[currInode->direct[k]] = 1;
                    currInode->direct[k] = -1;
                }
            }

        }

    }
    // printf("FileRemoved");
    return -1;
}

