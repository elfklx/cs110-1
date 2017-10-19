#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "inode.h"
#include "diskimg.h"

int inode_iget(struct unixfilesystem *fs, int inumber, struct inode *inp) {
  size_t inodeSize = sizeof(struct inode);
  int maxInumber = (fs->superblock).s_isize * DISKIMG_SECTOR_SIZE / inodeSize;
  if (inumber <= 0 || inumber > maxInumber) {
    fprintf(stderr, "0 < (inumber=%d) <= %d not satisfied.\n", inumber, maxInumber);
    return -1;
  }

  int sectorNum = (inumber - 1) * inodeSize / DISKIMG_SECTOR_SIZE + INODE_START_SECTOR;
  int locationInSector = (inumber - 1) * inodeSize % DISKIMG_SECTOR_SIZE;
  char buffer[DISKIMG_SECTOR_SIZE];
  int numRead = diskimg_readsector(fs->dfd, sectorNum, buffer);
  if (numRead == -1) {
    fprintf(stderr, "error occurred when calling diskimg_readsector.\n");
    return -1;
  } else if (numRead != DISKIMG_SECTOR_SIZE) {
    fprintf(stderr, "diskimg_readsector: num of bytes read from sector %d doesn't match DISKIMG_SECTOR_SIZE %d.\n",
      sectorNum, DISKIMG_SECTOR_SIZE);
    return -1;
  }
  memcpy(inp, buffer + locationInSector, inodeSize);
  return 0;
}

int inode_indexlookup(struct unixfilesystem *fs, struct inode *inp, int blockNum) {
  if (inp->i_mode & IALLOC == 0) {
    fprintf(stderr, "inode is unallocated.\n");
    return -1;
  }
  // if (inp->i_mode & ILARG != 0) {
  //   fprintf(stderr, "Large file is not supported yet.\n");
  //   return -1;
  // }
  int fileSize = inode_getsize(inp);
  int numBlocks = fileSize / DISKIMG_SECTOR_SIZE + (fileSize % DISKIMG_SECTOR_SIZE != 0);
  if (blockNum < 0 || blockNum >= numBlocks) {
    fprintf(stderr, "0 <= (blockNum=%d) < %d not satisfied.\n", blockNum, numBlocks);
    return -1;
  }
  return inp->i_addr[blockNum];
}

int inode_getsize(struct inode *inp) {
  return ((inp->i_size0 << 16) | inp->i_size1); 
}
