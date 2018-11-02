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

    // int x = 15;
    // unsigned char c;
    // c = (unsigned char)x;
    // printf("%x\n", c);

    //Mark master block as allocated (master block is at 0)
    BLOCK masterBlock;
    for(int i = 0; i <= N_INODE_BLOCKS + 1; ++i){
      //https://stackoverflow.com/questions/6848617/memory-efficient-flag-array-in-c
      masterBlock.master.block_allocated_flag[i/8] |= (1 << (i % 8));
    }
     if(vdisk_write_block(0, &masterBlock) != 0){ //Writes the block to the disk
       printf("ERROR\n");
       //TODO: add error checks
       }
}
