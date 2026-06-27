/**
 * @file utf8.h
 * @brief UTF-8 decoding, validation, and iteration.
 *
 * Provides a forward iterator over Unicode codepoints in a UTF-8
 * encoded C string, plus low-level decode and validation helpers.
 *
 * Define `UTF8_IMPLEMNTATION` in exactly one translation unit
 * to generate the implementation.
 *
 * Define `UTF8_DEF` to `static` before inclusion to give all functions
 * internal linkage (STB-style single-TU usage).
 */

#ifndef __UTF8_ITER_H
#define __UTF8_ITER_H

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
#include <stdint.h>

#ifndef UTF8_DEF
#  define UTF8_DEF
#endif /* !UTF8_DEF */

/**
 * @brief Forward iterator over UTF-8 codepoints.
 */
typedef struct utf8_iter_t {
	const unsigned char *current;
	uint32_t curr_codepoint;
	uint32_t prev_codepoint;
} utf8_iter_t;

/**
 * @brief Initialise a UTF-8 iterator.
 * @param iter  Iterator to initialise (out parameter).
 * @param str   Null-terminated UTF-8 string.
 * @return true on success, false if the string is not valid UTF-8.
 */
UTF8_DEF bool utf8_iter_init(utf8_iter_t *iter, const char *str);
/**
 * @brief Advance to the next codepoint.
 * @param iter  Iterator.
 * @return The number of bytes consumed, or -1 on error.
 */
UTF8_DEF int utf8_next(utf8_iter_t *iter);
/**
 * @brief Get the previous codepoint (before the last `utf8_next` call).
 * @param iter  Iterator (passed by value).
 * @return Unicode codepoint value.
 */
UTF8_DEF uint32_t utf8_prev(utf8_iter_t iter);
/**
 * @brief Peek at the current codepoint without advancing.
 * @param iter  Iterator (passed by value).
 * @return Current Unicode codepoint.
 */
UTF8_DEF uint32_t utf8_peek(utf8_iter_t iter);

/**
 * @brief Decode a single UTF-8 character from a byte sequence.
 * @param str  Pointer to the start of a UTF-8 character.
 * @param out  Receives the decoded codepoint (out parameter).
 * @return Number of bytes consumed (1-4), or -1 on invalid input.
 */
UTF8_DEF int decode_utf8(const unsigned char *str, uint32_t *out);
/**
 * @brief Check whether a C string is valid UTF-8.
 * @param str  The string to validate.
 * @return true if the entire string is valid UTF-8.
 */
UTF8_DEF bool is_valid_utf8_cstr(const unsigned char *str);
/**
 * @brief Determine the byte length of a UTF-8 character from its first byte.
 * @param first_byte  The leading byte.
 * @return 1, 2, 3, 4 for valid leading bytes, or 0 for invalid.
 */
UTF8_DEF size_t get_utf8_char_length(const unsigned char first_byte);

#ifdef UTF8_IMPLEMNTATION
UTF8_DEF bool utf8_iter_init(utf8_iter_t *iter, const char *str) {
	if (!is_valid_utf8_cstr((const unsigned char *)str)) {
		return false;
	}
	iter->curr_codepoint = 0;
	iter->prev_codepoint = 0;
	iter->current = (const unsigned char *)str;
	return utf8_next(iter) != -1;
}

UTF8_DEF int utf8_next(utf8_iter_t *iter) {
	size_t i;
	unsigned char first_byte;
	size_t expected_len;

	iter->prev_codepoint = iter->curr_codepoint;
	assert(iter->current != NULL);
	if (iter->current[0] == '\0') {
		iter->current += 1;
		iter->curr_codepoint = 0;
		return 1;
	}

	first_byte = iter->current[0];
	expected_len = get_utf8_char_length(first_byte);
	if (expected_len > 0) {
		for (i = 0; i < expected_len; i++) {
			if (iter->current[i] == '\0') {
				return -1;
			}
		}
	}

	int len = decode_utf8(iter->current, &iter->curr_codepoint);
	if (len == -1) return len;
	iter->current += len;
	return len;
}

UTF8_DEF uint32_t utf8_prev(utf8_iter_t iter) {
	return iter.prev_codepoint;
}

UTF8_DEF uint32_t utf8_peek(utf8_iter_t iter) {
	return iter.curr_codepoint;
}

UTF8_DEF size_t get_utf8_char_length(const unsigned char first_byte) {
	if (first_byte <= 0x7F) return 1;
	if ((first_byte & 0xE0) == 0xC0) return 2;
	if ((first_byte & 0xF0) == 0xE0) return 3;
	if ((first_byte & 0xF8) == 0xF0) return 4;
	return 0;
}

UTF8_DEF int decode_utf8(const unsigned char *str, uint32_t *out) {
	if (str[0] < 0x80) {
		*out = str[0];
		return 1;
	} else if ((str[0] & 0xE0) == 0xC0) {
		if ((str[1] & 0xC0) != 0x80)
			return -1;
		*out = ((str[0] & 0x1F) << 6) | (str[1] & 0x3F);
		if (*out < 0x80) return -1;
		return 2;
	} else if ((str[0] & 0xF0) == 0xE0) {
		if ((str[1] & 0xC0) != 0x80 || (str[2] & 0xC0) != 0x80)
			return -1;
		*out = ((str[0] & 0x0F) << 12) | ((str[1] & 0x3F) << 6) | (str[2] & 0x3F);
		if (*out < 0x800) return -1;
		if (*out >= 0xD800 && *out <= 0xDFFF) return -1;
		return 3;
	} else if ((str[0] & 0xF8) == 0xF0) {
		if ((str[1] & 0xC0) != 0x80 || (str[2] & 0xC0) != 0x80 || (str[3] & 0xC0) != 0x80)
			return -1;
		*out = ((str[0] & 0x07) << 18) | ((str[1] & 0x3F) << 12) | ((str[2] & 0x3F) << 6) |
			  (str[3] & 0x3F);
		if (*out < 0x10000) return -1;
		if (*out > 0x10FFFF) return -1;
		return 4;
	}
	return -1;
}

UTF8_DEF bool is_valid_utf8_cstr(const unsigned char *str) {
	const unsigned char *ptr = str;

	if (str == NULL)
		return false;

	while (*ptr != '\0') {
		size_t char_len;
		int len;
		uint32_t codepoint;

		char_len = get_utf8_char_length(*ptr);
		if (char_len > 0) {
			size_t j;
			for (j = 1; j < char_len; j++) {
				if (ptr[j] == '\0') return false;
			}
		}

		len = 0;
		codepoint = 0;
		if ((len = decode_utf8(ptr, &codepoint)) == -1) return false;
		ptr += len;
	}

	return true;
}
#endif /* UTF8_IMPLEMNTATION */

#endif /* !__UTF8_ITER_H */
