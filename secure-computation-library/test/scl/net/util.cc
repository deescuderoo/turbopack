#include "util.h"

int test_port = _SCL_DEFAULT_TEST_PORT;

int scl_tests::GetPort() { return test_port++; }

bool scl_tests::BufferEquals(const unsigned char *a, const unsigned char *b,
                             int n) {
  while (n-- > 0 && *a++ == *b++)
    ;
  return n < 0;
}
