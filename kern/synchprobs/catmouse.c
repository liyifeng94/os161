/*
 * catmouse.c
 *
 * 30-1-2003 : GWA : Stub functions created for CS161 Asst1.
 * 26-11-2007: KMS : Modified to use cat_eat and mouse_eat
 * 21-04-2009: KMS : modified to use cat_sleep and mouse_sleep
 * 21-04-2009: KMS : added sem_destroy of CatMouseWait
 * 05-01-2012: TBB : added comments to try to clarify use/non use of volatile
 * 22-08-2013: TBB: made cat and mouse eating and sleeping time optional parameters
 * 27-04-2014: KMS: change this to simulation driver that invokes student-implemented synch functions
 *
 */


/*
 *  CS350 Students Note!
 *
 *  You may not modify the code in this file in any way!
 *
 */


/*
 * 
 * Includes
 *
 */

#include <types.h>
#include <lib.h>
#include <test.h>
#include <clock.h>
#include <thread.h>
#include <synch.h>
#include <synchprobs.h>

/* functions defined and used internally */
static void initialize_bowls(void);
static void cleanup_bowls(void);
static void cat_eat(unsigned int bowlnumber, int eat_time);
static void cat_sleep(int sleep_time);
static void mouse_eat(unsigned int bowlnumber, int eat_time);
static void mouse_sleep(int sleep_time);
static void cat_simulation(void *ptr, unsigned long catnumber);
static void mouse_simulation(void *ptr, unsigned long mousenumber);
/* static void print_state(void); */

/*
 *
 * Problem parameters
 *
 * Values for these parameters are set by the main driver
 *  function, catmouse(), based on the problem parameters
 *  that are passed in from the kernel menu command or
 *  kernel command line.
 *  changing them.
 *
 * These are only ever modified by one thread, at creation time,
 * so they do not need to be volatile.
 */
static int NumBowls;  // number of food bowls
static int NumCats;   // number of cats
static int NumMice;   // number of mice
static int NumLoops;  // number of times each cat and mouse should eat

static int CatEatTime = 1;      // length of time a cat spends eating
static int CatSleepTime = 2;    // length of time a cat spends sleeping
static int MouseEatTime = 1;    // length of time a mouse spends eating
static int MouseSleepTime = 2;  // length of time a mouse spends sleeping

/*
 * Once the main driver function (catmouse()) has created the cat and mouse
 * simulation threads, it uses this semaphore to block until all of the
 * cat and mouse simulations are finished.
 */
static struct semaphore *CatMouseWait;

/*
 *
 * shared simulation state
 * 
 * note: this is state should be used only by the 
 *  functions in this file, hence the static declarations
 *
 */

/* a character array with one entry for each bowl
 *  bowl[i-1] = 'c' if a cat is eating at the ith bowl
 *  bowl[i-1] = 'm' if a mouse is eating at the ith bowl
 *  bowl[i-1] = '-' otherwise */

/* The elements within the array can be changed by multiple 
 * threads so the contents are volatile.
 */
static volatile char *bowls;

/* how many cats are currently eating? 
 * modified by multiple threads, so volatile 
 */
static volatile int eating_cats_count;

/* how many mice are currently eating? 
 * modified by different threads, so volatile 
 */
static volatile int eating_mice_count;

/* semaphore used to provide mutual exclusion
 * for reading and writing the shared simulation state 
 * The actual mutex is created/initizliaed by one thread and not 
 * modified by others: not volatile 
 */
static struct semaphore *mutex;

/* performance statistics */
static volatile time_t cat_total_wait_secs;
static volatile uint32_t cat_total_wait_nsecs;
static volatile int cat_wait_count;
static volatile time_t mouse_total_wait_secs;
static volatile uint32_t mouse_total_wait_nsecs;
static volatile int mouse_wait_count;

/* mutex to provide mutual exclusion to performance stats */
static struct semaphore *perf_mutex;


/*
 * initialize_bowls()
 * 
 * Purpose:
 *   initializes simulation of cats and mice and bowls
 *
 * Arguments:
 *   unsigned int bowlcount:  the number of food bowls to simulate
 *
 * Returns:
 *   0 on success, 1 otherwise
 */

