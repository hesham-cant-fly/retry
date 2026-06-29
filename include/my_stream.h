/**
 * @file my_stream.h
 * @brief Stream I/O with custom format specifiers.
 *
 * Provides an abstract stream interface (`stream_t`) backed by files
 * or memory buffers, and a formatting engine that uses `{specifier}`
 * and `{specifier:modifier}` syntax (inspired by Rust / Python).
 *
 * Define `MY_STREAM_IMPL` in exactly one translation unit to generate
 * the implementation.
 *
 * Define `MY_STREAM_DEF` to `static` before inclusion to give all public
 * functions internal linkage (STB-style single-TU usage).
 */

#ifndef MY_STREAM_H_
#define MY_STREAM_H_

/* #define MY_STREAM_IMPL */

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#ifndef MY_STREAM_DEF
#  define MY_STREAM_DEF
#endif /* !MY_STREAM_DEF */

/**
 * @brief Virtual method table for a stream.
 */
typedef struct stream_interface_t {
	int (*close)(void *data);
	int (*read)(void *data, unsigned char *out_buf, size_t amount);
	int (*write)(void *data, const unsigned char *in, size_t count);
	int (*seek)(void *data, long int offset, int whence);
	int (*flush)(void *data);
} stream_interface_t;

/**
 * @brief An abstract stream (vtable + opaque data pointer).
 */
typedef struct stream_t {
	void *data;
	const stream_interface_t *vtable;
} stream_t;

/**
 * @brief A memory-backed stream.
 */
typedef struct mem_stream_t {
	char *buffer;
	size_t len;
	size_t pos;
} mem_stream_t;

/**
 * @brief State for parsing format modifiers.
 */
typedef struct modifier_stream_t {
	const char *current;
	size_t len;
} modifier_stream_t;

/**
 * @brief Parsed format modifier info.
 *  Syntax: `{[flags][length/base].[precision]w[width]}`
 */
typedef struct standard_format_info_t {
	int has_len;
	int has_base;
	int has_preci;
	int has_width;

	int len;
	int base;
	int preci;
	int width;

	int alternate_form;
	int zero_padded;
} standard_format_info_t;

/**
 * @brief Parse a modifier string into a `standard_format_info_t`.
 * @param mod  The modifier stream to parse.
 * @param args Varargs for `*`-width specifiers.
 * @return Parsed info struct.
 */
MY_STREAM_DEF standard_format_info_t parse_format_info(modifier_stream_t *mod, va_list args);

/** @brief Signature of a custom format-specifier callback. */
typedef int (*format_fn_t)(stream_t stream, modifier_stream_t mod, va_list args);

/**
 * @brief A single named format specifier.
 */
typedef struct format_specifier_t {
	const char *specifier;
	format_fn_t format;
} format_specifier_t;

/**
 * @brief Registry of named format specifiers.
 */
typedef struct format_specifiers_t {
	size_t len, cap;
	format_specifier_t *items;
} format_specifiers_t;

/** @brief Global format specifier registry. */
extern format_specifiers_t format_specifiers;
/** @brief Standard output stream (initialised by `setup_io_stream`). */
extern stream_t sout;
/** @brief Standard error stream. */
extern stream_t serr;
/** @brief Standard input stream. */
extern stream_t stin;

/**
 * @brief Initialise the built-in I/O streams and register default specifiers.
 *
 * Must be called before using `print`, `println`, `eprint`, `eprintln`,
 * or any `{specifier}` in format strings.
 */
MY_STREAM_DEF void setup_io_stream(void);
/**
 * @brief Look up a format specifier by name.
 * @param name  The specifier name.
 * @param out   Receives the matching specifier, if found.
 * @return 1 if found, 0 otherwise.
 */
MY_STREAM_DEF int find_format_specifier(const char *name, format_specifier_t *out);
/**
 * @brief Register a custom format specifier.
 * @param name     The specifier name (must not contain `%{} \n\r\t\v`).
 * @param callback The formatting function.
 * @return 1 on success, 0 if the name is already registered.
 */
MY_STREAM_DEF int define_format_specifier(const char *name, format_fn_t callback);

/** @brief Print to stdout (requires `setup_io_stream()` first). */
MY_STREAM_DEF int print(const char *  fmt, ...);
/** @brief Print to stdout followed by a newline. */
MY_STREAM_DEF int println(const char *  fmt, ...);
/** @brief Print to stderr. */
MY_STREAM_DEF int eprint(const char *  fmt, ...);
/** @brief Print to stderr followed by a newline. */
MY_STREAM_DEF int eprintln(const char *  fmt, ...);
/** @brief Print to an arbitrary stream. */
MY_STREAM_DEF int sprint(stream_t stream, const char *  fmt, ...);
/** @brief Print to an arbitrary stream followed by a newline. */
MY_STREAM_DEF int sprintln(stream_t stream, const char *  fmt, ...);

/**
 * @brief Print into a fixed-size buffer (safe snprintf-style).
 * @param buf  Destination buffer.
 * @param n    Buffer size.
 * @param fmt  Format string.
 * @param ...  Arguments.
 * @return Number of characters written (not including null).
 */
MY_STREAM_DEF int snsprint(char *buf, size_t n, const char *  fmt, ...);
/* int snsprintln(char *buf, size_t n, const char *  fmt, ...); */

/** @brief Write a character to a stream. */
MY_STREAM_DEF int sputc(stream_t stream, int ch);
/** @brief Write a string to a stream. */
MY_STREAM_DEF int sputs(stream_t stream, const char *  s);
/** @brief Write a string of known length to a stream. */
MY_STREAM_DEF int snputs(stream_t stream, const int len, const char *  s);

