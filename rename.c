/* rename.c: Part of GNU CSSC.
 *
 *  Copyright (C) 1997 Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 *  USA.
 */
#if HAVE_UNISTD_H
#include <unistdh>
#endif

#ifndef HAVE_RENAME

int
rename(const char *from, const char *to) {
	if (link(from, to) == -1 || unlink(from) == -1) {
		return -1;
	}
	return 0;
}

#endif /* HAVE_RENAME */
