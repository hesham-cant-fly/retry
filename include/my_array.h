/**
 * @file my_array.h
 * @brief Dynamic array macros (struct-based, allocator-aware).
 *
 * Operates on any struct with `cap`, `len`, and `items` fields.
 * All operations take an allocator as the first parameter.
 *
 * Configure `INITIAL_CAP` and `GROW_FACTOR` before inclusion to
 * override defaults.
 */

#ifndef MY_ARRAY_H_
#define MY_ARRAY_H_

#include <stdlib.h>
#include <assert.h>
#include "my_allocator.h"

#ifndef INITIAL_CAP
/** @def INITIAL_CAP
 *  @brief Default initial capacity (overridable). */
# define INITIAL_CAP 10
#endif /* !INITIAL_CAP */
#ifndef GROW_FACTOR
/** @def GROW_FACTOR
 *  @brief Capacity multiplier when growing (overridable). */
# define GROW_FACTOR 2
#endif /* !GROW_FACTOR */

/** @def arrsize(array)
 *  @brief Total byte size of the allocated items buffer. */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#  define arrsize(...) (sizeof(*(__VA_ARGS__).items)) * (__VA_ARGS__).cap
#else
#  define arrsize(array__) (sizeof(*(array__).items)) * (array__).cap
#endif

/** @def arrfree(allocator, array)
 *  @brief Free the items buffer and reset len/cap to 0. */
#define arrfree(allocator_, array__) \
	do { \
		xdestroy(allocator_, arrsize(array__), (array__).items); \
		(array__).len = 0; \
		(array__).cap = 0; \
	} while (0)

/** @def arrpush(allocator, array, item)
 *  @brief Append an item to the array (auto-grows). */
#define arrpush(allocator_, array__, item__) \
	do { \
		if ((array__).len >= (array__).cap) { \
			arrgrow((allocator_), (array__)); \
		} \
		(array__).items[(array__).len++] = (item__);	\
	} while (0)

/** @def arrgrow(allocator, array)
 *  @brief Grow the array capacity by GROW_FACTOR (or initialise to INITIAL_CAP). */
#define arrgrow(allocator_, array__) \
	do { \
		const size_t new_cap__ = (array__).cap == 0 ? (INITIAL_CAP) : (array__).cap * (GROW_FACTOR); \
		(array__).items = xrecreate((allocator_), arrsize(array__), (new_cap__ * sizeof(*(array__).items)), (array__).items); \
		(array__).cap = new_cap__; \
	} while (0)

/** @def arrinsert(allocator, array, item, pos)
 *  @brief Insert an item at position `pos`, shifting elements right.
 *  @param allocator  The allocator.
 *  @param array      The array struct.
 *  @param item       Value to insert.
 *  @param pos        Target index (must be <= len). */
#define arrinsert(allocator_, array__, item__, pos__) \
	do { \
		size_t len__; \
		size_t i__; \
		len__ = (array__).len; \
		arrreserve((allocator_), (array__), len__ + 1); \
		assert((pos__) <= len__); \
		for (i__ = len__; i__ >= (pos__) ; i__ -= 1) { \
			(array__).items[i__] = (array__).items[i__ - 1]; \
		} \
		(array__).items[(pos__)] = (item__); \
		(array__).len += 1; \
	} while (0)

/** @def arrpop(array)
 *  @brief Decrement length (remove the last element, no memory free). */
#define arrpop(array__) \
	do { \
		if ((array__).len == 0) break; \
		(array__).len -= 1; \
	} while (0)

/** @def arrreserve(allocator, array, min_cap)
 *  @brief Ensure at least `min_cap` capacity. */
#define arrreserve(allocator_, array__, min_cap__) \
	do { \
		if ((array__).cap >= (min_cap__)) {	\
			break; \
		} \
		size_t new_cap__ = (array__).cap ? (array__).cap : INITIAL_CAP; \
		while (new_cap__ < (min_cap__)) { \
			new_cap__ *= (GROW_FACTOR);			  \
		} \
		(array__).items = xrecreate((allocator_), arrsize(array__), new_cap__ * sizeof(*(array__).items), (array__).items); \
		(array__).cap = new_cap__; \
	} while (0)

/** @def marrlen(array)
 *  @brief Get the current length of the array. */
#define marrlen(array__) ((array__).len)
/** @def marrcap(array)
 *  @brief Get the current capacity (NOTE: currently same as len). */
#define marrcap(array__) ((array__).len)
/** @def marrget(array, index)
 *  @brief Get the element at `index`. */
#define marrget(array__, index__) ((array__).items[index__])

#endif /* !MY_ARRAY_H_ */
