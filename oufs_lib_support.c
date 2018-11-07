#include <stdlib.h>
#include <libgen.h>
#include "oufs_lib.h"

#define debug 0

/**
 * Read the ZPWD and ZDISK environment variables & copy their values into cwd and disk_name.
 * If these environment variables are not set, then reasonable defaults are given.
 *
 * @param cwd String buffer in which to place the OUFS current working directory.
 * @param disk_name String buffer containing the file name of the virtual disk.
 */
void oufs_get_environment(char *cwd, char *disk_name)
{
  // Current working directory for the OUFS
  char *str = getenv("ZPWD");
  if(str == NULL) {
    // Provide default
    strcpy(cwd, "/");
  }else{
    // Exists
    strncpy(cwd, str, MAX_PATH_LENGTH-1);
  }

  // Virtual disk location
  str = getenv("ZDISK");
  if(str == NULL) {
    // Default
    strcpy(disk_name, "vdisk1");
  }else{
    // Exists: copy
    strncpy(disk_name, str, MAX_PATH_LENGTH-1);
  }

}

/**
 * Configure a directory entry so that it has no name and no inode
 *
 * @param entry The directory entry to be cleaned
 */
void oufs_clean_directory_entry(DIRECTORY_ENTRY *entry)
{
  entry->name[0] = 0;  // No name
  entry->inode_reference = UNALLOCATED_INODE;
}

/**
 * Initialize a directory block as an empty directory
 *
 * @param self Inode reference index for this directory
 * @param self Inode reference index for the parent directory
 * @param block The block containing the directory contents
 *
 */

void oufs_clean_directory_block(INODE_REFERENCE self, INODE_REFERENCE parent, BLOCK *block)
{
  // Debugging output
  if(debug)
    fprintf(stderr, "New clean directory: self=%d, parent=%d\n", self, parent);

  // Create an empty directory entry
  DIRECTORY_ENTRY entry;
  oufs_clean_directory_entry(&entry);

  // Copy empty directory entries across the entire directory list
  for(int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; ++i) {
    block->directory.entry[i] = entry;
  }

  // Now we will set up the two fixed directory entries

  // Self
  strncpy(entry.name, ".", 2);
  entry.inode_reference = self;
  block->directory.entry[0] = entry;

  // Parent (same as self
  strncpy(entry.name, "..", 3);
  entry.inode_reference = parent;
  block->directory.entry[1] = entry;

}

/**
 * Allocate a new data block
 *
 * If one is found, then the corresponding bit in the block allocation table is set
 *
 * @return The index of the allocated data block.  If no blocks are available,
 * then UNALLOCATED_BLOCK is returned
 *
 */
BLOCK_REFERENCE oufs_allocate_new_block()
{
  BLOCK block;
  // Read the master block
  vdisk_read_block(MASTER_BLOCK_REFERENCE, &block);

  // Scan for an available block
  int block_byte;
  int flag;

  // Loop over each byte in the allocation table.
  for(block_byte = 0, flag = 1; flag && block_byte < N_BLOCKS_IN_DISK / 8; ++block_byte) {
    if(block.master.block_allocated_flag[block_byte] != 0xff) {
      // Found a byte that has an opening: stop scanning
      flag = 0;
      break;
    };
  };
  // Did we find a candidate byte in the table?
  if(flag == 1) {
    // No
    if(debug)
      fprintf(stderr, "No blocks\n");
    return(UNALLOCATED_BLOCK);
  }

  // Found an available data block

  // Set the block allocated bit
  // Find the FIRST bit in the byte that is 0 (we scan in bit order: 0 ... 7)
  int block_bit = oufs_find_open_bit(block.master.block_allocated_flag[block_byte]);

  // Now set the bit in the allocation table
  block.master.block_allocated_flag[block_byte] |= (1 << block_bit);

  // Write out the updated master block
  vdisk_write_block(MASTER_BLOCK_REFERENCE, &block);

  if(debug)
    fprintf(stderr, "Allocating block=%d (%d)\n", block_byte, block_bit);

  // Compute the block index
  BLOCK_REFERENCE block_reference = (block_byte << 3) + block_bit;

  if(debug)
    fprintf(stderr, "Allocating block=%d\n", block_reference);

  // Done
  return(block_reference);
}


