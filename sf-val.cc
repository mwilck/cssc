/*
 * sf-val.cc: Part of GNU CSSC.
 * 
 * 
 *    Copyright (C) 1997,1998, Free Software Foundation, Inc. 
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
 *
 * Members of class sccs_file used by "val". 
 * 
 */

#include "cssc.h"
#include "sccsfile.h"
#include "delta.h"
#include "delta-table.h"
#include "delta-iterator.h"


#ifdef CONFIG_SCCS_IDS
static const char rcs_id[] = "CSSC $Id: sf-val.cc,v 1.3 1998/12/12 15:09:13 james Exp $";
#endif

const mystring
sccs_file::get_module_type_flag()
{
  if (flags.type)
    return *(flags.type);
  else
    return mystring("");
}


bool 
sccs_file::validate_seq_lists(const delta_iterator& d) const
{
  const char *sz_sid = d->id.as_string().c_str();
  int i;
  seq_no s;
  
  for (i=0; i<d->included.length(); ++i)
    {
      s = d->included[i];
      if (s > delta_table->highest_seqno())
	{
	  errormsg("%s: SID %s: included seqno %u does not exist\n",
		   name.c_str(), sz_sid, (unsigned)s);
	  return false;
	}
    }

   for (i=0; i<d->excluded.length(); ++i)
     {
       s = d->excluded[i];
       if (s > delta_table->highest_seqno())
	 {
	   errormsg("%s: SID %s: excluded seqno %u does not exist\n",
		    name.c_str(), sz_sid, (unsigned)s);
	   return false;
	 }
     }

   for (i=0; i<d->ignored.length(); ++i)
     {
       s = d->ignored[i];
       if (s > delta_table->highest_seqno())
	 {
	   errormsg("%s: SID %s: ignored seqno %u does not exist\n",
		    name.c_str(), sz_sid, (unsigned)s);
	   return false;
	 }
     }
   return true;
}

bool 
sccs_file::validate_isomorphism() const
{
  // First decide for each delta if it is a branch point or not.
  // then decide for each delta if it is on the branch or on the trunk.
  //
  // A delta is a branch point if more than one delta in the 
  // delta table references it as a predecessor.
  //
  // A delta is on a branch if it has an ancestor which is a branch point,
  // UNLESS it was the next revision on the trunk.  How to determine this?
  //
  // All branch deltas must have 4-component SIDs.
  // All trunk deltas must have 2-component SIDs.
  //

  // TODO: write this later.
  return true;
}