/** @brief Read from a stream. */
MY_STREAM_DEF int sread(stream_t stream, unsigned char *out, size_t size, size_t n);
/** @brief Write to a stream. */
MY_STREAM_DEF int swrite(stream_t stream, const unsigned char *in, size_t size, size_t n);
/** @brief Seek on a stream. */
MY_STREAM_DEF int sseek(stream_t stream, long int offset, int whence);
/** @brief Flush a stream. */
MY_STREAM_DEF int sflush(stream_t stream);

/**
 * @brief Open a memory-backed stream.
 * @param buffer  The buffer to read from / write to.
 * @param len     Buffer size.
 * @return A stream_t for the memory buffer.
 */
MY_STREAM_DEF stream_t smemopen(char *buffer, size_t len);
/**
 * @brief Open a file as a stream.
 * @param path  File path.
 * @param mode  fopen-style mode string.
 * @return A stream_t backed by the file, or a zeroed stream on failure.
 */
MY_STREAM_DEF stream_t sopen(const char *path, const char *mode);
/**
 * @brief Close a stream.
 * @param stream  The stream to close.
 * @return 0 on success, EOF on error.
 */
MY_STREAM_DEF int sclose(stream_t stream);

/** @brief Varargs version of `print`. */
MY_STREAM_DEF int vprint(const char *  fmt, va_list args);
/** @brief Varargs version of `sprint`. */
MY_STREAM_DEF int vsprint(stream_t stream, const char *  fmt, va_list args);
/** @brief Varargs version of `snsprint`. */
MY_STREAM_DEF int vsnsprint(char *buf, size_t n, const char *  fmt, va_list args);

/** @brief Check if a modifier stream has remaining characters. */
MY_STREAM_DEF int has_modifier(const modifier_stream_t *mod);
/** @brief Peek at the next modifier character without consuming. */
MY_STREAM_DEF char peek_modifier(const modifier_stream_t *mod);
/** @brief Consume and return the next modifier character. */
MY_STREAM_DEF char advance_modifier(modifier_stream_t *mod);
/** @brief Check if the next modifier character equals `ch` without consuming. */
MY_STREAM_DEF int check_modifier(const modifier_stream_t *mod, const char ch);
/** @brief If the next modifier character equals `ch`, consume it and return 1. */
MY_STREAM_DEF int match_modifier(modifier_stream_t *mod, const char ch);

# ifdef MY_STREAM_IMPL

/* C89 compat: snprintf is a C99 function */
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L
static int local_snprintf(char *buf, size_t sz, const char *fmt, ...) {
    int n;
    va_list args;
    va_start(args, fmt);
    n = vsprintf(buf, fmt, args);
    va_end(args);
    (void)sz;
    return n;
}
#  define snprintf local_snprintf
#endif

/* --- file stream implementation --- */
static int fs_close(void *data);
static int fs_read(void *data, unsigned char *out_buf, size_t amount);
static int fs_write(void *data, const unsigned char *, size_t);
static int fs_seek(void *data, long int offset, int whence);
static int fs_flush(void *data);

/* --- memory stream implementation --- */
static int mem_close(void *data);
static int mem_read(void *data, unsigned char *out_buf, size_t amount);
static int mem_write_altr(void *data, const unsigned char *in, size_t m);
static int mem_write(void *data, const unsigned char *, size_t);
static int mem_seek(void *data, long int offset, int whence);
static int mem_flush(void *data);

static const stream_interface_t file_vtable_ = {
	fs_close,
	fs_read,
	fs_write,
	fs_seek,
	fs_flush,
};

static const stream_interface_t mem_altr_vtable_ = {
	mem_close,
	mem_read,
	mem_write_altr,
	mem_seek,
	mem_flush,
};

static const stream_interface_t mem_vtable_ = {
	mem_close,
	mem_read,
	mem_write,
	mem_seek,
	mem_flush,
};

format_specifiers_t format_specifiers = {0};
stream_t sout = {0};
stream_t serr = {0};
stream_t stin = {0};

#  if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#    define file_into_stream(...) (stream_t){ .data = (__VA_ARGS__), .vtable = &file_vtable_ }
#  else
static stream_t file_into_stream_c89(void *data, const stream_interface_t *vtable) {
    stream_t s;
    s.data = data;
    s.vtable = vtable;
    return s;
}
#    define file_into_stream(fp) file_into_stream_c89((fp), &file_vtable_)
#  endif

/** @brief Write a string with optional width-based padding. */
static int swrite_width(
	stream_t stream,
	const unsigned char *in,
	const int len,
	const int width,
	const char *const padding_char)
{
	int i;
	int printed_amount = 0;
	for (i=width - len; 0 < i; i -= 1) {
		printed_amount += swrite(stream, (void*)padding_char, sizeof(char), 1);
	}

	printed_amount += swrite(stream, (void*)in, sizeof(char), len);

	for (i=width + len; 0 > i; i += 1) {
		printed_amount += swrite(stream, (void*)padding_char, sizeof(char), 1);
	}

	return printed_amount;
}

