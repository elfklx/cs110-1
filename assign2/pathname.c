#include "pathname.h"
#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

int pathname_lookup_recursive(struct unixfilesystem *fs, const char *pathname, int dirinumber) {
  if (strlen(pathname) == 0 || (*pathname) != '/') {
    fprintf(stderr, "wrong pathname syntax:%s\n", pathname);
    return -1;
  } else if (strlen(pathname) == 1) {
    return dirinumber;
  }
  char *nextSlash = strchr(pathname + 1, '/');
  char *dirEntName;
  if (nextSlash) {
    size_t dirEntNameLen = nextSlash - pathname;
    dirEntName = (char *) malloc(dirEntNameLen);
    assert(dirEntName != NULL);
    memcpy(dirEntName, pathname + 1, dirEntNameLen);
    dirEntName[dirEntNameLen - 1] = '\0';
  } else {
    dirEntName = pathname + 1;
  }
  struct direntv6 dirEnt;
  if (directory_findname(fs, dirEntName, dirinumber, &dirEnt) < 0) {
    return -1;
  }
  if (nextSlash) {
    free(dirEntName);
    return pathname_lookup_recursive(fs, nextSlash, dirEnt.d_inumber);
  } else {
    return dirEnt.d_inumber;
  }
}

int pathname_lookup(struct unixfilesystem *fs, const char *pathname) {
  return pathname_lookup_recursive(fs, pathname, 1);
}
