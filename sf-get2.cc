/*
 * sf-get2.cc: Part of GNU CSSC.
 * 
 * 
 *    Copyright (C) 1997,1998,1999, Free Software Foundation, Inc. 
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
 * Members of the class sccs_file only used by get.
 *
 */

#include "cssc.h"
#include "sccsfile.h"
#include "pfile.h"
#include "seqstate.h"
#include "delta-iterator.h"
#include "delta-table.h"


#include <ctype.h>

#ifdef CONFIG_SCCS_IDS
static const char rcs_id[] = "CSSC $Id: sf-get2.cc,v 1.37 2000/03/19 11:18:41 james Exp $";
#endif

/* Returns the SID of the delta to retrieve that best matches the
   requested SID. */
bool
sccs_file::find_requested_sid(sid requested, sid &found, bool include_branches) const
{
  if (requested.is_null())	// no sid specified?
    {				// get the default.
      requested = flags.default_sid; 
      if (requested.is_null())	// no default?
	{			// get the latest.
	  requested = sid(release::LARGEST);
	}
    }

  // Giving a SID of two components is a request for 
  // an exact match on the trunk, unless include_branches
  // is specified, in which case it is a request for
  // the latest SID of the specified release and level.
  int ncomponents = requested.components();
  if (2 == ncomponents && !include_branches)
    ncomponents = 4;		// want an exact match.

  ASSERT(ncomponents != 0);
  ASSERT(ncomponents <= 4);

  // Remember the best so far.
  bool got_best = false;
  sid best;
  
  const_delta_iterator iter(delta_table);

  if (1 == ncomponents)
    {
      // find highest SID of any level, which is less than or equal to
      // the requested one.  If include_branches is true, they don't
      // have to be on the trunk.
      while (iter.next())
	{
	  if ( (release)iter->id > (release)requested )
	    continue;
	  else if (!include_branches && !iter->id.on_trunk())
	    continue;
	  if (!got_best || iter->id.gte(best))
	    {
	      best = iter->id;
	      got_best = true;
	    }
	}
    }
  else if (3 == ncomponents)
    {
      while (iter.next())
	{
	  const sid &s = iter->id;
	  const int nc = s.components();
	  
	  // We ignore anything that is not on a branch.
	  if (nc >= ncomponents)
	    {
	      if ( (release)s == (release)requested )
		{
		  if (!got_best || s >= best)
		    {
		      best = s;
		      got_best = true;
		    }
		}
	    }
	}
    }
  else
    {
      while (iter.next())
	{
	  if (iter->id.matches(requested, ncomponents))
	    {
	      if (!got_best || iter->id.gte(best))
		{
		  best = iter->id;
		  got_best = true;
		}
	    }
	}
    }

  if (got_best)
    found = best;
  return got_best;
}


bool sccs_file::sid_in_use(sid id, sccs_pfile &pfile) const
{
  if (find_delta(id))
    return true;

  if (pfile.is_to_be_created(id))
    return true;

  return false;
}


/* Returns the SID of the new delta to be created. */
sid
sccs_file::find_next_sid(sid requested, sid got,
			 int want_branch,
			 sccs_pfile &pfile) const
{
  if (!flags.branch)
    want_branch = false;	// branches not allowed!
  
  const int ncomponents = requested.components();
  bool forced_branch = false;

  sid next = requested;
  
  if (requested.release_only())
    {
      next.next_level();
    }
  else if (requested.partial_sid())
    {
      next = got;
      ++next;
    }
  else
    {
      ++next;
    }
  
  
  if (want_branch)
    {
      if (ncomponents < 4)
	next = got;

      next.next_branch();
    }
  else
    {
      // We may be forced to create a branch anyway.

      // have we hit the release ceiling?
      const bool too_high = requested.on_trunk() 
	&& flags.ceiling.valid() && release(requested) > flags.ceiling;

      // have we collided with an existing SID?
      bool branch_again;
      const delta *pnext = find_any_delta(next);
      if (pnext && !pnext->removed() && !requested.partial_sid())
	branch_again = true;
      else
	branch_again = false;
	
      // If we have the revision sequence 1.1 -> 1.2 -> 2.1, then we
      // get 1.2 for editing, we must create a branch (1.2.1.1),
      // because we can't create a 1.3 (as 2.1 already exists).  If
      // the release number of the gotten SID is not the highest, we
      // have to branch.  Otherwise I think the normal anti-collision
      // rules take care of it.
      const bool not_trunk_top =
	release(got) < release(highest_delta_release());
      
      if (too_high || branch_again || not_trunk_top)
	{
	  next = got;
	  next.next_branch();
	  forced_branch = true;
	}
    }
  
  // If we have created a branch, and that branch is not unique, keep
  // looking for an empty branch.
  if (want_branch || forced_branch)
    {
      while (find_delta(next)
	     || (flags.joint_edit && pfile.is_to_be_created(next)))
	{
	  next.next_branch();
	}
    }

  ASSERT(!sid_in_use(next, pfile));
  return next;
}

