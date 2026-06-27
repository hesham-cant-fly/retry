/**
 * @file my_arena_allocator.h
 * @brief Arena (bump) allocator.
 *
 * Allocates from large blocks obtained from a child allocator.
 * Individual frees are no-ops — the entire arena is freed at once
 * by calling `arena_free`.
 *
 * Define `MY_ARENA_ALLOCATOR_IMPL` in exactly one translation unit
 * to generate the implementation.
 *
 * Define `MY_ARENA_ALLOCATOR_DEF` to `static` before inclusion to give all
 * functions internal linkage (STB-style single-TU usage).
 */

#ifndef MY_ARENA_ALLOCATOR_H_
#define MY_ARENA_ALLOCATOR_H_

/* #define MY_ARENA_ALLOCATOR_IMPL */

#include <stdlib.h>
#include <string.h>
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "my_allocator.h"
#include <stddef.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#ifndef MY_ARENA_ALLOCATOR_DEF
#  define MY_ARENA_ALLOCATOR_DEF
#endif /* !MY_ARENA_ALLOCATOR_DEF */

/**
 * @brief A linked-list block of arena memory.
 */
typedef struct arena_block_t arena_block_t;

struct arena_block_t {
	arena_block_t *next;
	void *current;
	void *end;
};

/**
 * @brief Arena allocator state.
 */
typedef struct arena_t {
	allocator_t child_allocator;
	arena_block_t *begin;
	arena_block_t *end;
	size_t total_size;
} arena_t;

#ifndef ARENA_REGION_DEFAULT_CAPACITY
/** @def ARENA_REGION_DEFAULT_CAPACITY
 *  @brief Default block size for new arena regions (overridable). */
# define ARENA_REGION_DEFAULT_CAPACITY (8 * 1024)
#endif /* ARENA_REGION_DEFAULT_CAPACITY */

/**
 * @brief Create an arena backed by a given child allocator.
 * @param child_allocator  The allocator used to allocate arena blocks.
 * @return An initialised arena_t (no blocks allocated yet).
 */
MY_ARENA_ALLOCATOR_DEF arena_t arena_new(allocator_t child_allocator);
/**
 * @brief Create an arena backed by the default allocator.
 * @return An initialised arena_t using the global default allocator.
 */
MY_ARENA_ALLOCATOR_DEF arena_t arena_new_default(void);
/**
 * @brief Free all arena blocks and reset the arena to zero.
 * @param self  The arena to destroy.
 */
MY_ARENA_ALLOCATOR_DEF void arena_free(arena_t *self);
/**
 * @brief Print arena block usage statistics to a file.
 * @param out   Output file (e.g. stdout, stderr).
 * @param arena The arena to inspect (passed by value).
 */
MY_ARENA_ALLOCATOR_DEF void arena_print(FILE *out, const arena_t arena);

/**
 * @brief Get an allocator_t that allocates from this arena.
 * @param arena  The arena (must outlive the returned allocator).
 * @return An allocator_t backed by the arena.
 */
MY_ARENA_ALLOCATOR_DEF allocator_t arena_get_allocator(arena_t *arena);

#ifdef MY_ARENA_ALLOCATOR_IMPL
/* --- arena vtable implementation --- */
static void *arena_allocate_virt(void *data, size_t alignment, size_t size);
static void *arena_reallocate_virt(void *data, size_t old_size, void *ptr, size_t alignment, size_t new_size);
static void arena_free_virt(void *data, size_t size, void *ptr);

/** @brief Vtable for arena allocations. */
static allocator_interface_t vtable = {
	arena_allocate_virt,
	arena_reallocate_virt,
	arena_free_virt,
};

MY_ARENA_ALLOCATOR_DEF arena_t arena_new(allocator_t child_allocator)
{
	arena_t result;
	memset(&result, 0, sizeof(result));
	result.child_allocator = child_allocator;
	return result;
}

MY_ARENA_ALLOCATOR_DEF arena_t arena_new_default(void)
{
	return arena_new(get_default_allocator());
}

MY_ARENA_ALLOCATOR_DEF void arena_free(arena_t *self)
{
	arena_block_t *current = self->begin;

	while (current != NULL) {
		arena_block_t *next = current->next;
		xdestroy(self->child_allocator, self->total_size, current);
		current = next;
	}

	memset(self, 0, sizeof(*self));
}

MY_ARENA_ALLOCATOR_DEF allocator_t arena_get_allocator(arena_t *arena)
{
	return allocator_new(arena, &vtable);
}

