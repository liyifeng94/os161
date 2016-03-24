
/* Tim Brecht: 
 * Added to uw-testbin : Sat  5 Jan 2013 14:36:26 EST
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
 
/*
 * This tests concurrent access to a file. 
 */ 

#define PROCS         (4)        
#define BUF_SIZE      (10)
#define NUM_WRITES    (500)
#define TOTAL_WRITES  (NUM_WRITES * PROCS)

char const *to_write = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static void do_writes(char c);

int 
main()
{
  char buffer[BUF_SIZE];
	int status = -1;
	int rc[PROCS];
  pid_t pid[PROCS];
  int rval = -1;
  int id = -1;
  int i,k;

  rval = open("TESTFILE", O_RDWR | O_CREAT | O_TRUNC);

  if (rval < 0)
  {
    printf("### TEST FAILED: Unable to create file\n"); 
    exit(0);
  }
	close(rval);

  /* do concurrent writes */ 
  for (i=0; i<PROCS; i++) {
    pid[i] = fork();
	  if (pid[i] < 0) {
      printf("Fork %d failed\n", i); 
      exit(0);
	  }

    /* This is the child have it do something */
    if (pid[i] == 0) {
      do_writes(to_write[i]);
      printf("Process number %d is done\n", i);
		  exit(0);
    }
  }

  /* Check that all processes were created */
  for (i=0; i<PROCS; i++) {
     if (pid[i] < 0) {
       printf("### TEST FAILED: Unable to create processes\n");
     }
  }

  /* Wait for all processes to finish */
  for (i=0; i<PROCS; i++) {
	     rc[i] = waitpid(pid[i], &status, 0);
       if (rc[i] != pid[i]) {
         printf("### TEST FAILED: wait for processes failed\n");
       }
       printf("Done waiting for process number %d\n", i);
  }

  /* Now check that the file contains the proper contents */
  id = open("TESTFILE", O_RDWR);
  if (id < 0)
  {
    printf("### TEST FAILED: Unable to open file\n");
    exit(1);
  }

  /* verify writes were atomic */ 
  for (i=0; i<TOTAL_WRITES; i++)
  {
    rval = read(id, buffer, BUF_SIZE);

    for (k=0; k<(BUF_SIZE-1); k++) {
        if (buffer[k] != buffer[k+1]) { 
      		printf("### TEST FAILED; Writes were not atomic\n"); 
					printf("buffer[%d] = %c != buffer[%d] = %c\n", 
							k, buffer[k], k+1, buffer[k+1]);
					close(id);
          exit(1); 
	      }
    }
  }

  rval = close(id); 

  if (rval < 0)
  {
    printf("### TEST FAILED: Unable to close file\n"); 
  } else {
    printf("PASSED\n");
	}
  exit(0); 

}

void
do_writes(char c)
{
  int rval = 0;
  int id = -1;
  int i = 0; 
  int j = 0;
	char buffer[BUF_SIZE];
  int volatile total = 0;

  for (j=0; j<BUF_SIZE; j++) {
		buffer[j] = c;
  }

  id = open("TESTFILE", O_RDWR);
  
  if (id < 0) {
    printf("### TEST FAILED: Unable to open file\n");
    _exit(1); 
  }

  for (i = 0; i < NUM_WRITES; i++) {
    rval = write(id, buffer, BUF_SIZE);

    if (rval != BUF_SIZE) {
      printf("### TEST FAILED: Unable to write %d bytes to file\n", BUF_SIZE);
			close(id);
      _exit(1); 
    }

    /* Do something else to try to give other processes a change to run */
    for (j=0; j<BUF_SIZE; j++) {
		  buffer[j] = c;
      total += j;
    }
  }

  rval = close(id); 

  if (rval < 0) {
    printf("### TEST FAILED: Unable to close file\n"); 
    _exit(1); 
  }

}
