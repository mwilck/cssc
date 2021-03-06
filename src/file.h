/*
 * file.h: Part of GNU CSSC.
 *
 *
 *    Copyright (C) 1997,2001,2007,2008 Free Software Foundation, Inc.
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * CSSC was originally Based on MySC, by Ross Ridge, which was
 * placed in the Public Domain.
 *
 *
 * Declares system dependent routines for accessing files.
 *
 * @(#) CSSC file.h 1.1 93/11/09 17:17:46
 *
 */

#ifndef CSSC__FILE_H__
#define CSSC__FILE_H__

#include <string>
using std::string;

#include <sys/types.h>

#include "filelock.h"

enum create_mode {
	CREATE_EXCLUSIVE     =  001,
	CREATE_READ_ONLY     =  002,
	CREATE_AS_REAL_USER  =  004,
	CREATE_FOR_UPDATE    =  010,
	CREATE_FOR_GET       =  020,
	OBSOLETE1            =  040, // was used by obsolete CONFIG_SHARE_LOCKING.
	CREATE_NFS_ATOMIC    = 0100,
	CREATE_EXECUTABLE    = 0200   // A SCO extension.
};

bool stdout_to_null();
FILE *stdout_to_stderr();
int stdin_is_a_tty();
FILE *open_null();
int is_readable(const char *name);
int is_directory(const char *name);
int file_exists(const char *name);
bool get_open_file_xbits (FILE *f, bool *is_executable);
const char *get_user_name();
int user_is_group_member(gid_t gid);
FILE *fcreate(const std::string& name, int mode);
FILE *fopen_as_real_user(const char *name, const char *mode);
bool set_file_mode(const string &gname, bool writable, bool executable);
bool set_gfile_writable(const string& gname, bool writable, bool executable);
bool unlink_gfile_if_present(const char *gfile_name);
bool unlink_file_as_real_user(const char *gfile_name);
void split_filename(const string& fullname,
		    string& dirname,
		    string& basename);


#ifdef CONFIG_SYNC_BEFORE_REOPEN
int ffsync(FILE *f);
#endif

void give_up_privileges();
void restore_privileges();

#ifdef CONFIG_USE_ARCHIVE_BIT
void clear_archive_bit(const char *name);
#endif

#endif /* __FILE_H__ */

/* Local variables: */
/* mode: c++ */
/* End: */
