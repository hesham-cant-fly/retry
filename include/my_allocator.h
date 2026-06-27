/**
 * @file my_allocator.h
 * @brief Abstract allocator interface (vtable pattern).
 *
 * All memory allocators in the library conform to the `allocator_t`
 * interface.  The file also defines convenience macros that work with
 * any allocator, and a global default allocator.
 *
 * Define `MY_ALLOCATOR_IMPL` in exactly one translation unit
 * to generate the implementation.
 *
 * Define `MY_ALLOCATOR_DEF` to `static` before inclusion to give all functions
 * internal linkage (STB-style single-TU usage).
 */

#ifndef MY_ALLOCATOR_H_
#define MY_ALLOCATOR_H_

/* #define MY_ALLOCATOR_IMPL */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stddef.h>
#include <string.h>
#include "my_assert.h"

#ifndef MY_ALLOCATOR_DEF
#  define MY_ALLOCATOR_DEF
#endif /* !MY_ALLOCATOR_DEF */

typedef struct allocator_interface_t allocator_interface_t;

/**
 * @brief An allocator instance (vtable pointer + opaque data).
 */
typedef struct allocator_t {
	allocator_interface_t *vtable;
	void *data;
} allocator_t;

/**
 * @brief Virtual method table for an allocator.
 */
struct allocator_interface_t {
	/** @brief Allocate memory.
	 *  @param self       Opaque state pointer.
	 *  @param alignment  Required alignment.
	 *  @param size       Number of bytes to allocate.
	 *  @return Pointer to the allocated memory, or NULL on failure. */
	void *(*allocate)(void *self, size_t alignment, size_t size);
	/** @brief Reallocate memory (may move).
	 *  @param self       Opaque state pointer.
	 *  @param old_size   Previous allocation size.
	 *  @param ptr        Previous pointer.
	 *  @param alignment  Required alignment.
	 *  @param new_size   New size in bytes.
	 *  @return Pointer to the resized memory, or NULL on failure. */
	void *(*reallocate)(void *self, size_t old_size, void *ptr, size_t alignment, size_t new_size);
	/** @brief Free memory.
	 *  @param self  Opaque state pointer.
	 *  @param size  Size of the allocation.
	 *  @param ptr   Pointer to free. */
	void (*free)(void *self, size_t size, void *ptr);
};

/** @brief Union for determining maximum alignment. */
union max_align_t_ {
	void *p;
	long l;
	double d;
	long double ld;
};
/** @def MY_DEFAULT_ALIGNMENT
 *  @brief The default alignment used when no specific alignment is requested. */
#define MY_DEFAULT_ALIGNMENT \
	(sizeof(union max_align_t_))

#ifndef CONCAT_RAW
#  define CONCAT_RAW(a, b) a ## b
#endif // !CONCAT_RAW
#ifndef CONCAT
#define CONCAT(a, b) CONCAT_RAW(a, b)
#endif // !CONCAT

/** @def GET_ALIGNMENT(T)
 *  @brief Get the alignment of a type (C89-compatible). */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#  define GET_ALIGNMENT(T_) ((CONCAT(struct dih_master_10000_, __LINE__) { char c_; T_ x_; }){0}, offsetof(CONCAT(struct dih_master_10000_, __LINE__), x_))
#else
/* C89: sizeof is always >= alignment, safe as a conservative overestimate */
#  define GET_ALIGNMENT(T_) (sizeof(T_))
#endif

/** @def alloc(allocator, size)
 *  @brief Allocate memory with default alignment. */
#define alloc(allocator_, size_)               allocator_alloc((allocator_), MY_DEFAULT_ALIGNMENT, (size_))
/** @def align_alloc(allocator, align, size)
 *  @brief Allocate memory with a specific alignment. */
#define align_alloc(allocator_, align_, size_) allocator_alloc((allocator_), align_, (size_))

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L

/* ---- C99+ variadic macros ---- */

/** @def create(allocator, T, ...)
 *  @brief Allocate and initialise a value of type T. */
#  define create(allocator_, T_, ...) \
	(allocator_alloc_with_value((allocator_), GET_ALIGNMENT(T_), sizeof((T_){ __VA_ARGS__ }), &((T_){ __VA_ARGS__ })))
/** @def destroy(allocator, ptr)
 *  @brief Free memory (size=0 variant). */
