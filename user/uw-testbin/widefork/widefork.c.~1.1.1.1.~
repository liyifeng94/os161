/*
 * widefork - parent process forks several children.  Children do not fork.
 *
 *  relies only on fork, console write, and _exit
 *  also uses getpid and waitpid, but will work (though differently) if they are
 *   not fully implemented
 *
 *  parent prints creates three children, printing "P" before each fork.
 *  children print their name (A, B, or C), then exit with unique return code.
 *  parent waits for children in birth order, prints lower case child name
 *  (a, b, or c) if return value from waitpid is correct.   Parent prints
 *  x instead of child name if return value is not correct.
 *
 *  Example of correct output:  PAPBPCabc
 *
 */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <err.h>

int dofork(int);
void dowait(int,int);

int
dofork(int childnum) 
{
  pid_t pid;
  pid = fork();
  if (pid < 0) {
    errx(1,"fork %d",childnum);
  }
  else if (pid == 0) {
    /* child */
    putchar('A'+childnum-1);
    putchar('\n');
    _exit(childnum);
  }
  return(pid);
}

void
dowait(int childpid, int childnum)
{
  int rval;
  if (waitpid(childpid,&rval,0) < 0) {
    warnx("waitpid 1");
    return;
  }
  if (WIFEXITED(rval)) {
    if ((WEXITSTATUS(rval)) == childnum) {
      putchar('a'+childnum-1);
      putchar('\n');
    }
  }
  else {
    putchar('x');
    putchar('\n');
  }
}

int
main(int argc, char *argv[])
{
  (void)argc;
  (void)argv;
  pid_t pid1,pid2,pid3;
  putchar('P');
  putchar('\n');
  pid1 = dofork(1);
  putchar('P');
  putchar('\n');
  pid2 = dofork(2);
  putchar('P');
  putchar('\n');
  pid3 = dofork(3);
  dowait(pid1,1);
  dowait(pid2,2);
  dowait(pid3,3);
  return(0);
}
