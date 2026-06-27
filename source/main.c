#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <time.h>
#include <string.h>

#include "my_macro_abuse.h"
#include "my_termcolor.h"
#include "my_array.h"
#include "my_allocator.h"
#include "my_c_allocator.h"
#include "my_commons.h"
#include "my_stream.h"
#include "my_string.h"
#include "my_string_builder.h"
#include "my_string_view.h"

#include "subprocess.h"

typedef struct {
	size_t len, cap;
	size_t *items;
} size_array_t;

typedef struct chars_t {
	size_t len, cap;
	const char **items;
} chars_t;

typedef struct config_t {
	size_t amount;
	size_t delay;
	size_array_t is_codes;
	size_array_t not_codes;
	chars_t command;
	const char *programs_name;
} config_t;

static int format_string_t(stream_t stream, modifier_stream_t mod, va_list args);
static int format_string_view_t(stream_t stream, modifier_stream_t mod, va_list args);
static int format_string_builder_t(stream_t stream, modifier_stream_t mod, va_list args);

static void print_usage(config_t config);
static config_t parse_options(allocator_t allocator, int argc, char *argv[argc]);
static inline int actual_main(allocator_t allocator, config_t config);

static void print_chars_t(stream_t stream, const chars_t chars);

int main(int argc, char *argv[argc])
{
	setup_io_stream();

	define_format_specifier("string", format_string_t);
	define_format_specifier("sv", format_string_view_t);
	define_format_specifier("sb", format_string_builder_t);

	const allocator_t c_allocator = get_c_allocator();
	set_default_allocator(c_allocator);

	config_t config = parse_options(c_allocator, argc, argv);

	const int result = actual_main(c_allocator, config);

	arrfree(c_allocator, config.is_codes);
	arrfree(c_allocator, config.not_codes);
	arrfree(c_allocator, config.command);
	return result;
}

static inline int actual_main(allocator_t allocator, config_t config)
{
	unused(allocator);
	int result = 0;

	if (config.command.len <= 1) {
		print_usage(config);
		return 1;
	}

	const char **command = config.command.items;

	size_t retries = 0;
	int return_code;
	for (; ; retries += 1) {
		if (retries > 0) {
			eprintln(ANSI_CODE_RED "Existed with status of {int} (!= 0). this is the attempt of {usize}" ANSI_CODE_RESET, return_code, retries);
		}
		if (retries >= config.amount) {
			result = 1;
			eprintln(ANSI_CODE_RED "Exhausted all {usize} retries. Giving up." ANSI_CODE_RESET, config.amount);
			goto done;
		}

		if (config.delay > 0) {
			struct timespec ts;
			ts.tv_sec = config.delay / 1000;
			ts.tv_nsec = (config.delay % 1000) * 1000000;
			nanosleep(&ts, NULL);
		}

		struct subprocess_s subprocess;
		const int status = subprocess_create(
			command,
			subprocess_option_search_user_path
			| subprocess_option_inherit_environment
			| subprocess_option_enable_async
			| subprocess_option_enable_async_no_wait,
			&subprocess);
		if (status != 0) {
			result = 1;
			eprint(ANSI_CODE_RED "Can't launch command: " ANSI_CODE_RESET);
			print_chars_t(serr, config.command);
			goto done;
		}

		char buf[4096];
		unsigned int n;
		while (subprocess_alive(&subprocess)) {
			n = subprocess_read_stdout(&subprocess, buf, sizeof(buf));
			if (n) fwrite(buf, 1, n, stdout);
			n = subprocess_read_stderr(&subprocess, buf, sizeof(buf));
			if (n) fwrite(buf, 1, n, stderr);
		}

		// Drain remaining after join
		if (subprocess_join(&subprocess, &return_code) != 0) {
			result = 1;
			eprint(ANSI_CODE_RED "Failed to wait for the process:" ANSI_CODE_RESET);
			print_chars_t(serr, config.command);
			goto done;
		}

		while ((n = subprocess_read_stdout(&subprocess, buf, sizeof(buf)))) fwrite(buf,1,n,stdout);
		while ((n = subprocess_read_stderr(&subprocess, buf, sizeof(buf)))) fwrite(buf,1,n,stderr);

		if (config.is_codes.len == 0 && config.not_codes.len == 0) {
			if (return_code == 0) break;
		}

		int matched_is = 0;
		for (size_t i = 0; i < config.is_codes.len; i++) {
			if ((size_t)return_code == config.is_codes.items[i]) {
				matched_is = 1;
				break;
			}
		}

		int matched_not = 0;
		for (size_t i = 0; i < config.not_codes.len; i++) {
			if ((size_t)return_code == config.not_codes.items[i]) {
				matched_not = 1;
				break;
			}
		}

		if (config.is_codes.len > 0 && !matched_is) break;
		if (matched_not && !matched_is) break;
	}

	println(ANSI_CODE_RED "Finished after {usize} retries" ANSI_CODE_RESET, retries);

done:
	return result;
}

static void print_chars_t(stream_t stream, const chars_t chars)
{
	iarreach (i, chars) {
		if (chars.items[i] == NULL) continue;
		if (i == 0) {
			sprint(stream, "{s}", chars.items[i]);
		} else {
			sprint(stream, " {s}", chars.items[i]);
		}
	}
	sprintln(stream, "");
}

