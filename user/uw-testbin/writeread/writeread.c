/* Tim Brecht
 * Added :Sat  5 Jan 2013 15:19:15 EST
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../lib/testutils.h"

#define NUM_TIMES   (1)
#define NUM_INTS    (4*1024) 

int
main()
{
   int i, rc, fd;
   int write_array[NUM_INTS];
   int read_array[NUM_INTS];

   /* Uncomment this when having failures and for debugging */
   // TEST_VERBOSE_ON();

   /* Initialize the array */
   for (i=0; i<NUM_INTS; i++) {
     write_array[i] = i;
   }

   /* Open the file for writing */
   fd = open("WRITE_READ_FILE", O_WRONLY | O_CREAT);
   TEST_POSITIVE(fd, "Open file named WRITE_READ_FILE failed\n");

   for (i=0; i<NUM_TIMES; i++) {
		 rc = write(fd, write_array, sizeof(write_array));
     TEST_EQUAL(rc, sizeof(write_array), "Failed to write all of the array");
   }

   close(fd);

   /* Open the file */
   fd = open("WRITE_READ_FILE", O_RDONLY);
   TEST_POSITIVE(fd, "Open file named WRITE_READ_FILE failed\n");

   for (i=0; i<NUM_TIMES; i++) {
		 rc = read(fd, read_array, sizeof(read_array));
     TEST_EQUAL(rc, sizeof(read_array), "Failed to read all of the array");
     for (i=0; i<NUM_INTS; i++) { 
       TEST_EQUAL(read_array[i], write_array[i], "Value read not equal to value written");
     }
   }

   TEST_STATS();

   exit(0);
}