/** @brief Allocate a new arena block of at least `min_size` bytes. */
static arena_block_t *arena_append_block(arena_t *self, size_t min_size)
{
	size_t block_size;
	arena_block_t* new_block;

	block_size = (ARENA_REGION_DEFAULT_CAPACITY > min_size ? ARENA_REGION_DEFAULT_CAPACITY : min_size) + sizeof(arena_block_t);
	if (min_size + sizeof(arena_block_t) > block_size) {
		block_size = min_size + sizeof(arena_block_t);
	}

	new_block = align_alloc(
		self->child_allocator,
		GET_ALIGNMENT(arena_block_t),
		block_size);
	if (new_block == NULL) {
		return NULL;
	}

	new_block->current = new_block + 1;
	new_block->end = (void*)((uintptr_t)new_block + block_size);
	new_block->next = NULL;

	self->total_size += block_size;
	if (self->begin == NULL) {
		assert(self->end == NULL);
		self->begin = new_block;
		self->end = new_block;
	} else {
		self->end->next = new_block;
		self->end = new_block;
	}

	return new_block;
}

/** @brief Used bytes inside a block. */
#if 0
static size_t block_used_size(const arena_block_t *self)
{
	return (uintptr_t)self->current - (uintptr_t)(self + 1);
}
#endif

/** @brief Remaining usable bytes in a block. */
static size_t block_available_size(const arena_block_t *self)
{
	return (uintptr_t)self->end - (uintptr_t)self->current;
}

/** @brief Total usable capacity of a block (excluding the header). */
static size_t block_actual_size(const arena_block_t *self)
{
	return (uintptr_t)self->end - (uintptr_t)(self + 1);
}

/** @brief Find which arena block a pointer belongs to. */
static arena_block_t *find_owner(arena_t *self, void *ptr)
{
	arena_block_t *current = self->begin;

	while (current != NULL) {
		arena_block_t *next = current->next;
		void *start = current + 1;
		if (ptr < current->end && ptr >= start) {
			return current;
		}
		current = next;
	}

	return NULL;
}

/** @brief Allocate from the arena (bump + align). */
static void *arena_allocate_virt(void *data, size_t alignment, size_t size)
{
	arena_t *self = data;
	arena_block_t *blk;
	uintptr_t curr;
	uintptr_t aligned;
	size_t padding;

	if (self->end == NULL) {
		assert(self->begin == NULL);
		arena_append_block(self, size);
	}

	blk = self->end;
	curr = (uintptr_t)blk->current;
	aligned = (curr + alignment - 1) & ~(alignment - 1);
	padding = aligned - curr;

	if (block_available_size(blk) < size + padding) {
		blk = arena_append_block(self, size);

		curr = (uintptr_t)blk->current;
		aligned = (curr + alignment - 1) & ~(alignment - 1);
		padding = aligned - curr;
	}

	blk->current = (void*)(aligned + size);
	return (void*)aligned;
}

#define MAX(a, b) (a) > (b) ? (a) : (b)
#define MIN(a, b) (a) < (b) ? (a) : (b)

static void *arena_reallocate_virt(void *data, size_t old_size, void *ptr, size_t alignment, size_t new_size)
{
	arena_t *const self = data;
	arena_block_t *const target = find_owner(self, ptr);
	size_t potential_allocation_size;
	void* result;

	(void)old_size;
	if (target == NULL) return NULL;

	potential_allocation_size = (uintptr_t)target->current - (uintptr_t)ptr;
	result = arena_allocate_virt(self, alignment, new_size);
	memcpy(result, ptr, MIN(potential_allocation_size, new_size));

	return result;
}

static void arena_free_virt(void *data, size_t size, void *ptr)
{
	(void)size;
	((void)data);
	((void)ptr);
}

MY_ARENA_ALLOCATOR_DEF void arena_print(FILE *out, const arena_t arena)
{
	const size_t width = 20;
	arena_block_t *current = arena.begin;
	size_t index = 0;
	char *const bar = calloc(sizeof(char), width + 1);
	if (bar == NULL) return;

	while (current != NULL) {
		arena_block_t *next = current->next;
		const float persentage = (float)(block_actual_size(current) - block_available_size(current)) / (float)block_actual_size(current);

		size_t i;
		memset(bar, ' ', width);
		for (i=0; i < width * persentage; i += 1) bar[i] = '=';

		fprintf(
			out,
			"Block(%04zu): %6.2f%% [%s] %zu/%zu bytes\n",
			index++, persentage * 100.0F, bar,
			block_actual_size(current) - block_available_size(current),
			block_actual_size(current));
		current = next;
	}

	{
		double size = (double)arena.total_size;
		const char *unit = "B";

		if (size >= 1024) { size /= 1024; unit = "kB"; }
		if (size >= 1024) { size /= 1024; unit = "MB"; }
		if (size >= 1024) { size /= 1024; unit = "GB"; }

		fprintf(out, "\t> Allocated: %zu bytes (%.2f %s)\n",
				arena.total_size, size, unit);
	}

	free(bar);
}

#endif /* !MY_ARENA_ALLOCATOR_IMPL */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !MY_ARENA_ALLOCATOR_H_ */
