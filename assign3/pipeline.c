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
  pid_t twinProcessPid = fork();
  if (twinProcessPid == 0) {
    close(fds[1]); // close child input
    dup2(fds[0], 0); // child input to stdin
    close(fds[0]);
    execvp(argv2[0], argv2);
  }
  pids[0] = getpid();
  pids[1] = twinProcessPid;
  close(fds[0]); // close parent output
  dup2(fds[1], 1); // parent output to stdout
  close(fds[1]);
  execvp(argv1[0], argv1);
}
