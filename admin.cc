/*
 * admin.cc: Part of GNU CSSC.
 * 
 *    Copyright (C) 1997,1998, Free Software Foundation, Inc. 
 * 
 *    This program is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU General Public License as
 *    published by the Free Software Foundation; either version 2 of
 *    the License, or (at your option) any later version.
 * 
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 * 
 *    You should have received a copy of the GNU General Public
 *    License along with this program; if not, write to the Free
 *    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139,
 *    USA.
 * 
 * CSSC was originally Based on MySC, by Ross Ridge, which was 
 * placed in the Public Domain.
 *
 *
 * Administer and create SCCS files.
 *
 */ 

#include "cssc.h"
#include "sccsfile.h"
#include "sf-chkmr.h"
#include "fileiter.h"
#include "sid_list.h"
#include "sl-merge.h"
#include "my-getopt.h"
#include "version.h"
#include "delta.h"

const char main_rcs_id[] = "CSSC $Id: admin.cc,v 1.28 1998/08/14 08:23:32 james Exp $";


static bool
well_formed_sccsname(const sccs_name& n)
{
  const char *s;
  
  if ( NULL != (s = strrchr(n.c_str(), '/')) )
    {
      s++;
    }
  else
    {
      s = n.c_str();
    }
  return ('s' == s[0]) && ('.' == s[1]);
}

void
usage() {
	fprintf(stderr,
"usage: %s [-nrzV] [-a users] [-d flags] [-e users] [-f flags]\n"
"\t[-i file] [-m MRs] [-t file] [-y comments] file ...\n",
	prg_name);
}

