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
#include <assert.h>
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

vector<int> imdb::getRecordNodes(const void *file) const {
  int numRecord = *((int *) file);
  int *start = ((int *) file) + 1;
  vector<int> v(start, start + numRecord);
  return v;
}

film imdb::getFilm(char *movieRecord) const {
  film f;
  f.title = movieRecord;
  int titleLen = strlen(movieRecord) + 1;
  char *year = movieRecord + titleLen;
  f.year = 1900 + (*year);
  return f;
}

bool imdb::getCredits(const string& player, vector<film>& films) const {
  const void *file = actorFile;
  vector<int> v = getRecordNodes(file);
  intptr_t target = player.c_str() - (char *) file;
  vector<int>::iterator lower;
  lower = lower_bound(v.begin(), v.end(), target, [file](intptr_t l, intptr_t r) -> bool {
    return strcmp((char *) file + l, (char *) file + r) < 0;
  });
  char *actorRecord = (char *) file + (*lower);
  bool found = strcmp(actorRecord, player.c_str()) == 0;
  if (!found) return false;
  int nameLen = strlen(actorRecord) + 1;
  if (nameLen % 2 == 1) nameLen += 1;
  int headerLen = nameLen + 2;
  if (headerLen % 4 != 0) headerLen += 2;
  short actorNumMovie = *(short *)(actorRecord + nameLen);
  int *actorMoviePayload = (int *)(actorRecord + headerLen);
  for (short i = 0; i < actorNumMovie; i++) {
    int *curr = actorMoviePayload + i;
    char *movieRecord = (char *) movieFile + (*curr);
    film f = getFilm(movieRecord);
    films.push_back(f);
  }
  return true;
}

bool imdb::getCast(const film& movie, vector<string>& players) const {
  const void *file = movieFile;
  vector<int> v = getRecordNodes(file);
  char *movieCopy = (char *) malloc(strlen(movie.title.c_str()) + 1 + 1);
  assert(movieCopy != NULL);
  memcpy(movieCopy, movie.title.c_str(), strlen(movie.title.c_str()) + 1);
  char year = movie.year - 1900;
  memcpy(movieCopy + strlen(movie.title.c_str()) + 1, &year, 1);
  intptr_t target = movieCopy - (char *) file;
  vector<int>::iterator lower;
  lower = lower_bound(v.begin(), v.end(), target, [this,file](intptr_t l, intptr_t r) -> bool {
    film fl = getFilm((char *) file + l);
    film fr = getFilm((char *) file + r);
    return fl < fr;
  });
  char *movieRecord = (char *) file + (*lower);
  film movie_found = getFilm(movieRecord);
  bool found = movie_found == movie;
  if (!found) {
    players.clear();
    return false;
  }
  int nameLen = strlen(movieRecord) + 1 + 1;
  if (nameLen % 2 == 1) nameLen += 1;
  int headerLen = nameLen + 2;
  if (headerLen % 4 != 0) headerLen += 2;
  short movieNumActor = *(short *)(movieRecord + nameLen);
  int *movieActorPayload = (int *)(movieRecord + headerLen);
  for (short i = 0; i < movieNumActor; i++) {
    int *curr = movieActorPayload + i;
    char *actorRecord = (char *) actorFile + (*curr);
    string player = actorRecord;
    players.push_back(player);
  }
  free(movieCopy);
  return true;
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
