/**
 * @file my_flags.h
 * @brief General purpose argument flag parsing library.
 *
 * Usage:
 * @code
 *  int main(int argc, char **argv)
 *  {
 *    int boolean = 0;
 *    int integer = 0;
 *    char *string = NULL;
 *
 *    def_flag(&boolean, FLAG_BOOl, "bool", "b", "Boolean");
 *    def_flag(&integer, FLAG_INT, "int", "i", "Integer");
 *    def_flag(&string, FLAG_STRING, "string", "s", "String");
 *
 *    if (argc < 2) {
 *      fprintf(stderr, "%s <options>\n", argv[0]);
 *      print_help_flag(stderr);
 *      return 0;
 *    }
 *
 *    parse_flag(&argc, &argv);
 *
 *    printf("%d\n", boolean);
 *    printf("%zu\n", integer);
 *    printf("%s\n", string);
 *    // the will print the rest of the arguments
 *    for (i=0; i < argc; i += 1) {
 *      printf("%d -> %s\n", i, argv[i]);
 *    }
 *  }
 * @endcode
 *
 * Define `MY_FLAGS_IMPL` in exactly one translation unit.
 * Define `MY_FLAG_DEF` to `static` or `inline` before inclusion for internal linkage or for just inline.
 */
#ifndef MY_FLAGS_H_
#define MY_FLAGS_H_

#include <stdio.h>
#include <stddef.h>

#ifdef __cpluscplus
extern "C" {
#endif /* __cpluscplus */

#ifndef MY_FLAG_DEF
#  define MY_FLAG_DEF
#endif

typedef enum flag_type_t {
	FLAG_NONE = 0,
	FLAG_SIZE_T,
	FLAG_INT,
	FLAG_STRING,
	FLAG_BOOL
} flag_type_t;

typedef void (*flag_call_t)(void *dest, const char *value);

/**
 * @brief define a flag
 * @param dest a pointer to a destination variable (see the example above)
 * @param type the type of the flag. this is used to determin the destination type.
 * @param long_name a name for the flag that start with `--`
 * @param short_name a name for the flag that starts with `-` usualy an abbriviation to `long_name`
 * @param description a description used for the help message
 */
MY_FLAG_DEF void def_flag(
	void *dest, flag_type_t type,
	const char *long_name,
	const char *short_name,
	const char *description);

/**
 * @brief Upon matching with the flag. it will call `callback`
 * @param callback a function pointer
 * @param long_name a name for the flag that start with `--`
 * @param short_name a name for the flag that starts with `-` usualy an abbriviation to `long_name`
 * @param description a description used for the help message
 */
MY_FLAG_DEF void def_flag_call(
	flag_call_t callback,
	void *dest,
	flag_type_t type,
	const char *long_name,
	const char *short_name,
	const char *description);

/**
 * @brief parse the arguments looking for a flag defined with `def_flag`.
 *
 * This function is special. As it parses command line arguments and at the same time
 * it shuffles them in a way that the remaining arguments are non-flag argument which
 * you can use later. e.g. suppose you defined `b` (which takes an integer as an argument) and `c` flag. and the user
 * passes `-c 123 -b 456 a b c`. the parser will take `./program-name -c 123 -b 456 a b c` and it shuffles it back to
 * `-c -b 456 ./program-name 123 a b c` where `argc` * is set to 4. and `argv` now points to `./program-name`
 * @param argc a pointer to `main`'s argc
 * @param argv a pointer to `main`'s argv
 */
MY_FLAG_DEF void parse_flag(int *argc, char ***argv);

/**
 * @brief Prints a help message to `stream`
 * @param stream a stream. e.g. `stdout`, `stderr` or even a regulare file
 */
MY_FLAG_DEF void print_help_flag(FILE *stream);

MY_FLAG_DEF void flag_stop_on_first_non_arg(void);

#ifdef __cplus_cplus
}
#endif /* __cpluscplus */

