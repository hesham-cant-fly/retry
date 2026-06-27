/**
 * @file cli_tools.h
 * @brief CLI argument parsing with subcommands and options.
 *
 * Supports long options (`--name=value` or `--name value`), short options
 * (`-n value`), flags (`--flag`, `-f`), and subcommand dispatch.
 *
 * Define `CLI_TOOLS_IMPLEMENTATION` in exactly one translation unit
 * to generate the implementation.
 *
 * Define `CLI_TOOLS_DEF` to `static` before inclusion to give all functions
 * internal linkage (STB-style single-TU usage).
 */

#ifndef __CLI_TOOLS_H
#define __CLI_TOOLS_H

/* #define CLI_TOOLS_IMPLEMENTATION */

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef CLI_TOOLS_DEF
#  define CLI_TOOLS_DEF
#endif /* !CLI_TOOLS_DEF */

/**
 * @brief Describes a single subcommand.
 */
typedef struct subcommand_t {
	const char *subcommand;
	int (*handle)(int argc, char **argv);
	const char *description;
} subcommand_t;

/**
 * @brief Kind of an option: flag (no value) or string (expects value).
 */
typedef enum OptionKind {
	OPT_FLAG,
	OPT_STRING
} option_kind_t;

/**
 * @brief Describes a single command-line option.
 */
typedef struct option_t {
	const char *long_name;
	char shortname;
	option_kind_t kind;
	void *out;
} option_t;

/**
 * @brief Print formatted usage text to stdout.
 * @param cli_name     Name of the program (argv[0]).
 * @param subcommands  Array of subcommand descriptors.
 * @param len          Number of subcommands.
 */
CLI_TOOLS_DEF void print_usage(const char *cli_name, subcommand_t *subcommands, size_t len);

/**
 * @brief Match and dispatch a subcommand.
 * @param argc         Argument count.
 * @param argv         Argument vector.
 * @param subcommands  Array of subcommand descriptors.
 * @param len          Number of subcommands.
 * @return Exit code from the matched subcommand, or 1 if no match.
 */
CLI_TOOLS_DEF int process_subcommands(int argc, char **argv, subcommand_t *subcommands, size_t len);

/**
 * @brief Parse options from argv, rearranging in place.
 *
 * Consumed options are removed; positional arguments remain at indices
 * 1..argc-1. `argv` is modified in place.
 * @param argc     Pointer to argument count (will be updated).
 * @param argv     Pointer to argument vector (will be rearranged).
 * @param options  Null-terminated array of `option_t` descriptors
 *                 (last entry has `long_name == NULL`).
 * @return true on success, false on unknown option (error printed to stderr).
 */
CLI_TOOLS_DEF bool parse_options(int *argc, char ***argv, option_t *options);

#ifdef CLI_TOOLS_IMPLEMENTATION
CLI_TOOLS_DEF void print_usage(const char *cli_name, subcommand_t *subcommands, size_t len) {
	size_t i;
	size_t size;
	subcommand_t subcommand;
	const size_t gap = 3;
	size_t longest_command_size = 0;
	printf("Usage: %s [command] [options]\n\n", cli_name);
	printf("Commands:\n");

	for (i = 0; i < len; ++i) {
		subcommand = subcommands[i];
		size = strlen(subcommand.subcommand);
		if (size > longest_command_size) {
			longest_command_size = size;
		}
	}

	for (i = 0; i < len; ++i) {
		subcommand = subcommands[i];
		printf("  %-*s%s\n", (int)(longest_command_size + gap),
			   subcommand.subcommand, subcommand.description);
	}
}

CLI_TOOLS_DEF int process_subcommands(int argc, char **argv, subcommand_t *subcommands,
					   size_t len) {
	size_t i;
	subcommand_t subcommand;
	int result;
	for (i = 0; i < len; ++i) {
		subcommand = subcommands[i];
		if (strcmp(argv[1], subcommand.subcommand) == 0) {
			result = subcommand.handle(argc - 2, argv + 2);
			return result;
		}
	}

	return 1;
}

static option_t *find_long(option_t *options, const char *name);
static option_t *find_short(option_t *options, char c);
static int set_option(option_t *o, char *val, int *i, int argc, char **argv);
static int process_long_option(char *arg, int *i, int argc, char **argv,
							   option_t *options);
static int process_short_options(char *arg, int *i, int argc, char **argv,
								 option_t *options);

CLI_TOOLS_DEF bool parse_options(int *argc, char ***argv, option_t *options) {
	int i;
	int j;
	int write_idx = 1;
	char *arg;
	for (i = 1; i < *argc; i++) {
		arg = (*argv)[i];

		if (strcmp(arg, "--") == 0) {
			for (j = i + 1; j < *argc; j++)
				(*argv)[write_idx++] = (*argv)[j];
			break;
		} else if (strncmp(arg, "--", 2) == 0) {
			if (!process_long_option(arg, &i, *argc, *argv, options))
				return false;
		} else if (arg[0] == '-' && arg[1]) {
			if (!process_short_options(arg, &i, *argc, *argv, options))
				return false;
		} else {
			(*argv)[write_idx++] = arg;
		}
	}
	*argc = write_idx;
	return true;
}

static option_t *find_long(option_t *options, const char *name) {
	option_t *o;
	for (o = options; o->long_name; o++) {
		if (strcmp(o->long_name, name) == 0)
			return o;
	}
	return NULL;
}

static option_t *find_short(option_t *options, char c) {
	option_t *o;
	for (o = options; o->long_name; o++) {
		if (o->shortname == c)
			return o;
	}
	return NULL;
}

static int set_option(option_t *o, char *val, int *i, int argc, char **argv) {
	if (o->kind == OPT_FLAG) {
		*(int *)o->out = 1;
		return 1;
	}
	if (o->kind == OPT_STRING) {
		if (val) {
			*(const char **)o->out = val;
			return 1;
		}
		if (*i + 1 < argc) {
			*(const char **)o->out = argv[++(*i)];
			return 1;
		}
		fprintf(stderr, "Option requires a value: --%s\n", o->long_name);
		return 0;
	}
	return 0;
}

static int process_long_option(char *arg, int *i, int argc, char **argv,
							   option_t *options) {
	char *opt;
	char *val;
	option_t *o;
	opt = arg + 2;
	val = strchr(opt, '=');
	if (val) {
		*val = '\0';
		val++;
	}

	o = find_long(options, opt);
	if (!o) {
		fprintf(stderr, "Unknown option: --%s\n", opt);
		return 0;
	}
	return set_option(o, val, i, argc, argv);
}

static int process_long_option(char *arg, int *i, int argc, char **argv,
							   option_t *options);
static int process_short_options(char *arg, int *i, int argc, char **argv,
								 option_t *options) {
	int j;
	option_t *o;
	char *val;
	for (j = 1; arg[j]; j++) {
		o = find_short(options, arg[j]);
		if (!o) {
			fprintf(stderr, "Unknown option: -%c\n", arg[j]);
			return 0;
		}
		val = NULL;
		if (o->kind == OPT_STRING) {
			if (arg[j + 1]) {
				val = &arg[j + 1];
				j = strlen(arg) - 1;
			}
		}
		if (!set_option(o, val, i, argc, argv))
			return 0;
	}
	return 1;
}

#endif /* CLI_TOOLS_IMPLEMENTATION */

#endif /* !__CLI_TOOLS_H */
