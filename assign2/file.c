#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "file.h"
#include "inode.h"
#include "diskimg.h"

#define MIN(a,b) (((a)<(b))?(a):(b))

int file_getblock(struct unixfilesystem *fs, int inumber, int blockNum, void *buf) {
  struct inode in;
  if (inode_iget(fs, inumber, &in) == -1) {
    fprintf(stderr, "error occurred when calling inode_iget.\n");
    return -1;
  }
  int actualBlockNum = inode_indexlookup(fs, &in, blockNum);
  if (actualBlockNum == -1) {
    fprintf(stderr, "error occurred when calling inode_indexlookup.\n");
    return -1;
  }
  int fileSize = inode_getsize(&in);
  int numValidBytes = MIN(fileSize - blockNum * DISKIMG_SECTOR_SIZE, DISKIMG_SECTOR_SIZE);
  char fileBuffer[DISKIMG_SECTOR_SIZE];
  int numRead = diskimg_readsector(fs->dfd, actualBlockNum, fileBuffer);
  if (numRead == -1 || numRead != DISKIMG_SECTOR_SIZE) {
    fprintf(stderr, "error occurred when calling diskimg_readsector.\n");
    return -1;
  }
  memcpy(buf, fileBuffer, numValidBytes);
  return numValidBytes;
}