int
main(int argc, char **argv) {
	int c;
	Cleaner arbitrary_name;
	release first_release = NULL;		/* -r */
	int new_sccs_file = 0;			/* -n */
	bool force_binary = false;              /* -b */
	const char *iname = NULL;		/* -i -I */
	const char *file_comment = NULL;	/* -t */
	list<mystring> set_flags, unset_flags;	/* -f, -d */
	list<mystring> add_users, erase_users;	/* -a, -e */
	mystring mrs, comments;			/* -m, -y */
	const int check = 0;			/* -h */
	int reset_checksum = 0;			/* -z */
	int suppress_mrs = 0;	                /* -m (no arg) */
	int suppress_comments = 0;              /* -y (no arg) */
	int empty_t_option = 0;	                /* -t (no arg) */
	int retval;
	

	// If we use the "-i" option, we read the initial body from the 
	// named file, or stdin if no file is named.  In this case, we use
	// fgetpos() or ftell() so that we can rewind the input in order to 
	// try again with encoding, should we find the body to need it.
	// GNU glibc version 2.0.6 has a bug which results in ftell()/fgetpos()
	// succeeding on stdin even if it is a pipe,fifo,tty or other 
	// nonseekable object.  We get around this bug by setting stdin to 
	// un-buffered, which avoids this bug.  
#ifdef __GLIBC__
	setvbuf(stdin, (char*)0, _IONBF, 0u);
#endif
	
	if (argc > 0) {
		set_prg_name(argv[0]);
	} else {
		set_prg_name("admin");
	}

	retval = 0;
	
	class CSSC_Options opts(argc, argv, "bni!r!t!f!d!a!e!m!y!hzV");
	for (c = opts.next();
	     c != CSSC_Options::END_OF_ARGUMENTS;
	     c = opts.next()) {
		switch (c) {
		default:
			errormsg("Unsupported option: '%c'", c);
			return 2;

		case 'n':
			new_sccs_file = 1;
			break;

		case 'b':
			force_binary = true;
			break;

		case 'i':
			if (strlen(opts.getarg()) > 0)
			  iname = opts.getarg();
			else
			  iname = "-";
			new_sccs_file = 1;
			break;

		case 'r':
		        {
			  const char *rel = opts.getarg();
			  first_release = release(rel);
			  if (!first_release.valid())
			    {
			      errormsg("Invaild release: '%s'", rel);
			      return 1;
			    }
			  break;
			}

		case 't':
			file_comment = opts.getarg();
			empty_t_option = (0 == strlen(file_comment));
			break;
		       
		case 'f':
			set_flags.add(opts.getarg());
			break;

		case 'd':
			unset_flags.add(opts.getarg());
			break;

		case 'a':
			add_users.add(opts.getarg());
			break;

		case 'e':
			erase_users.add(opts.getarg());
			break;

		case 'm':
			mrs = opts.getarg();
			suppress_mrs = (mrs == "");
			break;

		case 'y':
			comments = opts.getarg();
			suppress_comments = (comments == "");
			break;

#if 0
		case 'h':
			check = 1;
			break;
#endif

		case 'z':
			reset_checksum = 1;
			break;

		case 'V':
			version();
			if (2 == argc)
			  return 0; // "admin -V" should succeed.
		}
	}

	if (empty_t_option && new_sccs_file)
	  {
	    errormsg("The -t option must have an argument "
		     "if -n or -i is used.");
	    return 1;
	  }
	
	list<mystring> mr_list, comment_list;
	int was_first = 1;
	sccs_file_iterator iter(opts);

	while (iter.next()) {
	  // can't set first to 0 at end of loop because we may
	  // use "continue".
	  int me_first = was_first;
	  was_first = 0;
	  
		sccs_name &name = iter.get_name();

		if (reset_checksum && !check) {
			if (!name.valid()) {
			  errormsg("%s: Not a SCCS file.", name.c_str());
			  retval = 1;
			  continue; // with next file
			}
			if (!sccs_file::update_checksum(name.c_str()))
			  retval = 1; // some kind of error.
			continue;
		}

		if (!well_formed_sccsname(name))
		  {
		    errormsg("%s is an invalid name."
			     "  SCCS file names must begin `s.'\n",
			     name.c_str());
		    retval = 1;
		    continue;	// with next file
		  }
		
		/* enum */ sccs_file::_mode mode = sccs_file::UPDATE;
#if 0
		if (check) {
			mode = sccs_file::READ;
		} else
#endif
		if (new_sccs_file) {
			mode = sccs_file::CREATE;
		}

		sccs_file file(name, mode);

#if 0
		if (check) {
			file.check();
			continue;
		}
#endif

		
		if (!file.admin(file_comment,
				force_binary, set_flags, unset_flags,
				add_users, erase_users))
		  {
		    retval = 1;
		    continue;
		  }
		

		if (new_sccs_file) {
			if (iname != NULL && !me_first) {
				errormsg("The 'i' keyletter can't be used with"
					 " multiple files.");
				return 1;
			}

			if (me_first)
			  {
			    // The real thing does not prompt the user here.
			    // Hence we don't either.
			    if (!file.mr_required() && !mrs.empty())
			      {
				errormsg("MRs not enabled with 'v' flag, "
					 "can't use 'm' keyword.");
				retval = 1;
				continue; // with next file
			      }
			    
			    mr_list = split_mrs(mrs);
			    comment_list = split_comments(comments);
			  }

			if (file.mr_required()) {
				if (!suppress_mrs && mr_list.length() == 0) {
					errormsg("%s: MR number(s) must be "
						 " supplied.", name.c_str());
					retval = 1;
					continue; // with next file
					  
				}
				if (file.check_mrs(mr_list)) {
					errormsg("%s: Invalid MR number(s).",
						 name.c_str());
					retval = 1;
					continue; // with next file
				}
			}

			if (!file.create(first_release, iname,
					 mr_list, comment_list,
					 suppress_comments, force_binary))
			  {
			    retval = 1;
			    continue;
			  }
			
		} else {
			if (!file.update())
			  retval = 1;
		}		
	}

	return retval;
}

// Explicit template instantiations.
template class list<mystring>;
template class list<seq_no>;
template class list<delta>;
//template list<char const*>& operator+=(list<char const *> &, list<mystring> const &);
template class list<char const *>;
template list<mystring>& operator+=(list<mystring> &, list<mystring> const &);
template list<mystring>& operator-=(list<mystring> &, list<mystring> const &);
template class range_list<release>;

/* Local variables: */
/* mode: c++ */
/* End: */
