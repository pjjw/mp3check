/*GPL*START*
 * 
 * encapsulated POSIX regular expression (regex) interface header
 *
 * Copyright (C) 1998 by Johannes Overmann <Johannes.Overmann@gmx.de>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * *GPL*END*/  

#include "tregex.h"


// history:
// 1997:
// 03:00 Jul 04 created from 'rtest.cc', tested, usable
// 03:00 Jul 11 added substring match and substitution
// 21:00 Sep 04 added preserve case

// 1998:
// 22:38 Aug 05 added progmode (number and stderr)

// 1999: 
// 14:58 Jan 31 added pre/post_padstring for highlighting (h=106, cc=237)
// 20:58 Oct 14 fixed parameterSubstitution(): in and out may now be the same string

// 2000:
// 11:50 Jul 09 fatalError removed, replaced with exceptions

// 2001:
// 21:20 02 Apr v0.9.19: empty RHS caused TZeroBasedIndexOutOfRange bug fixed (226 lines)


// global data

// 0-9a-z
#define MAX_SUBSTRING 36  


// ctor & dtor

TRegEx::TRegEx(const char *regex, int cflags): preg(), nosub(false)
{
   memset(&preg, 0, sizeof(regex_t));
   int error = regcomp(&preg, regex, cflags);
   if(error) throw TErroneousRegexException(error, preg);
   if(cflags & REG_NOSUB) nosub = true;
}


TRegEx::~TRegEx() {
   regfree(&preg);
}


// matching

bool TRegEx::match(const char *str, int flags) const {
   return regexec(&preg, str, 0, 0, flags) == 0; // match
}


bool TRegEx::firstMatch(const char *str, int& start, int& len, int flags) const {
   if(nosub) throw TNoSubRegexException();
   regmatch_t m[1];
   if(regexec(&preg, str, 1, m, flags)==0) { // match
      start = m->rm_so;
      len   = m->rm_eo - m->rm_so;
      return true;      
   } else { // no match
      start = len = -1;
      return false;
   }
}


bool TRegEx::allMatches(const char *str, tvector<int>& all, int flags) const {
   if(nosub) throw TNoSubRegexException();
   regmatch_t m[1];
   const char *p = str;
   bool match_ = false;
   int start, len, off = 0;
   while(1) {
      if(regexec(&preg, p, 1, m, flags)) break;
      match_ = true;
      start = m->rm_so;
      len   = m->rm_eo - m->rm_so;
      all += start + off;
      all += len;
      if(len==0) break;
      flags |= REG_NOTBOL;
      p   += start + len;
      off += start + len;     
   }
   return match_;
}


bool TRegEx::allMatchesSubstring(const char *str, tvector<tvector<int> >& all, 
				 int flags, int progress, int progmode) const 
{
   if(nosub) throw TNoSubRegexException();
   const char *p = str;
   bool match_ = false;
   int start, len, off = 0, j=0;
   tvector<int> sub;
   regmatch_t m[MAX_SUBSTRING];
   FILE *pout = stdout;
   if(progress < 0) {
      pout = stderr;
      progress = -progress;
   }
   if(progmode & P_STDERR) pout = stderr;
   bool prognum = (progmode & P_NUMBER) > 0;
   while(1) {
      memset(m, -1, sizeof(regmatch_t)*MAX_SUBSTRING);
      if(regexec(&preg, p, MAX_SUBSTRING, m, flags)) break;
      match_ = true;
      sub.clear();
      for(int i=0; i < MAX_SUBSTRING; i++) {
	 if(m[i].rm_so != -1) {
	    sub += m[i].rm_so + off;
	    sub += m[i].rm_eo - m[i].rm_so;
	 }
      }
      start = m->rm_so;
      len   = m->rm_eo - m->rm_so;
      all += sub;
      if(len==0) break;
      flags |= REG_NOTBOL;
      p   += start + len;
      off += start + len;
      
      // show progress
      if(progress) {
	 if((j%progress)==0) {
	    if(prognum) fprintf(pout, "%6d   \r", j);
	    else putc('.', pout);	       
	    fflush(pout);
	 }	   
	 j++;
      }
   }
   return match_;
}


// ascii specific code
static int validatePos(char c) {
   if((c<'0') || (c>'z')) return -1; 
   if((c>'9') && (c<'a')) return -1;
   if(c>='a') return c-'a'+10;
   else return c-'0';
}


static tstring backslashParamSubstitute(const tstring& org, const tstring& sub,
					const tvector<int>& occ) {
   if(sub.len() < 2) return sub;
   int num = occ.size() / 2;
   if(num == 0) return sub;
   size_t i = 0, pos = 0;
   tstring r;
   
   while(i < (sub.len() - 1)) {
      if(sub[i]=='\\') {
	 char c = sub[i+1];
	 if(c=='\\') { // protection
	    r += sub.substr(pos, i+1);
	    i += 2; 
	    pos = i;
	 } else {
	    int p = validatePos(c);
	    if((p>=0) && (p<num)) { // match	       
	       r += sub.substr(pos, i);
	       i += 2;
	       pos=i;
	       r += org.substr(occ[2 * p], occ[2 * p] + occ[2 * p + 1]);
	    } else i++;
	 }
      } else i++;
   }
   r += sub.substr(pos);
   return r;
}


void parameterSubstitution(const tstring& in, tstring& outstr, const tstring& sub,
			   const tvector<tvector<int> >& occ, bool preserve_case,
			   int modify_case, int progress, const tstring& pre_padstring, 
			   const tstring& post_padstring, tvector<int> *match_pos) {
   tstring out;
   int pos = 0;
   for(size_t i = 0; i<occ.size(); i++) {
      out += in.substr(pos, occ[i][0]);
      if(match_pos) (*match_pos) += out.len(); // enter start      
      out += pre_padstring;      
      if(!preserve_case) {
	 if(modify_case) 
	   out += modifyCase(backslashParamSubstitute(in, sub, occ[i]),
			     modify_case);
	 else
	   out += backslashParamSubstitute(in, sub, occ[i]);
      } else out += preserveCase(in.substr(occ[i][0], occ[i][0] + occ[i][1]), 
			       backslashParamSubstitute(in, sub, occ[i]));
      out += post_padstring;      
      if(match_pos) (*match_pos) += out.len(); // enter end      
      pos = occ[i][0] + occ[i][1];
      if(progress>0) if((i%progress)==0) {putchar('.');fflush(stdout);}
   }
   out += in.substr(pos);
   outstr = out;
}