#  define destroy(allocator_, ...)               allocator_free((allocator_), 0, (__VA_ARGS__))
/** @def recreate(allocator, new_size, ptr)
 *  @brief Reallocate memory with default alignment (old_size=0). */
#  define recreate(allocator_, new_size_, ...)   allocator_realloc((allocator_), 0, (__VA_ARGS__), MY_DEFAULT_ALIGNMENT, (new_size_))
/** @def xdestroy(allocator, size, ptr)
 *  @brief Free memory with explicit size. */
#  define xdestroy(allocator_, size_, ...)      allocator_free((allocator_), size_, (__VA_ARGS__))
/** @def xrecreate(allocator, old_size, new_size, ptr)
 *  @brief Reallocate memory with explicit old and new sizes. */
#  define xrecreate(allocator_, old_size_, new_size_, ...) allocator_realloc((allocator_), old_size_, (__VA_ARGS__), MY_DEFAULT_ALIGNMENT, (new_size_))
/** @def new(T, ...)
 *  @brief Allocate and construct via the default allocator. */
#  define new(T_, ...)          create((default_allocator_), T_, __VA_ARGS__)
/** @def delete(ptr)
 *  @brief Free memory via the default allocator. */
#  define delete(...)           destroy((default_allocator_), (__VA_ARGS__))
/** @def renew(new_size, ptr)
 *  @brief Reallocate via the default allocator. */
#  define renew(new_size_, ...) recreate((default_allocator_), (new_size_), __VA_ARGS__)

#else

/* ---- C89 fixed-argument macros ---- */

#  define create(allocator_, T_, init_ptr_) \
	allocator_alloc_with_value((allocator_), GET_ALIGNMENT(T_), sizeof(T_), (init_ptr_))
#  define destroy(allocator_, ptr_)           allocator_free((allocator_), 0, (ptr_))
#  define recreate(allocator_, new_size_, ptr_) \
	allocator_realloc((allocator_), 0, (ptr_), MY_DEFAULT_ALIGNMENT, (new_size_))
#  define xdestroy(allocator_, size_, ptr_)   allocator_free((allocator_), size_, (ptr_))
#  define xrecreate(allocator_, old_size_, new_size_, ptr_) \
	allocator_realloc((allocator_), old_size_, (ptr_), MY_DEFAULT_ALIGNMENT, (new_size_))
#  define new(T_, init_ptr_)                  create((default_allocator_), T_, init_ptr_)
#  define delete(ptr_)                        destroy((default_allocator_), (ptr_))
#  define renew(new_size_, ptr_)              recreate((default_allocator_), (new_size_), ptr_)

#endif /* __STDC_VERSION__ >= 199901L */

/** @def make(size)
 *  @brief Allocate raw bytes via the default allocator. */
#define make(size_)   alloc((default_allocator_), (size_))
/** @def aligned_make(align, size)
 *  @brief Allocate aligned raw bytes via the default allocator. */
#define aligned_make(align_, size_)   alloc((default_allocator_), (align_), (size_))

/** @brief The global default allocator (initially zero-initialised). */
extern allocator_t default_allocator_;

/**
 * @brief Set the global default allocator.
 * @param allocator The allocator to use as default.
 */
MY_ALLOCATOR_DEF void set_default_allocator(allocator_t allocator);
/**
 * @brief Get the current global default allocator.
 * @return The default allocator.
 */
MY_ALLOCATOR_DEF allocator_t get_default_allocator(void);

/**
 * @brief Construct an allocator_t from a data pointer and vtable.
 * @param data    Opaque state pointer.
 * @param vtable  Method table.
 * @return A new allocator_t instance.
 */
MY_ALLOCATOR_DEF allocator_t allocator_new(
	void *data,
	allocator_interface_t *vtable);
/**
 * @brief Low-level allocate through an allocator.
 * @param allocator The allocator.
 * @param alignment Required alignment.
 * @param size      Number of bytes.
 * @return Pointer to allocated memory, or NULL.
 */
MY_ALLOCATOR_DEF void *allocator_alloc(
	allocator_t allocator,
	size_t alignment,
	size_t size);
/**
 * @brief Allocate and copy a value into the new memory.
 * @param allocator The allocator.
 * @param alignment Required alignment.
 * @param size      Size of the value.
 * @param value     Pointer to the value to copy.
 * @return Pointer to the allocated copy, or NULL.
 */