MY_STREAM_DEF standard_format_info_t parse_format_info(modifier_stream_t *mod, va_list args)
{
	standard_format_info_t result = {0};
	if (!has_modifier(mod)) return result;
	if (match_modifier(mod, '#')) result.alternate_form = 1;
	if (match_modifier(mod, '0')) result.zero_padded = 1;
	if (match_modifier(mod, '*')) {
		result.has_len = 1;
		result.len = va_arg(args, int);
		result.has_base = 1;
		result.base = result.len;
	} else {
		char *endptr = NULL;
		long m = strtol(mod->current, &endptr, 10);

		if (mod->current != endptr) {
			result.has_len = 1;
			result.len = (int)m;
			result.has_base = 1;
			result.base = result.len;
			mod->current = endptr;
		}
	}
	if (match_modifier(mod, '.')) {
		if (match_modifier(mod, '*')) {
			result.has_preci = 1;
			result.preci = result.width;
		} else {
			char *endptr = NULL;
			const long m = strtol(mod->current, &endptr, 10);

			if (mod->current != endptr) {
				mod->current = endptr;
				result.has_preci = 1;
				result.preci = (int)m;
			}
		}
	}
	if (match_modifier(mod, 'w')) {
		if (match_modifier(mod, '*')) {
			result.has_width = 1;
			result.width = va_arg(args, int);
		} else {
			char *endptr = NULL;
			const long m = strtol(mod->current, &endptr, 10);
			result.has_width = 1;
			result.width = (int)m;
		}
	}

	return result;
}

/** @brief Format a single character (`c`). */
static int format_char(stream_t stream, modifier_stream_t mod, va_list args)
{
	char ch;
	(void)mod;
	ch = (char)va_arg(args, int);
	return swrite(stream, (void*)&ch, sizeof(ch), 1);
}

/** @brief Format a string / C-string (`s`). */
static int format_string(
	stream_t stream,
	modifier_stream_t mod,
	va_list args)
{
	const char *str;
	standard_format_info_t fmt_info;
	int width;
	int input_len;
	const char *padding_char;

	str = va_arg(args, const char *);
	if (str == NULL) str = "(nil)";

	fmt_info = parse_format_info(&mod, args);
	width = fmt_info.width;
	input_len = fmt_info.len ? fmt_info.len : (int)strlen(str);
	padding_char = fmt_info.zero_padded ? "0" : " ";
	if (fmt_info.alternate_form) {
		int i;
		int len = 0;
		int printed_amount = 0;
		len += 1;
		for (i=0; i < input_len; i += 1) {
			const char ch = str[i];
			if (ch == '"'
				|| ch == '\a'
				|| ch == '\b'
				|| ch == '\f'
				|| ch == '\n'
				|| ch == '\r'
				|| ch == '\t'
				|| ch == '\v'
				|| ch == '\\'
				|| ch == '\0') {
				len += 1;
			}
			len += 1;
		}
		len += 1;

		for (i=width - len; 0 < i; i -= 1) {
			printed_amount += swrite(stream, (void*)padding_char, sizeof(char), 1);
		}

		printed_amount += swrite(stream, (void*)"\"", sizeof(char), 1);
		for (i=0; i<input_len; i += 1) {
			const char ch = str[i];
			switch (ch) {
			case '"':
				printed_amount += swrite(stream, (void*)"\\\"", sizeof(char), 2);
				break;
			case '\'':
				printed_amount += swrite(stream, (void*)"\\'", sizeof(char), 2);
				break;
			case '\a':
				printed_amount += swrite(stream, (void*)"\\a", sizeof(char), 2);
				break;
			case '\b':
				printed_amount += swrite(stream, (void*)"\\b", sizeof(char), 2);
				break;
			case '\f':
				printed_amount += swrite(stream, (void*)"\\f", sizeof(char), 2);
				break;
			case '\n':
				printed_amount += swrite(stream, (void*)"\\n", sizeof(char), 2);
				break;
			case '\r':
				printed_amount += swrite(stream, (void*)"\\r", sizeof(char), 2);
				break;
			case '\t':
				printed_amount += swrite(stream, (void*)"\\t", sizeof(char), 2);
				break;
			case '\v':
				printed_amount += swrite(stream, (void*)"\\v", sizeof(char), 2);
				break;
			case '\\':
				printed_amount += swrite(stream, (void*)"\\\\", sizeof(char), 2);
				break;
			case '\0':
				printed_amount += swrite(stream, (void*)"\\0", sizeof(char), 2);
				break;
			default:
				printed_amount += swrite(stream, (void*)&ch, sizeof(char), 1);
				break;
			}
		}
		printed_amount += swrite(stream, (void*)"\"", sizeof(char), 1);

		for (i=width + len; 0 > i; i += 1) {
			printed_amount += swrite(stream, (void*)padding_char, sizeof(char), 1);
		}

		return printed_amount;
	}

	return swrite_width(stream, (void*)str, input_len, width, padding_char);
}

/** @brief Convert a signed long long to a string in the given base. */
static char* ll_to_base_str(long long val, char* buf, int alternate, int base)
{
	int upper = 0;
	char *digits;
	int i = 0;
	int is_negative = 0;
	unsigned long long u_val;
	int start;
	int end;

	if (base < 0) {
		upper = 1;
		base *= -1;
	}

	if (base < 2 || base > 36) {
		buf[0] = '\0';
		return buf;
	}

	digits = "0123456789abcdefghijklmnopqrstuvwxyz";
	if (upper) {
		digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	}
	u_val = val;

	if (val == 0) {
		buf[i++] = '0';
		buf[i] = '\0';
		return buf;
	}

	if (val < 0) {
		is_negative = 1;
		u_val = (unsigned long long)(-val);
	} else {
		u_val = (unsigned long long)val;
	}

	while (u_val > 0) {
		buf[i++] = digits[u_val % base];
		u_val /= base;
	}

	if (is_negative) {
		buf[i++] = '-';
	}

	if (alternate) {
		if (base == 2) {
			buf[i++] = upper ? 'B' : 'b';
			buf[i++] = '0';
		} else if (base == 16) {
			buf[i++] = upper ? 'X' : 'x';
			buf[i++] = '0';
		} else if (base == 8) {
			buf[i++] = '0';
		}
	}

	buf[i] = '\0';

	start = 0;
	end = i - 1;
	while (start < end) {
		char temp = buf[start];
		buf[start] = buf[end];
		buf[end] = temp;
		start++;
		end--;
	}

	return buf;
}

