#include <stdio.h>

#include "my_array.h"
#include "my_allocator.h"
#include "my_c_allocator.h"
#include "my_commons.h"
#include "my_stream.h"
#include "my_string.h"
#include "my_string_builder.h"
#include "my_string_view.h"

typedef struct args_t {
	size_t len, cap;
	string_view_t *items;
} args_t;

static int format_string_t(stream_t stream, modifier_stream_t mod, va_list args);
static int format_string_view_t(stream_t stream, modifier_stream_t mod, va_list args);
static int format_string_builder_t(stream_t stream, modifier_stream_t mod, va_list args);

static void print_usage(const args_t args);
static inline int actual_main(const args_t args);

int main(const int argc, const char *const argv[argc]) {
	setup_io_stream();
	define_format_specifier("string", format_string_t);
	define_format_specifier("sv", format_string_view_t);
	define_format_specifier("sb", format_string_builder_t);

	const allocator_t c_allocator = get_c_allocator();
	set_default_allocator(c_allocator);

	args_t arguments = {0};
	for (int i=0; i < argc; i += 1) {
		arrpush(c_allocator, arguments, sv_from_chars(argv[i]));
	}

	const int result = actual_main(arguments);

	arrfree(c_allocator, arguments);
	return result;
}

static inline int actual_main(const args_t args)
{
	if (args.len == 0) {
		print_usage(args);
		return 1;
	}

	/* TODO: implement --amount --delay */
	return 0;
}

static void print_usage(const args_t args)
{
	const string_view_t programs_name = args.items[0];
	sprintln(serr, "Usage: {sv} <your command>", programs_name);
	sprintln(serr, "Example: {sv} echo \"Hello, World!\"", programs_name);
	sprintln(serr, "Description (FUCK YOU INWI):");
	sprintln(serr, "\t`{sv}` is a program which allows for re-executing the program until it succed. why I made this?"
	"The anwser is simple! which is just that my internet sucks ass. Imagine this: I'm trying to install a package"
	" called `foo` on arch. so the command is something like: sudo pacman -S foo. but because my internet is so ass. it will fail."
	" so I need to run that command manually again. and again. and again and again.. well, you get the idea."
	"so I made this program which will only require me to type `{sv} sudo pacman -S foo` and just let it retry until it succeed."
	, programs_name, programs_name);
	sprintln(serr, "Options:");
	sprintln(serr, "\t--amount=<number>        How many times the command should execute before completely failing.");
	sprintln(serr, "\t--delay=<milliseconds>   Interval between each retry.");
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
