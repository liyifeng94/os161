/*
 * argtesttest
 *
 * 	launch argtest with some arguments
 *
 *   relies on fork, execv
 *
 */

#include <unistd.h>
#include <err.h>

static char *xargv[4] = { (char *)"argtesttest", (char *)"first", (char *)"second", NULL };

static
void
spawnv(const char *prog, char **argv)
{
  pid_t pid = fork();
  switch (pid) {
  case -1:
    err(1, "fork");
  case 0:
    /* child */
    execv(prog, argv);
    err(1, "%s", prog);
  default:
    /* parent */
    break;
  }
}

int
main()
{
  spawnv("/testbin/argtest", xargv);
  return 0;
}
