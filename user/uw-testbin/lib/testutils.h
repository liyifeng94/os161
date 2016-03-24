#ifndef TESTUTILS_H
#define TESTUTILS_H

#define SUCCESS     (0)

#define TEST_EQUAL(a, b, s)  \
  test_equal(a, b, s, __FILE__, __FUNCTION__, __LINE__)

#define TEST_EQUAL_ONE_OF(a, b, c, s)  \
  test_equal_one_of(a, b, c, s, __FILE__, __FUNCTION__, __LINE__)

#define TEST_NOT_EQUAL(a, b, s) \
  test_not_equal(a, b, s, __FILE__, __FUNCTION__, __LINE__)

#define TEST_NEGATIVE(a, s) \
  test_negative(a, s, __FILE__, __FUNCTION__, __LINE__)

#define TEST_POSITIVE(a, s) \
  test_positive(a, s, __FILE__, __FUNCTION__, __LINE__)

#define TEST_STATS() \
  test_print_stats( __FILE__, __FUNCTION__, __LINE__)

#define TEST_VERBOSE_ON() \
  test_verbose_on()

#define TEST_VERBOSE_OFF() \
  test_verbose_off()

void test_equal(int ret_val, int expected_val, const char *str,
     const char *file, const char* func, int line);
void test_equal_one_of(int val, int expected_val1, int expected_val2, const char *str, 
     const char *file, const char *func, int line);
void test_positive(int ret_val, const char *str,
     const char *file, const char* func, int line);
void test_negative(int ret_val, const char *str,
     const char *file, const char* func, int line);
void test_not_equal(int ret_val, int expected_val, const char *str,
     const char *file, const char* func, int line);
void test_print_stats(const char *file, const char* func, int line);
void test_reset_stats(void);
void test_verbose_on(void);
void test_verbose_off(void);

#endif /* TESTUTILS_H */
