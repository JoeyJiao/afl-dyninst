#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <cstring>
#include <vector>
#include <algorithm>
#include "../afl/config.h"
#include <sys/types.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

static u8 *trace_bits;
static s32 shm_id;
static int __afl_temp_data;
static pid_t __afl_fork_pid;
static unsigned short prev_id;
static long saved_di;
register long rdi asm("di");  // the warning is fine - we need the warning because of a bug in dyninst

#define PRINT_ERROR(string) (void)(write(2, string, strlen(string))+1) // the (...+1) weirdness is so we do not get an ignoring return value warning

void initAflForkServer() {
  // we can not use fprint* stdout/stderr functions here, it fucks up some programs
  char *shm_env_var = getenv(SHM_ENV_VAR);

  if (!shm_env_var) {
    PRINT_ERROR("Error getting shm\n");
    return;
  }
  shm_id = atoi(shm_env_var);
  trace_bits = (u8 *) shmat(shm_id, NULL, 0);
  if (trace_bits == (u8 *) - 1) {
    PRINT_ERROR("Error: shmat\n");
    return;
  }
  // enter fork() server thyme!
  int n = write(FORKSRV_FD + 1, &__afl_temp_data, 4);

  if (n != 4) {
    PRINT_ERROR("Error writing fork server\n");
    return;
  }
  while (1) {
    n = read(FORKSRV_FD, &__afl_temp_data, 4);
    if (n != 4) {
      PRINT_ERROR("Error reading fork server\n");
      return;
    }

    __afl_fork_pid = fork();
    if (__afl_fork_pid < 0) {
      PRINT_ERROR("Error on fork()\n");
      return;
    }
    if (__afl_fork_pid == 0) {
      close(FORKSRV_FD);
      close(FORKSRV_FD + 1);
      break;
    } else {
      // parrent stuff
      n = write(FORKSRV_FD + 1, &__afl_fork_pid, 4);
      pid_t temp_pid = waitpid(__afl_fork_pid, &__afl_temp_data, 2);

      if (temp_pid == 0) {
        return;
      }
      n = write(FORKSRV_FD + 1, &__afl_temp_data, 4);
    }
  }
}

// Should be called on basic block entry
void bbCallback(unsigned short id) {
  if (trace_bits) {
    trace_bits[prev_id ^ id]++;
    prev_id = id >> 1;
  }
}

void forceCleanExit() {
  exit(0);
}

void save_rdi() {
  saved_di = rdi;
}

void restore_rdi() {
  rdi = saved_di;
}


void initOnlyAflForkServer() {
  // enter fork() server thyme!
  int n = write(FORKSRV_FD + 1, &__afl_temp_data, 4);

  if (n != 4) {
    PRINT_ERROR("Error writting fork server\n");
    return;
  }
  while (1) {
    n = read(FORKSRV_FD, &__afl_temp_data, 4);
    if (n != 4) {
      PRINT_ERROR("Error reading fork server\n");
      return;
    }

    __afl_fork_pid = fork();
    if (__afl_fork_pid < 0) {
      PRINT_ERROR("Error on fork()\n");
      return;
    }
    if (__afl_fork_pid == 0) {
      close(FORKSRV_FD);
      close(FORKSRV_FD + 1);
      break;
    } else {
      // parrent stuff
      n = write(FORKSRV_FD + 1, &__afl_fork_pid, 4);
      pid_t temp_pid = waitpid(__afl_fork_pid, &__afl_temp_data, 2);

      if (temp_pid == 0) {
        return;
      }
      n = write(FORKSRV_FD + 1, &__afl_temp_data, 4);
    }
  }
}
