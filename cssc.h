/*
 * cssc.h
 *
 * Master include file for CSSC.
 *
 * $Id: cssc.h,v 1.7 1997/11/09 11:54:32 james Exp $
 *
 */

#ifndef CSSC__CSSC_H__
#define CSSC__CSSC_H__

#undef TESTING

// Get the definitions deduced by "configure".
#include <aconf.h>


#define CONFIG_NO_TIMEZONE_VAR


#undef  CONFIG_DECLARE_ERRNO
#undef  CONFIG_DECLARE_STRERROR
#undef  CONFIG_DECLARE_MALLOC
#undef  CONFIG_DECLARE_STAT
#undef  CONFIG_DECLARE_GETPWUID
#undef  CONFIG_DECLARE_GETLOGIN
#undef  CONFIG_DECLARE_TIMEZONE
#undef  CONFIG_DECLARE_TZSET
#undef  CONFIG_DECLARE_FDOPEN




//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
//           TODO
//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

/* Ancient versions of SLS Linux apparently need this. */
#define CONFIG_WAIT_IS_A_USELESS_MACRO

/* Switches we want to pass to diff.  Perhaps we might want to */
/* pass special switches to GNU diff (e.g. heuristic mode).  Later maybe. */
#undef  CONFIG_DIFF_SWITCHES

/* Enable support for binary files.  I haven't done this yet anyway. */
#undef	CONFIG_BINARY_FILE



//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
//           Tunable
//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
#define CONFIG_LINEBUF_CHUNK_SIZE	1024
#define CONFIG_LIST_CHUNK_SIZE		16
#define CONFIG_FILE_NAME_GUESSING



//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
//           Deduced
//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

#if defined(HAVE_UNISTD_H) && defined(HAVE_GETEUID) && defined(HAVE_GETEGID)
#define CONFIG_UIDS
#endif



//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
//           MS-DOS
//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
#undef  CONFIG_USE_ARCHIVE_BIT
#define CONFIG_EOL_CHARACTER		'\n'
#undef  CONFIG_MSDOS_FILES
#undef  CONFIG_BORLANDC
#define CONFIG_DJGPP



#include "defaults.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>

#ifdef CONFIG_DECLARE_ERRNO
extern int errno;
#endif

#ifndef NO_COMMON_HEADERS

#include "quit.h"
#include "xalloc.h"
#include "mystring.h"
#include "file.h"

typedef unsigned short seq_no;

int split(char *s, char **args, int len, char c);
mystring prompt_user(const char *prompt);

#endif /* NO_COMMON_HEADERS */


#endif

/* Local variables: */
/* mode: c++ */
/* End: */
