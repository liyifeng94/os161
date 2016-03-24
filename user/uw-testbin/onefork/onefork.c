/*
 * onefork - simple test of fork
 *
 *  relies only on fork, console write, and _exit
 *
 *  parent prints "P", child prints "C", both exit
 *
 */

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <err.h>

int
main(int argc, char *argv[])
{
  (void)argc;
  (void)argv;
  pid_t pid;
  printf("starting fork\n");
  pid = fork();
  printf("\n new pid: %d\n", pid);
  if (pid < 0) {
    warn("fork");
  }
  else if (pid == 0) {
    /* child */
      putchar('c');
      putchar('\n');
  }
  else {
    /* parent */
      putchar('p');
      putchar('\n');
  }
  return(0);
}
