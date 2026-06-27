/**
 * @file my_managed_array.h
 * @brief Dynamic array macros using the Allocator interface.
 *
 * Like `my_array.h`, but the allocator is stored as a field inside the
 * struct (named `.allocator`).  Operations read the allocator from the
 * struct itself, so you don't need to pass it explicitly every time.
 *
 * Configure `INITIAL_CAP` and `GROW_FACTOR` before inclusion to
 * override defaults.
 */

#ifndef MY_MANAGED_ARRAY_H_
#define MY_MANAGED_ARRAY_H_

#include <stdlib.h>
#include <assert.h>
#include "my_allocator.h"

#ifndef INITIAL_CAP
/** @def INITIAL_CAP
 *  @brief Default initial capacity (overridable). */
# define INITIAL_CAP 256
#endif /* !INITIAL_CAP */
#ifndef GROW_FACTOR
/** @def GROW_FACTOR
 *  @brief Capacity multiplier when growing (overridable). */
# define GROW_FACTOR 2
#endif /* !GROW_FACTOR */

/** @def marrinit(T, allocator)
 *  @brief Initialise a managed array struct literal with the given allocator.
 *  @param T          The struct type (must have `.allocator` field).
 *  @param allocator  The Allocator to use. */
#define marrinit(T_, allocator_) (T_) { .allocator = (allocator_) }

/** @def marrsize(array)
 *  @brief Total byte size of the allocated items buffer. */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#  define marrsize(...) (sizeof(*(__VA_ARGS__).items)) * (__VA_ARGS__).cap
#else
#  define marrsize(array_) (sizeof(*(array_).items)) * (array_).cap
#endif

/** @def marrfree(array)
 *  @brief Free the items buffer and reset len/cap to 0. */
#define marrfree(array_) \
	do { \
		xdestroy((array_).allocator, marrsize(array_), (array_).items); \
		(array_).len = 0; \
		(array_).cap = 0; \
	} while (0)

/** @def marrpush(array, item)
 *  @brief Append an item (auto-grows using the embedded allocator). */
#define marrpush(array_, item_) \
	do { \
		if ((array_).len >= (array_).cap) { \
			marrgrow((array_)); \
		} \
		(array_).items[(array_).len++] = (item_);	\
	} while (0)

/** @def marrgrow(array)
 *  @brief Grow the array capacity. */
#define marrgrow(array_) \
	do { \
		const size_t old_cap_ = (array_).cap; \
		const size_t new_cap_ = old_cap_ == 0 ? (INITIAL_CAP) : old_cap_ * (GROW_FACTOR); \
		(array_).cap = new_cap_; \
		(array_).items = xrecreate((array_).allocator, old_cap_ * sizeof(*(array_).items), new_cap_ * sizeof(*(array_).items), (array_).items); \
	} while (0)

/** @def marrinsert(array, item, pos)
 *  @brief Insert an item at position `pos`, shifting elements right.
 *  @param array  The array struct.
 *  @param item   Value to insert.
 *  @param pos    Target index (must be <= len). */
#define marrinsert(array_, item_, pos_) \
	do { \
		size_t len_; \
		size_t i_; \
		len_ = (array_).len; \
		marrreserve((array_), len_ + 1); \
		assert((pos_) <= len_); \
		for (i_ = len_; i_ >= (pos_) ; i_ -= 1) { \
			(array_).items[i_] = (array_).items[i_ - 1]; \
		} \
		(array_).items[(pos_)] = (item_); \
		(array_).len += 1; \
	} while (0)

/** @def marrpop(array)
 *  @brief Decrement length (remove last element, no memory free). */
#define marrpop(array_) \
	do { \
		if ((array_).len == 0) break; \
		(array_).len -= 1; \
	} while (0)

/** @def marrreserve(array, min_cap)
 *  @brief Ensure at least `min_cap` capacity. */
#define marrreserve(array_, min_cap_) \
	do { \
		if ((array_).cap >= (min_cap_)) {	\
			break; \
		} \
		size_t new_cap_ = (array_).cap ? (array_).cap : INITIAL_CAP; \
		while (new_cap_ < (min_cap_)) { \
			new_cap_ *= (GROW_FACTOR);			  \
		} \
		(array_).items = xrecreate((array_).allocator, (array_).cap * sizeof(*(array_).items), new_cap_ * sizeof(*(array_).items), (array_).items); \
		(array_).cap = new_cap_; \
	} while (0)

/** @def arrlen(array)
 *  @brief Get the current length. */
#define arrlen(array_) ((array_).len)
/** @def arrcap(array)
 *  @brief Get the current capacity (NOTE: currently same as len). */
#define arrcap(array_) ((array_).len)
/** @def arrget(array, index)
 *  @brief Get the element at `index`. */
#define arrget(array_, index_) ((array_).items[index_])

#endif /* !MY_MANAGED_ARRAY_H_ */
