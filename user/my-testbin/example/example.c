/*
 * An example program.
 */
#include <unistd.h>

int
main()
{
	reboot(RB_POWEROFF);
	return 0; /* avoid compiler warnings */
}
