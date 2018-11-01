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

    //Step through all blocks in virtual disk
    for(int i = 0; i < 128; ++i){
      BLOCK block;
      // Master block
      if(i == 0){
        // Steps through each byte in the block and sets it to 0
        for(int j = 0; j < 8; ++j){
          block.master.inode_allocated_flag[j] = 0;
          block.master.block_allocated_flag[j] = 0;
        }
        if(vdisk_write_block(0, &block) != 0){
          // TODO: add error check
        }
      }
      if(i <= N_INODE_BLOCKS){
        for(int j = 0; j < INODES_PER_BLOCK; ++j){
          INODE inode;
        }
      }

    }
}
