#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "imdb.h"
#include <iostream>
#include <algorithm>
#include <stdio.h>
#include <vector>
#include <string.h>
using namespace std;

const char *const imdb::kActorFileName = "actordata";
const char *const imdb::kMovieFileName = "moviedata";
imdb::imdb(const string& directory) {
  const string actorFileName = directory + "/" + kActorFileName;
  const string movieFileName = directory + "/" + kMovieFileName;  
  actorFile = acquireFileMap(actorFileName, actorInfo);
  movieFile = acquireFileMap(movieFileName, movieInfo);
}

bool imdb::good() const {
  return !( (actorInfo.fd == -1) || 
	    (movieInfo.fd == -1) ); 
}

imdb::~imdb() {
  releaseFileMap(actorInfo);
  releaseFileMap(movieInfo);
}

bool imdb::getCredits(const string& player, vector<film>& films) const {
  const void * file = actorFile; 
  int numActor = *((int *) file);
  int *start = ((int *) file) + 1;
  vector<int> v(start, start + numActor);
  intptr_t target = player.c_str() - (char *) file;
  vector<int>::iterator lower;
  lower  = lower_bound(v.begin(), v.end(), target, [file](intptr_t l, intptr_t r) -> bool {
    printf("%s-----%s\n", (char *) file + l, (char *) file + r);
    return strcmp((char *) file + l, (char *) file + r);
  });
  cout << *lower << endl;
  cout << (lower - v.begin()) << endl;
  cout << numActor << endl;
  return false; 
}

bool imdb::getCast(const film& movie, vector<string>& players) const { 
  return false; 
}

const void *imdb::acquireFileMap(const string& fileName, struct fileInfo& info) {
  struct stat stats;
  stat(fileName.c_str(), &stats);
  info.fileSize = stats.st_size;
  info.fd = open(fileName.c_str(), O_RDONLY);
  return info.fileMap = mmap(0, info.fileSize, PROT_READ, MAP_SHARED, info.fd, 0);
}

void imdb::releaseFileMap(struct fileInfo& info) {
  if (info.fileMap != NULL) munmap((char *) info.fileMap, info.fileSize);
  if (info.fd != -1) close(info.fd);
}
