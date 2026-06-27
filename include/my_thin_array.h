/**
 * @file my_thin_array.h
 * @brief Dynamic thin array (pointer-based, header before the data).
 *
 * The array metadata (length, capacity) is stored in a header immediately
 * before the returned pointer.  The user interacts with a plain typed
 * pointer as if it were a normal C array.
 *
 * Define `MY_ARRAY_IMPL` in exactly one translation unit to generate
 * the implementation.
 *
 * Define `MY_THIN_ARRAY_DEF` to `static` before inclusion to give all
 * functions internal linkage (STB-style single-TU usage).
 */

#ifndef MY_THIN_ARRAY_H_
#define MY_THIN_ARRAY_H_

/* #define MY_ARRAY_IMPL */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifndef MY_THIN_ARRAY_DEF
#  define MY_THIN_ARRAY_DEF
#endif /* !MY_THIN_ARRAY_DEF */

/**
 * @brief Internal header stored before every thin array allocation.
 */
typedef struct array_list_header_t {
	size_t len;
	size_t cap;
} array_list_header_t;

#ifndef INITIAL_CAP
/** @def INITIAL_CAP
 *  @brief Default initial capacity (overridable before inclusion). */
# define INITIAL_CAP 10
#endif
#ifndef GROW_FACTOR
/** @def GROW_FACTOR
 *  @brief Growth factor when reallocating (overridable before inclusion). */
# define GROW_FACTOR 2
#endif

/** @def thinarrinit(T)
 *  @brief Create a new thin array of type T.
 *  @param T  Element type.
 *  @return Pointer to the first element slot (type T*). */
#define thinarrinit(T) (T *)(create_array(sizeof(T)))
/** @def thinarrfree(arr)
 *  @brief Free a thin array.
 *  @param arr  The array pointer. */
#define thinarrfree(arr) (assert((arr) != NULL), free(thinarrheader((arr))))
/** @def thinarrheader(arr)
 *  @brief Get the array_list_header_t pointer for a thin array. */
#define thinarrheader(arr) (assert((arr) != NULL), (array_list_header_t *)(((char *)(arr)) - sizeof(array_list_header_t)))
/** @def thinarrlen(arr)
 *  @brief Get the length of a thin array (0 if NULL). */
#define thinarrlen(arr) (((arr) != NULL) ? thinarrheader((arr))->len : 0)
/** @def thinarrsetlen(arr, new_len)
 *  @brief Set the length directly (unsafe — use with care). */
#define thinarrsetlen(arr, new_len) (assert((arr) != NULL), (thinarrheader((arr))->len = new_len))
/** @def thinarrcap(arr)
 *  @brief Get the capacity of a thin array. */
#define thinarrcap(arr) (assert((arr) != NULL), thinarrheader((arr))->cap)
/** @def thinarrpush(arr, val)
 *  @brief Append a value to a thin array (auto-grows). */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#  define thinarrpush(arr, ...) (assert((arr) != NULL), array_push((void **)&(arr), sizeof(*(arr))), arr[thinarrheader((arr))->len++] = (__VA_ARGS__))
#else
#  define thinarrpush(arr, val) (assert((arr) != NULL), array_push((void **)&(arr), sizeof(*(arr))), arr[thinarrheader((arr))->len++] = (val))
#endif
/** @def thinarrpop(arr)
 *  @brief Pop and return the last element. */
#define thinarrpop(arr) (assert((arr) != NULL), assert(thinarrheader((arr))->len != 0), (arr)[--thinarrheader((arr))->len])
/** @def thinarrreserve(arr, new_cap)
 *  @brief Ensure at least `new_cap` capacity.
 *  @param arr      The array.
 *  @param new_cap  Minimum desired capacity. */
#define thinarrreserve(__arr, __new_cap) (assert((__arr) != NULL), array_reserve((void **)&(__arr), sizeof(*(__arr)), (__new_cap)))

MY_THIN_ARRAY_DEF void *create_array(size_t element_size)
#ifdef MY_ARRAY_IMPL
{
	array_list_header_t *res =
		malloc(sizeof(array_list_header_t) + (element_size * INITIAL_CAP));
	if (res == NULL) {
		return NULL;
	}
	memset(res, 0, sizeof(array_list_header_t) + (element_size * INITIAL_CAP));
	res->len = 0;
	res->cap = INITIAL_CAP;
	return res + 1;
}
#else
	;
#endif

MY_THIN_ARRAY_DEF void array_grow(void **arr, size_t element_size)
#ifdef MY_ARRAY_IMPL
{
	array_list_header_t *header;
	array_list_header_t *new_header;
	header = thinarrheader(*arr);
	header->cap *= GROW_FACTOR;
	new_header =
		realloc(header, sizeof(array_list_header_t) + element_size * header->cap);
	*arr = new_header + 1;
}
#else
	;
#endif

MY_THIN_ARRAY_DEF void array_reserve(void **arr, size_t element_size, size_t new_cap)
#ifdef MY_ARRAY_IMPL
{
	if (thinarrcap(*arr) < new_cap)
	{
		while (thinarrcap(*arr) < new_cap)
		{
			array_grow(arr, element_size);
		}
	}
}
#else
;
#endif

MY_THIN_ARRAY_DEF void array_push(void **arr, size_t element_size)
#ifdef MY_ARRAY_IMPL
{
	array_list_header_t *header = thinarrheader(*arr);
	if (header->len >= header->cap) {
		array_grow(arr, element_size);
	}
}
#else
	;
#endif

#endif /* !MY_THIN_ARRAY_H_ */
