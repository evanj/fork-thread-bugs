/*
Compile with: clang -Wall -Wextra -Werror -o mac_crash mac_crash.c -lsqlite3
C version of the crash from osx_python_crash_v1.py

Why it crashes:

libdispatch: queue.c has as atfork handler that explicitly sets:
linked list pointers to a crash value.
sqlite3 schedules stuff in the main thread ... for reasons I don't understand
it explicitly calls dispatch_async from sqlite3_initialize, passing the main queue
*/

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

#include <sqlite3.h>

#include <dispatch/dispatch.h>

// Opens then closes a SQLite in-memory database
void open_close_sqlite_memory() {
  sqlite3* db = NULL;
  int err = sqlite3_open(":memory:", &db);
  if (err != SQLITE_OK) {
    printf("sqlite3_open failed\n");
    return;
  }
  err = sqlite3_close(db);
  if (err != SQLITE_OK) {
    printf("sqlite3_close failed\n");
    return;
  }
  printf("sqlite3 opened and closed\n");
}

void print_context(void* context) {
  printf("print_context called with context=%p\n", context);
}

void use_libdispatch_main_queue() {
  // adds a trivial function from the libdispatch main queue (which should never get run)
  dispatch_async_f(dispatch_get_main_queue(), NULL, print_context);
}

int main() {
  use_libdispatch_main_queue();
  // if the parent uses SQLite before the fork, the child won't touch the libdispatch main queue
  // and will not crash
  // open_close_sqlite_memory();

  pid_t pid = fork();
  if (pid > 0) {
    printf("parent waiting for pid %d\n", pid);
    int result = 0;
    waitpid(pid, &result, 0);
    printf("parent waitpid result: %d WTERMSIG: %d\n", result, WTERMSIG(result));
  } else {
    printf("child calling sqlite\n");
    open_close_sqlite_memory();
    // the child will also crash if it instead uses the libdispatch main queue (which is what
    // libsqlite3 is doing ... I think
    // use_libdispatch_main_queue();
    printf("child executed correctly\n");
  }
  return 0;
}