/**
 * @file my_string_view.h
 * @brief Non-owning immutable string view (slice over const char *).
 *
 * `string_view_t` is a view over a character buffer — it does not own or
 * null-terminate the data.
 *
 * Define `MY_STRING_VIEW_IMPL` in exactly one translation unit to generate
 * the implementation.
 *
 * Define `MY_STRING_VIEW_DEF` to `static` before inclusion to give all
 * functions internal linkage (STB-style single-TU usage).
 */

#ifndef MY_STRING_VIEW_H_
#define MY_STRING_VIEW_H_

#ifdef __cplusplus
extern "C" {
#endif

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

#ifndef MY_STRING_VIEW_DEF
#  define MY_STRING_VIEW_DEF
#endif

/**
 * @brief A non-owning view over an immutable character buffer.
 */
typedef struct string_view_t {
	const char *data;
	size_t      len;
} string_view_t;

/** @def sv_starts_with(self, other)
 *  @brief Check if `self` starts with `other`.
 *  @param self   A `string_view_t`.
 *  @param other  A `char`, `const char *`, or `string_view_t`.
 *  @return `true` if `self` starts with `other`. */
#define sv_starts_with(self, other) \
	_Generic((other), \
			char: sv_starts_with_char, \
			int: sv_starts_with_char, \
			const char *: sv_starts_with_cstr, \
			char *: sv_starts_with_cstr, \
			string_view_t: sv_starts_with_view)(self, other)

/** @def sv_ends_with(self, other)
 *  @brief Check if `self` ends with `other`.
 *  @param self   A `string_view_t`.
 *  @param other  A `char`, `const char *`, or `string_view_t`.
 *  @return `true` if `self` ends with `other`. */
#define sv_ends_with(self, other) \
	_Generic((other), \
			char: sv_ends_with_char, \
			int: sv_ends_with_char, \
			const char *: sv_ends_with_cstr, \
			char *: sv_ends_with_cstr, \
			string_view_t: sv_ends_with_view)(self, other)

/** @def sv_eq(self, other)
 *  @brief Check equality between `self` and `other`.
 *  @param self   A `string_view_t`.
 *  @param other  A `const char *` or `string_view_t`.
 *  @return `true` if both hold the same content. */
#define sv_eq(self, other) \
	_Generic((other), \
			const char *: sv_eq_cstr, \
			char *: sv_eq_cstr, \
			string_view_t: sv_eq_view)(self, other)

/** @brief Create a view from a null-terminated C string.
 *  @param chs  The source C string (not copied).
 *  @return A `string_view_t` pointing into `chs`. */
MY_STRING_VIEW_DEF string_view_t sv_from_chars(const char *chs);

/** @brief Create a view from a data pointer and length.
 *  @param data  Pointer to the character data.
 *  @param len   Number of bytes in the view.
 *  @return A `string_view_t` covering `[data, data + len)`. */
MY_STRING_VIEW_DEF string_view_t sv_from_parts(const char *data, size_t len);

/** @brief Compare with a null-terminated C string.
 *  @param self   A `string_view_t`.
 *  @param other  Null-terminated C string.
 *  @return `true` if contents are identical. */
MY_STRING_VIEW_DEF bool sv_eq_cstr(string_view_t self, const char *other);

/** @brief Compare two views.
 *  @param self   A `string_view_t`.
 *  @param other  Another `string_view_t`.
 *  @return `true` if contents are identical. */
MY_STRING_VIEW_DEF bool sv_eq_view(string_view_t self, string_view_t other);

/** @brief Check if `self` starts with a character.
 *  @param self  A `string_view_t`.
 *  @param c     The character.
 *  @return `true` if the first character matches `c`. */
MY_STRING_VIEW_DEF bool sv_starts_with_char(string_view_t self, char c);

/** @brief Check if `self` starts with a C string prefix.
 *  @param self    A `string_view_t`.
 *  @param prefix  The prefix.
 *  @return `true` if `self` begins with `prefix`. */
MY_STRING_VIEW_DEF bool sv_starts_with_cstr(string_view_t self, const char *prefix);

/** @brief Check if `self` starts with another view.
 *  @param self    A `string_view_t`.
 *  @param prefix  The prefix view.
 *  @return `true` if `self` begins with `prefix`. */
MY_STRING_VIEW_DEF bool sv_starts_with_view(string_view_t self, string_view_t prefix);

/** @brief Check if `self` ends with a character.
 *  @param self  A `string_view_t`.
 *  @param c     The character.
 *  @return `true` if the last character matches `c`. */
MY_STRING_VIEW_DEF bool sv_ends_with_char(string_view_t self, char c);

/** @brief Check if `self` ends with a C string suffix.
 *  @param self    A `string_view_t`.
 *  @param suffix  The suffix.
 *  @return `true` if `self` ends with `suffix`. */
MY_STRING_VIEW_DEF bool sv_ends_with_cstr(string_view_t self, const char *suffix);

/** @brief Check if `self` ends with another view.
 *  @param self    A `string_view_t`.
 *  @param suffix  The suffix view.
 *  @return `true` if `self` ends with `suffix`. */
MY_STRING_VIEW_DEF bool sv_ends_with_view(string_view_t self, string_view_t suffix);

/** @brief Extract a substring view.
 *  @param self   A `string_view_t`.
 *  @param start  Start index (inclusive).
 *  @param end    End index (exclusive).
 *  @return A view over `[start, end)`.  Clamped to bounds. */
MY_STRING_VIEW_DEF string_view_t sv_substr(string_view_t self, size_t start, size_t end);

/** @brief Trim leading and trailing whitespace.
 *  @param self  A `string_view_t`.
 *  @return A new view with whitespace removed from both ends. */
MY_STRING_VIEW_DEF string_view_t sv_trim(string_view_t self);

/** @brief Trim leading whitespace.
 *  @param self  A `string_view_t`.
 *  @return A new view with leading whitespace removed. */
MY_STRING_VIEW_DEF string_view_t sv_ltrim(string_view_t self);

/** @brief Trim trailing whitespace.
 *  @param self  A `string_view_t`.
 *  @return A new view with trailing whitespace removed. */
MY_STRING_VIEW_DEF string_view_t sv_rtrim(string_view_t self);

#ifdef MY_STRING_VIEW_IMPL

MY_STRING_VIEW_DEF string_view_t sv_from_chars(const char *chs) {
	size_t len = strlen(chs);
	{ string_view_t sv_; sv_.data = chs; sv_.len = len; return sv_; }
}

MY_STRING_VIEW_DEF string_view_t sv_from_parts(const char *data, size_t len) {
	{ string_view_t sv_; sv_.data = data; sv_.len = len; return sv_; }
}

MY_STRING_VIEW_DEF bool sv_eq_cstr(string_view_t self, const char *other) {
	size_t len = strlen(other);
	if (self.len != len)
		return false;
	return strncmp(self.data, other, len) == 0;
}

MY_STRING_VIEW_DEF bool sv_eq_view(string_view_t self, string_view_t other) {
	if (self.len != other.len)
		return false;
	return strncmp(self.data, other.data, self.len) == 0;
}

MY_STRING_VIEW_DEF bool sv_starts_with_char(string_view_t self, char c) {
	if (self.len < 1)
		return false;
	return self.data[0] == c;
}

MY_STRING_VIEW_DEF bool sv_starts_with_cstr(string_view_t self, const char *prefix) {
	size_t len = strlen(prefix);
	if (self.len < len)
		return false;
	return strncmp(self.data, prefix, len) == 0;
}

MY_STRING_VIEW_DEF bool sv_starts_with_view(string_view_t self, string_view_t prefix) {
	if (self.len < prefix.len)
		return false;
	return strncmp(self.data, prefix.data, prefix.len) == 0;
}

MY_STRING_VIEW_DEF bool sv_ends_with_char(string_view_t self, char c) {
	if (self.len < 1)
		return false;
	return self.data[self.len - 1] == c;
}

MY_STRING_VIEW_DEF bool sv_ends_with_cstr(string_view_t self, const char *suffix) {
	size_t len = strlen(suffix);
	if (self.len < len)
		return false;
	return strncmp(self.data + self.len - len, suffix, len) == 0;
}

MY_STRING_VIEW_DEF bool sv_ends_with_view(string_view_t self, string_view_t suffix) {
	if (self.len < suffix.len)
		return false;
	return strncmp(self.data + self.len - suffix.len, suffix.data, suffix.len) == 0;
}

MY_STRING_VIEW_DEF string_view_t sv_substr(string_view_t self, size_t start, size_t end) {
	if (start > self.len) start = self.len;
	if (end > self.len) end = self.len;
	if (start >= end)
		{ string_view_t sv_; sv_.data = NULL; sv_.len = 0; return sv_; }
	{ string_view_t sv_; sv_.data = self.data + start; sv_.len = end - start; return sv_; }
}

MY_STRING_VIEW_DEF string_view_t sv_ltrim(string_view_t self) {
	size_t start = 0;
	while (start < self.len &&
	       (self.data[start] == ' ' || self.data[start] == '\t' ||
	        self.data[start] == '\n' || self.data[start] == '\r')) {
		start++;
	}
	{ string_view_t sv_; sv_.data = self.data + start; sv_.len = self.len - start; return sv_; }
}

MY_STRING_VIEW_DEF string_view_t sv_rtrim(string_view_t self) {
	size_t len = self.len;
	while (len > 0 &&
	       (self.data[len - 1] == ' ' || self.data[len - 1] == '\t' ||
	        self.data[len - 1] == '\n' || self.data[len - 1] == '\r')) {
		len--;
	}
	{ string_view_t sv_; sv_.data = self.data; sv_.len = len; return sv_; }
}

MY_STRING_VIEW_DEF string_view_t sv_trim(string_view_t self) {
	self = sv_rtrim(self);
	self = sv_ltrim(self);
	return self;
}

#endif /* MY_STRING_VIEW_IMPL */

#ifdef __cplusplus
}
#endif

#endif /* MY_STRING_VIEW_H_ */
