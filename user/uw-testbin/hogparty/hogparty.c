/*
 * hogparty
 *
 * 	create some talking hogs
 *
 *   relies on fork, _exit, stdout and stderr, and execv
 *
 */

#include <unistd.h>
#include <err.h>

static char *xhargv[2] = { (char *)"xhog", NULL };
static char *yhargv[2] = { (char *)"yhog", NULL };
static char *zhargv[2] = { (char *)"zhog", NULL };

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
  spawnv("/uw-testbin/xhog", xhargv);
  spawnv("/uw-testbin/yhog", yhargv);
  spawnv("/uw-testbin/zhog", zhargv);
  return 0;
}
