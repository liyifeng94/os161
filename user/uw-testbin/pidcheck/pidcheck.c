/*
 * pidcheck - simple test of fork and getpid
 *
 *  relies only on fork, console write, getpid and _exit
 *
 *  child prints its pid, parent prints childs pid and its own
 *
 */

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <err.h>

/* declare this volatile to discourage the compiler from
   optimizing away the parent's delay loop */
volatile int tot;

int
main(int argc, char *argv[])
{
  (void)argc;
  (void)argv;
  pid_t pid,pid2;
  int i;
  pid = fork();
  if (pid < 0) {
    warn("fork");
  }
  else if (pid == 0) {
    /* child */
    pid2 = getpid();
    /* print the child's PID, as seen by the child */
    printf("C: %d\n",pid2);
  }
  else {
    /* parent */
    /* try to delay long enough for the child to finish printing */
    tot = 0;
    for(i=0;i<1000000;i++) {
      tot++;
    }
    /* print the child's PID, as seen by the parent */
    printf("PC: %d\n",pid);
    /* print the parent's PID */
    pid2 = getpid();
    printf("PP: %d\n",pid2);
  }
  return(0);
}
