#include "sfs_api.h"

void init_superBlock_t() {
    sb.magic = 0xABCD005;
    sb.blockSize = 1024;

    // root is j node in the super block;
    for (int i = 0; i < 14; i++) {
        sb.root.direct[i] = -1;
        sb.root.size = -1;
    }
}

void init_rootDirTable_t() {
    for (int i = 0; i < NUM_OF_FILES; i++) {
        rDT.fileEntries[i].inodeNum = 0; // means inode is not used.
        for (int k = 0; k < 10; k++) {
            rDT.fileEntries[i].filename[k] = '\0';
        }
    }
}

void init_fileDescriptorTable_t() {
    for (int i = 0; i < NUM_OF_FILES; i++) {

        fDT.table[i].fileopen = -1;
        fDT.table[i].inode.size = -1;
        fDT.table[i].inodeNum = -1;
        fDT.table[i].readPointer = -1;
        fDT.table[i].writePointer = -1;

    }
}

void init_InodeBlock_t() {
    for (int i = 0; i < 16; i++) {
        iNodeBlock.inodes[i].size = -1; // means inode is not used.
        for (int j = 0; j < 14; j++) {
            iNodeBlock.inodes[i].direct[j] = -1;
        }

        iNodeBlock.inodes[i].indirect = -1;
    }
}

void init_Inodes_t() {

    currInode.size = -1; // means inode is not used.
    for (int j = 0; j < 14; j++) {
        currInode.direct[j] = -1;
    }

    currInode.indirect = -1;
}

void init_freeBitMap_t() {
    for (int i = 0; i < NUM_BLOCKS; i++) {
        FBM.data[i] = 0; // means inode is not used.
    }
}

int allocateBlock() {

    read_blocks(1, 1, &FBM);
    for (int i = 0; i < NUM_BLOCKS; i++) {
        if (FBM.data[i] == 0) {
            FBM.data[i] = 1;
            write_blocks(1, 1, &FBM);
            return i;
        }
    }
    return -1;
}

int allocateNewInode() {

    // find inode by reading all the inode blocks if found return index
    for (int i = 0; i < NUM_INODE_DIRECT; i++) {
        if (sb.root.direct[i] != -1) {
            read_blocks(sb.root.direct[i], 1, &iNodeBlock);
            for (int j = 0; j < NUM_INODES_PER_BLOCK; j++) {
                currInode = iNodeBlock.inodes[j];
                if (currInode.size == -1) {
                    return j + (i * NUM_INODE_DIRECT);

                }
            }

        } else {
            //  if not found allocate(create) a new block with the first index
            int allocatedBlock = allocateBlock();
            read_blocks(allocatedBlock, 1, &iNodeBlock);
            currInode = iNodeBlock.inodes[0];
            currInode.size = 0;
            write_blocks(allocatedBlock, 1, &iNodeBlock);
            return (i * NUM_INODE_DIRECT);
        }
    }
    return -1;
}

void mkssfs(int fresh) {
    // INITIALIZING
    init_superBlock_t();
    init_rootDirTable_t();
    init_fileDescriptorTable_t();
    init_InodeBlock_t();
    init_freeBitMap_t();
    //    init_iNode_tracker_t();
    init_Inodes_t();
    int iblocks = 3;
    printf("initialized\n");

    sb.root.direct[0] = 3; // points to 1st inode block

    for (int i = 0; i < 14; i++) {
        sb.root.direct[i] = iblocks++;
    }
    FBM.data[0] = 1; //"sb";
    FBM.data[1] = 1; //"FBM";
    FBM.data[2] = 1; //"WM";
    for (int k = 3; k < 17; k++) {
        FBM.data[k] = 1;
    }//"iNodeBlock";
    for (int j = 17; j < 21; j++) {
        FBM.data[j] = 1;
    }; //"rDT";

    //   read_blocks(sb.root.direct[0], 1, &iNodeBlock);
    iNodeBlock.inodes[0].direct[0] = 17;
    iNodeBlock.inodes[0].size = 0;




    //OPEN A NEW DISK
    if (!fresh) {

        if (init_disk(FILE_NAME, BLOCK_SIZE, NUM_BLOCKS) == -1)
            printf("CANNOT OPEN the file");

    } else {
        if (init_fresh_disk(FILE_NAME, BLOCK_SIZE, NUM_BLOCKS) == -1)
            printf("CANNOT OPEN");
        else {
            // writing to the disk
            printf("I am writing the initial values to the disk\n");
            write_blocks(0, 1, &sb);
            write_blocks(1, 1, &FBM);
            write_blocks(2, 1, &WM);
            for(int i = 3;i<17;i++){
            write_blocks(i, 1, &iNodeBlock);
            }
            for(int j = 17;j<20;j++){
            write_blocks(j, 1, &rDT);
            }
            printf("end\n");
        }
    }

}