bool 
sccs_file::validate() const
{
  int i;
  bool retval = true;

  if (!checksum_ok())
    {
      return false;
    }

  // for each delta:-
  delta_iterator iter(delta_table);
  int *seen_ever = new int[delta_table->highest_seqno()];
  int *seen_in_ancestry = new int[delta_table->highest_seqno()];

  for (i=0; i<delta_table->highest_seqno(); ++i)
    {
      seen_ever[i] = 0;
    }
  
  while ( retval && iter.next())
    {
      seq_no s = iter->seq;
      
      for (i=0; i<delta_table->highest_seqno(); ++i)
	{
	  seen_in_ancestry[i] = 0;
	}
  
      const char *sz_sid = iter->id.as_string().c_str();
      
      // validate that the included/excluded/unchanged line counts are valid.
      // check that type is either R or D
      if ('R' != iter->type && 'D' != iter->type)
	{
	  errormsg("%s: SID %s: Unknown delta type %c",
		   name.c_str(), sz_sid, iter->type);
	  retval = false;
	}

      if (iter->inserted > 99999uL)
	{
	  errormsg("%s: SID %s: out-of-range inserted line count %lu",
		   name.c_str(), sz_sid, iter->inserted);
	  retval = false;
	}
      if (iter->deleted > 99999uL)
	{
	  errormsg("%s: SID %s: out-of-range deleted line count %lu",
		   name.c_str(), sz_sid, iter->deleted);
	  retval = false;
	}
      if (iter->unchanged > 99999uL)
	{
	  errormsg("%s: SID %s: out-of-range unchanged line count %lu",
		   name.c_str(), sz_sid, iter->unchanged);
	  retval = false;
	}
      
      if (!iter->date.valid())
	{
	  errormsg("%s: SID %s: invalid date", name.c_str(), sz_sid);
	  retval = false;
	}
      
      // check that username contains no colon.
      if (iter->user.empty())
	{
	  errormsg("%s: SID %s: empty username", name.c_str(), sz_sid);
	  retval = false;
	}
      
      else if (iter->user.find_last_of(':') != mystring::npos)
	{
	  errormsg("%s: SID %s: invalid username '%s'",
		   name.c_str(), sz_sid, iter->user.c_str());
	  retval = false;
	}

      // check seqno is valid - loops, dangling references.
      if (s > delta_table->highest_seqno())
	{
	  errormsg("%s: SID %s: invalid seqno %d",
		   name.c_str(), sz_sid, (int)s);
	  retval = false;
	}
      
      if (iter->prev_seq > delta_table->highest_seqno())
	{
	  errormsg("%s: SID %s: invalid predecessor seqno %d",
		   name.c_str(), sz_sid, (int)iter->prev_seq);
	  retval = false;
	}
      
      if (seen_ever[s - 1] > 1)
	{
	  errormsg("%s: seqno %d appears more than once (%d times)",
		   name.c_str(), (int)s, seen_ever[iter->seq - 1]);
	  retval = false;		// seqno appears more than once.
	}

      ++seen_ever[s - 1];

      // TODO:  Check for loops in predecessor list.


      s = delta_table->delta_at_seq(s).prev_seq;
      
      while (s != 0)
	{
	  if (s > delta_table->highest_seqno())
	    {
	      errormsg("%s: invalid seqno %d", name.c_str(), (int)s);
	      retval = false;
	      break;
	    }
	  else if (seen_in_ancestry[s - 1])
	    {
	      errormsg("%s: SID %s: "
		       "multiply-descended from seqno %d",
		       name.c_str(), sz_sid, (int)s);
	      retval = false;
	      break;
	    }
	  else
	    {
	      ++seen_in_ancestry[s - 1];
	      s = delta_table->delta_at_seq(s).prev_seq;
	    }
	}
      
      // check time doesn't go backward (warning only, because this is
      // possible if developers in different timezones are
      // collaborating on the same file).
      if (0 != iter->prev_seq)
	{
	  const delta& ancestor(delta_table->delta_at_seq(iter->prev_seq));
	  if (ancestor.date > iter->date)
	    {
	      // Time has apparently gone backward...
	      errormsg("%s: WARNING: date for version %s"
		       " is later than the date for version %s",
		       name.c_str(),
		       ancestor.id.as_string().c_str(),
		       iter->id.as_string().c_str());
	    }
	}
      
      // check included/excluded/ignored deltas actually exist.
      validate_seq_lists(iter);
    }
  delete[] seen_in_ancestry;
  delete[] seen_ever;
  
  if (false == retval)
    return retval;
  
  if (!validate_isomorphism())	// check that SIDs follow seqno structure.
    {
      return false;
    }
  
  // Check username list for invalid entries.
  for (i=0; i<users.length(); ++i)
    {
      const mystring& username(users[i]);
      
      if (   (username.find_last_of(':')  != mystring::npos) 
	  || (username.find_last_of(' ')  != mystring::npos)
	  || (username.find_last_of('\t') != mystring::npos))
	{
	  errormsg("%s: invalid username '%s'",
		   name.c_str(), username.c_str());
	  retval = false;
	}
    }
  
  
  // for the flags section:-
  // TODO: check for unknown flags
  // TODO: check for boolean flags with non-numeric value.

  // TODO: check the body (unclosed deltas, etc.)
  
  return retval;
}


/* Local variables: */
/* mode: c++ */
/* End: */