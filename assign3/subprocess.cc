/**
 * File: subprocess.cc
 * -------------------
 * Presents the implementation of the subprocess routine.
 */

#include "subprocess.h"
using namespace std;

subprocess_t subprocess(char *argv[], bool supplyChildInput, bool ingestChildOutput) throw (SubprocessException) {
  int supplyChildInputFds[2];
  int ingestChildOutputFds[2];
  int sp_supplyfd = kNotInUse;
  int sp_ingestfd = kNotInUse;
  if (supplyChildInput) {
    pipe(supplyChildInputFds);
    sp_supplyfd = supplyChildInputFds[1];
  }
  if (ingestChildOutput) {
    pipe(ingestChildOutputFds);
    sp_ingestfd = ingestChildOutputFds[0];
  }
  pid_t sp_pid = fork();
  if (sp_pid < 0) {
    throw SubprocessException("subprocess: fork failed.");
  }
  subprocess_t sp = {sp_pid, sp_supplyfd, sp_ingestfd};
  if (sp.pid == 0) {
    if (supplyChildInput) {
      close(supplyChildInputFds[1]);
      dup2(supplyChildInputFds[0], 0);
      close(supplyChildInputFds[0]);
    }
    if (ingestChildOutput) {
      close(ingestChildOutputFds[0]);
      dup2(ingestChildOutputFds[1], 1);
      close(ingestChildOutputFds[1]);
    }
    execvp(argv[0], argv);
    // if execvp succeed, it shouldn't reach here
    throw SubprocessException("subprocess: error occurred in subprocess.");
  }
  if (supplyChildInput) {
    close(supplyChildInputFds[0]);
  }
  if (ingestChildOutput) {
    close(ingestChildOutputFds[1]);
  }
  return sp;
}