/**
 *  Given an inode reference, read the inode from the virtual disk.
 *
 *  @param i Inode reference (index into the inode list)
 *  @param inode Pointer to an inode memory structure.  This structure will be
 *                filled in before return)
 *  @return 0 = successfully loaded the inode
 *         -1 = an error has occurred
 *
 */
int oufs_read_inode_by_reference(INODE_REFERENCE i, INODE *inode)
{
  if(debug)
    fprintf(stderr, "Fetching inode %d\n", i);

  // Find the address of the inode block and the inode within the block
  BLOCK_REFERENCE block = i / INODES_PER_BLOCK + 1;
  int element = (i % INODES_PER_BLOCK);

  BLOCK b;
  if(vdisk_read_block(block, &b) == 0) {
    // Successfully loaded the block: copy just this inode
    *inode = b.inodes.inode[element];
    return(0);
  }
  // Error case
  return(-1);
}

// WIll need to come back and complete
int oufs_find_open_bit(unsigned char value){
  int bit = -1;
  for(int i = 0; i < 8; ++i){
      if(!((1 << i) & value)){
        bit = i;
        break;
      }
  }
  return bit;
}

int oufs_mkdir(char* cwd, char* path){

  char fullPath[strlen(cwd) + strlen(path)];
  fullPath[0] = '\0';
  strcat(fullPath, cwd);
  strcat(fullPath, path);

  char dirnamePath[strlen(fullPath)];
  strncpy(dirnamePath, fullPath, strlen(fullPath));
  dirnamePath[strlen(fullPath)] = '\0';
  strncpy(dirnamePath, dirname(dirnamePath), strlen(dirnamePath));
  dirnamePath[strlen(dirnamePath)] = '\0';

  char basenamePath[strlen(fullPath)];
  strncpy(basenamePath, fullPath, strlen(fullPath));
  basenamePath[strlen(fullPath)] = '\0';
  strncpy(basenamePath, basename(basenamePath), strlen(basenamePath));
  basenamePath[strlen(basename(basenamePath))] = '\0';

  if(verify_parent_exists(fullPath) != -1){
    fprintf(stderr, "ERROR: Directory already exists\n");
    return -1;
  }

  int parentInodeReference = verify_parent_exists(dirnamePath);
  if(parentInodeReference == -1){
    return -1;
  }

  int parentInodeBlockReference = parentInodeReference / INODES_PER_BLOCK + 1;
  int parentInodeBlockIndex = parentInodeReference % INODES_PER_BLOCK;
  BLOCK parentBlock;
  vdisk_read_block(parentInodeBlockReference, &parentBlock);
  ++parentBlock.inodes.inode[parentInodeBlockIndex].size;
  vdisk_write_block(parentInodeBlockReference, &parentBlock);

  BLOCK_REFERENCE parentDataBlockReference;
  INODE parentInode;
  oufs_read_inode_by_reference(parentInodeReference, &parentInode);
  for(int i = 0; i < BLOCKS_PER_INODE; ++i){
      if(parentInode.data[i] != UNALLOCATED_BLOCK){
        BLOCK_REFERENCE parentDataBlockRef = parentInode.data[i];
        BLOCK parentDataBlock;
        vdisk_read_block(parentDataBlockRef, &parentDataBlock);
        for(int j = 0; j < DIRECTORY_ENTRIES_PER_BLOCK; ++j){
          if(parentDataBlock.directory.entry[j].inode_reference == UNALLOCATED_INODE){
              parentDataBlockReference = parentDataBlockRef;
              break;
          }
        }
      }
  }

  INODE_REFERENCE newInodeInodeReference = -1;
  for(int i = 0; i < N_INODES; ++i){
    INODE inode;
    oufs_read_inode_by_reference(i, &inode);
    if(inode.size == 0){
      newInodeInodeReference = i;
      break;
    }
  }

  //Gets the byte and bit of the new location so can assign values later
  int byte = newInodeInodeReference / INODES_PER_BLOCK + 1;
  int bit = newInodeInodeReference % INODES_PER_BLOCK;

  //Byte corresponds to the BLOCK_REFERENCE
  BLOCK_REFERENCE newInodeInodeBlockReference = byte;
  //Allocates a new block for this information and returns the location of that block
  BLOCK_REFERENCE newInodeDataBlockReference = oufs_allocate_new_block();

  //Opens a new block at the location referenced above
  BLOCK newInodeBlock;
  vdisk_read_block(newInodeInodeBlockReference, &newInodeBlock);

  //Fills in the inode information in the new block
  newInodeBlock.inodes.inode[bit].type = IT_DIRECTORY;
  newInodeBlock.inodes.inode[bit].n_references = 1;
  newInodeBlock.inodes.inode[bit].data[0] = newInodeDataBlockReference;
  for(int i = 1; i < BLOCKS_PER_INODE; ++i){
      newInodeBlock.inodes.inode[bit].data[i] = UNALLOCATED_BLOCK;
  }
  newInodeBlock.inodes.inode[bit].size = 2;

  BLOCK parentDataBlock;
  vdisk_read_block(parentDataBlockReference, &parentDataBlock);

  for(int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; ++i){
    if(parentDataBlock.directory.entry[i].inode_reference == UNALLOCATED_INODE){
      strncpy(parentDataBlock.directory.entry[i].name, basenamePath, strlen(basenamePath));
      parentDataBlock.directory.entry[i].inode_reference = newInodeInodeReference;
      break;
    }
  }

  BLOCK newInodeDataBlock;
  oufs_clean_directory_block(newInodeInodeReference, parentInodeReference, &newInodeDataBlock);

  vdisk_write_block(newInodeInodeBlockReference, &newInodeBlock);
  vdisk_write_block(parentDataBlockReference, &parentDataBlock);
  vdisk_write_block(newInodeDataBlockReference, &newInodeDataBlock);

  BLOCK masterBlock;
  vdisk_read_block(0, &masterBlock);
  masterBlock.master.inode_allocated_flag[byte - 1] |= (1 << (bit));

  vdisk_write_block(0, &masterBlock);

  return 0;
}