int ssfs_fopen(char *name) {

    int i, k; // k is for file index
    int inodenum;
    int blocknum;
    int found = 0;
    // Lookup for file if it currently resides in our table
    // printf("Looking for the file\n");
    if (!name || name[0] == '\0' || strlen(name) > 10) {
        return -1;
    }
    for (i = 0; i < 200; i++) {
        if (strcmp(name, rDT.fileEntries[i].filename) == 0) {
            found = 1;
            break;
        }
    }
    // Lookup for file if it is currently open or not
    // find the corresponding inode number
    if (found == 1) {
        inodenum = rDT.fileEntries[i].inodeNum;
        for (int j = 0; j < MAX_FILEDESCRIPTABLE_SIZE; j++) {
            if (fDT.table[j].inodeNum == inodenum) {
                printf("Already Open!!\n");
                return-1;
            } else {
                break;
            }
        }
        //open a new file descriptor entry for the file
        //find the block number that corresponds to the inode
        blocknum = sb.root.direct[inodenum / NUM_INODES_PER_BLOCK];
        read_blocks(blocknum, 1, &iNodeBlock);
        // read the block get the inode details
        // want to use the current currInode.
        write_blocks(blocknum, 1, &iNodeBlock);
        for (k = 0; k < MAX_FILEDESCRIPTABLE_SIZE; k++) {
            if (fDT.table[k].fileopen == -1) {
                fDT.table[k].fileopen = 1; //its open now
                fDT.table[k].inode = iNodeBlock.inodes[inodenum % 16];
                fDT.table[k].inodeNum = inodenum;
                fDT.table[k].readPointer = 0;
                fDT.table[k].writePointer = 0;
                return k;
            }
        }
    }

    // if the file is not in rdt create one and put it back in the block.
    if (found == 0) {

        read_blocks(RDIR_BLOCK_ADDRESS, RDIR_BLOCK_SIZE, &rDT);
        for (i = 0; i < NUM_OF_FILES; i++) {
            // search for an available entry in the root directory table
            if (strcmp(rDT.fileEntries[i].filename, "\0") == 0) {
                break;
            }
        }
        // create a new entry space
        rootDir_t *new = &rDT.fileEntries[i];
        // allocate this space to the new entry
        // name the file
        strcpy(new->filename, name);
        //need to allocate a data block for the inode;
        int allocatediNode = allocateNewInode();
        //need to find an inode for this new file.
        new->inodeNum = allocatediNode;
        write_blocks(RDIR_BLOCK_ADDRESS, RDIR_BLOCK_SIZE, &rDT);
        // assign  the inode to the new entry
        // access root directory block
        blocknum = sb.root.direct[allocatediNode / NUM_INODES_PER_BLOCK];
        // read the block we are gonna allocate to
        read_blocks(blocknum, 1, &iNodeBlock);
        // creates a file descriptor for the new entry and updates the content
        // updating the fbm to indicate the data block is used
        iNodeBlock.inodes[allocatediNode % 16].size = 0;

        write_blocks(blocknum, 1, &iNodeBlock);
        for (int j = 0; j < MAX_FILEDESCRIPTABLE_SIZE; j++) {
            if (fDT.table[j].fileopen == -1) {
                fDT.table[j].fileopen = 1; //its open now
                fDT.table[j].inode = iNodeBlock.inodes[allocatediNode % 16];
                fDT.table[j].inodeNum = allocatediNode;
                fDT.table[j].readPointer = 0;
                fDT.table[j].writePointer = 0;
                return j;
            }


        }
    }
    return -1;
}

int ssfs_fclose(int fileID) {

    if (fileID < 0) {
        printf("INDEX NOT DEFINED\n");
        return-1;
    }
    if (fileID > 200) {
        printf("Too Many files, cannot handle\n");
        return -1;
    }
    if (fDT.table[fileID].fileopen == -1) {
        return -1;
    }

    if (fDT.table[fileID].fileopen != -1) {
        fDT.table[fileID].fileopen = -1;
        fDT.table[fileID].inode.size = -1;
        fDT.table[fileID].readPointer = -1;
        fDT.table[fileID].writePointer = -1;
        fDT.table[fileID].inodeNum = -1;
        return 0;
    }
    return -1;
}


//   printf("fDT.index %i\n", fDT.table[fileID].fdindex);

int ssfs_frseek(int fileID, int loc) {
    if (fileID < 0) {
        return-1;
    } else if (fileID > 200) {
        printf("Too Many files, cannot handle");
        return -1;
    }
    if (fDT.table[fileID].fileopen == -1) {
        return -1;
    }
    if (loc < 0 || loc > fDT.table[fileID].inode.size) {
        return -1;
    }
    fDT.table[fileID].readPointer = loc;
    return 0;
}

