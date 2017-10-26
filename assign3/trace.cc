/**
 * File: trace.cc
 * ----------------
 * Presents the implementation of the trace program, which traces the execution of another
 * program and prints out information about ever single system call it makes.  For each system call,
 * trace prints:
 *
 *    + the name of the system call,
 *    + the values of all of its arguments, and
 *    + the system calls return value
 */

#include <cassert>
#include <iostream>
#include <map>
#include <set>
#include <unistd.h> // for fork, execvp
#include <string.h> // for memchr, strerror
#include <sys/ptrace.h>
#include <sys/reg.h>
#include <sys/wait.h>
#include "trace-options.h"
#include "trace-error-constants.h"
#include "trace-system-calls.h"
#include "trace-exception.h"
using namespace std;

int wait_for_syscall(pid_t child, int &status);

int main(int argc, char *argv[]) {
  bool simple = false, rebuild = false;
  int numFlags = processCommandLineFlags(simple, rebuild, argv);
  if (argc - numFlags == 1) {
    cout << "Nothing to trace... exiting." << endl;
    return 0;
  }

  // load system calls
  map<int, string> errorConstants;
  compileSystemCallErrorStrings(errorConstants);
  map<int, string> systemCallNumbers;
  map<string, int> systemCallNames;
  map<string, systemCallSignature> systemCallSignatures;
  compileSystemCallData(systemCallNumbers, systemCallNames, systemCallSignatures, rebuild);

  pid_t child = fork();
  if (child == 0) {
    char *args[argc - numFlags];
    memcpy(args, argv + 1 + numFlags, (argc - 1 - numFlags) * sizeof(char *));
    args[argc - numFlags - 1] = NULL;
    ptrace(PTRACE_TRACEME);
    raise(SIGSTOP);
    execvp(args[0], args);
  } else {
    int status;
    long syscall, retval;
    waitpid(child, &status, 0);
    ptrace(PTRACE_SETOPTIONS, child, 0, PTRACE_O_TRACESYSGOOD);
    while (1) {
      if (wait_for_syscall(child, status) != 0) break;
      syscall = ptrace(PTRACE_PEEKUSER, child, ORIG_RAX * sizeof(long));
      printf("syscall(%ld) = ", syscall);
      if (wait_for_syscall(child, status) != 0) {
        printf("<no return>\n");
        break;
      }
      retval = ptrace(PTRACE_PEEKUSER, child, RAX * sizeof(long));
      printf("%ld\n", retval);
    }
    printf("Program exited normally with status %d\n", WEXITSTATUS(status));
  }
  return 0;
}

int wait_for_syscall(pid_t child, int &status) {
  while (1) {
    ptrace(PTRACE_SYSCALL, child, 0, 0);
    waitpid(child, &status, 0);
    if (WIFSTOPPED(status) && WSTOPSIG(status) & 0x80) return 0;
    if (WIFEXITED(status)) return 1;
  }
}