static void
initialize_bowls()
{
  int i;

  KASSERT(NumBowls > 0);

  bowls = kmalloc(NumBowls*sizeof(char));
  if (bowls == NULL) {
    panic("initialize_bowls: unable to allocate space for %d bowls\n",NumBowls);
  }
  /* initialize bowls */
  for(i=0;i<NumBowls;i++) {
    bowls[i] = '-';
  }
  eating_cats_count = eating_mice_count = 0;

  /* intialize mutex semaphore */
  mutex = sem_create("bowl mutex",1);
  if (mutex == NULL) {
    panic("initialize_bowls: could not create mutex\n");
  }
  /* intialize perf_mutex semaphore */
  perf_mutex = sem_create("stats mutex",1);
  if (perf_mutex == NULL) {
    panic("initialize_bowls: could not create perf_mutex\n");
  }

  cat_total_wait_secs = 0;
  cat_total_wait_nsecs = 0;
  cat_wait_count = 0;
  mouse_total_wait_secs = 0;
  mouse_total_wait_nsecs = 0;
  mouse_wait_count = 0;
  
  return;
}


/*
 * cleanup_bowls()
 * 
 * Purpose:
 *   Releases resources created by initialize_bowls.
 *
 * Arguments:
 *   None
 *
 * Returns:
 *   Nothing
 */

static void
cleanup_bowls()
{
  if (mutex != NULL) {
    sem_destroy( mutex );
    mutex = NULL;
  }
  if (perf_mutex != NULL) {
    sem_destroy( perf_mutex );
    perf_mutex = NULL;
  }
  if (bowls != NULL) {
    kfree( (void *) bowls );
    bowls = NULL;
  }
}

/*
 * print_state()
 * 
 * Purpose:
 *   displays the simulation state
 *
 * Arguments:
 *   none
 *
 * Returns:
 *   nothing
 *
 * Notes:
 *   this assumes that it is being called from within
 *   a critical section - it does not provide its own
 *   mutual exclusion
 */

/*   not currently used */
/*
static void
print_state()
{
  int i;

  kprintf(" Eating Cats: %3d  Eating Mice: %3d   ",eating_cats_count,
    eating_mice_count);
  for(i=0;i<NumBowls;i++) {
    kprintf("%c",bowls[i]);
  }
  return;
}
*/


/*
 * cat_eat()
 *
 * Purpose:
 *   simulates a cat eating from a bowl, and checks to
 *   make sure that none of the simulation requirements
 *   have been violated.
 *
 * Arguments:
 *   unsigned int bowlnumber: which bowl the cat should eat from
 *
 * Returns:
 *   nothing
 *
 */

void
cat_eat(unsigned int bowlnumber, int eat_time)
{

  /* check the bowl number */
  KASSERT(bowlnumber > 0);
  KASSERT((int)bowlnumber <= NumBowls);

  /* check and update the simulation state to indicate that
   * the cat is now eating at the specified bowl */
  P(mutex);

  /* first check whether allowing this cat to eat will
   * violate any simulation requirements */
  if (bowls[bowlnumber-1] == 'c') {
    /* there is already a cat eating at the specified bowl */
    panic("cat_eat: attempt to make two cats eat from bowl %d!\n",bowlnumber);
  }
  if (eating_mice_count > 0) {
    /* there is already a mouse eating at some bowl */
    panic("cat_eat: attempt to make a cat eat while mice are eating!\n");
  }
  KASSERT(bowls[bowlnumber-1]=='-');
  KASSERT(eating_mice_count == 0);

  /* now update the state to indicate that the cat is eating */
  eating_cats_count += 1;
  bowls[bowlnumber-1] = 'c';

  DEBUG(DB_SYNCPROB,"cat starts to eat at bowl %d [%d:%d]\n",
	bowlnumber,eating_cats_count,eating_mice_count);
  V(mutex);  // end critical section

  /* simulate eating by introducing a delay
   * note that eating is not part of the critical section */
  clocksleep(eat_time);

  /* update the simulation state to indicate that
   * the cat is finished eating */
  P(mutex);  // start critical section
  KASSERT(eating_cats_count > 0);
  KASSERT(bowls[bowlnumber-1]=='c');
  eating_cats_count -= 1;
  bowls[bowlnumber-1]='-';

  DEBUG(DB_SYNCPROB,"cat finished eating at bowl %d [%d:%d]\n",
	bowlnumber,eating_cats_count,eating_mice_count);
  V(mutex);  // end critical section

  return;
}