int ssfs_fwseek(int fileID, int loc) {
    if (fileID < 0) {
        return-1;
    } else if (fileID > 200) {
        printf("Too Many files, cannot handle");
        return -1;
    }
    if (fDT.table[fileID].fileopen == -1) {
        return -1;
    }
    if (loc < 0 || loc > fDT.table[fileID].inode.size) {
        return -1;
    }
    fDT.table[fileID].writePointer = loc;
    return 0;

}

int ssfs_fwrite(int fileID, char *buf, int length) {

    int blocknum;
    int byteswritten;
    int allocatecacheindisk;
    int incSize=0;


    if (fileID < 0) {
        printf("INDEX NOT DEFINED");
        return -1;
    }
    if (fileID > 200) {
        printf("Too Many files, cannot handle");
        return -1;
    }
    if (fDT.table[fileID].fileopen == -1) {
        printf("Writing to a closed file\n");
        return -1;
    }
    if (fDT.table[fileID].inode.size == 0) {
        printf("Writing to a newly created file\n");
        blocknum = sb.root.direct[fDT.table[fileID].inodeNum / NUM_INODES_PER_BLOCK];
        read_blocks(blocknum, 1, &iNodeBlock);
        inode_t currInode = iNodeBlock.inodes[fDT.table[fileID].inodeNum % 16];
        // What we want to write is greater than a block
        if (length > 1024) {
            for (int j = 0; j < length / 1024; j++) {
                allocatecacheindisk = allocateBlock();
                currInode.direct[j] = allocatecacheindisk;
                write_blocks(currInode.direct[j], 1, buf);
                buf += 1024;
            }
            iNodeBlock.inodes[fDT.table[fileID].inodeNum % 16].size += length;
            write_blocks(blocknum, 1, &iNodeBlock);
            fDT.table[fileID].writePointer = length;
        }//what we want to write fits in 1 data Block
        else {
            allocatecacheindisk = allocateBlock();
            currInode.direct[0] = allocatecacheindisk;
            read_blocks(currInode.direct[0], 1, &cache);
            memcpy(cache.data, buf, length);
            write_blocks(currInode.direct[0], 1, &cache);
            currInode.size = length;
            iNodeBlock.inodes[fDT.table[fileID].inodeNum % 16] = currInode;
            write_blocks(blocknum, 1, &iNodeBlock);
            fDT.table[fileID].writePointer = length;
            fDT.table[fileID].inode = currInode;
            return length;
        }


    }
    printf("Writing to an existing file\n");
    // 2 cases 
    // Writing to a block where there is space  
    blocknum = sb.root.direct[fDT.table[fileID].inodeNum / NUM_INODES_PER_BLOCK];
    read_blocks(blocknum, 1, &iNodeBlock);
   inode_t currInode = iNodeBlock.inodes[fDT.table[fileID].inodeNum % 16];
    // find the inodeData Block corresponding to the write pointer
    int num_inodeDataBlock = (fDT.table[fileID].writePointer) / 1024;
  
    // We will face two cases here as well: 
    //1) if length + writepointer is < 1024
    if (length + fDT.table[fileID].writePointer <= 1024) {
        // read the data block from the disk                                                                      
        read_blocks(currInode.direct[num_inodeDataBlock], 1, &bufferin);
        //memcopy the buffer starting write pointer
        memcpy(bufferin + fDT.table[fileID].writePointer, buf, length);
        //push back to disk
        write_blocks(currInode.direct[num_inodeDataBlock], 1, &bufferin);
        // num of bytes written
        byteswritten = length;
        
        //increment the write pointer and increment inode size in FDT
        fDT.table[fileID].writePointer += length;
        incSize = fDT.table[fileID].writePointer - currInode.size;
        if(incSize>0){
        fDT.table[fileID].inode.size += incSize;
        }     
        //increment the write pointer and increment inode size in the disk
        iNodeBlock.inodes[fDT.table[fileID].inodeNum % 16] = currInode;
        write_blocks(blocknum, 1, &iNodeBlock);
        return length;
    }
    //2) if length + writepointer is >= 1024
    if (length + fDT.table[fileID].writePointer > 1024) {
        // we will have to write to multiple data blocks
        // lets first find the number of data blocks we will have to write to
        int num_dataBlocks = (length + fDT.table[fileID].writePointer) / 1024;
        for (int j = num_inodeDataBlock; j < num_dataBlocks; j++) {
            //read the data block from the disk                                                                      
            //read_blocks(currInode->direct[j], 1, &cache);
            //memcopy the contents of the data block corresponding to the writepointer into the bufferin
            //memcpy(bufferin, cache, fDT.table[fileID].writePointer);
            //update bufferin pointer
            bufferin  + fDT.table[fileID].writePointer;
            // note that we need to  calculate the remaining bytes or available bytes in the block
            //memcopy the new content into the bufferin starting at the updated index
            int available_byte_space = 1024 - fDT.table[fileID].writePointer;
            memcpy(bufferin, buf, available_byte_space);
              // update buf // this corresponds to the remaining data that neets to be written
            buf = buf + available_byte_space;
            //push back to disk
            write_blocks(currInode.direct[j], 1, &bufferin);
            // clear bufferin for each block
            memset(bufferin, '\0', 1024);

        }
        //increment the write pointer and increment inode size in FDT
        fDT.table[fileID].inode.size += (length + 1);
        fDT.table[fileID].writePointer += (length + 1);
        //increment the write pointer and increment inode size in the disk
        iNodeBlock.inodes[fDT.table[fileID].inodeNum % 16].size += (length + 1);
        write_blocks(blocknum, 1, &iNodeBlock);

    }

    return 0;
}

