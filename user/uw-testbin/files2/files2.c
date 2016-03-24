/*
 * Title   : files2
 * Author  : Tim Brecht
 * Date    : Sat Oct  2 19:08:17 EST 1999
 *
 * Some more example tests using open, close, read and write.
 * Assumes that files name FILE1 and FILE2 do not exist in current directory
 *
 * Modified: Thu Dec 23 17:15:50 EST 2004
 *   TBB - updated for Winter 2005 term.
 *   TBB - cleaned up and added to uw-tests for Winter 2013 term.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include "../lib/testutils.h"

/* Define/Uncomment this if/when returning specific error codes using errno.h */
/*
#define USING_ERR_CODES
*/
#define USING_ERR_CODES

/* This is a name that doesn't exist so it opening it should fail */
#define BOGUS_NAME "ZZ12ZT"

/* Limt the number of times we try to open one file to avoid
 * potentally really long executions
 */
#define COUNT_LIMIT  (4*1024)

static int fd_array[COUNT_LIMIT];

int
main()
{
  int f1, f2, f3;
  int i = 42;
  int j = -999;
  int rc = 0;
  int count = 0;
	int saved_errno = 0;

  /* Uncomment this when you have failures and you are trying to debug */
  // TEST_VERBOSE_ON();

  /* Check that we can create the file */
  rc = open("FILE1", O_RDWR | O_CREAT | O_TRUNC);
  TEST_POSITIVE(rc, "Unable to create FILE1 (assumes that it doesn't exist)");

  /* Check that we can create the file */
  rc = open("FILE2", O_RDWR | O_CREAT | O_TRUNC);
  TEST_POSITIVE(rc, "Unable to create FILE2 (assumes that it doesn't exist)");

  /* check that we can open the same file many times */
  f1 = open("FILE1", O_RDWR);
  TEST_POSITIVE(f1, "Unable to open FILE1 first time");

  f2 = open("FILE1", O_RDWR);
  TEST_POSITIVE(f2, "Unable to open FILE1 second time");

  f3 = open("FILE1", O_RDWR);
  TEST_POSITIVE(f3, "Unable to open FILE1 third time");

  /* Check that f1 != f2 != f3 */
  TEST_NOT_EQUAL(f1, f2, "Using same fd for multiple opens f1 = f2");
  TEST_NOT_EQUAL(f2, f3, "Using same fd for multiple opens f2 = f3");

  rc = close(f1);
  TEST_EQUAL(rc, SUCCESS, "Unable to close f1");
  rc = close(f2);
  TEST_EQUAL(rc, SUCCESS, "Unable to close f2");
  rc = close(f3);
  TEST_EQUAL(rc, SUCCESS, "Unable to close f3");

  /* Try writing to a closed file should fail */
  rc = write(f1, (char *) &i, sizeof(i));
	saved_errno = errno;
  TEST_NEGATIVE(rc, "write to closed file f1 should fail");
#ifdef USING_ERR_CODES
  TEST_EQUAL(saved_errno, EBADF, "Expected EBADF when writing to closed file f1");
#endif /* USING_ERR_CODES */

  /* Try reading from a closed file should fail */
  rc = read(f2, (char *) &j, sizeof(j));
	saved_errno = errno;
  TEST_NEGATIVE(rc, "read from closed file f2 should fail");
#ifdef USING_ERR_CODES
  TEST_EQUAL(saved_errno, EBADF, "Expected EBADF when reading from closed file f2");
#endif /* USING_ERR_CODES */

  rc = close(0xdeadbeef);
	saved_errno = errno;
  TEST_NEGATIVE(rc, "close on invalid file id didn't return error code");
#ifdef USING_ERR_CODES
  TEST_EQUAL(saved_errno, EBADF, "Expected EBADF when closing invalid file fd");
#endif /* USING_ERR_CODES */

  rc = open(BOGUS_NAME, O_RDWR);
	saved_errno = errno;
  TEST_NEGATIVE(rc, "open non-existant file returns incorrect value");
#ifdef USING_ERR_CODES
  TEST_EQUAL(saved_errno, ENOENT, "Expected ENOENT when opening non existant file");
#endif /* USING_ERR_CODES */

  /* Open read only */
  f1 = open("FILE1", O_RDONLY);
  TEST_POSITIVE(f1, "Unable to open FILE1");

  /* Try writing to read only file */
  rc = write(f1, "hello", 5);
	saved_errno = errno;
  TEST_NEGATIVE(rc, "Trying to write to read only file does not fail");
#ifdef USING_ERR_CODES
  TEST_EQUAL(saved_errno, EBADF, "Expected EBAD when trying to write to read only file");
#endif /* USING_ERR_CODES */
  
  /* Open write only */
  f2 = open("FILE2", O_WRONLY);
  TEST_POSITIVE(f1, "Unable to open FILE2");

  /* Try reading from write only file */
  rc = read(f2, &i, 1);
	saved_errno = errno;
  TEST_NEGATIVE(rc, "Trying to read from write only file does not fail");
#ifdef USING_ERR_CODES
  TEST_EQUAL(saved_errno, EBADF, "Expected EBAD when trying to read from write only file");
#endif /* USING_ERR_CODES */

  rc = close(f1);
  TEST_EQUAL(rc, SUCCESS, "Unable to close f1");

  rc = close(f2);
  TEST_EQUAL(rc, SUCCESS, "Unable to close f2");
  
  do {
    f1 = open("FILE1", O_RDWR);
		saved_errno = errno;
    if (f1 >= 0) {
      fd_array[count] = f1;
			count++;
		}
  } while (f1 >= 0 && count < COUNT_LIMIT);

  if (count == COUNT_LIMIT) {
    printf("WARNING: THERE MAY NOT BE A LIMIT ON THE NUMBER OF OPEN FILES\n");
  } else {
    TEST_NEGATIVE(f1, "Opening too many files doesn't return error code");
#ifdef USING_ERR_CODES
    TEST_EQUAL_ONE_OF(saved_errno, EMFILE, ENFILE, "Expected one of EMFILE or ENFILE when opening too many files");
#endif /* USING_ERR_CODES */
  }

  TEST_POSITIVE(count, "Count of open files expected to be > 0");
  printf("Number of files opened = %d\n", count);

  /* Close them all so we have more fds available to do other things with */
  for (i=0; i<count; i++) {
    rc = close(fd_array[i]);
    TEST_EQUAL(rc, 0, "Expected close to return 0 for success");
  }

  /* Try passing a bogus address, should return failure code, should not crash kernel */
  rc = open((char *) 0xffffffff, O_RDWR);
	saved_errno = errno;
  TEST_NEGATIVE(rc, "open file using bad address doesn't return error code");
#ifdef USING_ERR_CODES
  TEST_EQUAL(saved_errno, EFAULT, "Expected EFAULT for invalid address for filename");
#endif /* USING_ERR_CODES */

  TEST_STATS();

  exit(0);
}
