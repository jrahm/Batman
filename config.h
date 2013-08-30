#ifndef CONFIG_H_
#define CONFIG_H_

#include <stdlib.h>
#include <unistd.h>

/* Config Files are in the following format:
 *
 * [section]
 * attributte = <string> <number> ...
 *
 */

#define CONFIG_PARSER_BUFFER_LEN 1024

struct StringList {
	char* data;
	struct StringList* next;
};

/*
 * Struct that holds the data we need
 * to parse a configuration file
 */
struct ConfigParser {
	/* The current section
	 * the config parser is
	 * on */
	char* current_section;

	/* The current attribute
	 * the config parser
	 * is on */
	char* current_attribute;

	/* The file descriptor
	 * this config parser
	 * is reading from */
	int fd;

	/* We read into these buffers */
	char mbuffer[ CONFIG_PARSER_BUFFER_LEN ];

	size_t buffer_len;
	size_t buffer_ptr;

	struct StringList* conf_line;

	/* Error if the config parser
	 * does not succeed */
	char* error;
};

/*
 * Null the struct, and initialize the buffer
 */
void ConfigParser_init( struct ConfigParser* parser, int fd );

/*
 * Frees all internal comoponents of the parser, but does
 * not free the parser itself
 */
void ConfigParser_Delete( struct ConfigParser* parser );

/* Read the next string */
char* ConfigParser_NextString( struct ConfigParser* parser );

/* Skips to the next attribute */

/* An explicit call to NextAttributte _must_ be made inn
 * order for the next values to be read. If there is no more
 * data to be read from an attribute, then the other calls
 * to NextStrings will return their error codes
 */
void ConfigParser_NextAttributte( struct ConfigParser* parser );

/* Attempts to parse all Decimal, Hexidecimal, Binary, and Octal numbers */
/* Returns 0 on success and 1 on error */
int ConfigParser_NextInteger( struct ConfigParser* parser, int* i );

/* returns 0 on success and 1 on error */
int ConfigParser_NextIntegerFromRadix( struct ConfigParser* parser, int* i, int radix );

/* Read the next integer,e xcept if it fails, return def */
int ConfigParser_NextIntegerWithDefault( struct ConfigParser* parser, int def );

/* Skip the next token */
int ConfigParser_SkipToken( struct ConfigParser* parser );

#endif