int ssfs_fread(int fileID, char *buf, int length) {

    DataBlock_t cache;
    int blocknum;
    inode_t currInode;


    if (fDT.table[fileID].fileopen == -1) {
        return -1;
    }

    if (fileID < 0) {
        printf("INDEX NOT DEFINED");
        return-1;
    }
    if (fileID >= 200) {
        printf("Too Many files, cannot handle");
        return -1;
    }
    blocknum = sb.root.direct[fDT.table[fileID].inodeNum / NUM_INODES_PER_BLOCK];
    read_blocks(blocknum, 1, &iNodeBlock);

    currInode = iNodeBlock.inodes[(fDT.table[fileID].inodeNum % 16)];

    // find the data block corresponding to the read pointer
    int num_inodeDataBlock = (fDT.table[fileID].readPointer) / 1024;
    // lets first find the number of data blocks we will have to read
    int num_dataBlocks = (length + fDT.table[fileID].readPointer) / 1024;
    if (length > 1024) {
        for (int j = num_inodeDataBlock; j < num_dataBlocks; j++) {
            read_blocks(currInode.direct[j], 1, buf);
            buf += 1024;
            fDT.table[fileID].readPointer += length;
            return length;

        }
    } else {
        // read from the disk

        read_blocks(currInode.direct[num_inodeDataBlock], 1, &cache);
        memcpy(buf, cache.data+fDT.table[fileID].readPointer, length);
        fDT.table[fileID].readPointer += length;
        return length;
    }

    return -1;
}

int ssfs_remove(char *file) {

    int getInodenum;
    int blocknum;
    int inodeIndex;
    int i, j; // file descriptor index, j is root dir index
    read_blocks(RDIR_BLOCK_ADDRESS, RDIR_BLOCK_SIZE, &rDT);

    // access the root directory
    // check if file is open
    for (j = 0; j < NUM_OF_FILES; j++) {
        //compare inode number too
        if (strcmp(rDT.fileEntries[j].filename, file) == 0) {
            break;
        }
    }
    // if it is close it
    for (i = 0; i < NUM_OF_FILES; i++) {
        if (fDT.table[i].inodeNum == rDT.fileEntries[j].inodeNum) {
            ssfs_fclose(i);
            break;
        }
    }
    getInodenum = rDT.fileEntries[j].inodeNum;
    // get inodenum corresponding to the file
    // Clear the tracker
    // release entry from RDT
    for (int k = 0; k < 10; k++) {
        rDT.fileEntries[j].filename[k] = '\0';
    }
    rDT.fileEntries[j].inodeNum = -1;
    // release it from the disk
    // find the block number that corresponds to the inode
    blocknum = sb.root.direct[getInodenum / NUM_INODES_PER_BLOCK];
    // read the block get the inode details
    inodeIndex = getInodenum % NUM_INODES_PER_BLOCK;
    read_blocks(blocknum, 1, &iNodeBlock);
    // get the inode
    inode_t *currInode = &(iNodeBlock.inodes[inodeIndex]);
    // want to release the current currInode.
    currInode->size = -1;
    for (int k = 0; k < 14; k++) {
        if (currInode->direct[k] != -1) {
            FBM.data[currInode->direct[k]] = 1;
            currInode->direct[k] = -1;
            write_blocks(blocknum, 1, &iNodeBlock);
            write_blocks(RDIR_BLOCK_ADDRESS, RDIR_BLOCK_SIZE, &rDT);
            return 0;
        }
    }
    return -1;
}


