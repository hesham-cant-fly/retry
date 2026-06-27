/**
 * @file my_string_builder.h
 * @brief Dynamic string builder (heap-allocated, null-terminated).
 *
 * Define `MY_STRING_BUILDER_IMPL` in exactly one translation unit to generate
 * the implementation.
 *
 * Define `MY_STRING_BUILDER_DEF` to `static` before inclusion to give all
 * functions internal linkage (STB-style single-TU usage).
 *
 * Requires `my_allocator.h` and `my_stream.h` in the include path.
 * `setup_io_stream()` must be called before using `string_builder_format`
 * or `string_builder_pushf`.
 *
 * Configure `GROW_FACTOR` before inclusion to override the default (2).
 */

#ifndef __MY_STRING_BUILDER_H
#define __MY_STRING_BUILDER_H

#include "my_string_view.h"
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "my_allocator.h"
#include "my_stream.h"
#include "my_string.h"

#ifndef MY_STRING_BUILDER_DEF
#  define MY_STRING_BUILDER_DEF
#endif /* !MY_STRING_BUILDER_DEF */

#ifndef GROW_FACTOR
/** @def GROW_FACTOR
 *  @brief Capacity multiplier when growing (overridable before inclusion). */
#  define GROW_FACTOR 2
#endif

/**
 * @brief A dynamic, heap-allocated, null-terminated string builder.
 *
 * Allocates and owns its buffer via the provided `allocator_t`.
 */
typedef struct string_builder_t {
	allocator_t allocator; /**< Allocator used for all buffer management. */
	char   *data;          /**< Null-terminated character buffer. */
	size_t  len;           /**< Current string length (excluding null terminator). */
	size_t  cap;           /**< Allocated capacity (excluding null terminator). */
} string_builder_t;

/** @def string_builder_push(self, target)
 *  @brief Append a value to the string builder (type-dispatched).
 *  @param self    Pointer to a `string_builder_t`.
 *  @param target  A `char`, `char *`, or `string_builder_t *` to append. */
#define string_builder_push(self, target) \
	_Generic((target), \
			char: string_builder_push_char, \
			char *: string_builder_push_cstr, \
			string_builder_t *: string_builder_push_string)(self, target)

/** @def string_builder(allocator_, x)
 *  @brief Convenience shorthand for `string_builder_from_chars_copy`. */
#define string_builder(allocator_, x) string_builder_from_chars_copy((allocator_), (x))

/** @brief Create a new empty string builder with the given initial capacity.
 *  @param allocator  The allocator to use.
 *  @param cap        Initial capacity (excluding null terminator).
 *  @return A new `string_builder_t` owning a zero-initialised buffer. */
MY_STRING_BUILDER_DEF string_builder_t string_builder_new(allocator_t allocator, size_t cap);

/** @brief Create a string builder by copying a C string.
 *  @param allocator  The allocator to use.
 *  @param chs        Null-terminated source string (copied).
 *  @return A new `string_builder_t` owning a copy of `chs`. */
MY_STRING_BUILDER_DEF string_builder_t string_builder_from_chars_copy(allocator_t allocator, const char *  chs);

/** @brief Shrink the allocated buffer and return `string_t`
 * @param self Pointer to the string builder
 * @returns a `string_t` of the built string */
MY_STRING_BUILDER_DEF string_t string_builder_build(string_builder_t *self);

/** @brief Shrink the allocated buffer and return `string_view_t`
 * @param self Pointer to the string builder
 * @returns a `string_t` of the built string */
MY_STRING_BUILDER_DEF string_view_t string_builder_build_view(string_builder_t *self);

/** @brief Create a string builder from a printf-style format string.
 *  @param allocator  The allocator to use.
 *  @param fmt        Printf-style format string.
 *  @param ...        Format arguments.
 *  @return A new `string_builder_t` containing the formatted output.
 *  @note `setup_io_stream()` must be called before using this function. */