/*
 * cat_sleep()
 *
 * Purpose:
 *   simulates a cat sleeping
 *
 * Arguments: none
 *
 * Returns: nothing
 *
 */
void
cat_sleep(int sleep_time)
{
  /* simulate sleeping by introducing a delay */
  clocksleep(sleep_time);
  return;
}


/*
 * mouse_eat()
 *
 * Purpose:
 *   simulates a mouse eating from a bowl, and checks to
 *   make sure that none of the simulation requirements
 *   have been violated.
 *
 * Arguments:
 *   unsigned int bowlnumber: which bowl the mouse should eat from
 *
 * Returns:
 *   nothing
 *
 */

void
mouse_eat(unsigned int bowlnumber, int eat_time)
{
  /* check the bowl number */
  KASSERT(bowlnumber > 0);
  KASSERT((int)bowlnumber <= NumBowls);

  /* check and updated the simulation state to indicate that
   * the mouse is now eating at the specified bowl. */
  P(mutex);  // start critical section

  /* first check whether allowing this mouse to eat will
   * violate any simulation requirements */
  if (bowls[bowlnumber-1] == 'm') {
    /* there is already a mouse eating at the specified bowl */
    panic("mouse_eat: attempt to make two mice eat from bowl %d!\n",bowlnumber);
  }
  if (eating_cats_count > 0) {
    /* there is already a cat eating at some bowl */
    panic("mouse_eat: attempt to make a mouse eat while cats are eating!\n");
  }
  KASSERT(bowls[bowlnumber-1]=='-');
  KASSERT(eating_cats_count == 0);

  /* now update the state to indicate that the mouse is eating */
  eating_mice_count += 1;
  bowls[bowlnumber-1] = 'm';

  DEBUG(DB_SYNCPROB,"mouse starts to eat at bowl %d [%d:%d]\n",
	bowlnumber,eating_cats_count,eating_mice_count);
  V(mutex);  // end critical section

  /* simulate eating by introducing a delay
   * note that eating is not part of the critical section */
  clocksleep(eat_time);

  /* update the simulation state to indicate that
   * the mouse is finished eating */
  P(mutex); // start critical section

  KASSERT(eating_mice_count > 0);
  eating_mice_count -= 1;
  KASSERT(bowls[bowlnumber-1]=='m');
  bowls[bowlnumber-1]='-';

  DEBUG(DB_SYNCPROB,"mouse finishes eating at bowl %d [%d:%d]\n",
	bowlnumber,eating_cats_count,eating_mice_count);
  V(mutex);  // end critical section
  return;
}

/*
 * mouse_sleep()
 *
 * Purpose:
 *   simulates a mouse sleeping
 *
 * Arguments: none
 *
 * Returns: nothing
 *
 */
void
mouse_sleep(int sleep_time)
{
  /* simulate sleeping by introducing a delay */
  clocksleep(sleep_time);
  return;
}

/*
 * cat_simulation()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long catnumber: holds cat identifier from 0 to NumCats-1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Each cat simulation thread runs this function.
 *
 */

static
void
cat_simulation(void * unusedpointer, 
               unsigned long catnumber)
{
  int i;
  unsigned int bowl;
  time_t before_sec, after_sec, wait_sec;
  uint32_t before_nsec, after_nsec, wait_nsec;

  /* avoid unused variable warnings. */
  (void) unusedpointer;
  (void) catnumber;


  for(i=0;i<NumLoops;i++) {

    /* make the cat sleep */
    cat_sleep(CatSleepTime);

    /* choose bowl.  legal bowl numbers range from 1 to NumBowls */
    bowl = ((unsigned int)random() % NumBowls) + 1;

    gettime(&before_sec,&before_nsec);
    cat_before_eating(bowl); /* student-implemented function */
    gettime(&after_sec,&after_nsec);

    /* make the cat eat */
    cat_eat(bowl, CatEatTime);

    cat_after_eating(bowl); /* student-implemented function */

    /* update wait time statistics */
    getinterval(before_sec,before_nsec,after_sec,after_nsec,&wait_sec,&wait_nsec);
    P(perf_mutex);
    cat_total_wait_secs += wait_sec;
    cat_total_wait_nsecs += wait_nsec;
    if (cat_total_wait_nsecs > 1000000000) {
      cat_total_wait_nsecs -= 1000000000;
      cat_total_wait_secs ++;
    }
    cat_wait_count++;
    V(perf_mutex);
  }

  /* indicate that this cat simulation is finished */
  V(CatMouseWait); 
}

