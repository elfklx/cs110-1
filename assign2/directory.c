#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include "file.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int directory_findname(struct unixfilesystem *fs, const char *name,
                       int dirinumber, struct direntv6 *dirEnt) {
  int nameLen = strlen(name);
  if (nameLen > 14) {
    fprintf(stderr, "file name greater than 14: %s\n", name);
    return -1;
  }
  struct inode dirInode;
  if (inode_iget(fs, dirinumber, &dirInode) == -1) {
    fprintf(stderr, "error occurred when calling inode_iget.\n");
    return -1;
  }
  if ((dirInode.i_mode & IFMT) != IFDIR) {
    fprintf(stderr, "the given inode %d is not a directory.\n", dirinumber);
    return -1;
  }
  int dirSize = inode_getsize(&dirInode);
  if (dirSize == 0) return -1; // empty directory
  char dirBuffer[dirSize];
  int numBlocks = dirSize / DISKIMG_SECTOR_SIZE + ((dirSize % DISKIMG_SECTOR_SIZE != 0) ? 1 : 0);
  for (int i = 0; i < numBlocks; i++) {
    int numValidBytes = file_getblock(fs, dirinumber, i, dirBuffer + i * DISKIMG_SECTOR_SIZE);
    if (numValidBytes == -1) return -1;
  }
  size_t dirEntSize = sizeof(struct direntv6);
  int numDirEnt = dirSize / dirEntSize;
  for (int i = 0; i < numDirEnt; i++) {
    char *dirEntP = dirBuffer + dirEntSize * i;
    if (memcmp(dirEntP + sizeof(uint16_t), name, nameLen) == 0) {
      memcpy(dirEnt, dirEntP, dirEntSize);
      return 0;
    }
  }
  return -1; // not found
}