/** @brief Convert an unsigned long long to a string in the given base. */
static char* ull_to_base_str(unsigned long long val, char* buf, int alternate, int base)
{
	int upper = 0;
	char *digits;
	int i = 0;
	unsigned long long u_val;
	int start;
	int end;

	if (base < 0) {
		upper = 1;
		base *= -1;
	}

	if (base < 2 || base > 36) {
		buf[0] = '\0';
		return buf;
	}

	digits = "0123456789abcdefghijklmnopqrstuvwxyz";
	if (upper) {
		digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	}
	u_val = val;

	if (val == 0) {
		buf[i++] = '0';
		buf[i] = '\0';
		return buf;
	}

	while (u_val > 0) {
		buf[i++] = digits[u_val % base];
		u_val /= base;
	}

	if (alternate) {
		if (base == 2) {
			buf[i++] = upper ? 'B' : 'b';
			buf[i++] = '0';
		} else if (base == 16) {
			buf[i++] = upper ? 'X' : 'x';
			buf[i++] = '0';
		} else if (base == 8) {
			buf[i++] = '0';
		}
	}

	buf[i] = '\0';

	start = 0;
	end = i - 1;
	while (start < end) {
		char temp = buf[start];
		buf[start] = buf[end];
		buf[end] = temp;
		start++;
		end--;
	}

	return buf;
}

/** @brief Convert a size_t to a string in the given base. */
static char* size_to_base_str(size_t val, char* buf, int alternate, int base)
{
	int upper = 0;
	char *digits;
	int i = 0;
	size_t u_val;
	int start;
	int end;

	if (base < 0) {
		upper = 1;
		base *= -1;
	}

	if (base < 2 || base > 36) {
		buf[0] = '\0';
		return buf;
	}

	digits = "0123456789abcdefghijklmnopqrstuvwxyz";
	if (upper) {
		digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	}
	u_val = val;

	if (val == 0) {
		buf[i++] = '0';
		buf[i] = '\0';
		return buf;
	}

	while (u_val > 0) {
		buf[i++] = digits[u_val % base];
		u_val /= base;
	}

	if (alternate) {
		if (base == 2) {
			buf[i++] = upper ? 'B' : 'b';
			buf[i++] = '0';
		} else if (base == 16) {
			buf[i++] = upper ? 'X' : 'x';
			buf[i++] = '0';
		} else if (base == 8) {
			buf[i++] = '0';
		}
	}

	buf[i] = '\0';

	start = 0;
	end = i - 1;
	while (start < end) {
		char temp = buf[start];
		buf[start] = buf[end];
		buf[end] = temp;
		start++;
		end--;
	}

	return buf;
}

/** @brief Convert a pointer to a hex string representation. */
#if 0
static char* ptr_to_base_str(void *val, char* buf, int alternate, int base)
{
	int upper = 0;
	char *digits;
	int i = 0;
	uintptr_t u_val;
	int start;
	int end;

	if (base < 0) {
		upper = 1;
		base *= -1;
	}

	if (base < 2 || base > 36) {
		buf[0] = '\0';
		return buf;
	}

	digits = "0123456789abcdefghijklmnopqrstuvwxyz";
	if (upper) {
		digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	}
	u_val = (uintptr_t)val;

	if (val == 0) {
		buf[i++] = '0';
		buf[i] = '\0';
		return buf;
	}

	while (u_val > 0) {
		buf[i++] = digits[u_val % base];
		u_val /= base;
	}

	if (alternate) {
		if (base == 2) {
			buf[i++] = upper ? 'B' : 'b';
			buf[i++] = '0';
		} else if (base == 16) {
			buf[i++] = upper ? 'X' : 'x';
			buf[i++] = '0';
		} else if (base == 8) {
			buf[i++] = '0';
		}
	}

	buf[i] = '\0';

	start = 0;
	end = i - 1;
	while (start < end) {
		char temp = buf[start];
		buf[start] = buf[end];
		buf[end] = temp;
		start++;
		end--;
	}

	return buf;
}
#endif

/* --- typed format helpers for integers --- */
static int format_int8(
	stream_t stream,
	modifier_stream_t mod,
	va_list args)
{
	const int8_t integer = va_arg(args, int);
	const standard_format_info_t info = parse_format_info(&mod, args);
	const int base = info.base ? info.base : 10;
	char buffer[512] = {0};
	const char *const padding_char = info.zero_padded ? "0" : " ";
	ll_to_base_str((long long)integer, buffer, info.alternate_form, base);
	return swrite_width(stream, (void*)buffer, strlen(buffer), info.width, padding_char);
}

static int format_int16(
	stream_t stream,
	modifier_stream_t mod,
	va_list args)
{
	const int16_t integer = va_arg(args, int);
	const standard_format_info_t info = parse_format_info(&mod, args);
	const int base = info.base ? info.base : 10;
	char buffer[512] = {0};
	const char *const padding_char = info.zero_padded ? "0" : " ";
	ll_to_base_str((long long)integer, buffer, info.alternate_form, base);
	return swrite_width(stream, (void*)buffer, strlen(buffer), info.width, padding_char);
}

