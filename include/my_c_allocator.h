/**
 * @file my_c_allocator.h
 * @brief Standard library allocator adapter (malloc / realloc / free).
 *
 * Wraps the C runtime heap allocator into the `allocator_t` interface.
 *
 * Define `MY_C_ALLOCATOR_IMPL` in exactly one translation unit
 * to generate the implementation.
 *
 * Define `MY_C_ALLOCATOR_DEF` to `static` before inclusion to give all functions
 * internal linkage (STB-style single-TU usage).
 */

#ifndef MY_C_ALLOCATOR_H_
#define MY_C_ALLOCATOR_H_

/* #define MY_C_ALLOCATOR_IMPL */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "my_allocator.h"

#ifndef MY_C_ALLOCATOR_DEF
#  define MY_C_ALLOCATOR_DEF
#endif /* !MY_C_ALLOCATOR_DEF */

/**
 * @brief Return an allocator_t backed by libc's malloc / realloc / free.
 * @return An allocator_t that delegates to the standard C heap functions.
 */
MY_C_ALLOCATOR_DEF allocator_t get_c_allocator(void);

#ifdef MY_C_ALLOCATOR_IMPL
#include <stdlib.h>

static void *c_alloc_virt(void *self, size_t alignment, size_t size);
static void *c_realloc_virt(void *self, size_t old_size, void *ptr, size_t alignment, size_t size);
static void  c_free_virt(void *self, size_t size, void *ptr);

/** @brief Vtable for the C allocator. */
static allocator_interface_t c_allocator_vtable = {
	c_alloc_virt,
	c_realloc_virt,
	c_free_virt,
};

MY_C_ALLOCATOR_DEF allocator_t get_c_allocator(void)
{
	{ allocator_t r_; r_.vtable = &c_allocator_vtable; r_.data = NULL; return r_; }
}

/** @brief Allocate via malloc (alignment ignored). */
static void *c_alloc_virt(void *self, size_t alignment, size_t size)
{
	((void)self);
	((void)alignment);
	return malloc(size);
}

/** @brief Reallocate via realloc (old_size / alignment ignored). */
static void *c_realloc_virt(void *self, size_t old_size, void *ptr, size_t alignment, size_t size)
{
	((void)self);
	(void)old_size;
	((void)alignment);
	return realloc(ptr, size);
}

/** @brief Free via free (size / alignment ignored). */
static void c_free_virt(void *self, size_t size, void *ptr)
{
	((void)self);
	(void)size;
	free(ptr);
}

#endif /* MY_C_ALLOCATOR_IMPL */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !MY_C_ALLOCATOR_H_ */
