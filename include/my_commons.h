/**
 * @file my_commons.h
 * @brief Common utility macros and functions.
 *
 * Provides `panic`, `unreachable`, `unimplemented`, and iteration
 * macros for arrays and linked lists.
 *
 * Define `MY_COMMONS_IMPLEMENTATION` in exactly one translation unit
 * to generate the implementation.
 *
 * Define `MY_COMMONS_DEF` to `static` before inclusion to give all functions
 * internal linkage (STB-style single-TU usage).
 */

#ifndef __MY_COMMONS_H
#define __MY_COMMONS_H

/* #define MY_COMMONS_IMPLEMENTATION */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdio.h>

#ifndef MY_COMMONS_DEF
#  define MY_COMMONS_DEF
#endif /* !MY_COMMONS_DEF */

/** @def unused(x)
 *  @brief Suppress unused-variable warnings. */
#define unused(__x) ((void)(__x))
/** @def panic(fmt)
 *  @brief Print file:line and formatted message, then exit with code 69. */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#  define panic(...) _panic(__FILE__, __LINE__, __VA_ARGS__)
#else
#  define panic(msg) _panic(__FILE__, __LINE__, msg)
#endif
/** @def unreachable()
 *  @brief Print file:line and abort — marks code paths that must never execute. */
#define unreachable() _unreachable(__FILE__, __LINE__)

/**
 * @brief Internal panic function (use the `panic` macro instead).
 * @param file  Source file name.
 * @param line  Source line number.
 * @param fmt   Printf-like format string.
 * @param ...   Format arguments.
 */
MY_COMMONS_DEF void _panic(const char *file, int line, const char *  fmt, ...);
/**
 * @brief Internal unreachable handler (use the `unreachable` macro instead).
 */
MY_COMMONS_DEF void _unreachable(const char *file, int line);

/** @def unimplemented()
 *  @brief Print file:line and exit — marks a stub that is not yet implemented. */
#define unimplemented() \
	do { \
		fprintf(stderr, "%s:%d: Error: this is not implemented yet.\n", __FILE__, __LINE__);\
		Break(); \
		exit(1); \
	} while (0)

#ifdef MY_COMMONS_IMPLEMENTATION
#include <stdlib.h>
MY_COMMONS_DEF void _panic(const char *file, int line, const char *  fmt, ...) {
	fprintf(stderr, "%s:%d: Error: Panic: %s.\n", file, line, fmt);
	exit(69);
}

MY_COMMONS_DEF void _unreachable(const char *file, int line) {
	fprintf(stderr, "%s:%d: Error: reached an unreachable.\n", file, line);
	exit(1);
}
#endif /* MY_COMMONS_IMPLEMENTATION */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !__MY_COMMONS_H */