int oufs_rmdir(char *cwd, char *path){

  char fullPath[strlen(cwd) + strlen(path)];
  fullPath[0] = '\0';
  strcat(fullPath, cwd);
  strcat(fullPath, path);

  int inodeToRemoveReference = verify_parent_exists(fullPath);

  if(inodeToRemoveReference == -1){
    fprintf(stderr, "Path does not exist\n");
  }
  else{

    INODE inodeToRemove;
    oufs_read_inode_by_reference(inodeToRemoveReference, &inodeToRemove);


    if(inodeToRemove.size > 2){
      fprintf(stderr, "ERROR: Directory not empty\n");
      return -1;
    }

    //Get parent inode reference
    int parentInodeReference;
    for(int i = 0; i < BLOCKS_PER_INODE; ++i){
      if(inodeToRemove.data[i] != UNALLOCATED_BLOCK){
        int ref = inodeToRemove.data[i];
        BLOCK block;
        vdisk_read_block(ref, &block);
        for(int j = 0; j < DIRECTORY_ENTRIES_PER_BLOCK; ++j){
          if(!strcmp(block.directory.entry[j].name, "..")){
            parentInodeReference = block.directory.entry[j].inode_reference;

            BLOCK masterBlock;
            vdisk_read_block(MASTER_BLOCK_REFERENCE, &masterBlock);
            masterBlock.master.block_allocated_flag[ref / 8] &= ~(1  << (ref % 8));
            masterBlock.master.inode_allocated_flag[inodeToRemoveReference / 8] &= ~(1 << (inodeToRemoveReference % 8));
            vdisk_write_block(MASTER_BLOCK_REFERENCE, &masterBlock);
            break;
          }
        }
      }
    }

    int parentInodeBlockReference = parentInodeReference / INODES_PER_BLOCK + 1;
    int parentInodeBlockIndex = parentInodeReference % INODES_PER_BLOCK;

    BLOCK parentBlock;
    vdisk_read_block(parentInodeBlockReference, &parentBlock);

    for(int i = 0; i < BLOCKS_PER_INODE; ++i){
      int ref = parentBlock.inodes.inode[parentInodeBlockIndex].data[i];
      if(ref != UNALLOCATED_BLOCK){
        BLOCK block;
        vdisk_read_block(ref, &block);
        for(int j = 0; j < DIRECTORY_ENTRIES_PER_BLOCK; ++j){
          int inodeRef = block.directory.entry[j].inode_reference;
          if(inodeRef == inodeToRemoveReference){
            strncpy(block.directory.entry[j].name, "", 1);
            block.directory.entry[j].inode_reference = UNALLOCATED_INODE;
            vdisk_write_block(ref, &block);
          }
        }
      }
    }
    --parentBlock.inodes.inode[parentInodeBlockIndex].size;
    vdisk_write_block(parentInodeBlockReference, &parentBlock);
    int inodeBlockReference = inodeToRemoveReference / INODES_PER_BLOCK + 1;
    int index = inodeToRemoveReference % INODES_PER_BLOCK;

    BLOCK inodeBlock;
    vdisk_read_block(inodeBlockReference, &inodeBlock);

    inodeBlock.inodes.inode[index].type = 0;
    inodeBlock.inodes.inode[index].n_references = 0;
    for(int i = 0; i < BLOCKS_PER_INODE; ++i){
      inodeBlock.inodes.inode[index].data[i] = 0;
    }
    inodeBlock.inodes.inode[index].size = 0;

    vdisk_write_block(inodeBlockReference, &inodeBlock);
  }

  return 0;
}