/* Quits if the user isn't authorized to make deltas, if the release
   requested is locked or if the requested SID has an edit lock and
   the joint edit flag isn't set. */

bool
sccs_file::test_locks(sid got, sccs_pfile &pfile) const {
	int i;
	int len;

	const char *user = get_user_name();

	len = users.length();
	if (len != 0) {
		int found = 0;

		for(i = 0; i < len; i++) {
			const char *s = users[i].c_str();
			char c = s[0];

			if (c == '!') {
				s++;
				if (isdigit(c)) {
					if (user_is_group_member(atoi(s))) {
						found = 0;
						break;
					}
				} else if (strcmp(s, user) == 0) {
					found = 0;
					break;
				}
			} else {
				if (isdigit(c)) {
					if (user_is_group_member(atoi(s))) {
						found = 1;
					}
				} else if (strcmp(s, user) == 0) {
					found = 1;
					break;
				}
			}
		}
		
		if (!found) {
			errormsg("%s: You are not authorized to make deltas.",
			     name.c_str());
			return false;
		}
	}

	if (flags.all_locked 
	    || (flags.floor.valid() && flags.floor > got)
	    || (flags.ceiling.valid() && flags.ceiling < got)
	    || flags.locked.member(got)) {
		errormsg("%s: Requested release is locked.",
			 name.c_str());
		return false;
	}
	
	if (pfile.is_locked(got) && !flags.joint_edit)
	  {
	    mystring when(pfile->date.as_string());
	    errormsg("%s: Requested SID locked by '%s' at %s.\n",
		     name.c_str(),
		     pfile->user.c_str(),
		     when.c_str());
		return false;
	}
	return true;
}


/* Write a line of a file after substituting any id keywords in it.
   Returns true if an error occurs. */

int
sccs_file::write_subst(const char *start,
		       struct subst_parms *parms,
		       const delta& d) const
{
  FILE *out = parms->out;

  const char *percent = strchr(start, '%');
  while (percent != NULL)
    {
      char c = percent[1];
      if (c != '\0' && percent[2] == '%')
	{
	  if (start != percent
	      && fwrite(start, percent - start, 1, out) != 1)
	    {
	      return 1;
	    }

	  percent += 3;

	  int err = 0;

	  switch (c)
	    {
	      const char *s;

	    case 'M':
	      {
		const char *mod = get_module_name().c_str();
		err = fputs_failed(fputs(mod, out));
	      }
	    break;
			
	    case 'I':
	      err = d.id.print(out);
	      break;

	    case 'R':
	      err = d.id.printf(out, 'R', 1);
	      break;

	    case 'L':
	      err = d.id.printf(out, 'L', 1);
	      break;

	    case 'B':
	      err = d.id.printf(out, 'B', 1);
	      break;

	    case 'S':
	      err = d.id.printf(out, 'S', 1);
	      break;

	    case 'D':
	      err = parms->now.printf(out, 'D');
	      break;
				
	    case 'H':
	      err = parms->now.printf(out, 'H');
	      break;

	    case 'T':
	      err = parms->now.printf(out, 'T');
	      break;

	    case 'E':
	      err = d.date.printf(out, 'D');
	      break;

	    case 'G':
	      err = d.date.printf(out, 'H');
	      break;

	    case 'U':
	      err = d.date.printf(out, 'T');
	      break;

	    case 'Y':
	      if (flags.type)
		{
		  err = fputs_failed(fputs(flags.type->c_str(), out));
		}
	      break;

	    case 'F':
	      err =
		fputs_failed(fputs(base_part(name.sfile()).c_str(),
				   out));
	      break;

	    case 'P':
	      if (1) // introduce new scope...
		{
		  mystring path(canonify_filename(name.c_str()));
		  err = fputs_failed(fputs(path.c_str(), out));
		}
	      break;

	    case 'Q':
	      if (flags.user_def)
		{
		  err = fputs_failed(fputs(flags.user_def->c_str(), out));
		}
	      break;

	    case 'C':
	      err = printf_failed(fprintf(out, "%d",
					  parms->out_lineno));
	      break;

	    case 'Z':
	      if (fputc_failed(fputc('@', out))
		  || fputs_failed(fputs("(#)", out)))
		{
		  err = 1;
		}
	      else
		{
		  err = 0;
		}
	      break;

	    case 'W':
	      s = parms->wstring;
	      if (0 == s)
		{
		  /* TODO: SunOS 4.1.4 apparently uses 
		   * a space rather than a tab here.
		   * I want to find a third opinion before
		   * comitting myself...
		   */
		  s = "%Z" "%%M" "%\t%" "I%";
		}
	      else
		{
		  /* protect against recursion */
		  parms->wstring = 0; 
		}
	      err = write_subst(s, parms, d);
	      if (0 == parms->wstring)
		{
		  parms->wstring = s;
		}
	      break;

	    case 'A':
	      err = write_subst("%Z""%%Y""% %M""% %I"
				"%%Z""%",
				parms, d);
	      break;

	    default:
	      start = percent - 3;
	      percent = percent - 1;
	      continue;
	    }

	  parms->found_id = 1;

	  if (err)
	    {
	      return 1;
	    }
	  start = percent;
	}
      else
	{
	  percent++;
	}
      percent = strchr(percent, '%');
    }

  return fputs_failed(fputs(start, out));
}

