/**
 * parse options test
 *
 * Copyright (c) 2010 lixiaofan
 *
 * @author lixiaofan <gerseay@gmail.com>
 */

#include <stdlib.h>
#include <string.h>
#include "test-assert.h"
#include "parse-options.h"

#define ARG_COUNT(argv) (sizeof(argv) / sizeof((argv)[0]) - 1)

static int boolean = 0;
static int another_boolean = 0;
static int only_long = 0;
static int show_help = 0;
static int integer = 0;
static int number = 0;
static const char* string = NULL;
static const char* callback = NULL;

static int test_cb(const struct option* opt, const char* arg, int unset);
static int number_cb(const struct option* opt, const char* arg, int unset);

struct option options[] = {
	OPT_GROUP("Boolean group"),
	OPT_INC('b', "boolean", &boolean, "get a boolean"),
	OPT_INC('a', "another-boolean", &another_boolean, "get another boolean"),
	OPT_INC(0, "only-long", &only_long, "only long option to get a boolean"),
	OPT_INC('h', "help", &show_help, "display this help and exit"),
	OPT_GROUP("With argument group"),
	OPT_STRING('s', "string", &string, "string", "get a string"),
	OPT_INTEGER('i', "integer", &integer, "get a integer"),
	OPT_GROUP("Callback group"),
	OPT_CALLBACK('c', "callback", &callback, "str", "get a string in callback", test_cb),
	OPT_NUMBER_CALLBACK(&number, "get a number", number_cb),
	OPT_END()
};

void reset_values()
{
	boolean = 0;
	another_boolean = 0;
	only_long = 0;
	show_help = 0;
	integer = 0;
	number = 0;
	string = NULL;
	callback = NULL;
}

void test_no_arg()
{
	const char* argv[] = {"self", NULL};
	int argc = ARG_COUNT(argv);
	reset_values();
	argc = parse_options(argc, argv, options, NULL, 0);
	TEST_ASSERT(!argc);
	TEST_ASSERT(!boolean && !another_boolean && !only_long && !show_help);
	TEST_ASSERT(!integer && !string);
}

void test_boolean_arg()
{
	const char* argv[] = {"self", "-b", NULL};
	int argc = ARG_COUNT(argv);
	reset_values();
	argc = parse_options(argc, argv, options, NULL, 0);
	TEST_ASSERT(!argc);
	TEST_ASSERT(boolean && !another_boolean && !only_long && !show_help);
	TEST_ASSERT(!integer && !string);
}

void test_string_arg()
{
	const char* argv[] = {"self", "-s", "string value", NULL};
	int argc = ARG_COUNT(argv);
	reset_values();
	argc = parse_options(argc, argv, options, NULL, 0);
	TEST_ASSERT(!argc);
	TEST_ASSERT(!boolean && !another_boolean && !only_long && !show_help);
	TEST_ASSERT(!integer);
	TEST_ASSERT(!strcmp(string, "string value"));
}

void test_integer_arg()
{
	const char* argv[] = {"self", "-i", "77", NULL};
	int argc = ARG_COUNT(argv);
	reset_values();
	argc = parse_options(argc, argv, options, NULL, 0);
	TEST_ASSERT(!argc);
	TEST_ASSERT(!boolean && !another_boolean && !only_long && !show_help);
	TEST_ASSERT(integer == 77);
	TEST_ASSERT(!string);
}

void test_long_option()
{
	const char* argv[] = {"self",
		"--boolean", "--only-long",
		"--string", "string value", "--integer", "77",
		NULL
	};
	int argc = ARG_COUNT(argv);
	reset_values();
	argc = parse_options(argc, argv, options, NULL, 0);
	TEST_ASSERT(!argc);
	TEST_ASSERT(boolean && !another_boolean && only_long && !show_help);
	TEST_ASSERT(integer == 77);
	TEST_ASSERT(!strcmp(string, "string value"));
}

void test_multiple_short()
{
	const char* argv[] = {"self", "-bsstring value", "-ai77", NULL};
	int argc = ARG_COUNT(argv);
	reset_values();
	argc = parse_options(argc, argv, options, NULL, 0);
	TEST_ASSERT(!argc);
	TEST_ASSERT(boolean && another_boolean && !only_long && !show_help);
	TEST_ASSERT(integer == 77);
	TEST_ASSERT(!strcmp(string, "string value"));
}

