#include <stdio.h>
#include <string.h>

#include "oufs.h"
#include "oufs_lib.h"

int main(int argc, char** argv){
  //Get working directory for the OUFS
  //Use oufs_get_environment
  char path[MAX_PATH_LENGTH];
  char diskName[MAX_PATH_LENGTH];
  oufs_get_environment(path, diskName);

  vdisk_disk_open(diskName);

  //If no arguments are provided, list cwd
  if(argc == 1){
    //Assume path is '/'
    //Go to inode 0 and store the inode
    //Step through data in inode
    //  If data[i] is NOT an UNALLOCATED_BLOCK, store that block
    //  Step through the entries in that block
    //    if entry[i] is NOT an UNALLOCATED_INODE, print the name of the entry


    if(!strcmp(path, "/")){
      INODE inode;
      oufs_read_inode_by_reference(0, &inode);
      for(int i = 0; i < BLOCKS_PER_INODE; ++i){
        BLOCK block;
        if(inode.data[i] != UNALLOCATED_BLOCK){
          printf("inode.data[%i]: %i\n", i, inode.data[i]);
          vdisk_read_block(inode.data[i], &block);
          for(int j = 0; j < DIRECTORY_ENTRIES_PER_BLOCK; ++j){
            if(block.directory.entry[j].inode_reference != UNALLOCATED_INODE){
              printf("%s/\n", block.directory.entry[j].name);
            }
          }
        }
      }
    }
  }

  vdisk_disk_close();


  return 0;
}
