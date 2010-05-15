/*
 * rel_list.cc: Part of GNU CSSC.
 *
 *    Copyright (C) 1997,1999,2007 Free Software Foundation, Inc.
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
 */
#include <cstdlib>

#include "cssc.h"
#include "rel_list.h"
#include "ioerr.h"
#include "quit.h"

// Because we use member() all the time, this
// is a quadratic algorithm...but N is usually very small.
release_list::release_list(const char *s)
{
  ASSERT(NULL != s);

  char *p;
  while (*s)
    {
      long int n = strtol(s, &p, 10);
      if (p == s)
	break;			// not numeric.

      if (n < 0)
	ctor_fail(-1, "ranges not allowed in release lists");

      // add the entry if not already a member.
      const release r(n);
      if (!member(r))
	l.add(r);

      s = p;
      if (',' == *s)
	s++;
    }
}

// linear search for possible member.
bool release_list::member(release r) const
{
  const release_list::size_type len = l.length();
  for(release_list::size_type i=0; i<len; i++)
    {
      if (l[i] == r)
	return true;
    }
  return false;
}


release_list::release_list()
{
}

release_list::release_list(const release_list &r)
{
  l = r.l;
}

release_list::~release_list()
{
}

bool release_list::print(FILE * out) const
{
  const release_list::size_type len = l.length();
  for(release_list::size_type i=0; i<len; i++)
    {
      if (i > 0)
	{
	  if (putc_failed(fputc(' ', out)))
	    return false;
	}

      if (l[i].print(out))
	return false;
    }
  return true;
}


/* Local variables: */
/* mode: c++ */
/* End: */