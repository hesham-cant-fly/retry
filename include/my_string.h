/**
 * @file my_string.h
 * @brief Non-owning string view with comparison, prefix/suffix, and trimming.
 *
 * `string_t` is a view over a character buffer — it does not own or
 * null-terminate the data.  Created via `string_from_chars()`.
 *
 * Define `MY_STRING_IMPL` in exactly one translation unit to generate
 * the implementation.
 *
 * Define `MY_STRING_DEF` to `static` before inclusion to give all functions
 * internal linkage (STB-style single-TU usage).
 */

#ifndef __MY_STRING_H
#define __MY_STRING_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

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
#include <stddef.h>
#include <string.h>

#ifndef MY_STRING_DEF
#  define MY_STRING_DEF
#endif /* !MY_STRING_DEF */

/**
 * @brief A non-owning view over a character buffer.
 */
typedef struct string_t {
	const char *data; /**< Pointer to the character data (not null-terminated). */
	size_t len;       /**< Length of the view in bytes. */
} string_t;

/** @def string_ends_with(self, other)
 *  @brief Check if `self` ends with `other`.
 *  @param self   Pointer to a `string_t`.
 *  @param other  A `char`, `const char *`, `char *`, or `string_t *`.
 *  @return `true` if `self` ends with `other`. */
#define string_ends_with(self, other) \
	_Generic((other), \
			char: string_ends_with_char, \
			int: string_ends_with_char, \
			const char *: string_ends_with_cstr, \
			char *: string_ends_with_cstr, \
			string_t *: string_ends_with_string)(self, other)

/** @def string_starts_with(self, other)
 *  @brief Check if `self` starts with `other`.
 *  @param self   Pointer to a `string_t`.
 *  @param other  A `char`, `const char *`, `char *`, or `string_t *`.
 *  @return `true` if `self` starts with `other`. */
#define string_starts_with(self, other) \
	_Generic((other), \
			char: string_starts_with_char, \
			int: string_starts_with_char, \
			const char *: string_starts_with_cstr, \
			char *: string_starts_with_cstr, \
			string_t *: string_starts_with_string)(self, other)

/** @def string_eq(self, target)
 *  @brief Check equality between `self` and `target`.
 *  @param self    Pointer to a `string_t`.
 *  @param target  A `const char *`, `char *`, or `string_t *`.
 *  @return `true` if both strings hold the same content. */
#define string_eq(self, target) \
	_Generic((target), \
			const char *: string_eq_cstr, \
			char *: string_eq_cstr, \
			string_t *: string_eq_string)(self, target)

/** @brief Create a `string_t` view from a null-terminated C string.
 *  @param chs  The source C string (not copied).
 *  @return A `string_t` pointing into `chs` with `len == strlen(chs)`. */
MY_STRING_DEF string_t string_from_chars(const char *chs);

/** @brief Compare a `string_t` with a C string.
 *  @param self   Pointer to a `string_t`.
 *  @param other  Null-terminated C string.
 *  @return `true` if contents are identical. */
MY_STRING_DEF bool string_eq_cstr(const string_t *self, const char *other);

/** @brief Compare two `string_t` values.
 *  @param self   Pointer to the first string.
 *  @param other  Pointer to the second string.
 *  @return `true` if contents are identical. */
MY_STRING_DEF bool string_eq_string(const string_t *self, const string_t *other);

/** @brief Check if `self` starts with a single character.
 *  @param self   Pointer to a `string_t`.
 *  @param other  The character to test.
 *  @return `true` if the first character matches `other`. */
MY_STRING_DEF bool string_starts_with_char(const string_t *self, char other);

/** @brief Check if `self` starts with a C string prefix.
 *  @param self   Pointer to a `string_t`.
 *  @param other  The prefix to test.
 *  @return `true` if `self` begins with `other`. */
MY_STRING_DEF bool string_starts_with_cstr(const string_t *self, const char *other);

/** @brief Check if `self` starts with another `string_t` prefix.
 *  @param self   Pointer to a `string_t`.
 *  @param other  The prefix to test.
 *  @return `true` if `self` begins with `other`. */
MY_STRING_DEF bool string_starts_with_string(const string_t *self, const string_t *other);

