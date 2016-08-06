// This program nearly always hangs due to a fork "deadlock". Sometimes the
// child even crashes with a segfault.
//
// compile with:
// clang -Wall -Wextra -o getaddrinfo_a_fork getaddrinfo_a_fork.c -lanl
#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include <netdb.h>

struct gaicb** start_resolve_parallel(const char** hosts, size_t num_hosts) {
  // allocate a single array of structures, stored in cblist[0]
  struct gaicb** cblist = (struct gaicb**) malloc(sizeof(struct gaicb*) * num_hosts);
  struct gaicb* cbs = (struct gaicb*) malloc(sizeof(struct gaicb) * num_hosts);
  memset(cbs, 0, sizeof(struct gaicb) * num_hosts);
  for (size_t i = 0; i < num_hosts; i++) {
    cblist[i] = &cbs[i];
    cblist[i]->ar_name = hosts[i];
  }

  struct sigevent sevent;
  memset(&sevent, 0, sizeof(sevent));
  sevent.sigev_notify = SIGEV_NONE;

  int error = getaddrinfo_a(GAI_NOWAIT, cblist, num_hosts, &sevent);
  assert(error == 0);
  return cblist;
}

void finish_resolve_parallel(const char* prefix, struct gaicb** cblist, size_t num_hosts) {
  for (size_t j = 0; j < num_hosts; j++) {
    const int remaining_hosts = num_hosts-j;
    // explicit cast to avoid a const warning: this is safe in this case
    // http://c-faq.com/ansi/constmismatch.html
    const struct gaicb** const_cblist = (const struct gaicb**) cblist;
    int error = gai_suspend(const_cblist, remaining_hosts, NULL);
    // if the requests are all completed before we call gai_suspend, we get ALLDONE
    // this happens under valgrind, and probably in rare cases in reality
    // TODO: Also handle EAI_AGAIN and EAI_INTR
    assert(error == 0 || error == EAI_ALLDONE);

    // find the host that resolved
    for (int i = 0; i < remaining_hosts; i++) {
      int status = gai_error(cblist[i]);
      assert(status == 0 || status == EAI_INPROGRESS);
      if (status == 0) {
        // found it! print it and move it out of the list
        char host[NI_MAXHOST];
        error = getnameinfo(cblist[i]->ar_result->ai_addr, cblist[i]->ar_result->ai_addrlen, host,
          sizeof(host), NULL, 0, NI_NUMERICHOST);
        assert(error == 0);
        printf("%s host %s = %s\n", prefix, cblist[i]->ar_name, host);

        // move the last element to position i, then shrink the list
        cblist[i] = cblist[remaining_hosts-1];
        cblist[remaining_hosts-1] = NULL;
        break;
      }
    }
  }

  // free the backing array of struct gaicb, as well as the list itself
  free(cblist[0]);
  free(cblist);
}

int main() {
  // glibc will start up to 20 helper threads. If there are a total of <= 20 requests, it doesn't
  // *always* hang, but does frequently.
  static const char* HOSTS[] = {
    "www.google.com",
    "mail.google.com",
    "docs.google.com",
    "sheets.google.com",
    "cloud.google.com",
    "console.cloud.google.com",
    "images.google.com",
    "maps.google.com",
    "news.google.com",
    "books.google.com",
  };
  static const size_t NUM_HOSTS = sizeof(HOSTS)/sizeof(*HOSTS);
  struct gaicb** cblist = start_resolve_parallel(HOSTS, NUM_HOSTS);

  // start a child: the child and parent resolver threads will race
  pid_t pid = fork();
  assert(pid >= 0);
  if (pid > 0) {
    finish_resolve_parallel("parent", cblist, NUM_HOSTS);

    printf("parent waiting for pid %d\n", pid);
    int result = 0;
    waitpid(pid, &result, 0);
    printf("parent waitpid result: %d WTERMSIG: %d\n", result, WTERMSIG(result));
  } else {
    // child: start a new parallel resolve
    struct gaicb** cblist2 = start_resolve_parallel(HOSTS, NUM_HOSTS);
    printf("child pid %d started parallel DNS resolution; waiting ...\n", getpid());
    finish_resolve_parallel("child", cblist2, NUM_HOSTS);
    printf("child finished exiting normally\n");
  }

  return 0;
}
