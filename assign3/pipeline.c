/**
 * File: pipeline.c
 * ----------------
 * Presents the implementation of the pipeline routine.
 */

#include "pipeline.h"
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>

void pipeline(char *argv1[], char *argv2[], pid_t pids[]) {
  int fds[2];
  pipe(fds);
  pid_t firstChild = fork();
  if (firstChild == 0) {
    close(fds[0]);
    dup2(fds[1], 1);
    close(fds[1]);
    execvp(argv1[0], argv1);
  }
  pid_t secondChild = fork();
  if (secondChild == 0) {
    close(fds[1]);
    dup2(fds[0], 0);
    close(fds[0]);
    execvp(argv2[0], argv2);
  }
  close(fds[0]);
  close(fds[1]);
  pids[0] = firstChild;
  pids[1] = secondChild;
}