MY_STRING_BUILDER_DEF string_builder_t string_builder_format(allocator_t allocator, const char *  fmt, ...);

/** @brief Free the string builder's buffer and reset its fields.
 *  @param self  Pointer to the string builder. */
MY_STRING_BUILDER_DEF void string_builder_delete(string_builder_t *  self);

/** @brief Manually set the length of the string.
 *  @param self  Pointer to the string builder.
 *  @param len   New length (must be <= `cap`).
 *  @return The new length. */
MY_STRING_BUILDER_DEF size_t string_builder_set_len(string_builder_t *self, size_t len);

/** @brief Ensure the buffer can hold at least `new_cap` characters (plus null terminator).
 *  @param self     Pointer to the string builder.
 *  @param new_cap  Desired capacity. */
MY_STRING_BUILDER_DEF void string_builder_reserve(string_builder_t *  self, size_t new_cap);

/** @brief Resize the string to `new_size`, zero-filling any new bytes.
 *  @param self      Pointer to the string builder.
 *  @param new_size  New size (may be larger or smaller than current length). */
MY_STRING_BUILDER_DEF void string_builder_resize(string_builder_t *  self, size_t new_size);

/** @brief Append formatted text to the string builder.
 *  @param self  Pointer to the string builder.
 *  @param fmt   Printf-style format string.
 *  @param ...   Format arguments.
 *  @note `setup_io_stream()` must be called before using this function. */
MY_STRING_BUILDER_DEF void string_builder_pushf(string_builder_t *  self, const char *  fmt, ...);

/** @brief Append a single character.
 *  @param self  Pointer to the string builder.
 *  @param ch    The character to append. */
MY_STRING_BUILDER_DEF void string_builder_push_char(string_builder_t *  self, char ch);

/** @brief Append a null-terminated C string.
 *  @param self  Pointer to the string builder.
 *  @param cstr  The C string to append (copied). */
MY_STRING_BUILDER_DEF void string_builder_push_cstr(string_builder_t *  self, const char *  cstr);

/** @brief Append the contents of another string builder.
 *  @param self   Pointer to the destination.
 *  @param other  Pointer to the source string builder. */
MY_STRING_BUILDER_DEF void string_builder_push_string(string_builder_t *  self, const string_builder_t *  other);

#ifdef MY_STRING_BUILDER_IMPL

/** @brief Return the capacity including the null terminator slot.
 *  @param self  Pointer to the string builder.
 *  @return `self->cap + 1`. */
MY_STRING_BUILDER_DEF size_t string_builder_actual_cap(const string_builder_t *  self) {
	return self->cap + 1;
}

/** @brief Return the length including the null terminator.
 *  @param self  Pointer to the string builder.
 *  @return `self->len + 1`. */
MY_STRING_BUILDER_DEF size_t string_builder_actual_len(const string_builder_t *  self) {
	return self->len + 1;
}

MY_STRING_BUILDER_DEF string_builder_t string_builder_from_chars_copy(allocator_t allocator, const char *  chs) {
	size_t len = strlen(chs);
	char *buffer = alloc(allocator, len + 1);
	memcpy(buffer, chs, len + 1);
	{ string_builder_t res_; res_.allocator = allocator; res_.cap = len; res_.len = len; res_.data = buffer; return res_; }
}

MY_STRING_BUILDER_DEF string_builder_t string_builder_new(allocator_t allocator, size_t cap) {
	{ string_builder_t res_; res_.allocator = allocator; res_.cap = cap; res_.len = 0; res_.data = alloc(allocator, sizeof(char) * cap + 1); res_.data[0] = '\0'; return res_; }
}

MY_STRING_BUILDER_DEF size_t string_builder_set_len(string_builder_t *self, size_t len) {
	self->len = len;
	self->data[len] = '\0';
	return self->len;
}