static int format_int32(
	stream_t stream,
	modifier_stream_t mod,
	va_list args)
{
	const int32_t integer = va_arg(args, int32_t);
	const standard_format_info_t info = parse_format_info(&mod, args);
	const int base = info.base ? info.base : 10;
	char buffer[512] = {0};
	const char *const padding_char = info.zero_padded ? "0" : " ";
	ll_to_base_str((long long)integer, buffer, info.alternate_form, base);
	return swrite_width(stream, (void*)buffer, strlen(buffer), info.width, padding_char);
}

static int format_int64(
	stream_t stream,
	modifier_stream_t mod,
	va_list args)
{
	const int64_t integer = va_arg(args, int64_t);
	const standard_format_info_t info = parse_format_info(&mod, args);
	const int base = info.base ? info.base : 10;
	char buffer[512] = {0};
	const char *const padding_char = info.zero_padded ? "0" : " ";
	ll_to_base_str((long long)integer, buffer, info.alternate_form, base);
	return swrite_width(stream, (void*)buffer, strlen(buffer), info.width, padding_char);
}

static int format_uint8(
	stream_t stream,
	modifier_stream_t mod,
	va_list args)
{
	const uint8_t integer = va_arg(args, unsigned int);
	const standard_format_info_t info = parse_format_info(&mod, args);
	const int base = info.base ? info.base : 10;
	char buffer[512] = {0};
	const char *const padding_char = info.zero_padded ? "0" : " ";
	ll_to_base_str((long long)integer, buffer, info.alternate_form, base);
	return swrite_width(stream, (void*)buffer, strlen(buffer), info.width, padding_char);
}

static int format_uint16(
	stream_t stream,
	modifier_stream_t mod,
	va_list args)
{
	const uint16_t integer = va_arg(args, unsigned int);
	const standard_format_info_t info = parse_format_info(&mod, args);
	const int base = info.base ? info.base : 10;
	char buffer[512] = {0};
	const char *const padding_char = info.zero_padded ? "0" : " ";
	ll_to_base_str((long long)integer, buffer, info.alternate_form, base);
	return swrite_width(stream, (void*)buffer, strlen(buffer), info.width, padding_char);
}

static int format_uint32(
	stream_t stream,
	modifier_stream_t mod,
	va_list args)
{
	const uint32_t integer = va_arg(args, uint32_t);
	const standard_format_info_t info = parse_format_info(&mod, args);
	const int base = info.base ? info.base : 10;
	char buffer[512] = {0};
	const char *const padding_char = info.zero_padded ? "0" : " ";
	ll_to_base_str((long long)integer, buffer, info.alternate_form, base);
	return swrite_width(stream, (void*)buffer, strlen(buffer), info.width, padding_char);
}

static int format_uint64(
	stream_t stream,
	modifier_stream_t mod,
	va_list args)
{
	const uint64_t integer = va_arg(args, uint64_t);
	const standard_format_info_t info = parse_format_info(&mod, args);
	const int base = info.base ? info.base : 10;
	char buffer[512] = {0};
	const char *const padding_char = info.zero_padded ? "0" : " ";
	ll_to_base_str((long long)integer, buffer, info.alternate_form, base);
	return swrite_width(stream, (void*)buffer, strlen(buffer), info.width, padding_char);
}

static int format_int(
	stream_t stream,
	modifier_stream_t mod,
	va_list args)
{
	const int integer = va_arg(args, int);
	const standard_format_info_t info = parse_format_info(&mod, args);
	const int base = info.base ? info.base : 10;
	char buffer[512] = {0};
	const char *const padding_char = info.zero_padded ? "0" : " ";
	ll_to_base_str((long long)integer, buffer, info.alternate_form, base);
	return swrite_width(stream, (void*)buffer, strlen(buffer), info.width, padding_char);
}

static int format_l_int(
	stream_t stream,
	modifier_stream_t mod,
	va_list args)
{
	const long int integer = va_arg(args, long int);
	const standard_format_info_t info = parse_format_info(&mod, args);
	const int base = info.base ? info.base : 10;
	char buffer[512] = {0};
	const char *const padding_char = info.zero_padded ? "0" : " ";
	ll_to_base_str((long long)integer, buffer, info.alternate_form, base);
	return swrite_width(stream, (void*)buffer, strlen(buffer), info.width, padding_char);
}

static int format_ll_int(
	stream_t stream,
	modifier_stream_t mod,
	va_list args)
{
	const long long int integer = va_arg(args, long long int);
	const standard_format_info_t info = parse_format_info(&mod, args);
	const int base = info.base ? info.base : 10;
	char buffer[512] = {0};
	const char *const padding_char = info.zero_padded ? "0" : " ";
	ll_to_base_str((long long)integer, buffer, info.alternate_form, base);
	return swrite_width(stream, (void*)buffer, strlen(buffer), info.width, padding_char);
}

static int format_uint(
	stream_t stream,
	modifier_stream_t mod,
	va_list args)
{
	const unsigned int integer = va_arg(args, unsigned int);
	const standard_format_info_t info = parse_format_info(&mod, args);
	const int base = info.base ? info.base : 10;
	char buffer[512] = {0};
	const char *const padding_char = info.zero_padded ? "0" : " ";
	ull_to_base_str((unsigned long long)integer, buffer, info.alternate_form, base);
	return swrite_width(stream, (void*)buffer, strlen(buffer), info.width, padding_char);
}

