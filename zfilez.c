#include <stdio.h>
#include <string.h>

#include "oufs.h"
#include "oufs_lib.h"

int main(int argc, char** argv){
  //Get working directory for the OUFS
  //Use oufs_get_environment
  char cwd[MAX_PATH_LENGTH];
  char diskName[MAX_PATH_LENGTH];
  oufs_get_environment(cwd, diskName);

  vdisk_disk_open(diskName);

  if(argc == 2)
    oufs_list(cwd, argv[1]);
  if(argc == 1)
    oufs_list(cwd, "");

  vdisk_disk_close();


  return 0;
}
