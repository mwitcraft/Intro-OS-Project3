#include <stdlib.h>
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

  //----------------------------------------------------------------
  //----------------------------------------------------------------
  //----------------------------------------------------------------
  //----------------------------------------------------------------

  // printf("Byte: %i Bit: %i\n", block_byte, block_bit);
  // return 0;


  //----------------------------------------------------------------
  //----------------------------------------------------------------
  //----------------------------------------------------------------
  //----------------------------------------------------------------
  //----------------------------------------------------------------

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

  //if cwd == '/' (it is the root directory, or inode 0 (block 1)) and path does not contain a '/' (new folder in root)
    // Create a new block in next available data block space, initialize with 2 entries, using oufs_clean_directory_block
    //    Next available data block space is found from oufs_allocate_new_block
    //    one for '.' and '..' with '.' pointing to own inode and '..' pointing to root inode
    // Create a new inode in next available slot with type directory, first data slot pointing to new block
    //    and size of 2


  //Assume cwd = '/'
  INODE_REFERENCE newInodeInodeReference = -1;
  for(int i = 0; i < N_INODES; ++i){
    INODE inode;
    oufs_read_inode_by_reference(i, &inode);
    if(inode.size == 0){
      newInodeInodeReference = i;
      break;
    }
  }

  int byte = newInodeInodeReference / INODES_PER_BLOCK + 1;
  int bit = newInodeInodeReference % INODES_PER_BLOCK;

  BLOCK_REFERENCE newInodeInodeBlockReference = byte;
  BLOCK_REFERENCE newInodeDataBlockReference = oufs_allocate_new_block();

  BLOCK newInodeBlock;
  vdisk_read_block(newInodeInodeBlockReference, &newInodeBlock);

  newInodeBlock.inodes.inode[bit].type = IT_DIRECTORY;
  newInodeBlock.inodes.inode[bit].n_references = 1;
  newInodeBlock.inodes.inode[bit].data[0] = newInodeDataBlockReference;
  for(int i = 1; i < BLOCKS_PER_INODE; ++i){
      newInodeBlock.inodes.inode[bit].data[i] = UNALLOCATED_BLOCK;
  }

  BLOCK parentBlockReference;
  vdisk_read_block(9, &parentBlockReference);

  for(int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; ++i){
    if(parentBlockReference.directory.entry[i].inode_reference == UNALLOCATED_INODE){
      // parentBlockReference.directory.entry[i].name = path;
      strcat(parentBlockReference.directory.entry[i].name, path);
      parentBlockReference.directory.entry[i].inode_reference = newInodeInodeReference;
      break;
    }
  }

  BLOCK newInodeDataBlock;
  oufs_clean_directory_block(newInodeInodeReference, 0, &newInodeDataBlock);

  vdisk_write_block(newInodeInodeBlockReference, &newInodeBlock);
  vdisk_write_block(9, &parentBlockReference);
  vdisk_write_block(newInodeDataBlockReference, &newInodeDataBlock);

  BLOCK masterBlock;
  vdisk_read_block(0, &masterBlock);
  masterBlock.master.inode_allocated_flag[byte - 1] |= (1 << (bit));

  vdisk_write_block(0, &masterBlock);




  // if(!strcmp(cwd, "/")){
    // INODE_REFERENCE inodeRef = -1;
    // for(int i = 0; i < N_INODES; ++i){
    //   INODE inode;
    //   oufs_read_inode_by_reference(i, &inode);
    //   if(inode.size == 0){ //Empty inode
    //       inodeRef = i;
    //       // printf("inodeLocation: %i\n", inodeRef);
    //       break;
    //   }
    // }
    //
    // BLOCK_REFERENCE dataBlockLocation = oufs_allocate_new_block();
    // // printf("dataBlockLocation: %i\n", dataBlockLocation);
    // // return 0;
    //
    // INODE newInode;
    // BLOCK_REFERENCE newInodeBlockRef = inodeRef / INODES_PER_BLOCK + 1;
    // newInode.type = IT_DIRECTORY;
    // newInode.n_references = 1;
    // newInode.data[0] = dataBlockLocation;
    // for(int i = 1; i < BLOCKS_PER_INODE; ++i){
    //   newInode.data[i] = UNALLOCATED_BLOCK;
    // }
    // newInode.size = 2;
    //
    // BLOCK newBlock;
    // vdisk_read_block(newInodeBlockRef, &newBlock);
    // for(int i = 0; i < INODES_PER_BLOCK; ++i){
    //   if(newBlock.inodes.inode[i].size == 0){
    //     newBlock.inodes.inode[i] = newInode;
    //   }
    // }
    //
    //
    // BLOCK rootBlock;
    // vdisk_read_block(9, &rootBlock);
    // for(int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; ++i){
    //   if(rootBlock.directory.entry[i].inode_reference == UNALLOCATED_INODE){
    //     // rootBlock.directory.entry[i].name = path;
    //     strcpy(rootBlock.directory.entry[i].name, path);
    //     rootBlock.directory.entry[i].inode_reference = newInodeBlockRef;
    //   }
    // }
    //
    // BLOCK dirBlock;
    // oufs_clean_directory_block(inodeRef, 0, &dirBlock);
    //
    // vdisk_write_block(newInodeBlockRef, &newBlock);
    // vdisk_write_block(9, &rootBlock);
    // vdisk_write_block(newInodeBlockRef, &dirBlock);


  // }


  //   BLOCK_REFERENCE newInodeBlockRef = inodeRef / INODES_PER_BLOCK + 1;
  //
  //
  //   INODE_REFERENCE parentInodeRef = 0;
  //   BLOCK newBlock;
  //   oufs_clean_directory_block(inodeRef, parentInodeRef, &newBlock);
  //
  //
  //   INODE parentInode;
  //   oufs_read_inode_by_reference(parentInodeRef, &parentInode);
  //
  //   for(int i = 0; i < BLOCKS_PER_INODE; ++i){
  //       if(parentInode.data[i] != UNALLOCATED_BLOCK){
  //         printf("BEFORE: %i\n", parentInode.data[i]);
  //       }
  //   }
  //   // return 0;
  //
  //
  //
  //   // for(int i = 0; i < BLOCKS_PER_INODE; ++i){
  //   //   printf("i: %i\n", i);
  //   //   if(parentInode.data[i] != UNALLOCATED_BLOCK){
  //   //     for(int j = 0; j < DIRECTORY_ENTRIES_PER_BLOCK; ++j){
  //   //       if(parentInode.data[i].entries)
  //   //     }
  //   //     // parentInode.data[i] = inodeRef;
  //   //     break;
  //   //   }
  //   // }
  //
  //   for(int i = 0; i < BLOCKS_PER_INODE; ++i){
  //       if(parentInode.data[i] != UNALLOCATED_BLOCK){
  //         printf("AFTER: %i\n", parentInode.data[i]);
  //       }
  //   }
  //
  // }



  return 0;
}
