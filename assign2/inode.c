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
  if (numRead == -1 || numRead != DISKIMG_SECTOR_SIZE) {
    fprintf(stderr, "error occurred when calling diskimg_readsector.\n");
    return -1;
  }
  memcpy(inp, buffer + locationInSector, inodeSize);
  return 0;
}

int inode_indexlookup(struct unixfilesystem *fs, struct inode *inp, int blockNum) {
  if ((inp->i_mode & IALLOC) == 0) {
    fprintf(stderr, "inode is unallocated.\n");
    return -1;
  }

  int fileSize = inode_getsize(inp);
  int numBlocks = fileSize / DISKIMG_SECTOR_SIZE + ((fileSize % DISKIMG_SECTOR_SIZE != 0) ? 1 : 0);
  if (blockNum < 0 || blockNum >= numBlocks) {
    fprintf(stderr, "0 <= (blockNum=%d) < %d not satisfied.\n", blockNum, numBlocks);
    return -1;
  }

  if ((inp->i_mode & ILARG) == 0) {
    if (blockNum >= N_BLOCKS) return -1;
    return inp->i_addr[blockNum];
  } else {
    uint16_t actualBlockNum;
    int numPerBlock = DISKIMG_SECTOR_SIZE / sizeof(uint16_t);
    int firstIndirectIndex = blockNum / numPerBlock;
    firstIndirectIndex = (firstIndirectIndex < N_BLOCKS - 1) ? firstIndirectIndex : (N_BLOCKS - 1);
    char buffer[DISKIMG_SECTOR_SIZE];
    int numRead = diskimg_readsector(fs->dfd, inp->i_addr[firstIndirectIndex], buffer);
    if (numRead == -1 || numRead != DISKIMG_SECTOR_SIZE) return -1;
    if (firstIndirectIndex != N_BLOCKS - 1) { // singly indirect
      actualBlockNum = *(((uint16_t *) buffer) + blockNum % numPerBlock);
    } else { // doubly indirect
      int restBlockNum = blockNum - numPerBlock * (N_BLOCKS - 1);
      int secondIndirectIndex = restBlockNum / numPerBlock;
      if (secondIndirectIndex >= numPerBlock) {
        fprintf(stderr, "file size %d not supported.\n", fileSize);
        return -1;
      }
      uint16_t singlyIndirectBlockNum = *(((uint16_t *) buffer) + secondIndirectIndex);
      int numRead = diskimg_readsector(fs->dfd, singlyIndirectBlockNum, buffer);
      if (numRead == -1 || numRead != DISKIMG_SECTOR_SIZE) return -1;
      actualBlockNum = *(((uint16_t *) buffer) + restBlockNum % numPerBlock);
    }
    return actualBlockNum;
  }
}

int inode_getsize(struct inode *inp) {
  return ((inp->i_size0 << 16) | inp->i_size1); 
}
