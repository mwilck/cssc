/*
 * pfile.c: Part of GNU CSSC.
 * 
 * Defines the function _chmod for MS-DOS systems.
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
 * Common members of the class sccs_pfile.
 *
 */

#include "cssc.h"
#include "linebuf.h"
#include "pfile.h"

#ifdef CONFIG_SCCS_IDS
static const char rcs_id[] = "CSSC $Id: pfile.cc,v 1.5 1997/07/02 18:02:02 james Exp $";
#endif

NORETURN
sccs_pfile::corrupt(int lineno, const char *msg) const {
	quit(-1, "%s: line %d: p-file is corrupt.  (%s)",
	     (const char *) pname, lineno, msg);
}

sccs_pfile::sccs_pfile(sccs_name &n, enum _mode m)
		: name(n), mode(m), pos(-1) {

        if (!name.valid()) {
		quit(-1, "%s: Not a SCCS file.", (const char *) name);
	}

	pname = name.pfile();

	if (mode != READ) {
		if (name.lock()) {
			quit(-1, "%s: SCCS file is locked.  Try again later.",
			     (const char *) name);
		}
	}

	FILE *pf = fopen(pname, "r");
	if (pf == NULL) {
		if (errno != ENOENT) {
			quit(-1, "%s: Can't open p-file for reading.",
			     (const char *) pname);
		}
	} else {
		class _linebuf linebuf;

		int lineno = 0;
		while(!linebuf.read_line(pf)) {
			linebuf[strlen(linebuf) - 1] = '\0';
			lineno++;

			char *args[7];

			int argc = split(linebuf, args, 7, ' ');

			if (argc < 5) {
				corrupt(lineno, "Expected 5-7 args");
			}

			char *incl = NULL, *excl = NULL;
			int i;
			for(i = 5; i < argc; i++) {
				if (args[i][0] == '-') {
					if (args[i][1] == 'i') {
						incl = args[i] + 2;
					} else if (args[i][1] == 'x') {
						excl = args[i] + 2;
					}
				}
			}

			if ((argc == 6 && excl == NULL && incl == NULL)
			    || (argc == 7 && (excl == NULL || incl == NULL))) {
				corrupt(lineno, "Bad -i or -x option");
			}
				      
				
			struct edit_lock tmp(args[0], args[1], args[2],
					     args[3], args[4], 
					     incl, excl);

			if (!tmp.got.valid() || !tmp.got.valid()) {
				corrupt(lineno, "Invalid SID");
			}

			if (!tmp.date.valid()) {
				corrupt(lineno, "Invalid date");
			}
					
			if (!tmp.include.valid() || !tmp.exclude.valid()) {
				corrupt(lineno, "Invalid SID list");
			}
			edit_locks.add(tmp);
		}

		if (ferror(pf)) {
			quit(errno, "%s: Read error.", (const char *) pname);
		}

		fclose(pf);
	}
}

int
sccs_pfile::is_locked(sid id) {
	rewind();

	sccs_pfile &it = *this;
	while(next()) {
		if (it->got == id) {
			return 1;
		}
	}

	return 0;
}

int
sccs_pfile::is_to_be_created(sid id) {
	rewind();

	sccs_pfile &it = *this;
	while(next()) {
		if (it->delta == id) {
			return 1;
		}
	}

	return 0;
}

int 
sccs_pfile::print_lock_sid(FILE *fp)
{
  const edit_lock& l = edit_locks.select(pos);
  const sid& s       = l.delta;
  return s.print(fp);
}



sccs_pfile::~sccs_pfile() {
	if (mode != READ) {
		name.unlock();
	}
}


/* Local variables: */
/* mode: c++ */
/* End: */
