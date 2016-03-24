
/*
 * Title    : testutils
 * Author   : Tim Brecht
 * Date     : Thu Dec 23 21:19:00 EST 2004
 * Modified : Wed  1 Jan 2014 14:17:19 EST
 *            Added TEST_EQUAL_ONE_OF
 *            Fri Jun 28 22:37:24 EDT 2013 for use with OS161
 *
 * Some simple utilities that can be helpful for testing.
 * Feel free to add/modify this code to suit your needs.
 * NOTE: this hasn't been fully tested.
 *
 * See test-testutil, file1 and file2 for example use cases.
 */

#include <stdio.h>
#include "testutils.h"

/* Used to track the number of tests and failures */
static int test_num = 0;
static int num_failures = 0;
static int verbose = 0;

/* Use to print the source file, function and line */
static void
print_location(const char *file, const char *func, int line, const char *in_test)
{
  printf("   %s : function = %s, line = %d test was %s\n", file, func, line, in_test);
}

/* Check if val is equal to expected_val, if not it is an error */
void
test_equal(int val, int expected_val, const char *str, 
            const char *file, const char *func, int line)
{
  int failed = 0;
  char const *out = "SUCCESS";

  test_num++;

  if (val != expected_val) {
    num_failures++;
    failed = 1;
    out = "FAILURE";
  }

  if (failed || verbose) {
    printf("%s ON TEST = %d : Got %d : expected %d\n", 
      out, test_num, val, expected_val);

    if (failed) {
      printf("    %s\n", str);
    }
    print_location(file, func, line, __FUNCTION__);
  }

} /* test_equal */

/* Check if val is equal to expected_val, if not it is an error */
void
test_equal_one_of(int val, int expected_val1, int expected_val2, const char *str, 
            const char *file, const char *func, int line)
{
  int failed = 0;
  char const *out = "SUCCESS";

  test_num++;

  if (val != expected_val1 && val != expected_val2) {
    num_failures++;
    failed = 1;
    out = "FAILURE";
  }

  if (failed || verbose) {
    printf("%s ON TEST = %d : Got %d : expected one of %d or %d\n", 
      out, test_num, val, expected_val1, expected_val2);

    if (failed) {
      printf("    %s\n", str);
    }
    print_location(file, func, line, __FUNCTION__);
  }

} /* test_equal */

/* Check if val is > 0, if not it is an error */
void
test_positive(int val, const char *str,
              const char *file, const char *func, int line)
{
  int failed = 0;
  char const *out = "SUCCESS";

  test_num++;

  if (val <= 0) {
    num_failures++;
    failed = 1;
    out = "FAILURE";
  }

  if (failed || verbose) {
    printf("%s ON TEST = %d : Got %d : expected Positive Value\n", 
      out, test_num, val);

    if (failed) {
      printf("    %s\n", str);
    }
    print_location(file, func, line, __FUNCTION__);
  }
}

/* Check if val is < 0, if not it is an error */
void
test_negative(int val, const char *str,
              const char *file, const char *func, int line)
{
  int failed = 0;
  char const *out = "SUCCESS";

  test_num++;

  if (val >= 0) {
    num_failures++;
    failed = 1;
    out = "FAILURE";
  }

  if (failed || verbose) {
    printf("%s ON TEST = %d : Got %d : expected Negative Value\n", 
      out, test_num, val);

    if (failed) {
      printf("    %s\n", str);
    }
    print_location(file, func, line, __FUNCTION__);
  }
}

/* Check if val1 is not equal to val2, if not it is an error */
void
test_not_equal(int val1, int val2, const char *str,
              const char *file, const char *func, int line)
{
  int failed = 0;
  char const *out = "SUCCESS";

  test_num++;

  if (val1 == val2) {
    num_failures++;
    failed = 1;
    out = "FAILURE";
  }

  if (failed || verbose) {
    printf("%s ON TEST = %d : Got %d : Expected anything but %d\n", 
      out, test_num, val1, val2);

    if (failed) {
      printf("    %s\n", str);
    }
    print_location(file, func, line, __FUNCTION__);
  }
}

/* Reset the statistic counters */
void
test_reset_stats(void)
{
  num_failures = 0;
  test_num = 0;
}

void
test_verbose_on(void)
{
  printf("TEST VERBOSE ON\n");
  verbose = 1;
}

void
test_verbose_off(void)
{
  printf("TEST VERBOSE OFF\n");
  verbose = 0;
}

void
test_print_stats(const char *file, const char* func, int line)
{
  printf("TEST STATS for %s : from function = %s, line = %d\n", 
    file, func, line);
  printf("    Number of failures = %d    Number of successes = %d   Number of Tests = %d\n",
    num_failures, test_num - num_failures, test_num);
  printf("\n");
}

// #define UNIT_TEST
#ifdef UNIT_TEST
int
main()
{
   TEST_VERBOSE_ON();

   TEST_EQUAL(1, 1, "Should pass\n");
   TEST_EQUAL(1, 2, "Should fail\n");
   TEST_EQUAL_ONE_OF(1, 1, 2, "Should pass\n");
   TEST_EQUAL_ONE_OF(1, 2, 1, "Should pass\n");
   TEST_EQUAL_ONE_OF(1, 2, 3, "Should fail\n");
   TEST_NOT_EQUAL(1, 2, "Should pass\n");
   TEST_NOT_EQUAL(2, 1, "Should pass\n");
   TEST_NOT_EQUAL(2, 2, "Should fail\n");

   TEST_NEGATIVE(-1, "Should pass\n");
   TEST_NEGATIVE(0, "Should fail\n");
   TEST_NEGATIVE(1, "Should fail\n");
   TEST_POSITIVE(1, "Should pass\n");
   TEST_POSITIVE(0, "Should fail\n");
   TEST_POSITIVE(-1, "Should fail\n");

   TEST_STATS();
   printf("Should have 7 passes and 7 failures\n");
}
#endif /* UNIT_TEST */
