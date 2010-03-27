/**
 * parse options
 *
 * Copyright (c) 2010 lixiaofan
 *
 * @author lixiaofan <gerseay@gmail.com>
 * @see git parse-options
 */

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parse-options.h"

enum {
	OPT_SHORT = 1,
};

static void xexit(const char* format, ...)
{
	char msg[4096];
	va_list params;

	va_start(params, format);
	// The vsnprintf is non-standard, which cannot be compiled in msvc6.
	//vsnprintf(msg, sizeof(msg), format, params);
	vsprintf(msg, format, params);
	va_end(params);
	fprintf(stderr, "%s\n", msg);
	exit(EXIT_FAILURE);
}

/**
 * Allocates (len + 1) bytes of memory, duplicates "len" bytes of "data" to
 * the allocated memory, zero terminates the allocated memory, and returns
 * a pointer to the allocated memory.
 * If the allocation fails, return NULL.
 */
static void* xmemdupz(const void* data, size_t len)
{
	void* ret;

	if (len + 1 < len) { // len is too large
		return NULL;
	}
	ret = malloc(len + 1);
	if (!ret) {
		return NULL;
	}
	((char*)ret)[len] = 0;
	memcpy(ret, data, len);
	return ret;
}

static const char* skip_prefix(const char* str, const char* prefix)
{
	size_t len = strlen(prefix);
	return strncmp(str, prefix, len) ? NULL : str + len;
}

static int option_error(const struct option* opt, const char* reason, int flags)
{
	if (flags & OPT_SHORT) {
		fprintf(stderr, "error: switch '%c' %s\n", opt->short_name, reason);
		return POE_ERROR;
	}
	fprintf(stderr, "error: option '%s' %s\n", opt->long_name, reason);
	return POE_ERROR;
}

static int get_arg(
	struct parse_option_context* c,
	const struct option* opt, int flags, const char** arg
)
{
	if (c->opt) {
		*arg = c->opt;
		c->opt = NULL;
	}
	else if (c->argc > 1) {
		--c->argc;
		*arg = *++c->argv;
	}
	else {
		return option_error(opt, "requires a value", flags);
	}
	return 0;
}

static int get_value(struct parse_option_context* c, const struct option* opt, int flags)
{
	const char* arg;
	char* s;

	if (!(flags & OPT_SHORT) && c->opt) { // "--opt=val"
		switch (opt->type) {
		case OPTION_CALLBACK:
			if (!(opt->flags & OPTION_NOARG)) {
				break;
			}
			// Fall through
		case OPTION_INC:
			return option_error(opt, "takes no value", flags);
		default: break;
		}
	}

	switch (opt->type) {
	case OPTION_INC:
		*(int*)opt->value += 1;
		return 0;
	case OPTION_STRING:
		if ((opt->flags & OPTION_OPTARG) && !c->opt) {
			*(const char**)opt->value = (const char*)opt->def_val.ptr;
		}
		else {
			return get_arg(c, opt, flags, (const char**)opt->value);
		}
		return 0;
	case OPTION_INTEGER:
		if ((opt->flags & OPTION_OPTARG) && !c->opt) {
			*(int*)opt->value = opt->def_val.integer;
			return 0;
		}
		if (get_arg(c, opt, flags, &arg)) {
			return POE_ERROR;
		}
		*(int*)opt->value = strtol(arg, &s, 10);
		if (*s) {
			return option_error(opt, "expects a numerical value", flags);
		}
		return 0;
	case OPTION_CALLBACK:
		if (opt->flags & OPTION_NOARG) {
			return opt->callback(opt, NULL, 0) ? POE_ERROR : 0;
		}
		if ((opt->flags & OPTION_OPTARG) && !c->opt) {
			return opt->callback(opt, NULL, 0) ? POE_ERROR : 0;
		}
		if (get_arg(c, opt, flags, &arg)) {
			return POE_ERROR;
		}
		return opt->callback(opt, arg, 0) ? POE_ERROR : 0;
	default:
		xexit("Fatal: should not happen.");
		return POE_ERROR; // In case the compiler warning...
	}
}

static int parse_short_opt(struct parse_option_context* c, const struct option* opts)
{
	const struct option* num_opt = NULL;

	for (; opts->type != OPTION_END; ++opts) {
		if (opts->short_name == *c->opt) {
			c->opt = c->opt[1] ? c->opt + 1 : NULL;
			return get_value(c, opts, OPT_SHORT);
		}
		// Handle the numerical option later.
		if (opts->type == OPTION_NUMBER) {
			num_opt = opts;
		}
	}
	if (num_opt && isdigit(*c->opt)) {
		size_t len = 1;
		char* arg;
		int rc;

		for (; isdigit(c->opt[len]); ++len) {
		}
		arg = xmemdupz(c->opt, len);
		if (!arg) {
			return POE_NO_MEM;
		}
		c->opt = c->opt[len] ? c->opt + len : NULL;
		rc = num_opt->callback(num_opt, arg, 0) ? POE_ERROR : 0;
		free(arg);
		return rc;
	}
	return POE_OPTION_UNKNOWN;
}

