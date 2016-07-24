#include <assert.h>
#include <stdio.h>

#define _GNU_SOURCE
#include <netdb.h>

int main(int argc, char *argv[]) {
  static const char[] HOSTS = {"www.google.com", "www.facebook.com"};
  static const int NUM_HOSTS = sizeof(HOSTS)/sizeof(*HOSTS);
  struct gaicb* cblist[NUM_HOSTS];
  struct gaicb cbs[NUM_HOSTS];
  memset(0, &cbs, sizeof(cbs))
  for (int i = 0; i < NUM_HOSTS; i++) {
    cblist[i] = &cbs[i];
    cbs[i].ar_name = HOSTS[i];
  }

  printf("starting resolution of hosts...\n")
  struct sigevent sevent;
  memset(&sevent, 0, sizeof(sevent));
  sevent.sigev_notify = SIGEV_NONE;

  int error = getaddrinfo_a(GAI_NOWAIT, cblist, NUM_HOSTS, &sevent);
  assert(error == 0);

  printf("requests started; waiting ...\n");
  for (int j = 0; j < NUM_HOSTS; j++) {
    error = gai_suspend(cblist, NUM_HOSTS, NULL);
    assert(error == 0);

    for (int i = 0; i < NUM_HOSTS; i++) {
      int status = gai_error(&cbs[i]);
      assert(status == 0 || status == EAI_INPROGRESS);
      if (status == 0) {
        char host[NI_MAXHOST];
        printf("host:%s = %s\n", HOSTS[i], cbs[i].)

      }
    }

   char *cmdline;
   char *cmd;

   while ((cmdline = getcmd()) != NULL) {
       cmd = strtok(cmdline, " ");

       if (cmd == NULL) {
           list_requests();
       } else {
           switch (cmd[0]) {
           case 'a':
               add_requests();
               break;
           case 'w':
               wait_requests();
               break;
           case 'c':
               cancel_requests();
               break;
           case 'l':
               list_requests();
               break;
           default:
               fprintf(stderr, "Bad command: %c\n", cmd[0]);
               break;
           }
       }
   }
   exit(EXIT_SUCCESS);
}