MY_STRING_BUILDER_DEF string_t string_builder_build(string_builder_t *self) {
	self->data = xrecreate(self->allocator, self->cap + 1, self->len + 1, self->data);
	self->cap = self->len;
	{ string_t s_; s_.data = self->data; s_.len = self->len; return s_; }
}

MY_STRING_BUILDER_DEF string_view_t string_builder_build_view(string_builder_t *self) {
	self->data = xrecreate(self->allocator, self->cap + 1, self->len + 1, self->data);
	self->cap = self->len;
	{ string_view_t s_; s_.data = self->data; s_.len = self->len; return s_; }
}

MY_STRING_BUILDER_DEF string_builder_t string_builder_format(allocator_t allocator, const char *  fmt, ...) {
	va_list args;

	va_start(args, fmt);
	{ int len = vsnsprint(NULL, 0, fmt, args);
	va_end(args);

	{ string_builder_t result = string_builder_new(allocator, len);

	va_start(args, fmt);
	vsnsprint(result.data, len + 1, fmt, args);
	va_end(args);

	result.len = len;
	return result; } }
}

MY_STRING_BUILDER_DEF void string_builder_reserve(string_builder_t *  self, size_t new_cap) {
	self->data = xrecreate(self->allocator, self->cap + 1, new_cap + 1, self->data);
	self->cap = new_cap;
}

MY_STRING_BUILDER_DEF void string_builder_resize(string_builder_t *  self, size_t new_size) {
	if (self->cap < new_size) {
		self->data = xrecreate(self->allocator, self->cap + 1, new_size + 1, self->data);
	}

	if (self->len < new_size) {
		memset(self->data + self->len, 0, new_size - self->len);
	}

	self->len = new_size;
	self->data[self->len] = '\0';
}

MY_STRING_BUILDER_DEF void string_builder_delete(string_builder_t *  self) {
	xdestroy(self->allocator, self->cap + 1, self->data);
	self->len = 0;
	self->cap = 0;
	self->data = NULL;
}

MY_STRING_BUILDER_DEF void string_builder_push_char(string_builder_t *  self, char ch) {
	if (self->len + 1 >= self->cap) {
		string_builder_reserve(self, self->cap * GROW_FACTOR);
	}

	self->data[self->len++] = ch;
	self->data[self->len] = '\0';
}

MY_STRING_BUILDER_DEF void string_builder_push_cstr(string_builder_t *  self, const char *  cstr) {
	size_t len = strlen(cstr);
	if (self->len + len >= self->cap) {
		size_t newcap = self->cap * GROW_FACTOR;
		if (self->len + len > newcap)
			newcap = self->len + len;
		string_builder_reserve(self, newcap);
	}

	memcpy(self->data + self->len, cstr, len);
	self->len += len;
	self->data[self->len] = '\0';
}

MY_STRING_BUILDER_DEF void string_builder_push_string(string_builder_t *  self, const string_builder_t *  other) {
	if (self->len + other->len >= self->cap) {
		size_t newcap = self->cap * GROW_FACTOR;
		if (self->len + other->len > newcap)
			newcap = self->len + other->len;
		string_builder_reserve(self, newcap);
	}

	memcpy(self->data + self->len, other->data, other->len);
	self->len += other->len;
	self->data[self->len] = '\0';
}

MY_STRING_BUILDER_DEF void string_builder_pushf(string_builder_t *  self, const char *  fmt, ...) {
	va_list args;

	int len;

	va_start(args, fmt);
	len = vsnsprint(NULL, 0, fmt, args);
	va_end(args);

	if (self->len + len > self->cap) {
		size_t new_cap = self->cap * GROW_FACTOR;
		if (new_cap < self->len + len) new_cap = self->len + len;
		string_builder_reserve(self, new_cap);
	}

	va_start(args, fmt);
	vsnsprint(self->data + self->len, len + 1, fmt, args);
	va_end(args);

	self->len += len;
}
#endif /* MY_STRING_BUILDER_IMPL */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MY_STRING_BUILDER_H */