/** @brief Check if `self` ends with a single character.
 *  @param self   Pointer to a `string_t`.
 *  @param other  The character to test.
 *  @return `true` if the last character matches `other`. */
MY_STRING_DEF bool string_ends_with_char(const string_t *self, char other);

/** @brief Check if `self` ends with a C string suffix.
 *  @param self   Pointer to a `string_t`.
 *  @param other  The suffix to test.
 *  @return `true` if `self` ends with `other`. */
MY_STRING_DEF bool string_ends_with_cstr(const string_t *self, const char *other);

/** @brief Check if `self` ends with another `string_t` suffix.
 *  @param self   Pointer to a `string_t`.
 *  @param other  The suffix to test.
 *  @return `true` if `self` ends with `other`. */
MY_STRING_DEF bool string_ends_with_string(const string_t *self, const string_t *other);

/** @brief Trim leading and trailing whitespace (space, tab, newline, carriage return) in place.
 *  @param self  Pointer to the string to trim. */
MY_STRING_DEF void string_trim(string_t *self);

/** @brief Trim leading whitespace in place.
 *  @param self  Pointer to the string to trim. */
MY_STRING_DEF void string_ltrim(string_t *self);

/** @brief Trim trailing whitespace in place.
 *  @param self  Pointer to the string to trim. */
MY_STRING_DEF void string_rtrim(string_t *self);

#ifdef MY_STRING_IMPL

MY_STRING_DEF string_t string_from_chars(const char *chs) {
  size_t len = strlen(chs);
  { string_t s_; s_.data = chs; s_.len = len; return s_; }
}

MY_STRING_DEF bool string_eq_cstr(const string_t *self, const char *other) {
  size_t len = strlen(other);
  if (self->len != len)
    return false;
  return strncmp(self->data, other, len) == 0;
}

MY_STRING_DEF bool string_eq_string(const string_t *self, const string_t *other) {
  if (self->len != other->len)
    return false;
  return strncmp(self->data, other->data, self->len) == 0;
}

MY_STRING_DEF bool string_starts_with_char(const string_t *self, char other) {
  if (self->len < 1)
    return false;
  return self->data[0] == other;
}

MY_STRING_DEF bool string_starts_with_cstr(const string_t *self, const char *other) {
  size_t len = strlen(other);
  if (self->len < len)
    return false;
  return strncmp(self->data, other, len) == 0;
}

MY_STRING_DEF bool string_starts_with_string(const string_t *self,
                                              const string_t *other) {
  if (self->len < other->len)
    return false;
  return strncmp(self->data, other->data, other->len) == 0;
}

MY_STRING_DEF bool string_ends_with_char(const string_t *self, char other) {
  if (self->len < 1)
    return false;
  return self->data[self->len - 1] == other;
}

MY_STRING_DEF bool string_ends_with_cstr(const string_t *self,
                                          const char *other) {
  size_t len = strlen(other);
  if (self->len < len)
    return false;
  return strncmp(self->data + self->len - len, other, len) == 0;
}

MY_STRING_DEF bool string_ends_with_string(const string_t *self,
                                           const string_t *other) {
  if (self->len < other->len)
    return false;
  return strncmp(self->data + self->len - other->len, other->data,
                 other->len) == 0;
}

MY_STRING_DEF void string_ltrim(string_t *self) {
  size_t start = 0;
  while (start < self->len &&
         (self->data[start] == ' ' || self->data[start] == '\t' ||
          self->data[start] == '\n' || self->data[start] == '\r')) {
    start++;
  }
  if (start > 0) {
    self->data += start;
    self->len -= start;
  }
}

MY_STRING_DEF void string_rtrim(string_t *self) {
  while (self->len > 0 &&
         (self->data[self->len - 1] == ' ' || self->data[self->len - 1] == '\t' ||
          self->data[self->len - 1] == '\n' || self->data[self->len - 1] == '\r')) {
    self->len--;
  }
}

MY_STRING_DEF void string_trim(string_t *self) {
  string_rtrim(self);
  string_ltrim(self);
}

#endif /* MY_STRING_IMPL */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !__MY_STRING_H */