static int format_l_uint(
	stream_t stream,
	modifier_stream_t mod,
	va_list args)
{
	const unsigned long int integer = va_arg(args, unsigned long int);
	const standard_format_info_t info = parse_format_info(&mod, args);
	const int base = info.base ? info.base : 10;
	char buffer[512] = {0};
	const char *const padding_char = info.zero_padded ? "0" : " ";
	ull_to_base_str((unsigned long long)integer, buffer, info.alternate_form, base);
	return swrite_width(stream, (void*)buffer, strlen(buffer), info.width, padding_char);
}

static int format_ll_uint(
	stream_t stream,
	modifier_stream_t mod,
	va_list args)
{
	const long long unsigned int integer = va_arg(args, long long unsigned int);
	const standard_format_info_t info = parse_format_info(&mod, args);
	const int base = info.base ? info.base : 10;
	char buffer[512] = {0};
	const char *const padding_char = info.zero_padded ? "0" : " ";
	ull_to_base_str((unsigned long long)integer, buffer, info.alternate_form, base);
	return swrite_width(stream, (void*)buffer, strlen(buffer), info.width, padding_char);
}

static int format_size_t(
	stream_t stream,
	modifier_stream_t mod,
	va_list args)
{
	const size_t integer = va_arg(args, size_t);
	const standard_format_info_t info = parse_format_info(&mod, args);
	const int base = info.base ? info.base : 10;
	char buffer[512] = {0};
	const char *const padding_char = info.zero_padded ? "0" : " ";
	size_to_base_str(integer, buffer, info.alternate_form, base);
	return swrite_width(stream, (void*)buffer, strlen(buffer), info.width, padding_char);
}

static int format_ptr(
	stream_t stream,
	modifier_stream_t mod,
	va_list args)
{
	const void *ptr = va_arg(args, void*);
	const standard_format_info_t info = parse_format_info(&mod, args);
	char buffer[512] = {0};
	const char *const padding_char = info.zero_padded ? "0" : " ";
	ull_to_base_str((unsigned long long)ptr, buffer, 1, 16);
	return swrite_width(stream, (void*)buffer, strlen(buffer), info.width, padding_char);
}

static int format_float(
	stream_t stream,
	modifier_stream_t mod,
	va_list args)
{
	const double f = va_arg(args, double);
	const standard_format_info_t info = parse_format_info(&mod, args);
	const char *const padding_char = info.zero_padded ? "0" : " ";
	char buffer[30] = {0};
	snprintf(buffer, sizeof(buffer), "%f", f);
	return swrite_width(stream, (void*)buffer, strlen(buffer), info.width, padding_char);
}

static int format_double(
	stream_t stream,
	modifier_stream_t mod,
	va_list args)
{
	const double f = va_arg(args, double);
	const standard_format_info_t info = parse_format_info(&mod, args);
	const char *const padding_char = info.zero_padded ? "0" : " ";
	char buffer[30] = {0};
	snprintf(buffer, sizeof(buffer), "%lf", f);
	return swrite_width(stream, (void*)buffer, strlen(buffer), info.width, padding_char);
}

static int format_long_double(
	stream_t stream,
	modifier_stream_t mod,
	va_list args)
{
	const long double f = va_arg(args, long double);
	const standard_format_info_t info = parse_format_info(&mod, args);
	const char *const padding_char = info.zero_padded ? "0" : " ";
	char buffer[30] = {0};
	snprintf(buffer, sizeof(buffer), "%Lf", f);
	return swrite_width(stream, (void*)buffer, strlen(buffer), info.width, padding_char);
}

MY_STREAM_DEF void setup_io_stream(void)
{
	sout.data = stdout;
	sout.vtable = &file_vtable_;
	serr.data = stderr;
	serr.vtable = &file_vtable_;
	stin.data = stdin;
	stin.vtable = &file_vtable_;

	define_format_specifier("c",   format_char);
	define_format_specifier("s",   format_string);

	define_format_specifier("int", format_int);

	define_format_specifier("f", format_float);
	define_format_specifier("lf", format_double);
	define_format_specifier("Lf", format_long_double);

	define_format_specifier("i8", format_int8);
	define_format_specifier("i16", format_int16);
	define_format_specifier("i32", format_int32);
	define_format_specifier("i64", format_int64);

	define_format_specifier("int8", format_int8);
	define_format_specifier("int16", format_int16);
	define_format_specifier("int32", format_int32);
	define_format_specifier("int64", format_int64);

	define_format_specifier("u8", format_uint8);
	define_format_specifier("u16", format_uint16);
	define_format_specifier("u32", format_uint32);
	define_format_specifier("u64", format_uint64);

	define_format_specifier("unt8", format_uint8);
	define_format_specifier("unt16", format_uint16);
	define_format_specifier("unt32", format_uint32);
	define_format_specifier("unt64", format_uint64);

	define_format_specifier("i",   format_int);
	define_format_specifier("li",  format_l_int);
	define_format_specifier("lli", format_ll_int);

	define_format_specifier("u",   format_uint);
	define_format_specifier("lu",  format_l_uint);
	define_format_specifier("llu", format_ll_uint);

	define_format_specifier("d",   format_int);
	define_format_specifier("ld",  format_l_int);
	define_format_specifier("lld", format_ll_int);

	define_format_specifier("z", format_size_t);
	define_format_specifier("usize", format_size_t);

	define_format_specifier("iz", format_size_t);
	define_format_specifier("isize", format_size_t);

	define_format_specifier("p", format_ptr);
	define_format_specifier("ptr", format_ptr);
}

#if 0
static int is_valid_specifier_name(const char *name)
{
	const char *current;
	const char *ch;
	const char *const banned_characters = "%{} \n\r\t\v\0";
	for (current = name; *current; current += 1) {
		for (ch = banned_characters; *ch; ch += 1) {
			if (*current == *ch) return 0;
		}
	}
	return 1;
}
#endif

