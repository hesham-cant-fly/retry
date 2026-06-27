/**
 * @file my_error.h
 * @brief Rust-style Result type macros.
 *
 * Provides a generic `Result(T, E)` type and macros for constructing,
 * inspecting, and unwrapping results.  Inspired by Rust's `Result`.
 *
 * Core macros (`Result`, `Ok`, `Err`, `is_ok`, `is_err`, `unwrap`,
 * `unwrap_err`, `expect`, `unwrap_or`) work in C99.
 *
 * Advanced macros (`propagate`, `try`, `catch`) use the `auto`
 * type-inference keyword (C23 / GNU extension) and may not be
 * available in strict C99 mode.
 */

#ifndef MY_ERROR_H_
#define MY_ERROR_H_

/* C89 compat: bool is a C99 keyword */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#  include <stdbool.h>
#else
#  if !defined(bool)
#    define bool int
#    define true 1
#    define false 0
#  endif
#endif
#include "my_commons.h"

/** @def Result(T, E)
 *  @brief Create a Result type holding either an Ok value of type T or an Err value of type E.
 *  @param T  The Ok type.
 *  @param E  The Err type. */
#define Result(T_, E_) struct { bool _is_err; union { T_ _ok; E_ _err; } _u; }

/** @def Ok(result_type, value)
 *  @brief Construct an Ok result.
 *  @param result_type  The Result type (e.g. `Result(int, const char*)`).
 *  @param value        The Ok value. */
#define Ok(R, v)   ((R){ ._is_err = false, ._u._ok = (v) })

/** @def Err(result_type, value)
 *  @brief Construct an Err result.
 *  @param result_type  The Result type.
 *  @param value        The Err value. */
#define Err(R, e)  ((R){ ._is_err = true,  ._u._err = (e) })

/** @def is_ok(result)
 *  @brief Check if a Result is Ok.
 *  @return true if Ok, false if Err. */
#define is_ok(r)   (!(r)._is_err)

/** @def is_err(result)
 *  @brief Check if a Result is Err.
 *  @return true if Err, false if Ok. */
#define is_err(r)  ((r)._is_err)

/** @def unwrap(result)
 *  @brief Unwrap an Ok value. Aborts if the Result is Err.
 *  @return The contained Ok value. */
#define unwrap(r)     (is_err(r) ? unreachable() : (void)0, (r)._u._ok)

/** @def unwrap_err(result)
 *  @brief Unwrap an Err value. Aborts if the Result is Ok.
 *  @return The contained Err value. */
#define unwrap_err(r) (!is_err(r) ? unreachable() : (void)0, (r)._u._err)

/** @def expect(result, msg)
 *  @brief Unwrap an Ok value, printing a message on Err before aborting.
 *  @param r    The Result.
 *  @param msg  Message printed to stderr on Err.
 *  @return The contained Ok value. */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#  define expect(r, ...) \
	(is_err(r) ? panic(__VA_ARGS__) : (void)0, (r)._u._ok)
#else
#  define expect(r, msg) \
	(is_err(r) ? panic(msg) : (void)0, (r)._u._ok)
#endif

/** @def unwrap_or(result, default)
 *  @brief Unwrap an Ok value, or return a default on Err.
 *  @param r  The Result.
 *  @param d  Default value returned if Err.
 *  @return The Ok value, or the default. */
#define unwrap_or(r, d) (is_err(r) ? (d) : (r)._u._ok)

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
/** @def propagate(...)
 *  @brief Return early if the expression is Err (requires `auto` / C23).
 *  @param ...  A Result expression. */
#  define propagate(...) do { auto _ = (__VA_ARGS__); if (_._is_err) return _; } while(0)

/** @def try(name, expr)
 *  @brief Bind the Ok value of a Result, returning early on Err (requires `auto` / C23).
 *  @param name  Variable name for the unwrapped Ok value.
 *  @param ...   A Result expression. */
#  define try(name_, ...) \
	for (int _ok_ = 1, _run_ = 1; _ok_; _ok_ = 0) \
	for (auto _ = (__VA_ARGS__); _run_; _run_ = 0) \
	if (_._is_err) return _; \
	else for (auto name_ = _._u._ok; _ok_; _ok_ = 0)

/** @def catch(name, expr)
 *  @brief Capture the Err value of a Result, skipping Ok (requires `auto` / C23).
 *  @param name  Variable name for the captured Err value.
 *  @param ...   A Result expression. */
#  define catch(name_, ...) \
	for (int _ok_ = 1, _run_ = 1; _ok_; _ok_ = 0) \
	for (auto _ = (__VA_ARGS__); _run_; _run_ = 0) \
	if (_._is_err) \
		for (auto name_ = _._u._err; _ok_; _ok_ = 0)
#endif

#endif /* !MY_ERROR_H_ */
