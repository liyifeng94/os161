
/*
 * UW - Synchronization test code.
 * Tim Brecht July, 2013
 * UW - uwvmstats tests code.
 * Tim Brecht January, 2014
 */

#include <types.h>
#include <synch.h>
#include <thread.h>
#include <test.h>
#include <uw-vmstats.h>

#define NAME_LEN (30)

static struct lock *testlock = NULL;
static struct semaphore *donesem = NULL;

#define NTESTLOOPS    (5000)  /* Needs to be evenly divisible by 8 */
#define NTESTTHREADS  (8)
#define START_VALUE   (0)
static volatile int test_value = START_VALUE;
static int use_locks = 1;

static
void
cleanitems(void)
{
	kprintf("cleanitems: Destroying sems and locks\n");
	lock_destroy(testlock);
  testlock = NULL;
	sem_destroy(donesem);
  donesem = NULL;
  test_value = START_VALUE;
}

static
void
inititems(void)
{
	if (testlock==NULL) {
		testlock = lock_create("testlock");
		if (testlock == NULL) {
			panic("synchtest: lock_create failed\n");
		}
	}

	if (donesem==NULL) {
		donesem = sem_create("donesem", 0);
		if (donesem == NULL) {
			panic("synchtest: sem_create failed\n");
		}
	}
}

/* This thread adds values to a global variable. */
static
void
add_thread(void *junk, unsigned long num)
{
	int i;
	(void) num;
	(void) junk;

	for (i=0; i<NTESTLOOPS; i++) {
		if (use_locks) {
			lock_acquire(testlock);
		}

      /* This loop is unrolled to possibly avoid optimizations
       * and to provide more instructions that could be interrupted.
       * This may or may not be necessary.
       */
		  test_value = test_value + 1;
		  test_value = test_value + 1;
		  test_value = test_value + 1;
		  test_value = test_value + 1;
		  test_value = test_value + 1;

		if (use_locks) {
			lock_release(testlock);
		}

	}
	V(donesem);
	thread_exit();
}

/* This thread substract values from a global variable. */
static
void
sub_thread(void *junk, unsigned long num)
{
	int i;
	(void)num;
	(void)junk;

	for (i=0; i<NTESTLOOPS; i++) {
		if (use_locks) {
			lock_acquire(testlock);
		}

      /* This loop is unrolled to avoid optimizations
       * and to provide more instructions that could be interrupted.
       * This may or may not be necessary.
       */
		  test_value = test_value - 1;
		  test_value = test_value - 1;
		  test_value = test_value - 1;
		  test_value = test_value - 1;
		  test_value = test_value - 1;

		if (use_locks) {
			lock_release(testlock);
		}

	}
	V(donesem);
	thread_exit();
}

int
uwlocktest1(int nargs, char **args)
{
	int i, result;
  char name[NAME_LEN];

	(void)nargs;
	(void)args;

	inititems();
	kprintf("Starting uwlocktest1...\n");

	for (i=0; i<NTESTTHREADS; i++) {
    snprintf(name, NAME_LEN, "add_thread %d", i);
		result = thread_fork(name, NULL, add_thread, NULL, i);
		if (result) {
			panic("uwlocktest1: thread_fork failed: %s\n",
			      strerror(result));
		}
	}

	for (i=0; i<NTESTTHREADS; i++) {
    snprintf(name, NAME_LEN, "sub_thread %d", i);
		result = thread_fork(name, NULL, sub_thread, NULL, i);
		if (result) {
			panic("uwlocktest1: thread_fork failed: %s\n",
			      strerror(result));
		}
	}

	for (i=0; i<NTESTTHREADS*2; i++) {
		P(donesem);
	}

	kprintf("value of test_value = %d should be %d\n", test_value, START_VALUE);
	if (test_value == START_VALUE) {
  	kprintf("TEST SUCCEEDED\n");
  } else {
  	kprintf("TEST FAILED\n");
  }
	KASSERT(test_value == START_VALUE);

	cleanitems();
	kprintf("uwlocktest1 done.\n");

	return 0;
}

/*-----------------------------------------------------------------------*/

/* Each thread makes some calls to vmstats functions */
static
void
vmstats_thread(void *junk, unsigned long num)
{
	int i;
	int j;
	(void)num;
	(void)junk;

	for (i=0; i<NTESTLOOPS; i++) {
    for (j=0; j<VMSTAT_COUNT; j++) {
        /* NOTE: The number of calls to vmstats_inc below have been manipulated
         * so the checks during printing add up properly and pass the various tests
         */
        switch(j) { 
          /* Need twice as many TLB faults */
          case VMSTAT_TLB_FAULT:
            vmstats_inc(j);
            vmstats_inc(j);
            break;

          case VMSTAT_TLB_FAULT_FREE:
            vmstats_inc(j);
            break;

          case VMSTAT_TLB_FAULT_REPLACE:
            vmstats_inc(j);
            break;

          /* Just reduce these to compare (not necessary) */
          case VMSTAT_TLB_INVALIDATE:
            if (i % 2 == 0) {
               vmstats_inc(j);
            }
            break;

          case VMSTAT_TLB_RELOAD:
            vmstats_inc(j);
            break;

          /* VMSTAT_TLB_FAULT = VMSTAT_TLB_RELOAD + VMSTAT_PAGE_FAULT_DISK + VMSTAT_SWAP_FILE_ZERO */
          case VMSTAT_PAGE_FAULT_ZERO:
            if (i % 2 == 0) {
               vmstats_inc(j);
            }
            break;

          /* VMSTAT_PAGE_FAULT_DISK = VMSTAT_ELF_FILE_READ + VMSTAT_SWAP_FILE_READ */
          case VMSTAT_PAGE_FAULT_DISK:
            if (i % 2 == 0) {
               vmstats_inc(j);
            }
            break;

          case VMSTAT_ELF_FILE_READ:
            if (i % 4 == 0) {
               vmstats_inc(j);
            }
            break;

          case VMSTAT_SWAP_FILE_READ:
            if (i % 4 == 0) {
               vmstats_inc(j);
            }
            break;

          case VMSTAT_SWAP_FILE_WRITE:
            if (i % 8 == 0) {
               vmstats_inc(j);
            }
            break;

          default:
            kprintf("Unknown stat %d\n", j);
            break;
      }
    }
	}

	V(donesem);
	thread_exit();
}

int
uwvmstatstest(int nargs, char **args)
{
	int i, result;
  char name[NAME_LEN];

	(void)nargs;
	(void)args;

	inititems();
	kprintf("Starting uwvmstatstest...\n");

  kprintf("Initializing vmstats\n");
  vmstats_init();

	for (i=0; i<NTESTTHREADS; i++) {
    snprintf(name, NAME_LEN, "vmstatsthread %d", i);
		result = thread_fork(name, NULL, vmstats_thread, NULL, i);
		if (result) {
			panic("uwvmstatstest: thread_fork failed: %s\n",
			      strerror(result));
		}
	}

	for (i=0; i<NTESTTHREADS; i++) {
		P(donesem);
	}

  vmstats_print();

	cleanitems();
	kprintf("uwvmstatstest done.\n");

	return 0;
}


