#include <cassert>
#include <ctime>
#include <cctype>
#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <sys/wait.h>
#include <unistd.h>
#include <sched.h>
#include "subprocess.h"

using namespace std;

struct worker {
  worker() {}
  worker(char *argv[]) : sp(subprocess(argv, true, false)), available(false) {}
  subprocess_t sp;
  bool available;
};

static const size_t kNumCPUs = sysconf(_SC_NPROCESSORS_ONLN);
static vector<worker> workers(kNumCPUs);
static size_t numWorkersAvailable = 0;

static void markWorkersAsAvailable(int sig) {
  while (true) {
    pid_t pid = waitpid(-1, NULL, WNOHANG | WUNTRACED);
    if (pid <= 0) break;
    for (worker& w : workers) {
      if (w.sp.pid == pid) {
        w.available = true;
        numWorkersAvailable++;
        break;
      }
    }
  }
}

static const char *kWorkerArguments[] = {"./factor.py", "--self-halting", NULL};
static void spawnAllWorkers() {
  // TODO specify CPU
  cout << "There are this many CPUs: " << kNumCPUs << ", numbered 0 through " << kNumCPUs - 1 << "." << endl;
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGCHLD);
  for (size_t i = 0; i < kNumCPUs; i++) {
    // sigprocmask(SIG_BLOCK, &mask, NULL); // block SIGCHLD
    struct worker w(const_cast<char **>(kWorkerArguments));
    workers[i] = w;
    // sigprocmask(SIG_UNBLOCK, &mask, NULL);
    cout << "Worker " << workers[i].sp.pid << " is set to run on CPU " << i << "." << endl;
  }
}

static size_t getAvailableWorker() {
  return 0;
}

static void broadcastNumbersToWorkers() {
  while (true) {
    string line;
    getline(cin, line);
    if (cin.fail()) break;
    size_t endpos;
    long long num = stoll(line, &endpos);
    if (endpos != line.size()) break;
    // you shouldn't need all that many lines of additional code
    size_t availableWorker = getAvailableWorker();
    workers[availableWorker].available = false;
    write(workers[availableWorker].sp.supplyfd, line.c_str(), line.size());
    write(workers[availableWorker].sp.supplyfd, "\n", 2);
  }
}

// wait for all workers to be available
static void waitForAllWorkers() {
  while (true) {
    bool allAvailable = true;
    for (const worker& w : workers) {
      if (!w.available) {
        allAvailable = false;
        break;
      }
    }
    if (allAvailable) return;
  }
}

static void closeAllWorkers() {
  signal(SIGCHLD, SIG_DFL);
  for (const worker& w : workers) {
    close(w.sp.supplyfd);
    kill(w.sp.pid, SIGCONT);
  }
  while (true) {
    pid_t pid = waitpid(-1, NULL, 0);
    if (pid == -1) break;
  }
}

int main(int argc, char *argv[]) {
  signal(SIGCHLD, markWorkersAsAvailable);
  spawnAllWorkers();
  // broadcastNumbersToWorkers();
  waitForAllWorkers();
  closeAllWorkers();
  return 0;
}