/*
 * mouse_simulation()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long mousenumber: holds mouse identifier from 0 to NumMice-1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      each mouse simulation thread runs this function
 *
 */

static
void
mouse_simulation(void * unusedpointer,
          unsigned long mousenumber)
{
  int i;
  unsigned int bowl;
  time_t before_sec, after_sec, wait_sec;
  uint32_t before_nsec, after_nsec, wait_nsec;

  /* Avoid unused variable warnings. */
  (void) unusedpointer;
  (void) mousenumber;

  for(i=0;i<NumLoops;i++) {

    /* make the mouse sleep */
    mouse_sleep(MouseSleepTime);

    /* choose bowl.  legal bowl numbers range from 1 to NumBowls */
    bowl = ((unsigned int)random() % NumBowls) + 1;

    gettime(&before_sec,&before_nsec);
    mouse_before_eating(bowl); /* student-implemented function */
    gettime(&after_sec,&after_nsec);

    /* make the mouse eat */
    mouse_eat(bowl, MouseEatTime);

    mouse_after_eating(bowl); /* student-implemented function */

    /* update wait time statistics */
    getinterval(before_sec,before_nsec,after_sec,after_nsec,&wait_sec,&wait_nsec);
    P(perf_mutex);
    mouse_total_wait_secs += wait_sec;
    mouse_total_wait_nsecs += wait_nsec;
    if (mouse_total_wait_nsecs > 1000000000) {
      mouse_total_wait_nsecs -= 1000000000;
      mouse_total_wait_secs ++;
    }
    mouse_wait_count++;
    V(perf_mutex);
  }

  /* indicate that this mouse is finished */
  V(CatMouseWait); 
}

/*
 * catmouse()
 *
 * Arguments:
 *      int nargs: should be 5 or 9
 *      char ** args: args[1] = number of food bowls
 *                    args[2] = number of cats
 *                    args[3] = number of mice
 *                    args[4] = number of loops
 * Optional parameters
 *                    args[5] = cat eating time
 *                    args[6] = cat sleeping time
 *                    args[7] = mouse eating time
 *                    args[8] = mouse sleeping time
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up cat_simulation() and
 *      mouse_simulation() threads.
 */