/* Output the specified version to a file with possible modifications.
   Most of the actual work is done with a seqstate object that 
   figures out whether or not given line of the SCCS file body
   should be included in the output file. */
	
/* struct */ sccs_file::get_status
sccs_file::get(FILE *out, mystring gname, sid id, sccs_date cutoff_date,
	       sid_list include, sid_list exclude,
	       int keywords, const char *wstring,
	       int show_sid, int show_module, int debug)
{
  
  /* Set the return status. */
  struct get_status status;
  status.success = true;
  
  ASSERT(0 != delta_table);
  
  seq_state state(highest_delta_seqno());
  const delta *d = find_delta(id);
  ASSERT(d != NULL);
  
  ASSERT(0 != delta_table);

  if (!prepare_seqstate(state, d->seq))
    {
      status.success = false;
      return status;
    }
  
  ASSERT(0 != delta_table);
  if (!prepare_seqstate(state, include, exclude, cutoff_date))
    {
      status.success = false;
      return status;
    }

  if (getenv("CSSC_SHOW_SEQSTATE"))
    {
      for (seq_no s = d->seq; s>0; s--)
	{
	  const struct delta & d = delta_table->delta_at_seq(s);
	  const sid & id = d.id;

	  id.dprint(stderr);

	  putc(':', stderr);
	  putc(' ', stderr);
	  if (state.is_explicitly_tagged(s))
	    {
	      fprintf(stderr, "explicitly ");
	    }
	  
	  if (state.is_included(s))
	    {
	      fprintf(stderr, "included\n");
	    }
	  else if (state.is_excluded(s))
	    {
	      fprintf(stderr, "excluded\n");
	    }
	  else
	    {
	      fprintf(stderr, "irrelevant\n");
	    }
	}
    }

  
  // The subst_parms here may not be the Whole Truth since
  // the cutoff date may affect which version is actually
  // gotten.  That's taken care of; the correct delta is
  // passed as a parameter to the substitution function.
  // (eugh...)
  struct subst_parms parms(out, wstring, *d,
			   0, sccs_date::now());
  
  
  if (keywords)
    {
      status.success = get(gname, state, parms, &sccs_file::write_subst,
			   show_sid, show_module, debug);
    }
  else
    {
      status.success = get(gname, state, parms, (subst_fn_t) 0,
			   show_sid, show_module, debug);
    }
  if (status.success == false)
    {
      return status;
    }
  // only issue a warning about there being no keywords
  // substituted, IF keyword substitution was being done.
  if (keywords && !parms.found_id)
    {
      no_id_keywords(name.c_str());
      // this function normally returns.
    }
  
  status.lines = parms.out_lineno;
  
  seq_no seq;	
  for(seq = 1; seq <= highest_delta_seqno(); seq++)
    {
      const sid id = seq_to_sid(seq);
      
      if (state.is_explicitly_tagged(seq))
	{
	  if (state.is_included(seq))
	    status.included.add(id);
	  else if (state.is_excluded(seq))
	    status.excluded.add(id);
	}
    }
  
  return status;
}

/* Local variables: */
/* mode: c++ */
/* End: */
