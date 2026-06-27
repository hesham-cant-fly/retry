/**
 * @file my_temporary_allocator.h
 * @brief Temporary bump allocator (global fixed-size buffer).
 *
 * Provides a fast bump allocator backed by a global fixed-size buffer
 * (default 2048 bytes).  The buffer can be replaced with a larger
 * heap allocation via `setup_temporary_allocator`.  Reset is O(1) —
 * just set the bump pointer back to 0.
 *
 * Define `MY_TEMPORARY_ALLOCATOR_IMPL` in exactly one translation unit
 * to generate the implementation.
 *
 * Define `MY_TEMPORARY_ALLOCATOR_DEF` to `static` before inclusion to give
 * all functions internal linkage (STB-style single-TU usage).
 */

#ifndef MY_TEMPORARY_ALLOCATOR_H_
#define MY_TEMPORARY_ALLOCATOR_H_

/* #define MY_TEMPORARY_ALLOCATOR_IMPL */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "my_allocator.h"
#include <stdint.h>

#ifndef MY_TEMPORARY_ALLOCATOR_DEF
#  define MY_TEMPORARY_ALLOCATOR_DEF
#endif /* !MY_TEMPORARY_ALLOCATOR_DEF */

/**
 * @brief Optionally increase the internal buffer size.
 *
 * When called, the internal buffer is replaced with a heap allocation
 * of the requested size (via the default allocator). Must be matched
 * with a call to `free_temporary_allocator`.
 */
MY_TEMPORARY_ALLOCATOR_DEF void setup_temporary_allocator(size_t size);

/**
 * @brief Free the heap buffer (if any) and revert to the static default buffer.
 */
MY_TEMPORARY_ALLOCATOR_DEF void free_temporary_allocator(void);

/**
 * @brief Get an allocator_t that allocates from the temporary bump region.
 * @return An allocator_t backed by the temporary buffer.
 */
MY_TEMPORARY_ALLOCATOR_DEF allocator_t get_temporary_allocator(void);

/**
 * @brief Reset the bump pointer to the beginning (fast clear).
 *
 * Does not free any memory, just marks everything as reusable.
 */
MY_TEMPORARY_ALLOCATOR_DEF void reset_temporary_allocator(void);

#ifdef MY_TEMPORARY_ALLOCATOR_IMPL
#include <stdio.h>
#include <stdlib.h>

#define TEMPORARY_ALLOCATOR_DEFAULT_CAP 2048

/** @brief Check whether the temporary allocator uses a heap buffer. */
#define is_using_heap() (global_temporary.buffer != global_temporary_buffer)

/** @brief Internal state of the temporary allocator. */
typedef struct temporary_allocator_t {
	size_t used;
	size_t cap;
	unsigned char *buffer;
} temporary_allocator_t;

static unsigned char global_temporary_buffer[TEMPORARY_ALLOCATOR_DEFAULT_CAP] = {0};
static temporary_allocator_t global_temporary = {
	0,
	TEMPORARY_ALLOCATOR_DEFAULT_CAP,
	global_temporary_buffer,
};

static void *temp_allocate_virt(void *self, size_t alignment, size_t size);
static void *temp_reallocate_virt(void *self, size_t old_size, void *ptr, size_t alignment, size_t new_size);
static void temp_free_virt(void *self, size_t size, void *ptr);

static allocator_interface_t temporary_allocator_vtable = {
	temp_allocate_virt,
	temp_reallocate_virt,
	temp_free_virt,
};

MY_TEMPORARY_ALLOCATOR_DEF void setup_temporary_allocator(size_t size)
{
	if (is_using_heap())
	{
		global_temporary.buffer = renew(size, global_temporary.buffer);
		global_temporary.cap = size;
		return;
	}

	global_temporary.buffer = make(size);
	global_temporary.cap = size;
}

MY_TEMPORARY_ALLOCATOR_DEF void free_temporary_allocator(void)
{
	if (is_using_heap())
	{
		delete(global_temporary.buffer);
	}

	global_temporary.cap = TEMPORARY_ALLOCATOR_DEFAULT_CAP;
	global_temporary.used = 0;
	global_temporary.buffer = global_temporary_buffer;
}

MY_TEMPORARY_ALLOCATOR_DEF void reset_temporary_allocator(void)
{
	global_temporary.used = 0;
}

MY_TEMPORARY_ALLOCATOR_DEF allocator_t get_temporary_allocator(void)
{
	return allocator_new(&global_temporary, &temporary_allocator_vtable);
}

/** @brief Bump-allocate from the temporary buffer. */
static void *temp_allocate_virt(void *data, size_t alignment, size_t size)
{
	temporary_allocator_t *const self = data;

	uintptr_t curr = (uintptr_t)(self->buffer + self->used);
	uintptr_t aligned = (curr + alignment - 1) & ~(alignment - 1);
	size_t padding = aligned - curr;

	self->used += size + padding;
	if (self->used >= self->cap)
	{
		fprintf(stderr, "FATAL ERROR: not enough temporary space.\n");
		exit(1);
		return NULL;
	}

	return (void*)aligned;
}

#define MAX(a, b) (a) > (b) ? (a) : (b)
#define MIN(a, b) (a) < (b) ? (a) : (b)

/** @brief Reallocate within the temporary buffer (bump + copy). */
static void *temp_reallocate_virt(void *data, size_t old_size, void *ptr, size_t alignment, size_t new_size)
{
	temporary_allocator_t *const self = data;
	size_t potential_allocation_size;
	void *result;

	(void)old_size;
	potential_allocation_size = (uintptr_t)(self->buffer + self->used) - (uintptr_t)ptr;
	result = temp_allocate_virt(self, alignment, new_size);
	memcpy(result, ptr, MIN(potential_allocation_size, new_size));

	return result;
}

static void temp_free_virt(void *data, size_t size, void *ptr)
{
	temporary_allocator_t *const self = data;
	(void)self;
	(void)size;
	(void)ptr;
}

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !MY_TEMPORARY_ALLOCATOR_H_ */