int
catmouse(int nargs,
         char ** args)
{
  int catindex, mouseindex, error;
  int i;
  int mean_cat_wait_usecs, mean_mouse_wait_usecs;
  time_t before_sec, after_sec, wait_sec;
  uint32_t before_nsec, after_nsec, wait_nsec;
  int total_bowl_milliseconds, total_eating_milliseconds, utilization_percent;

  /* check and process command line arguments */
  if ((nargs != 9) && (nargs != 5)) {
    kprintf("Usage: <command> NUM_BOWLS NUM_CATS NUM_MICE NUM_LOOPS\n");
    kprintf("or\n");
    kprintf("Usage: <command> NUM_BOWLS NUM_CATS NUM_MICE NUM_LOOPS ");
    kprintf("CAT_EATING_TIME CAT_SLEEPING_TIME MOUSE_EATING_TIME MOUSE_SLEEPING_TIME\n");
    return 1;  // return failure indication
  }

  /* check the problem parameters, and set the global variables */
  NumBowls = atoi(args[1]);
  if (NumBowls <= 0) {
    kprintf("catmouse: invalid number of bowls: %d\n",NumBowls);
    return 1;
  }
  NumCats = atoi(args[2]);
  if (NumCats < 0) {
    kprintf("catmouse: invalid number of cats: %d\n",NumCats);
    return 1;
  }
  NumMice = atoi(args[3]);
  if (NumMice < 0) {
    kprintf("catmouse: invalid number of mice: %d\n",NumMice);
    return 1;
  }
  NumLoops = atoi(args[4]);
  if (NumLoops <= 0) {
    kprintf("catmouse: invalid number of loops: %d\n",NumLoops);
    return 1;
  }

  if (nargs == 9) {
    CatEatTime = atoi(args[5]);
    if (CatEatTime < 0) {
      kprintf("catmouse: invalid cat eating time: %d\n",CatEatTime);
      return 1;
    }
  
    CatSleepTime = atoi(args[6]);
    if (CatSleepTime < 0) {
      kprintf("catmouse: invalid cat sleeping time: %d\n",CatSleepTime);
      return 1;
    }
  
    MouseEatTime = atoi(args[7]);
    if (MouseEatTime < 0) {
      kprintf("catmouse: invalid mouse eating time: %d\n",MouseEatTime);
      return 1;
    }
  
    MouseSleepTime = atoi(args[8]);
    if (MouseSleepTime < 0) {
      kprintf("catmouse: invalid mouse sleeping time: %d\n",MouseSleepTime);
      return 1;
    }
  }

  kprintf("Using %d bowls, %d cats, and %d mice. Looping %d times.\n",
          NumBowls,NumCats,NumMice,NumLoops);
  kprintf("Using cat eating time %d, cat sleeping time %d\n", CatEatTime, CatSleepTime);
  kprintf("Using mouse eating time %d, mouse sleeping time %d\n", MouseEatTime, MouseSleepTime);

  /* create the semaphore that is used to make the main thread
     wait for all of the cats and mice to finish */
  CatMouseWait = sem_create("CatMouseWait",0);
  if (CatMouseWait == NULL) {
    panic("catmouse: could not create semaphore\n");
  }

  /* initialize our simulation state */
  initialize_bowls();

  /* initialize the synchronization functions */
  catmouse_sync_init(NumBowls);

  /* get current time, for measuring total simulation time */
  gettime(&before_sec,&before_nsec);

  /*
   * Start NumCats cat_simulation() threads and NumMice mouse_simulation() threads.
   * Alternate cat and mouse creation.
   */
  for (catindex = 0; catindex < NumCats; catindex++) {
    error = thread_fork("cat_simulation thread", NULL, cat_simulation, NULL, catindex);
    if (error) {
      panic("cat_simulation: thread_fork failed: %s\n", strerror(error));
    }
    if (catindex < NumMice) {
      error = thread_fork("mouse_simulation thread", NULL, mouse_simulation, NULL, catindex);
      if (error) {
	panic("mouse_simulation: thread_fork failed: %s\n",strerror(error));
      }
    } 
  }
  /* launch any remaining mice */
  for(mouseindex = catindex; mouseindex < NumMice; mouseindex++) {
    error = thread_fork("mouse_simulation thread", NULL, mouse_simulation, NULL, mouseindex);
    if (error) {
      panic("mouse_simulation: thread_fork failed: %s\n",strerror(error));
    }
  }
  
  /* wait for all of the cats and mice to finish before
     terminating */  
  for(i=0;i<(NumCats+NumMice);i++) {
    P(CatMouseWait);
  }

  /* get current time, for measuring total simulation time */
  gettime(&after_sec,&after_nsec);
  /* compute total simulation time */
  getinterval(before_sec,before_nsec,after_sec,after_nsec,&wait_sec,&wait_nsec);
  /* compute and report bowl utilization */
  total_bowl_milliseconds = (wait_sec*1000 + wait_nsec/1000000)*NumBowls;
  total_eating_milliseconds = (NumCats*CatEatTime + NumMice*MouseEatTime)*NumLoops*1000;
  if (total_bowl_milliseconds > 0) {
    utilization_percent = total_eating_milliseconds*100/total_bowl_milliseconds;
    kprintf("Bowl utilization: %d%%\n",utilization_percent);
  }

  /* clean up the semaphore that we created */
  sem_destroy(CatMouseWait);

  /* clean up the synchronization state */
  catmouse_sync_cleanup(NumBowls);

  /* clean up resources used for tracking bowl use */
  cleanup_bowls();

  if (cat_wait_count > 0) {
    /* some rounding error here - not significant if cat_wait_count << 1000000 */
    mean_cat_wait_usecs = (cat_total_wait_secs*1000000+cat_total_wait_nsecs/1000)/cat_wait_count;
    kprintf("Mean cat waiting time: %d.%d seconds\n",mean_cat_wait_usecs/1000000,mean_cat_wait_usecs%1000000);
  }
  if (mouse_wait_count > 0) {
    /* some rounding error here - not significant if mouse_wait_count << 1000000 */
    mean_mouse_wait_usecs = (mouse_total_wait_secs*1000000+mouse_total_wait_nsecs/1000)/mouse_wait_count;
    kprintf("Mean mouse waiting time: %d.%d seconds\n",mean_mouse_wait_usecs/1000000,mean_mouse_wait_usecs%1000000);
  }

  return 0;
}

/*
 * End of catmouse.c
 */
