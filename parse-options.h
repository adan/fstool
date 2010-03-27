#ifndef PARSE_OPTIONS_H
#define PARSE_OPTIONS_H

enum option_type {
	// special types
	OPTION_END,
	OPTION_GROUP,
	OPTION_NUMBER,
	// options with no arguments
	OPTION_INC,
	// options with arguments
	OPTION_STRING,
	OPTION_INTEGER,
	OPTION_CALLBACK,
};

enum option_flag {
	OPTION_OPTARG = 1,
	OPTION_NOARG = 2,
};

enum parse_option_flag {
	POF_KEEP_ARGV0 = 1,
	POF_KEEP_DASHDASH = 2,
	POF_STOP_AT_NON_OPTION = 4,
};

enum { // parse option error code
	POE_ERROR = -1,
	POE_OPTION_UNKNOWN = -2,
	POE_NO_MEM = -3,
};

struct option;
typedef int parse_option_callback(const struct option* opt, const char* arg, int unset);

struct option {
	enum option_type type;
	int short_name;
	const char* long_name;
	void* value;
	const char* argh;
	const char* help;
	int flags;
	parse_option_callback* callback;
	union { // Use union of void* and int instead of intptr_t in git.
		void* ptr;
		int integer;
	} def_val;
};

#define OPT_END()                   {OPTION_END}
#define OPT_GROUP(h)                {OPTION_GROUP, 0, NULL, NULL, NULL, (h)}
#define OPT_INC(s, l, v, h)         {OPTION_INC, (s), (l), (v), NULL, (h), OPTION_NOARG}
#define OPT_STRING(s, l, v, a, h)   {OPTION_STRING, (s), (l), (void*)(v), (a), (h)}
#define OPT_INTEGER(s, l, v, h)     {OPTION_INTEGER, (s), (l), (v), "n", (h)}
#define OPT_CALLBACK(s, l, v, a, h, f) \
	{OPTION_CALLBACK, (s), (l), (void*)(v), (a), (h), 0, (f)}
#define OPT_NUMBER_CALLBACK(v, h, f) \
	{OPTION_NUMBER, 0, NULL, (v), NULL, (h), OPTION_NOARG, (f)}

/**
 * parse_options will process the options and leave the non-option arguments
 * in argv[].
 * Returns the number of arguments left in argv[].
 */
int parse_options(
	int argc, const char** argv, const struct option* opts,
	const char* const* usage_str, int flags
);

void usage_with_options(const char* const* usage_str, const struct option* opts);

/**
 * Advanced APIs
 */

struct parse_option_context {
	const char** argv;
	const char** out;
	int argc, index;
	const char* opt;
	int flags;
};

void parse_options_start(
	struct parse_option_context* c,
	int argc, const char** argv, int flags
);

int parse_options_step(
	struct parse_option_context* c,
	const struct option* opts, const char* const* usage_str
);

int parse_options_end(struct parse_option_context* c);

#endif // #ifndef PARSE_OPTIONS_H
