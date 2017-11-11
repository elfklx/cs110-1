/**
 * File: stsh.cc
 * -------------
 * Defines the entry point of the stsh executable.
 */

#include "stsh-parser/stsh-parse.h"
#include "stsh-parser/stsh-readline.h"
#include "stsh-parser/stsh-parse-exception.h"
#include "stsh-signal.h"
#include "stsh-job-list.h"
#include "stsh-job.h"
#include "stsh-parse-utils.h"
#include "stsh-process.h"
#include <cstring>
#include <iostream>
#include <string>
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>  // for fork
#include <signal.h>  // for kill
#include <sys/wait.h>
#include <cassert>
using namespace std;

static STSHJobList joblist; // the one piece of global data we need so signal handlers can access it
static const string kFgUsage = "Usage: fg <jobid>.";
static const string kSlayUsage = "Usage: slay <jobid> <index> | <pid>.";

static size_t getArgvLen(const command& cmd) {
  for (size_t i = 0; i < kMaxArguments + 1; i++) {
    if (cmd.tokens[i] == NULL) return i;
  }
  return kMaxArguments;
}

static void updateJobList(STSHJobList& jobList, pid_t pid, STSHProcessState state) {
  if (!jobList.containsProcess(pid)) return;
  STSHJob& job = jobList.getJobWithProcess(pid);
  assert(job.containsProcess(pid));
  STSHProcess& process = job.getProcess(pid);
  process.setState(state);
  jobList.synchronize(job);
}

static void waitForFgJobToFinish() {
  sigset_t additions, existingmask;
  sigemptyset(&additions);
  sigaddset(&additions, SIGCHLD);
  sigprocmask(SIG_BLOCK, &additions, &existingmask);
  while (joblist.hasForegroundJob()) {
    sigsuspend(&existingmask);
  }
  sigprocmask(SIG_UNBLOCK, &additions, NULL);
}

static void fg(const command& cmd) {
  const string& usage = kFgUsage;
  // input sanity check
  size_t argc = getArgvLen(cmd);
  if (argc != 1) throw STSHException(usage);
  size_t jobnum = parseNumber(cmd.tokens[0], usage);

  if (!joblist.containsJob(jobnum)) {
    throw STSHException("fg " + to_string(jobnum) + ": No such job.");
  }
  STSHJob& job = joblist.getJob(jobnum);
  assert(job.getState() != kForeground);
  for (const auto& p : job.getProcesses()) {
    if (p.getState() == kStopped) {
      kill(-job.getGroupID(), SIGCONT);
      job.setState(kForeground);
      waitForFgJobToFinish();
      return;
    }
  }
}

static void slay(const command& cmd) {
  const string& usage = kSlayUsage;
}

/**
 * Function: handleBuiltin
 * -----------------------
 * Examines the leading command of the provided pipeline to see if
 * it's a shell builtin, and if so, handles and executes it.  handleBuiltin
 * returns true if the command is a builtin, and false otherwise.
 */
static const string kSupportedBuiltins[] = {"quit", "exit", "fg", "bg", "slay", "halt", "cont", "jobs"};
static const size_t kNumSupportedBuiltins = sizeof(kSupportedBuiltins)/sizeof(kSupportedBuiltins[0]);
static bool handleBuiltin(const pipeline& pipeline) {
  const command& cmd = pipeline.commands[0];
  const string& command = cmd.command;
  auto iter = find(kSupportedBuiltins, kSupportedBuiltins + kNumSupportedBuiltins, command);
  if (iter == kSupportedBuiltins + kNumSupportedBuiltins) return false;
  size_t index = iter - kSupportedBuiltins;

  switch (index) {
  case 0:
  case 1: exit(0);
  case 2: fg(cmd); break;
  case 4: slay(cmd); break;
  case 7: cout << joblist; break;
  default: throw STSHException("Internal Error: Builtin command not supported."); // or not implemented yet
  }
  
  return true;
}

static void reapChild(int sig) {
  pid_t pid;
  while (true) {
    int status;
    pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED);
    if (pid <= 0) break;
    STSHProcessState state;
    if (WIFEXITED(status) || WIFSIGNALED(status)) {
      state = kTerminated;
    } else if (WIFSTOPPED(status)) {
      state = kStopped;
    } else if (WIFCONTINUED(status)) {
      state = kRunning;
    } else {
      state = kTerminated;
    }
    updateJobList(joblist, pid, state);
  }
}

static void passSigToFgJob(int sig) {
  if (joblist.hasForegroundJob()) {
    kill(-joblist.getForegroundJob().getGroupID(), sig);
  }
}

/**
 * Function: installSignalHandlers
 * -------------------------------
 * Installs user-defined signals handlers for four signals
 * (once you've implemented signal handlers for SIGCHLD, 
 * SIGINT, and SIGTSTP, you'll add more installSignalHandler calls) and 
 * ignores two others.
 */
static void installSignalHandlers() {
  installSignalHandler(SIGQUIT, [](int sig) { exit(0); });
  installSignalHandler(SIGTTIN, SIG_IGN);
  installSignalHandler(SIGTTOU, SIG_IGN);
  installSignalHandler(SIGCHLD, reapChild);
  installSignalHandler(SIGINT, passSigToFgJob);
  installSignalHandler(SIGTSTP, passSigToFgJob);
}

/**
 * Function: createJob
 * -------------------
 * Creates a new job on behalf of the provided pipeline.
 */
static void createJob(const pipeline& p) {
  pid_t pid = fork();
  if (pid == 0) {
    assert(setpgid(0, 0) == 0);
    command cmd = p.commands[0];
    char *new_argv[kMaxArguments + 1 + 1];
    new_argv[0] = cmd.command;
    for (size_t i = 1; i <= kMaxArguments + 1; i++) {
      new_argv[i] = cmd.tokens[i - 1];
      if (new_argv[i] == NULL) break;
    }
    execvp(cmd.command, new_argv);
    exit(1); // TODO: how to handle this?
  }
  STSHJobState jobState = kForeground;
  if (p.background) jobState = kBackground;
  STSHJob& job = joblist.addJob(jobState);
  job.addProcess(STSHProcess(pid, p.commands[0]));
  if (p.background) {
    cout << "[" << job.getNum() << "]";
    for (const auto& p : job.getProcesses()) {
      cout << " " << p.getID();
    }
    cout << endl;
    return;
  }
  waitForFgJobToFinish();
}

/**
 * Function: main
 * --------------
 * Defines the entry point for a process running stsh.
 * The main function is little more than a read-eval-print
 * loop (i.e. a repl).  
 */
int main(int argc, char *argv[]) {
  pid_t stshpid = getpid();
  installSignalHandlers();
  rlinit(argc, argv); // configures stsh-readline library so readline works properly
  while (true) {
    string line;
    if (!readline(line)) break;
    if (line.empty()) continue;
    try {
      pipeline p(line);
      bool builtin = handleBuiltin(p);
      if (!builtin) createJob(p);
    } catch (const STSHException& e) {
      cerr << e.what() << endl;
      if (getpid() != stshpid) exit(0); // if exception is thrown from child process, kill it
    }
  }

  return 0;
}

// static void addToJobList(STSHJobList& jobList, const vector<pair<pid_t, string>>& children) {
//   STSHJob& job = jobList.addJob(kBackground);
//   for (const pair<string, pid_t>& child: children) {
//     pid_t pid = child.first;
//     const string& command = child.second;
//     job.addProcess(STSHProcess(pid, command)); // third argument defaults to kRunning
//   }
//   cout << jobList;
// }