void test_equal_char()
{
	const char* argv[] = {"self", "--string=test", "--integer=77", NULL};
	int argc = ARG_COUNT(argv);
	reset_values();
	argc = parse_options(argc, argv, options, NULL, 0);
	TEST_ASSERT(!argc);
	TEST_ASSERT(!boolean && !another_boolean && !only_long && !show_help);
	TEST_ASSERT(integer == 77);
	TEST_ASSERT(!strcmp(string, "test"));
}

void test_keep_argv0()
{
	const char* argv[] = {"self", "-ab", NULL};
	int argc = ARG_COUNT(argv);
	reset_values();
	argc = parse_options(argc, argv, options, NULL, POF_KEEP_ARGV0);
	TEST_ASSERT(argc == 1);
	TEST_ASSERT(!strcmp(argv[0], "self"));
	TEST_ASSERT(boolean && another_boolean && !only_long && !show_help);
	TEST_ASSERT(!integer && !string);
}

void test_dash_dash()
{
	const char* argv[] = {"self", "-i77", "--", NULL};
	int argc = ARG_COUNT(argv);
	reset_values();
	argc = parse_options(argc, argv, options, NULL, POF_KEEP_DASHDASH);
	TEST_ASSERT(argc == 1);
	TEST_ASSERT(!strcmp(argv[0], "--"));
	TEST_ASSERT(!boolean && !another_boolean && !only_long && !show_help);
	TEST_ASSERT(integer == 77);
	TEST_ASSERT(!string);
}

void test_remain_arg()
{
	const char* argv[] = {"self", "--only-long", "not option", "another", NULL};
	int argc = ARG_COUNT(argv);
	reset_values();
	argc = parse_options(argc, argv, options, NULL, 0);
	TEST_ASSERT(argc == 2);
	TEST_ASSERT(!strcmp(argv[0], "not option"));
	TEST_ASSERT(!strcmp(argv[1], "another"));
	TEST_ASSERT(!boolean && !another_boolean && only_long && !show_help);
	TEST_ASSERT(!integer && !string);
}

void test_unknown()
{
	const char* argv[] = {"self", "-be", NULL};
	int argc = ARG_COUNT(argv);
	const char* argv_long[] = {"self", "--only-long", "--error", NULL};
	int argc_long = ARG_COUNT(argv_long);
	argc = parse_options(argc, argv, options, NULL, 0);
	TEST_ASSERT(argc < 0);
	argc_long = parse_options(argc_long, argv_long, options, NULL, 0);
	TEST_ASSERT(argc_long < 0);
}

static int test_cb(const struct option* opt, const char* arg, int unset)
{
	*((const char**)opt->value) = arg;
	return 0;
}

void test_callback()
{
	const char* argv[] = {"self", "-c", "the callback string", NULL};
	int argc = ARG_COUNT(argv);
	argc = parse_options(argc, argv, options, NULL, 0);
	TEST_ASSERT(!argc);
	TEST_ASSERT(!strcmp(callback, "the callback string"));
}

static int number_cb(const struct option* opt, const char* arg, int unset)
{
	*((int*)opt->value) = strtol(arg, NULL, 10);
	return 0;
}

void test_number_callback()
{
	const char* argv[] = {"self", "-15", NULL};
	int argc = ARG_COUNT(argv);
	argc = parse_options(argc, argv, options, NULL, 0);
	TEST_ASSERT(!argc);
	TEST_ASSERT(number == 15);
}

void test_user_input(int argc, const char** argv)
{
	int i;
	reset_values();
	argc = parse_options(argc, argv, options, NULL, 0);
	if (show_help) {
		const char* usage[] = {"test-parse-options [OPTION]...", NULL};
		usage_with_options(usage, options);
	}
	else if (argc >= 0) {
		printf("argc(after): %d\n", argc);
		printf("boolean:     %d\n", boolean);
		printf("another:     %d\n", another_boolean);
		printf("only-long:   %d\n", only_long);
		printf("show-help:   %d\n", show_help);
		printf("string:      %s\n", string ? string : "(null)");
		printf("integer:     %d\n", integer);
		printf("callback:    %s\n", callback ? callback : "(null)");
		printf("number:      %d\n", number);
		for (i = 0; i < argc; ++i) {
			printf("arg %d: %s\n", i, argv[i]);
		}
	}
}

int main(int argc, const char* argv[])
{
	test_no_arg();
	test_boolean_arg();
	test_string_arg();
	test_integer_arg();
	test_long_option();
	test_multiple_short();
	test_equal_char();
	test_keep_argv0();
	test_dash_dash();
	test_remain_arg();
	test_unknown();
	test_callback();
	test_number_callback();

	if (argc > 1) {
		test_user_input(argc, argv);
	}
	return 0;
}
