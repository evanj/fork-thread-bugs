// clang -Wall -Wextra -o getaddrinfo_a_fork getaddrinfo_a_fork.c -lanl
#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <netdb.h>

int main() {
  static const char* HOSTS[] = {"www.google.com", "www.facebook.com", "www.microsoft.com"};
  static const int NUM_HOSTS = sizeof(HOSTS)/sizeof(*HOSTS);
  struct gaicb* cblist[NUM_HOSTS];
  struct gaicb cbs[NUM_HOSTS];

  memset(&cbs, 0, sizeof(cbs));
  for (int i = 0; i < NUM_HOSTS; i++) {
    cblist[i] = &cbs[i];
    cbs[i].ar_name = HOSTS[i];
  }

  printf("starting resolution of hosts...\n");
  struct sigevent sevent;
  memset(&sevent, 0, sizeof(sevent));
  sevent.sigev_notify = SIGEV_NONE;

  int error = getaddrinfo_a(GAI_NOWAIT, cblist, NUM_HOSTS, &sevent);
  assert(error == 0);

  printf("requests started; waiting ...\n");
  for (int j = 0; j < NUM_HOSTS; j++) {
    const int remaining_hosts = NUM_HOSTS-j;
    // explicit cast to avoid a const warning: this is safe in this case
    const struct gaicb** const_cblist = (const struct gaicb**) cblist;
    error = gai_suspend(const_cblist, remaining_hosts, NULL);
    assert(error == 0);

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
        printf("host %s = %s\n", cblist[i]->ar_name, host);

        // move the last element to position i, then shrink the list
        cblist[i] = cblist[remaining_hosts-1];
        cblist[remaining_hosts-1] = NULL;
        break;
      }
    }
  }
}
