#ifndef _TEST_SCL_NET_UTIL_H
#define _TEST_SCL_NET_UTIL_H

#include <cstddef>
namespace scl_tests {

/**
 * @brief The default starting point for ports.
 */
#ifndef _SCL_DEFAULT_TEST_PORT
#define _SCL_DEFAULT_TEST_PORT 14421
#endif

/**
 * @brief Get a fresh port for use in tests that require ports.
 * @note Not thread safe.
 */
int GetPort();

/**
 * @brief Test if two buffers are equal.
 * @param a the first buffer
 * @param b the second buffer
 * @param n the number of bytes to check
 * @param true if \p a and \p b coincide on the first \p n bytes.
 */
bool BufferEquals(const unsigned char* a, const unsigned char* b, int n);

}  // namespace scl_tests

#endif /* _TEST_SCL_NET_UTIL_H */
