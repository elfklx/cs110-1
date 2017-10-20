#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include "file.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int directory_findname(struct unixfilesystem *fs, const char *name,
                       int dirinumber, struct direntv6 *dirEnt) {
  struct inode dirInode;
  if (inode_iget(fs, dirinumber, &dirInode) == -1) {
    fprintf(stderr, "error occurred when calling inode_iget.\n");
    return -1;
  }
  if ((dirInode.i_mode & IFMT) != IFDIR) {
    fprintf(stderr, "the given inode %d is not a directory.\n", dirinumber);
    return -1;
  }
  fprintf(stderr, "directory_lookupname(name=%s dirinumber=%d)\n", name, dirinumber);
  return -1;
}