MY_ALLOCATOR_DEF void *allocator_alloc_with_value(
	allocator_t allocator,
	size_t alignment,
	size_t size,
	void *value);
/**
 * @brief Reallocate through an allocator.
 * @param allocator The allocator.
 * @param old_size  Previous allocation size.
 * @param ptr       Previous pointer.
 * @param alignment Required alignment.
 * @param new_size  New size.
 * @return Pointer to the resized memory, or NULL.
 */
MY_ALLOCATOR_DEF void *allocator_realloc(
	allocator_t allocator,
	size_t old_size,
	void *ptr,
	size_t alignment,
	size_t new_size);
/**
 * @brief Free memory through an allocator.
 * @param allocator The allocator.
 * @param size      Size of the allocation.
 * @param ptr       Pointer to free.
 */
MY_ALLOCATOR_DEF void allocator_free(allocator_t allocator, size_t size, void *ptr);

/**
 * @brief Clone a block of memory through an allocator.
 * @param allocator The allocator.
 * @param ptr       Source memory.
 * @param len       Number of bytes to copy.
 * @return A new allocated copy.
 */
MY_ALLOCATOR_DEF void *clone_memory(allocator_t allocator, const void *ptr, const size_t len);
/**
 * @brief Clone a null-terminated string through an allocator.
 * @param allocator The allocator.
 * @param str       Source string.
 * @return A new allocated copy of the string.
 */
MY_ALLOCATOR_DEF char *clone_string(allocator_t allocator, const char *str);
/**
 * @brief Clone up to `len` characters of a string through an allocator.
 * @param allocator The allocator.
 * @param str       Source string.
 * @param len       Maximum number of characters to copy.
 * @return A new allocated copy (always null-terminated).
 */
MY_ALLOCATOR_DEF char *nclone_string(allocator_t allocator, const char *str, const size_t len);

#ifdef MY_ALLOCATOR_IMPL
allocator_t default_allocator_ = {0};

MY_ALLOCATOR_DEF void set_default_allocator(allocator_t allocator)
{
	default_allocator_ = allocator;
}

MY_ALLOCATOR_DEF allocator_t get_default_allocator(void)
{
	return default_allocator_;
}

MY_ALLOCATOR_DEF allocator_t allocator_new(
	void *data,
	allocator_interface_t *vtable)
{
	allocator_t result;
	result.data = data;
	result.vtable = vtable;
	return result;
}

MY_ALLOCATOR_DEF void *allocator_alloc_with_value(
	allocator_t allocator,
	size_t alignment,
	size_t size,
	void *value)
{
	void *result = allocator_alloc(allocator, alignment, size);
	if (result == NULL) return NULL;

	memcpy(result, value, size);

	return result;
}

MY_ALLOCATOR_DEF void *allocator_alloc(
	allocator_t allocator,
	size_t alignment,
	size_t size)
{
	assert(allocator.vtable);
	return allocator.vtable->allocate(allocator.data, alignment, size);
}

MY_ALLOCATOR_DEF void *allocator_realloc(
	allocator_t allocator,
	size_t old_size,
	void *ptr,
	size_t alignment,
	size_t new_size)
{
	assert(allocator.vtable);
	return allocator.vtable->reallocate(allocator.data, old_size, ptr, alignment, new_size);
}

MY_ALLOCATOR_DEF void allocator_free(
	allocator_t allocator,
	size_t size,
	void *ptr)
{
	assert(allocator.vtable);
	allocator.vtable->free(allocator.data, size, ptr);
}

MY_ALLOCATOR_DEF void *clone_memory(allocator_t allocator, const void *ptr, const size_t len)
{
	void *result = alloc(allocator, len);
	memcpy(result, ptr, len);
	return result;
}

MY_ALLOCATOR_DEF char *clone_string(allocator_t allocator, const char *str)
{
	const size_t len = strlen(str);
	return nclone_string(allocator, str, len);
}

MY_ALLOCATOR_DEF char *nclone_string(allocator_t allocator, const char *str, const size_t len)
{
	char *result = alloc(allocator, len + 1);
	memcpy(result, str, len);
	result[len] = '\0';
	return result;
}
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !MY_ALLOCATOR_H_ */