static void print_usage(config_t config)
{
	const string_view_t programs_name = sv_from_chars(config.programs_name);
	eprintln(ANSI_CODE_BOLD "Usage:" ANSI_CODE_RESET " {sv} <your command>", programs_name);
	eprintln("");
	eprintln(ANSI_CODE_BOLD "Example:" ANSI_CODE_RESET " {sv} echo \"Hello, World!\"", programs_name);
	eprintln("");
	eprintln(ANSI_CODE_BOLD "Description " ANSI_CODE_RED "(FUCK YOU INWI):" ANSI_CODE_RESET);
	eprintln("\t`{sv}` is a program which allows for re-executing the program until it succed. why I made this?"
	" The anwser is simple! which is just that my internet sucks ass. Imagine this: I'm trying to install a package"
	" called `foo` on arch. so the command is something like: sudo pacman -S foo. but because my internet is so ass. it will fail."
	" so I need to run that command manually again. and again. and again and again.. well, you get the idea."
	"so I made this program which will only require me to type `{sv} sudo pacman -S foo` and just let it retry until it succeed."
	, programs_name, programs_name);
	eprintln("");
	eprintln(ANSI_CODE_BOLD "Options:" ANSI_CODE_RESET);
	eprintln("\t--amount=<number>        How many times the command should execute before completely failing.");
	eprintln("\t--delay=<milliseconds>   Interval between each retry.");
	eprintln("\t--is=<code>[,<code>...]  Only retry if exit code matches (comma-separated or repeated).");
	eprintln("\t--not=<code>[,<code>...] Retry if exit code does NOT match (comma-separated or repeated).");
}

typedef void (*option_handler_t)(config_t *config, allocator_t allocator, string_view_t value, const char *raw);

typedef struct {
	const char *name;
	option_handler_t handler;
} option_entry_t;

static void handle_amount(config_t *config, allocator_t allocator, string_view_t value, const char *raw)
{
	unused(allocator);
	if (value.len == 0) panic("--amount= requires a value");
	char *endptr;
	unsigned long val = strtoul(value.data, &endptr, 10);
	if (endptr != value.data + value.len) panic("Invalid value for --amount: %s", raw);
	config->amount = (size_t)val;
}

static void handle_delay(config_t *config, allocator_t allocator, string_view_t value, const char *raw)
{
	unused(allocator);
	if (value.len == 0) panic("--delay= requires a value");
	char *endptr;
	unsigned long val = strtoul(value.data, &endptr, 10);
	if (endptr != value.data + value.len) panic("Invalid value for --delay: %s", raw);
	config->delay = (size_t)val;
}

static void push_uint_list(allocator_t allocator, size_array_t *arr, string_view_t value, const char *raw, const char *optname)
{
	const char *p = value.data;
	const char *end = value.data + value.len;
	while (p < end) {
		char *endptr;
		unsigned long val = strtoul(p, &endptr, 10);
		if (endptr == p) panic("Invalid value for %s: %s", optname, raw);
		arrpush(allocator, *arr, (size_t)val);
		p = endptr;
		if (p < end && *p == ',') p++;
	}
}

static void handle_is(config_t *config, allocator_t allocator, string_view_t value, const char *raw)
{
	if (value.len == 0) panic("--is= requires a value");
	push_uint_list(allocator, &config->is_codes, value, raw, "--is");
}

static void handle_not(config_t *config, allocator_t allocator, string_view_t value, const char *raw)
{
	if (value.len == 0) panic("--not= requires a value");
	push_uint_list(allocator, &config->not_codes, value, raw, "--not");
}

static const option_entry_t OPTIONS[] = {
	{ "--amount=", handle_amount },
	{ "--delay=",  handle_delay },
	{ "--is=",     handle_is },
	{ "--not=",    handle_not },
};

static config_t parse_options(allocator_t allocator, int argc, char *argv[argc])
{
	config_t config = {
		.amount = SIZE_MAX,
		.delay = 0,
		.is_codes = {0},
		.not_codes = {0},
		.command = {0},
		.programs_name = argv[0],
	};

	bool inserting_commands = false;
	for (int i = 1; i < argc; i++) {
		string_view_t arg = sv_from_chars(argv[i]);
		int matched = 0;

		if (!inserting_commands) {
			for (size_t j = 0; j < sizeof(OPTIONS) / sizeof(OPTIONS[0]); j++) {
				if (sv_starts_with(arg, OPTIONS[j].name)) {
					string_view_t value = sv_substr(arg, strlen(OPTIONS[j].name), arg.len);
					OPTIONS[j].handler(&config, allocator, value, argv[i]);
					matched = 1;
					break;
				}
			}
		}

		if (!matched || inserting_commands) {
			inserting_commands = true;
			arrpush(allocator, config.command, argv[i]);
		}
	}

	arrpush(allocator, config.command, NULL);

	return config;
}

static int format_string_t(stream_t stream, modifier_stream_t mod, va_list args)
{
	unused(mod);
	string_t string = va_arg(args, string_t);
	return sprint(stream, "{s:*}", string.data, (int)string.len);
}

static int format_string_view_t(stream_t stream, modifier_stream_t mod, va_list args)
{
	unused(mod);
	string_view_t string_view = va_arg(args, string_view_t);
	return sprint(stream, "{s:*}", string_view.data, (int)string_view.len);
}

static int format_string_builder_t(stream_t stream, modifier_stream_t mod, va_list args)
{
	unused(mod);
	string_builder_t string_builder = va_arg(args, string_builder_t);
	return sprint(stream, "{s:*}", string_builder.data, (int)string_builder.len);
}
