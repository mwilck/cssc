/* 
 * what.c: Part of GNU CSSC.
 * 
 * 
 *    Copyright (C) 1997, Free Software Foundation, Inc. 
 * 
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 * 
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 * 
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * CSSC was originally Based on MySC, by Ross Ridge, which was 
 * placed in the Public Domain.
 *
 *
 * Prints out SCCS identification keywords marked with "@(#)" in files.
 *
 */

#define NO_COMMON_HEADERS

#include "cssc.h"
#include "quit.h"
#include "getopt.h"
#include "version.h"

const char main_rcs_id[] = "CSSC $Id: what.cc,v 1.4 1997/07/02 18:06:13 james Exp $";

#ifdef CONFIG_WHAT_USE_STDIO

/* Inline fuctions for reading files with stdio. */

typedef FILE *XFILE;

const XFILE XOPEN_FAILURE = NULL;

inline XFILE
xopen(const char *name) {
#ifdef CONFIG_BINARY_FILE
	return fopen(name, "rb");
#else
	return fopen(name, "r");
#endif
}

inline int
xclose(XFILE f) {
	return fclose(f) == EOF ? -1 : 0;
}

inline int
xread(XFILE f, char *buf, int len) {
	int ret = fread(buf, len, 1, f);
	if (ret != len && ferror(f)) {
		return -1;
	}
	return len;
}

inline int
xgetc(XFILE f) {
	return getc(f);
}

#else /* CONFIG_WHAT_USE_STDIO */

/* Inline functions for reading files with Unix style I/O */

#include "sysdep.h"

typedef int XFILE;

const XFILE XOPEN_FAILURE = -1;

inline XFILE
xopen(const char *name) {
#ifdef CONFIG_BINARY_FILE
	return open(name, O_RDONLY | O_BINARY);
#else
	return open(name, O_RDONLY);
#endif
}

inline int
xclose(XFILE f) {
	return close(f);
}

inline int
xread(XFILE f, char *buf, int len) {
	return read(f, buf, len);
}

inline int
xgetc(XFILE f) {
	char c;
	int ret = read(f, &c, 1);
	if (ret == -1) {
		quit(errno, "Read error.");
	}
	if (ret == 0) {
		return EOF;
	}
	return c;
}
	
#endif /* CONFIG_WHAT_USE_STDIO */

void
usage() {
	fprintf(stderr, "usage: %s [-sV] file ...\n", prg_name);
}


/* Print what's found after a "@(#)" in a file */

inline char *
print_what(char *s, char *end, XFILE f) {
	putchar('\t');
	
	while(s < end) {
		char c = *s;
		switch (c) {
		case '"':
		case '>':
		case '\n':
#if CONFIG_EOL_CHARACTER != '\n'
		case CONFIG_EOL_CHARACTER:
#endif			
		case '\\':
		case '\0':
			return s;
		}
		putchar(c);
		s++;
	}

	char c = xgetc(f);
	while(c != EOF) {
		switch (c) {
		case '"':
		case '>':
		case '\n':
#if CONFIG_EOL_CHARACTER != '\n'
		case CONFIG_EOL_CHARACTER:
#endif			
		case '\\':
		case '\0':
			return end;
		}
		putchar(c);
		c = xgetc(f);
	}

	return NULL;
}

int
main(int argc, char **argv) {
	int one_match = 0;

	if (argc > 0) {
		set_prg_name(argv[0]);
	} else {
		set_prg_name("what");
	}

	int c;
	class getopt opts(argc, argv, "r:snV");
	for(c = opts.next(); c != getopt::END_OF_ARGUMENTS; c = opts.next()) {
		switch (c) {
		case 's':
			one_match = 1;
			break;

		case 'V':
			version();
		}
	}

	int arg;
	for(arg = opts.get_index(); arg < argc; arg++) {
		XFILE f = xopen(argv[arg]);
		if (f == XOPEN_FAILURE) {
			quit(errno, "%s: Can't open file.", argv[arg]);
		}
		
		printf("%s:\n", argv[arg]);

		static char buf[CONFIG_WHAT_BUFFER_SIZE + 3];
		buf[0] = buf[1] = buf[2] = '\0';

		int read_len = xread(f, buf + 3, CONFIG_WHAT_BUFFER_SIZE);
		while(read_len > 0) {
			int done = 0;
			char *end = buf + read_len;
			char *at = (char *) memchr(buf, '@', read_len);
			while(at != NULL) {
				if (at[1] == '(' && at[2] == '#'
				    && at[3] == ')') {
					at = print_what(at, end + 3, f);
					putchar('\n');
					if (at == NULL || one_match) {
						done = 1;
						break;
					}
				}
				at++;
				if (at >= end) {
					break;
				}
				at = (char *) memchr(at, '@', end - at);
			}
			if (done) {
				break;
			}
			buf[0] = end[0];
			buf[1] = end[1];
			buf[2] = end[2];
			read_len = xread(f, buf + 3, CONFIG_WHAT_BUFFER_SIZE);
		}
		if (read_len == -1) {
			quit(errno, "%s: Read error.", argv[arg]);
		}
		putchar('\n');
		xclose(f);
	}

	return 0;
}

/* Local variables: */
/* mode: c++ */
/* End: */
