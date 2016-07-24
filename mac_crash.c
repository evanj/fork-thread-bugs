// https://bugs.python.org/issue13829
// http://bugs.python.org/issue27126
// http://www.linuxprogrammingblog.com/threads-and-fork-think-twice-before-using-them
// https://github.com/google/googletest/blob/master/googletest/docs/AdvancedGuide.md#death-tests-and-threads
// http://thorstenball.com/blog/2014/10/13/why-threads-cant-fork/
// https://github.com/google/googletest/issues/33
// https://mail.scipy.org/pipermail/numpy-discussion/2012-August/063593.html

// libdispatch: queue.c has as atfork handler that explicitly sets:
// linked list pointers to a crash value.
// sqlite3 schedules stuff in the main thread ... for reasons I don't understand
// it explicitly calls dispatch_async from sqlite3_initialize, passing the main queue
// 
// DISPATCH_EXPORT DISPATCH_NOTHROW
// void
// dispatch_atfork_child(void)
// {
//   void *crash = (void *)0x100;
//   size_t i;

// #if HAVE_MACH
//   _dispatch_mach_host_port_pred = 0;
//   _dispatch_mach_host_port = MACH_VOUCHER_NULL;
// #endif
//   _voucher_atfork_child();
//   if (_dispatch_safe_fork) {
//     return;
//   }
//   _dispatch_child_of_unsafe_fork = true;

//   _dispatch_main_q.dq_items_head = crash;
//   _dispatch_main_q.dq_items_tail = crash;

//   _dispatch_mgr_q.dq_items_head = crash;
//   _dispatch_mgr_q.dq_items_tail = crash;

//   for (i = 0; i < DISPATCH_ROOT_QUEUE_COUNT; i++) {
//     _dispatch_root_queues[i].dq_items_head = crash;
//     _dispatch_root_queues[i].dq_items_tail = crash;
//   }
// }

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