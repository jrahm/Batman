#include "config.h"

#include <stdlib.h>
#include <unistd.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>


/*
 * Null the struct, and initialize the buffer
 */
void ConfigParser_init( struct ConfigParser* parser, int fd ) {
	parser->fd = fd;
	parser->current_section = NULL;
	parser->current_attribute = NULL;
	parser->buffer_len = 0;
	parser->buffer_ptr = 0;
	parser->error = NULL;
};

size_t __ConfigParser_ReadBlock( struct ConfigParser* parser ) {
	parser->buffer_ptr = 0;
	return parser->buffer_len = read( parser->fd, parser->mbuffer, CONFIG_PARSER_BUFFER_LEN );
}

int __ConfigParser_NextChar( struct ConfigParser* parser ) {
	if( parser->buffer_ptr == parser->buffer_len ) {
		if( ! __ConfigParser_ReadBlock( parser ) ) {
			return -1;
		}
	}

	return parser->mbuffer[ parser->buffer_ptr ++ ] ;
}

void __ConfigParser_TokenizeStatement( struct ConfigParser* parser ) {
	int ch;
	char c;

	char buf[1024];
	int bufptr = 0;

	struct StringList* list = NULL;

	while( (ch = __ConfigParser_NextChar( parser ) ) != -1 && ch != ';' ) {
		c = (char)ch;
		if( isspace(c) ) {
			buf[bufptr] = '\0';
			if( list == NULL ) {
				parser->conf_line = calloc( 1, sizeof( struct StringList ) );
				list = parser->conf_line;
			} else {
				list->next = calloc( 1, sizeof( struct StringList ) );
				list = list->next;
			}
			list->data = strdup( buf );
			bufptr = 0;
		} else {
			buf[ bufptr ++ ] = c;
		}
	}
}


/*
 * Frees all internal comoponents of the parser, but does
 * not free the parser itself
 */
void ConfigParser_Delete( struct ConfigParser* parser ) {
	free( parser->current_section );
	free( parser->current_attribute );
	free( parser->error );
}

/* Read the next string */
char* ConfigParser_NextString( struct ConfigParser* parser ) {
	if( parser -> conf_line == NULL )
		return NULL;

	char* str = parser->conf_line->data;

	struct StringList* tmp = parser->conf_line;
	parser->conf_line = tmp->next;

	free( tmp );

	/* Take ownership of the string */
	return str;
}

/* Skips to the next attribute */

/* An explicit call to NextAttributte _must_ be made inn
 * order for the next values to be read. If there is no more
 * data to be read from an attribute, then the other calls
 * to NextStrings will return their error codes
 */
void ConfigParser_NextAttributte( struct ConfigParser* parser ) {
	char buf[1024];
	int bufptr = 0;
	int ch;

	free( parser->current_attribute );

	while( (ch = __ConfigParser_NextChar( parser ) ) != -1 && ch != '=' ) {
		if( ! isspace( ch ) ) {
			buf[bufptr ++] = ch;
		}
	}

	buf[ bufptr ] = '0';
	parser->current_attribute = strdup( buf );
}

int __ParseRadix( const char* str, int* i, int radix ) {
	unsigned char chr;
	int tmp = 0;

	if( str == NULL ) {
		return -1 ;
	}

	if( radix > 36 || radix < 2 )
		return -2;

	if( radix > 10 ) {
		while( (chr = *str ++) != '\0' ) {
			if( chr < 0x40 && chr >= 0x30 ) {
				tmp = tmp * radix + (chr - 0x30);
			} else if ( chr >= 0x61 && chr < 0x61 + (radix - 10) ) {
				tmp = tmp * radix + (chr - 0x61 + 10);
			} else if ( chr >= 0x41 && chr < 0x41 + (radix - 10) ) {
				tmp = tmp * radix + (chr - 0x41 + 10);
			} else {
				return -1;
			}
		}
	} else {
		while( (chr = *str ++) != '\0' ) {
			if( chr < 0x30 || chr > 0x30 + radix ) {
				return -1;
			}
			tmp = tmp * radix + (chr - 0x30);
		}
	}

	return 0;
}

/* Attempts to parse all Decimal, Hexidecimal, Binary, and Octal numbers */
/* Returns 0 on success and 1 on error */
int ConfigParser_NextInteger( struct ConfigParser* parser, int* i ) {
	char* tmpstr = ConfigParser_NextString( parser );
	int code;

	if( tmpstr[0] == '0' ) {
		if( tmpstr[1] == 'x' ) {
			code = __ParseRadix( tmpstr + 2, i, 16 );
		} else if( tmpstr[1] == 'b' ) {
			code = __ParseRadix( tmpstr + 2, i, 2 );
		} else {
			code = __ParseRadix( tmpstr + 1, i, 8 );
		}
	} else {
		code = __ParseRadix( tmpstr, i, 10 );
	}

	free( tmpstr );
	return code;
}

/* returns 0 on success and 1 on error */
int ConfigParser_NextIntegerFromRadix( struct ConfigParser* parser, int* i, int radix ) {
	char* tmpstr = ConfigParser_NextString( parser );
	int code = __ParseRadix( tmpstr, i, radix );
	free( tmpstr );
	return code;
}

/* Read the next integer,e xcept if it fails, return def */
int ConfigParser_NextIntegerWithDefault( struct ConfigParser* parser, int def ) {
	int ret;
	if( ConfigParser_NextInteger( parser, &ret ) < 0 )
		return def;
	return ret;
}

/* Skip the next token */
int ConfigParser_SkipToken( struct ConfigParser* parser ) {
	struct StringList* tmp = parser->conf_line;
	free( tmp -> data );
	parser->conf_line = tmp -> next;
	free( tmp );
	return 0;
}