#ifdef MY_FLAGS_IMPL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cpluscplus
extern "C" {
#endif /* __cpluscplus */

#ifndef FLAGS_DEFAULT_CAP
#  define FLAGS_DEFAULT_CAP 16
#endif /* !FLAGS_DEFAULT_CAP */

#define MAX_FLAG_NAME_LEN ((size_t)512)

typedef struct flag_t {
	flag_type_t type;
	const char *long_name;
	const char *short_name;
	const char *description;
	flag_call_t callback;
	void *dest;
} flag_t;

typedef struct flags_t {
	size_t len, cap;
	size_t max_name_len;
	flag_t *items;
	int stop_on_first_non_arg;
} flags_t;

static flags_t flags = {0};

static void add_flag(flag_t flag);
static const char *flag_type_as_str(flag_type_t type);

MY_FLAG_DEF void def_flag(
	void *dest, flag_type_t type,
	const char *long_name,
	const char *short_name,
	const char *description)
{
	flag_t flag = {0};
	const size_t long_name_len = long_name == NULL ? 0 : strlen(long_name);
	const size_t short_name_len = short_name == NULL ? 0 : strlen(short_name);

	if (long_name_len == 0 && short_name_len == 0) {
		fprintf(stderr, "flag error: You can't have both of the `short_name` and `long_name` arguments as NULL. only one of them can be NULL. never both!!\n");
		fprintf(stderr, "e.g. flag(dest, FLAG_TYPE, NULL, \"short_name\", \"description\"); // good\n");
		fprintf(stderr, "e.g. flag(dest, FLAG_TYPE, \"long_name\", NULL, \"description\"); // good\n");
		fprintf(stderr, "e.g. flag(dest, FLAG_TYPE, NULL, NULL, \"description\"); // BAD\n");
		abort();
	}

	if (long_name_len >= MAX_FLAG_NAME_LEN) {
		fprintf(stderr, "`long_name` argument is too long (%zu > %zu)\n", strlen(long_name), MAX_FLAG_NAME_LEN);
		abort();
	}

	if (short_name_len >= MAX_FLAG_NAME_LEN) {
		fprintf(stderr, "`short_name` argument is too long (%zu > %zu)\n", strlen(short_name), MAX_FLAG_NAME_LEN);
		abort();
	}

	flag.type = type;
	flag.long_name = long_name && strlen(long_name) == 0 ? NULL : long_name;
	flag.short_name = short_name && strlen(short_name) == 0 ? NULL : short_name;
	flag.description = description;
	flag.dest = dest;

	add_flag(flag);
}

MY_FLAG_DEF void def_flag_call(
	flag_call_t callback,
	void *dest,
	flag_type_t type,
	const char *long_name,
	const char *short_name,
	const char *description)
{
  def_flag(dest, type, long_name, short_name, description);
	flags.items[flags.len - 1].callback = callback;
}

#define starts_with(a_, b_) (strncmp(a_, b_, strlen(b_)) == 0)
#define equals(a_, b_) (strcmp(a_, b_) == 0)
#define ended() (*i >= *argc)
#define peek() (*argv)[i]
#define consume() i += 1

static void parse_flag2(flag_t flag, char *arg);
static flag_t find_long_flag(const char *const name);
static flag_t find_short_flag(const char *const name);

MY_FLAG_DEF void parse_flag(int *argc, char ***argv)
{
	int write_pos = 1;
	int i = 1;
	if (*argc <= 0) return;

	while (i < *argc) {
		int is_flag = 0;

		if (starts_with((*argv)[i], "--")) {
			const char *flag_char = peek() + 2;
			char *sep = strchr(flag_char, '=');
			char *value = NULL;
			flag_t flag = {0};
			if (sep == NULL) {
				sep = strchr(flag_char, ':');
			}
			if (sep != NULL) {
				value = sep + 1;
				*sep = '\0';
			}

			flag = find_long_flag(flag_char);
			if (flag.type != FLAG_NONE) {
				is_flag = 1;
				i++;
				if (value == NULL && flag.type != FLAG_BOOL) {
					value = peek();
					consume();
				}
				parse_flag2(flag, value);
			}
		} else if (starts_with((*argv)[i], "-")) {
			const char *flag_char = peek() + 1;
			char *sep = strchr(flag_char, '=');
			char *value = NULL;
			flag_t flag = {0};
			if (sep == NULL) {
				sep = strchr(flag_char, ':');
			}
			if (sep != NULL) {
				value = sep + 1;
				*sep = '\0';
			}

			flag = find_short_flag(flag_char);
			if (flag.type != FLAG_NONE) {
				is_flag = 1;
				i++;
				if (value == NULL && flag.type != FLAG_BOOL) {
					value = peek();
					consume();
				}
				parse_flag2(flag, value);
			}
		} else if (flags.stop_on_first_non_arg) {
			if (!is_flag) {
				(*argv)[write_pos] = (*argv)[i];
				write_pos++;
				i++;
			}
			break;
		}

		if (!is_flag) {
			(*argv)[write_pos] = (*argv)[i];
			write_pos++;
			i++;
		}
	}

	*argc = write_pos;
}

static void parse_flag2(flag_t flag, char *value)
{
	/* TODO: also this should work with `=` and `:` */
	if (flag.callback != NULL) {
		flag.callback(flag.dest, value);
		return;
	}

	switch (flag.type) {
	case FLAG_SIZE_T:
		/* TODO: Check for errors */
		*((size_t*)flag.dest) = strtoul(value, NULL, 10);
		break;
	case FLAG_INT:
		/* TODO: Check for errors */
		*((int*)flag.dest) = atoi(value);
		break;
	case FLAG_STRING:
		*((char **)flag.dest) = value;
		break;
	case FLAG_BOOL:
		*((int*)flag.dest) = 1;
		break;
	default:
		fprintf(stderr, "Reached unreachable case: %d\n", flag.type);
		abort();
	}
}

static flag_t find_long_flag(const char *const name)
{
	const flag_t fail = {0};
	size_t i = 0;
	for (i=0; i < flags.len; i += 1) {
		const flag_t flag = flags.items[i];
		if (equals(flag.long_name, name)) {
			return flag;
		}
	}
	return fail;
}

static flag_t find_short_flag(const char *const name)
{
	const flag_t fail = {0};
	size_t i = 0;
	for (i=0; i < flags.len; i += 1) {
		const flag_t flag = flags.items[i];
		if (equals(flag.short_name, name)) {
			return flag;
		}
	}
	return fail;
}

#undef ended
#undef peek
#undef consume
#undef starts_with

MY_FLAG_DEF void print_help_flag(FILE *stream)
{
	size_t i = 0;
	const char *indent = "  ";
	char buffer[MAX_FLAG_NAME_LEN * 2 + 67] = {0};
	size_t pos = 0;

	for (i=0; i < flags.len; i += 1) {
		flag_t flag = flags.items[i];
		fprintf(stream, "%s", indent);

		if (flag.long_name != NULL) {
			pos += sprintf(buffer + pos, "--%s ", flag.long_name);
		}
		if (flag.short_name != NULL) {
			pos += sprintf(buffer + pos, "-%s ", flag.short_name);
		}

		buffer[pos - 1] = '\0';
		fprintf(stream, "%-*s ", (int)flags.max_name_len, buffer);
		fprintf(stream, "%-10s ", flag.type == FLAG_BOOL ? "" : flag_type_as_str(flag.type));
		fprintf(stream, "%s\n", flag.description == NULL ? "" : flag.description);

		pos = 0;
		memset(buffer, 0, sizeof(char) * MAX_FLAG_NAME_LEN * 2);
	}
}

MY_FLAG_DEF void flag_stop_on_first_non_arg(void)
{
	flags.stop_on_first_non_arg = 1;
}

static void add_flag(flag_t flag)
{
	size_t len = 4;
	if (flags.len >= flags.cap) {
		flags.cap = flags.cap == 0 ? FLAGS_DEFAULT_CAP : flags.cap * 2;
		flags.items = realloc(flags.items, sizeof(flag_t) * flags.cap);
	}

	if (flag.long_name != NULL) {
		len += strlen(flag.long_name);
	}
	if (flag.short_name != NULL) {
		len += strlen(flag.short_name);
	}
	if (len > flags.max_name_len) {
		flags.max_name_len = len;
	}
	flags.items[flags.len] = flag;
	flags.len += 1;
}

static const char *flag_type_as_str(flag_type_t type)
{
	switch (type) {
	case FLAG_NONE:   return "<none>";
	case FLAG_SIZE_T: return "<number>";
	case FLAG_INT:    return "<number>";
	case FLAG_STRING: return "<string>";
	case FLAG_BOOL:   return "<bool>";
	}
}

#ifdef __cplus_cplus
}
#endif /* __cpluscplus */
#endif /* !MY_FLAGS_IMPL */

#endif /* !MY_FLAGS_H_ */