int oufs_list(char *cwd, char *path){
  char fullPath[strlen(cwd) + strlen(path)];
  fullPath[0] = '\0';
  strcat(fullPath, cwd);
  strcat(fullPath, path);

  INODE inode;
  int inodeReference = verify_parent_exists(fullPath);
  if(inodeReference == -1){
    fprintf(stderr, "ERROR: Directory does not exist\n");
    return -1;
  }
  else{
    oufs_read_inode_by_reference(inodeReference, &inode);
  }

  char* dirNames[DIRECTORY_ENTRIES_PER_BLOCK];
  int dirNamesSize = 0;
      for(int i = 0; i < BLOCKS_PER_INODE; ++i){
        BLOCK block;
        if(inode.data[i] != UNALLOCATED_BLOCK){
          vdisk_read_block(inode.data[i], &block);
          for(int j = 0; j < DIRECTORY_ENTRIES_PER_BLOCK; ++j){
            if(block.directory.entry[j].inode_reference != UNALLOCATED_INODE){
              dirNames[j] = block.directory.entry[j].name;
              ++dirNamesSize;
            }
          }
        }
      }

  qsort(dirNames, dirNamesSize, sizeof(char*), comparator);
  for(int i = 0; i < dirNamesSize; ++i){
    if(strcmp(dirNames[i], "")){
      printf("%s/\n", dirNames[i]);
      fflush(stdout);
    }
  }
  return 0;
}

int verify_parent_exists(char* path){

    int currentParentInodeReference = 0;
    char* token = strtok(path, "/");
    while(token != NULL){
        currentParentInodeReference = check_helper(currentParentInodeReference, token);
        if(currentParentInodeReference == -1){
          return -1;
        }
        token = strtok(NULL, "/");
    }
    return currentParentInodeReference;
}

int check_helper(INODE_REFERENCE parentInodeReference, char* name){

  if(!strcmp(name, "/")){
    return 0;
  }

  int returner = -1;
  INODE inode;
  oufs_read_inode_by_reference(parentInodeReference, &inode);
  for(int i = 0; i < BLOCKS_PER_INODE; ++i){
    if(inode.data[i] != UNALLOCATED_BLOCK){
      BLOCK_REFERENCE currentBlockRef = inode.data[i];
      BLOCK dirBlock;
      vdisk_read_block(currentBlockRef, &dirBlock);
      for(int j = 0; j < DIRECTORY_ENTRIES_PER_BLOCK; ++j){
        if(dirBlock.directory.entry[j].inode_reference != UNALLOCATED_INODE){
          if(!strncmp(dirBlock.directory.entry[j].name, name, strlen(name))){
              returner = dirBlock.directory.entry[j].inode_reference;
              break;
          }
        }
      }
    }
  }
  return returner;
}

// https://stackoverflow.com/questions/43099269/qsort-function-in-c-used-to-compare-an-array-of-strings
int comparator(const void* p, const void* q){
  char* a = *(char**)p;
  char* b = *(char**)q;
  return strcmp(a, b);
}
