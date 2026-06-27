/**
 * @file my_assert.h
 * @brief Custom assert macro with detailed diagnostic output.
 *
 * Overrides the standard `assert` macro to print the file, line, and
 * failing expression before calling `abort()`.
 *
 * @note This header replaces the standard library `assert` macro.
 */

#ifndef MY_ASSERT_H_
# define MY_ASSERT_H_

#include <stdio.h>
#include <stdlib.h>

#ifdef assert
# undef assert
#endif

/** @def assert(expr)
 * @brief Assert that an expression is true.
 *
 * If the expression evaluates to false, prints a diagnostic message
 * to stderr and calls `abort()`.
 * @param expr  The expression to test.
 */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#  define assert(...) \
	do { \
		if (!(__VA_ARGS__)) { \
			fprintf(stderr, "%s:%d: Assertion Failed: %s\n", __FILE__, __LINE__, #__VA_ARGS__); \
			abort(); \
		} \
	} while (0)
#else
#  define assert(expr_) \
	do { \
		if (!(expr_)) { \
			fprintf(stderr, "%s:%d: Assertion Failed: %s\n", __FILE__, __LINE__, #expr_); \
			abort(); \
		} \
	} while (0)
#endif

#endif /* !MY_ASSERT_H_ */