static int nfind_format_specifier(const char *name, const size_t n, format_specifier_t *out)
{
	size_t i;
	if (n == 0) return 0;
	for (i=0; i < format_specifiers.len; i++) {
		if (strncmp(name, format_specifiers.items[i].specifier, n) == 0) {
			if (out != NULL) {
				*out = format_specifiers.items[i];
			}
			return 1;
		}
	}
	return 0;
}

MY_STREAM_DEF int find_format_specifier(const char *name, format_specifier_t *out)
{
	size_t i;
	for (i=0; i < format_specifiers.len; i++) {
		if (strcmp(name, format_specifiers.items[i].specifier) == 0) {
			if (out != NULL) {
				*out = format_specifiers.items[i];
			}
			return 1;
		}
	}
	return 0;
}

MY_STREAM_DEF int define_format_specifier(const char *name, format_fn_t callback)
{
	if (find_format_specifier(name, NULL)) {
		return 0;
	}
	if (format_specifiers.len >= format_specifiers.cap) {
		const size_t new_cap__ = format_specifiers.cap == 0 ? 10 : format_specifiers.cap * 2;
		format_specifiers.items = realloc(format_specifiers.items , (new_cap__ * sizeof(format_specifier_t)));
		format_specifiers.cap = new_cap__ ;
	}
	format_specifiers.items[format_specifiers.len].specifier = name;
	format_specifiers.items[format_specifiers.len].format = callback;
	format_specifiers.len += 1;
	return 1;
}

MY_STREAM_DEF int print(const char *  fmt, ...)
{
	va_list args;
	int result;
	va_start(args, fmt);
	result = vprint(fmt, args);
	va_end(args);
	return result;
}

MY_STREAM_DEF int println(const char *  fmt, ...)
{
	va_list args;
	int result;
	va_start(args, fmt);
	result = vprint(fmt, args);
	va_end(args);
	result += print("\n");
	return result;
}

MY_STREAM_DEF int eprint(const char *  fmt, ...)
{
	va_list args;
	int result;
	va_start(args, fmt);
	result = vsprint(serr, fmt, args);
	va_end(args);
	return result;
}

MY_STREAM_DEF int eprintln(const char *  fmt, ...)
{
	va_list args;
	int result;
	va_start(args, fmt);
	result = vsprint(serr, fmt, args);
	va_end(args);
	result += sprint(serr, "\n");
	return result;
}

MY_STREAM_DEF int sprint(stream_t stream, const char *  fmt, ...)
{
	va_list args;
	int result;
	va_start(args, fmt);
	result = vsprint(stream, fmt, args);
	va_end(args);
	return result;
}

MY_STREAM_DEF int sprintln(stream_t stream, const char *  fmt, ...)
{
	va_list args;
	int result;
	va_start(args, fmt);
	result = vsprint(stream, fmt, args);
	va_end(args);
	result += sprint(stream, "\n");
	return result;
}

MY_STREAM_DEF int snsprint(char *buf, size_t n, const char *  fmt, ...)
{
	va_list args;
	int amount;
	va_start(args, fmt);
	amount = vsnsprint(buf, n, fmt, args);
	va_end(args);
	return amount;
}

/* int snsprintln(char *buf, size_t n, const char *  fmt, ...) */
/* { */
/* 	va_list args; va_start(args, fmt); */
/* 	int amount = vsnsprint(buf, n, fmt, args); */
/* 	va_end(args); */
/* 	amount += println(""); */
/* 	return amount; */
/* } */

MY_STREAM_DEF int vsnsprint(char *buf, size_t n, const char *  fmt, va_list args)
{
	mem_stream_t data;
	stream_t strm;
	int amount;
	data.buffer = buf;
	data.len = n;
	data.pos = 0;
	strm.data = &data;
	strm.vtable = &mem_altr_vtable_;
	amount = vsprint(strm, fmt, args);
	amount += sflush(strm);
	return amount;
}

MY_STREAM_DEF int sputc(stream_t stream, int ch)
{
	unsigned char c;
	unsigned char chars[2];
	int count;
	c = (unsigned char)ch;
	chars[0] = c;
	chars[1] = '\0';
	count = swrite(stream, chars, 1, 1);
	if (count != 1) return EOF;

	return (int)c;
}

MY_STREAM_DEF int sputs(stream_t stream, const char *  s)
{
	const size_t len = strlen(s);
	int count = swrite(stream, (const unsigned char*)s, sizeof(char), len);
	if ((size_t)count != len) return EOF;
	return (int)len;
}

MY_STREAM_DEF int snputs(stream_t stream, const int len, const char *  s)
{
	int count = swrite(stream, (const unsigned char*)s, sizeof(char), len);
	if (count != len) return EOF;
	return (int)len;
}

MY_STREAM_DEF int sread(stream_t stream, unsigned char *out, size_t size, size_t n)
{
	return stream.vtable->read(stream.data, out, size * n);
}

MY_STREAM_DEF int swrite(stream_t stream, const unsigned char *in, size_t size, size_t n)
{
	return stream.vtable->write(stream.data, in, size * n);
}

MY_STREAM_DEF int sseek(stream_t stream, long int offset, int whence)
{
	return stream.vtable->seek(stream.data, offset, whence);
}

MY_STREAM_DEF int sflush(stream_t stream)
{
	return stream.vtable->flush(stream.data);
}

MY_STREAM_DEF stream_t smemopen(char *buffer, size_t len)
{
	stream_t result;
	mem_stream_t *data;
	data = calloc(1, sizeof(mem_stream_t));
	result.data = data;
	result.vtable = &mem_vtable_;
	data->buffer = buffer;
	data->len = len;
	data->pos = 0;
	return result;
}

