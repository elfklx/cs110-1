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

static const int systemCallArgRegs[8] = {RDI, RSI, RDX, R10, R8, R9};

int wait_for_syscall(pid_t child, int &status);
char *read_string(pid_t child, void *addr);

/*
 * Ref: https://blog.nelhage.com/2010/08/write-yourself-an-strace-in-70-lines-of-code/
 */
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
      if (simple) {
        printf("syscall(%ld) = ", syscall);
      } else {
        string syscallName = systemCallNumbers[syscall];
        printf("%s(", syscallName.c_str());
        if (systemCallSignatures.find(syscallName) == systemCallSignatures.end()) {
          printf("<signature-information-missing>");
        } else {
          systemCallSignature arguments = systemCallSignatures[syscallName];
          for (size_t i = 0; i < arguments.size(); i++) {
            auto arg = arguments[i];
            long argVal = ptrace(PTRACE_PEEKUSER, child, systemCallArgRegs[i] * sizeof(long));
            if (arg == SYSCALL_INTEGER) {
              printf("%ld", argVal);
            } else if (arg == SYSCALL_STRING) {
              char *strArgVal = read_string(child, (void *) argVal);
              printf("\"%s\"", strArgVal);
              free(strArgVal);
            } else if (arg == SYSCALL_POINTER) {
              argVal == 0 ? printf("NULL") : printf("%#lx", argVal);
            } else {
              cout << "SYSCALL_UNKNOWN_TYPE";
            }
            if (i != arguments.size() - 1) {
              cout << ", ";
            }
          }
        }
        cout << ") = ";
      }
      if (wait_for_syscall(child, status) != 0) {
        printf("<no return>\n");
        break;
      }
      retval = ptrace(PTRACE_PEEKUSER, child, RAX * sizeof(long));
      if (simple) {
        printf("%ld\n", retval);
      } else {
        if (syscall == systemCallNames["brk"] || syscall == systemCallNames["mmap"]) {
          // ignore brk, mmap failure
          printf("%#lx\n", retval);
        } else if (retval < 0) {
          // system call error
          printf("%d %s (%s)\n", -1, errorConstants[-retval].c_str(), strerror(-retval));
        } else {
          printf("%ld\n", retval);
        }
      }
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

char *read_string(pid_t child, void *addr) {
  char *val = (char *) malloc(4096);
  size_t allocated = 4096, read = 0;
  while (1) {
    if (read + sizeof(long) > allocated) {
      allocated *= 2;
      val = (char *) realloc(val, allocated);
    }
    long tmp = ptrace(PTRACE_PEEKDATA, child, (char *) addr + read);
    if (errno != 0) {
      val[read] = 0;
      break;
    }
    memcpy(val + read, &tmp, sizeof(tmp));
    if (memchr(&tmp, 0, sizeof(tmp)) != NULL) break;
    read += sizeof(tmp);
  }
  return val;
}
