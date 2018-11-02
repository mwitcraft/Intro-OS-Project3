// Write 0s to all the bytes in the virtual disk
// Initialize master block
//    -Mark master block, all inode blocks, and first data block (root directory) as allocated
//    -Mark the first inode as allocated (root)
// Initialize the first inode (root)
// Initialize the first data block as an empty directory, but with '.' and '..' referring both to inode 0

#include <stdio.h>
#include <string.h>

#include "Given/oufs_lib.h"
#include "Given/vdisk.h"

int main(int argc, char** argv){
    // Creates a virtual disk with name 'vdisk1'
    if(vdisk_disk_open("vdisk1") != 0)
      return -1;

    // Steps through all bytes in disk and sets to 0
    for(int num_block = 0; num_block < N_BLOCKS_IN_DISK; ++num_block){ //Steps through each block
      BLOCK block;
      for(int byte = 0; byte < BLOCK_SIZE; ++byte){ //Steps through each byte in the block
        block.data.data[byte] = 0; //Sets the byte to 0
      }
      if(vdisk_write_block(num_block, &block) != 0){ //Writes the block to the disk
        //TODO: add error checks
      }
    }

    // Inside while loop because for some reason it wasn't working without it - variables were changing
    while(1){
      BLOCK masterBlock;
      for(int i = 0; i <= N_INODE_BLOCKS + 1; ++i){ // Steps through master block, inode blocks, and first data block
        //https://stackoverflow.com/questions/6848617/memory-efficient-flag-array-in-c
        masterBlock.master.block_allocated_flag[i/8] |= (1 << (i % 8)); //Marks corresponding bits as allocated
      }
      masterBlock.master.inode_allocated_flag[0] |= (1 << (0)); //Marks first inode as allocated
      if(vdisk_write_block(0, &masterBlock) != 0){ //Writes the block to the disk
      //TODO: add error checks
      }
      break;
    }

    //Initializes the first inode
    while(1){
      INODE firstInode;
      firstInode.type = IT_DIRECTORY; //Sets as directory
      firstInode.n_references = 1; //Number of directory references
      firstInode.data[0] = N_INODE_BLOCKS + 1; //The first data block contains the data for this inode
      for(int i = 1; i < BLOCKS_PER_INODE; ++i){ //Steps through the rest of the inode data array
        firstInode.data[i] = UNALLOCATED_BLOCK; //Sets the value at each location in the array not 0 as unallocated
      }
      firstInode.size = 2; //Initial size is 2 for '.' and '..'

      BLOCK newInodeBlock;
      newInodeBlock.inodes.inode[0] = firstInode; //Adds the first inode to a inode block
      if(vdisk_write_block(1, &newInodeBlock) != 0){ //Writes the block to the disk at location 1, which is the first inode block
        printf("ERROR\n");
      //TODO: add error checks
      }
      break;
    }

    // while(1){
    //   DIRECTORY_ENTRY currentDir;
    //   char* curDirName = ".";
    //   strncpy(currentDir.name, curDirName, strlen(curDirName));
    //   currentDir.name[strlen(curDirName)] = '\0';
    //   currentDir.inode_reference = 0;
    //   DIRECTORY_ENTRY parentDir;
    //   char* parentDirName = "..";
    //   strncpy(parentDir.name, parentDirName, strlen(parentDirName));
    //   parentDir.name[strlen(parentDirName)] = '\0';
    //   parentDir.inode_reference = 0;
    //
    //   DIRECTORY_BLOCK dirBlock;
    //   dirBlock.entry[0] = currentDir;
    //   dirBlock.entry[1] = parentDir;
    //
    //   BLOCK newBlock;
    //   newBlock.directory = dirBlock;
    //
    //   if(vdisk_write_block(1, &newBlock) != 0){ //Writes the block to the disk at location 1, which is the first inode block
    //     printf("ERROR\n");
    //   //TODO: add error checks
    //   }
    //   break;
    //
    // }
}