static int parse_long_opt(
	struct parse_option_context* c,
	const char* arg, const struct option* opts
)
{
	const char* arg_end = strchr(arg, '=');

	if (!arg_end) {
		arg_end = arg + strlen(arg);
	}
	for (; opts->type != OPTION_END; ++opts) {
		const char* rest;

		if (!opts->long_name) {
			continue;
		}
		rest = skip_prefix(arg, opts->long_name);
		if (!rest) {
			continue;
		}
		if (*rest) {
			if (*rest != '=') {
				continue;
			}
			c->opt = rest + 1;
		}
		return get_value(c, opts, 0);
	}
	return POE_OPTION_UNKNOWN;
}

void parse_options_start(
	struct parse_option_context* c,
	int argc, const char** argv, int flags
)
{
	memset(c, 0, sizeof(*c));
	c->argc = argc - 1;
	c->argv = argv + 1;
	c->out = argv;
	c->index = (flags & POF_KEEP_ARGV0) ? 1 : 0;
	c->flags = flags;
}

int parse_options_step(
	struct parse_option_context* c,
	const struct option* opts, const char* const* usage_str
)
{
	c->opt = NULL; // reset c->opt

	for (; c->argc; --c->argc, ++c->argv) {
		const char* arg = c->argv[0];

		if (arg[0] != '-' || !arg[1]) { // Non option or "-"
			if (c->flags & POF_STOP_AT_NON_OPTION) {
				break;
			}
			c->out[c->index++] = c->argv[0];
			continue;
		}

		// short option
		if (arg[1] != '-') {
			c->opt = arg + 1;
			do {
				switch (parse_short_opt(c, opts)) {
				case POE_ERROR:
					return POE_ERROR;
				case POE_OPTION_UNKNOWN:
					goto unknown;
				case POE_NO_MEM:
					return POE_NO_MEM;
				default: break;
				}
			} while (c->opt);
			continue;
		}

		if (!arg[2]) { // "--"
			if (!(c->flags & POF_KEEP_DASHDASH)) {
				--c->argc;
				++c->argv;
			}
			break;
		}

		// long option
		switch (parse_long_opt(c, arg + 2, opts)) {
		case POE_ERROR:
			return POE_ERROR;
		case POE_OPTION_UNKNOWN:
			goto unknown;
		default: break;
		}
		continue;
unknown:
		return POE_OPTION_UNKNOWN;
	}
	return 0;
}

int parse_options_end(struct parse_option_context* c)
{
	memmove((void*)(c->out + c->index), c->argv, c->argc * sizeof(*c->out));
	c->out[c->index + c->argc] = NULL;
	return c->index + c->argc;
}

int parse_options(
	int argc, const char** argv, const struct option* opts,
	const char* const* usage_str, int flags
)
{
	struct parse_option_context c;
	int rc;

	parse_options_start(&c, argc, argv, flags);
	rc = parse_options_step(&c, opts, usage_str);
	switch (rc) {
	case 0:
		break;
	case POE_OPTION_UNKNOWN:
		if (c.argv[0][1] == '-') {
			fprintf(stderr, "error: unknown option '%s'\n", c.argv[0]);
		}
		else {
			fprintf(stderr, "error: unknown switch '%c'\n", *c.opt);
		}
		return POE_OPTION_UNKNOWN;
	default:
		return rc;
	}
	return parse_options_end(&c);
}

#define USAGE_OPT_WIDTH 24
#define USAGE_GAP 2

void usage_with_options(const char* const* usage_str, const struct option* opts)
{
	if (usage_str) {
		fprintf(stderr, "usage: %s\n", *usage_str++);
		while (*usage_str && **usage_str) {
			fprintf(stderr, "   or: %s\n", *usage_str++);
		}
		while (*usage_str) {
			fprintf(stderr, "%s%s\n",
				**usage_str ? "    " : "", *usage_str
			);
			++usage_str;
		}
	}

	if (opts->type != OPTION_GROUP) {
		fputc('\n', stderr);
	}

	for (; opts->type != OPTION_END; ++opts) {
		size_t pos, pad;

		if (opts->type == OPTION_GROUP) {
			fputc('\n', stderr);
			if (*opts->help) {
				fprintf(stderr, "%s\n", opts->help);
			}
			continue;
		}
		pos = fprintf(stderr, "  ");
		if (opts->short_name) {
			pos += fprintf(stderr, "-%c", opts->short_name);
		}
		if (opts->long_name) {
			if (opts->short_name) {
				pos += fprintf(stderr, ", ");
			}
			else {
				pos += fprintf(stderr, "    ");
			}
			pos += fprintf(stderr, "--%s", opts->long_name);
		}
		if (opts->type == OPTION_NUMBER) {
			pos += fprintf(stderr, "-NUM");
		}
		if (!(opts->flags & OPTION_NOARG)) {
			pos += fprintf(stderr, " <%s>", opts->argh);
		}
		if (pos <= USAGE_OPT_WIDTH) {
			pad = USAGE_OPT_WIDTH - pos;
		}
		else {
			fputc('\n', stderr);
			pad = USAGE_OPT_WIDTH;
		}
		fprintf(stderr, "%*s%s\n", pad + USAGE_GAP, "", opts->help);
	}
}
