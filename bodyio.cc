/*
 * bodyio.cc: Part of GNU CSSC.
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
 * Code for performing I/O on the body of an SCCS file.
 * See also sf-admin.cc and encoding.cc.
 *
 */

#include "cssc.h"
#include "filepos.h"
#include "bodyio.h"
#include "sccsfile.h"
#include "linebuf.h"

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif

/* body_insert_text()
 *
 * Insert a file into an SCCS file (e.g. for admin).
 * return false (and restore initial file positions)
 * for failure.
 *
 * We fail if the input contains a ^A immediately following
 * a newline (which is special to SCCS), or if the input
 * does not end with a newline.
 *
 *  1) otherwise the control-character (^A) immediately
 *     following will not be recognised; SCCS utils
 *     only look for them at the beginning of a line
 * 
 *  2) many diff(1) programs only cope with text files
 *     that end with a newline.
 */
bool
body_insert_text(const char iname[], const char oname[],
		 FILE *in, FILE *out,
		 unsigned long int *lines,
		 bool *idkw)
{
  char ch, last;
  unsigned long int nl;		// number of lines.
  bool found_id;

  // If we fail, rewind these files to try binary encoding.
  FilePosSaver i_saver(in), o_saver(out);
  
  *idkw = found_id = false;
  nl = 0uL;
  last = '\n';
  
  while ( EOF != (ch=getc(in)) )
    {
      if (CONFIG_EOL_CHARACTER == ch)
	++nl;

      // check for ^A at start of line.
      if ('\n' == last && CONFIG_CONTROL_CHARACTER == ch)
	{
	  fprintf(stderr,
		  "%s: control character at start of line, "
		  "treating as binary.\n",
		  iname);
	  return false;		// file pointers implicitly rewound
	}

      
      // FIXME TODO: if we get "disk full" while trying to write the
      // body of an SCCS file, we will retry with binary (which of
      // course uses even more space)
      if (putc_failed(putc(ch, out)))
	quit(errno, "%s: Write error.", oname);

      if (!found_id)		// Check for ID keywords.
	{
	  if ('%' == last && is_id_keyword_letter(ch))
	    {
	      const char peek = getc(in);
	      if ('%' == peek)
		*idkw = found_id = true;
	      ungetc(peek, in);
	    }
	}
      
      last = ch;
    }

  if (ferror(in))
    quit(errno, "%s: Read error.", iname);

  // Make sure the file ended with a newline.
  if ('\n' != last)
    {
      fprintf(stderr, "%s: no newline at end of file, treating as binary\n",
	      iname);
      return false;		// file pointers implicitly rewound
    }

  // Success; do not rewind the input and output files.
  i_saver.disarm();
  o_saver.disarm();
  *lines = nl;
  return true;
}

// Keywords: a file containing the octal characters 
// 0026 0021 0141 (hex 0x16 0x11 0x61) will produce
// begin 0644 x
// #%A%A
// `
// end
// 
//
// Stupidly imho, SCCS checks the ENCODED form of the file
// in order to support the "no ID keywords" warning.  Still,
// at least it doesn't expand those on output!


void
body_insert_binary(const char iname[], const char oname[],
		   FILE *in, FILE *out,
		   unsigned long int *lines,
		   bool *idkw)
{
  const int max_chunk = 45;
  char inbuf[max_chunk], outbuf[80];
  unsigned long int nl;
  int len;
  bool kw;
  *idkw = kw = false;
  
  while ( 0 < (len = fread(inbuf, sizeof(char), max_chunk, in)) )
    {
      encode_line(inbuf, outbuf, len); // see encoding.cc.

      if (!kw)
	{
	  // For some odd reason, SCCS seems to check
	  // the encoded form for ID keywords!  We know
	  // that strlen() on the UUENCODEd data is safe.
	  if (check_id_keywords(inbuf, strlen(inbuf)))
	    *idkw = kw = true;
	}
      
      if (fputs_failed(fputs(outbuf, out)))
	quit(errno, "%s: Write error.", oname);
      
      ++nl;
    }
  // A space character indicates a count of zero bytes and hence
  // the end of the encoded file.
  if (fputs_failed(fputs(" \n", out)))
    quit(errno, "%s: Write error.", oname);
  ++nl;
      
  
  if (ferror(in))
    quit(errno, "%s: Read error.", iname);

  *lines = nl;
}

void
body_insert(bool *binary,
	    const char iname[], const char oname[],
	    FILE *in, FILE *out,
	    unsigned long int *lines,
	    bool *idkw)
{
  // If binary mode has not been forced, try text mode.
  if (!*binary)
    {
      if (body_insert_text(iname, oname, in, out, lines, idkw))
	return;			// Success.
    }

  *binary = true;
  body_insert_binary(iname, oname, in, out, lines, idkw);
}

int output_body_line_text(FILE *fp, const cssc_linebuf* plb)
{
  return plb->write(fp) || fputc_failed(fputc('\n', fp));
}

int output_body_line_binary(FILE *fp, const cssc_linebuf* plb)
{
  // Curiously, if the file is encoded, we know that
  // the encoded form is only about 60 characters
  // and contains no 8-bit or zero data.
  int n;
  char outbuf[80];
  
  n = decode_line(plb->c_str(), outbuf); // see encoding.cc
  return fwrite_failed(fwrite(outbuf, sizeof(char), n, fp), n);
}


int 
encode_file(const char *nin, const char *nout)
{
  FILE *fin = fopen(nin, "rb");	// binary
  if (0 == fin)
    quit(errno, "Failed to open \"%s\" for reading.\n", nin);
  
  FILE *fout = fopen(nout, "w"); // text
  if (0 == fout)
    quit(errno, "Failed to open \"%s\" for writing.\n", nout);
  
  
  encode_stream(fin, fout);

  
  if (ferror(fin) || fclose_failed(fclose(fin)))
    quit(errno, "%s: Read error.\n", nin);
  
  if (ferror(fout) || fclose_failed(fclose(fout)))
    quit(errno, "%s: Write error.\n", nout);

  return 0;
}