MY_STREAM_DEF stream_t sopen(const char *path, const char *mode)
{
	FILE *f = fopen(path, mode);
	return file_into_stream(f);
}

MY_STREAM_DEF int sclose(stream_t stream)
{
	sflush(stream);
	if (stream.vtable != &file_vtable_) {
		free(stream.data);
	}
	return stream.vtable->close(stream.data);
}

MY_STREAM_DEF int vprint(const char *  fmt, va_list args)
{
	return vsprint(sout, fmt, args);
}

MY_STREAM_DEF int vsprint(stream_t stream, const char *  fmt, va_list args)
{
	int printed_amount = 0;
	const char *current;

	const char *printing_span = NULL;
	size_t printing_span_len = 0;

	const char *specifier = NULL;
	size_t specifier_len = 0;

	const char *modifier = NULL;
	size_t modifier_len = 0;

	for (current = fmt; *current; current += 1) {
		if (*current == '{') {
			if (printing_span != NULL) {
				printed_amount += swrite(stream, (const unsigned char *)printing_span, sizeof(char), printing_span_len);
			}
			printing_span = NULL;
			printing_span_len = 0;

			current += 1;
			if (*current == '\0') {
				printed_amount += swrite(stream, (void*)"{", sizeof(char), 1);
				break;
			}
			if (*current == '{') {
				printed_amount += swrite(stream, (void*)"{", sizeof(char), 1);
				continue;
			}

			for (specifier = current; *current && *current != ':' && *current != '}'; current += 1) {
				specifier_len += 1;
			}

			if (*current && *current == ':' && current[1] != '}') {
				current += 1;
				for (modifier = current; *current && *current != '}'; current += 1) {
					modifier_len += 1;
				}
			}

			{
			format_specifier_t sp = {0};
			modifier_stream_t mod_st;
			if (nfind_format_specifier(specifier, specifier_len, &sp)) {
				mod_st.current = modifier;
				mod_st.len = modifier_len;
				printed_amount += sp.format(stream, mod_st, args);
			} else {
				current -= specifier_len;
				current -= modifier_len;
				current -= 1;
				printed_amount += sputc(stream, '{');
			}
		}

		specifier = NULL;
		specifier_len = 0;
		modifier = NULL;
		modifier_len = 0;
		continue;
		}

		if (printing_span_len == 0) {
			printing_span = current;
		}
		printing_span_len += 1;
	}

	if (printing_span != NULL && printing_span_len > 0) {
		printed_amount += swrite(stream, (const unsigned char *)printing_span, sizeof(char), printing_span_len);
	}

	return printed_amount;
}

MY_STREAM_DEF int has_modifier(const modifier_stream_t *mod)
{
	if (mod->current == NULL) return 0;
	return mod->len != 0;
}

MY_STREAM_DEF char peek_modifier(const modifier_stream_t *mod)
{
	if (!has_modifier(mod)) {
		return 0x0;
	}

	return mod->current[0];
}

MY_STREAM_DEF char advance_modifier(modifier_stream_t *mod)
{
	char prev;
	if (!has_modifier(mod)) return 0x0;
	prev = peek_modifier(mod);

	mod->current += 1;
	mod->len -= 1;

	return prev;
}

MY_STREAM_DEF int check_modifier(const modifier_stream_t *mod, const char ch)
{
	if (has_modifier(mod)) {
		return peek_modifier(mod) == ch;
	}
	return 0;
}

MY_STREAM_DEF int match_modifier(modifier_stream_t *mod, const char ch)
{
	if (check_modifier(mod, ch)) {
		advance_modifier(mod);
		return 1;
	}
	return 0;
}

static int fs_close(void *data)
{
	FILE *self = data;
	return fclose(self);
}

static int fs_read(void *data, unsigned char *out_buf, size_t amount)
{
	FILE *self = data;
	return fread(out_buf, 1, amount, self);
}

static int fs_write(void *data, const unsigned char *in, size_t count)
{
	FILE *self = data;
	return fwrite(in, 1, count, self);
}

static int fs_seek(void *data, long int offset, int whence)
{
	FILE *self = data;
	return fseek(self, offset, whence);
}

static int fs_flush(void *data)
{
	FILE *self = data;
	return fflush(self);
}

static int mem_close(void *data)
{
	(void)data;
	return 0;
}

static int mem_read(void *data, unsigned char *out_buf, size_t amount)
{
	(void)data;
	(void)out_buf;
	(void)amount;
	exit(1);
}

static int mem_write_altr(void *data, const unsigned char *in, size_t m)
{
	int amount = mem_write(data, in, m);
	if ((size_t)amount < m) {
		return (int)m;
	}

	return amount;
}

static int mem_write(void *data, const unsigned char *in, size_t m)
{
	mem_stream_t *self = data;
	const size_t remaining = self->len - self->pos;
	int write_amount;

	if (remaining == 0) return 0;

	write_amount = m < remaining ? (int)m : (int)(remaining - 1);
	memcpy(self->buffer + self->pos, in, sizeof(char) * write_amount);
	self->pos += write_amount;
	self->buffer[self->pos] = '\0';

	return write_amount;
}

static int mem_seek(void *data, long int offset, int whence)
{
	(void)data;
	(void)offset;
	(void)whence;
	exit(1);
}

static int mem_flush(void *data)
{
	(void)data;
	return 0;
}

# endif /* MY_STREAM_IMPL */

#endif /* !MY_STREAM_H_ */
