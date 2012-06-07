/*GPL*START*
 *
 * NUL byte safe string implementation
 * 
 * Copyright (C) 1997-2001 by Johannes Overmann <Johannes.Overmann@gmx.de>
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

#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include "tstring.h"
#include "texception.h"


// todo:
// - make Split,Unquote,ReadLine,extractFilename,extractPath 0 byte safe
// - separat functions using tvector<> for better modularity


// 1997:
// 01:45 11 Jun split(): backslash behavior fixed (601 lines)
// 23:50 11 Jun strings may contain 0 bytes
// 12:00 19 Jun some filename extracting added
// 17:00 19 Jun more sophisticated search: ignore_case and whole_words
// 02:00 08 Jul substring extraction via operator() (start,end)
// 02:00 31 Jul new ContainsNulChar, new ReadFile, fixed \ \\ in ExpUnPrint
// 12:00 08 Aug new Upper Lower Capitalize
// 23:30 19 Aug improved collapseSpace()
// 00:00 27 Aug cropSpace() bug fixed (1 byte out of bound zero write)
// 20:00 30 Aug now cons accept 0 pointer as empty string
// 21:00 30 Aug addDirSlash() added (809 lines)
// 13:00 02 Sep isLower ... added, preserve_case for SearchReplace added (867)
// 23:45 16 Dec normalizePath() added
// 15:00 24 Dec started conversion to Rep reference model
// 18:00 27 Dec finished. debugging starts ... :)

// 1998:
// 00:30 09 Jan scanTools started (cc=817) (h=462)
// 00:05 12 Jan compare operators fixed (0 byte ...)
// 19:00 09 Oct zeroRep and fast string(int i) for i=0 
// 14:30 10 Oct xc16emu emuwid.s problem solved: memset()
// 14:36 10 Oct string(0) 80 times faster than string(1)! (zero_rep)
// 01:53 17 Oct createNulRep and createZeroRep non inline

// 1999:
// 14:55 31 Jan +=string speedup for empty string (cc=919, h=532)
// 15:08 31 Jan searchReplace: pre/post_padstring added
// 00:36 03 Feb getFitWordsBlock added (954)
// 23:02 04 Feb search/searchReplace match_pos added (954)
// 23:49 15 Feb class string renamed to class tstring, tappframe simplified (1003)
// 00:46 16 Feb toLong/toDouble/toInt/toBool added (from old str2value.cc) (1016)
// 23:51 03 Mar cropSpaceEnd added, getFitWords space semantics change
// 23:46 13 Apr trelops.h replaces != and > operator (1034)
// 00:31 16 Apr started: replace fatalErrors by exceptions
// 23:48 20 Aug remove html tags added
// 22:17 09 Dec added operator != and > because trelops will not instantiate them for two different types

// 2000:
// 23:30 30 Jun loop changed from while(1) to for(;;) ;-)
// 22:50 01 Jul toInt/Long pointer p initialized to 0, quotes feature added to expandUnprintable
// 22:00 06 Jul progressBar() added

// 2001:
// 00:15 08 Feb extractPath now removed trailing slash (1090 lines)
// 00:45 15 Mar searchReplace max_num parameter added
// 22:00 18 Sep palmos fixes

// 2002:
// 22:25 08 Apr expandUnpritable: allow high ISO graphical characters (ASCII 161-255), better nul_mem and zero_mem sizes for 64 bit systems

// 2003:
// 22:20 27 Jan length of nul_mem and zero_mem fixed

// 2006:
// 27 Jul: palmos support removed


// global static null and zero rep members
tstring::Rep* tstring::Rep::nul = 0;
char tstring::Rep::nul_mem[sizeof(Rep) + 1];
tstring::Rep* tstring::Rep::zero = 0;
char tstring::Rep::zero_mem[sizeof(Rep) + 2];


// non inline Rep implementations

// copy this representation
tstring::Rep *tstring::Rep::clone(size_t minmem) {
   Rep *p = create(minmem >= len ? minmem : len);
   p->len = len;
   memcpy(p->data(), data(), len+1);
   return p; 
}

// create a new representation
tstring::Rep *tstring::Rep::create(size_t tmem) {
   size_t m = sizeof(Rep) << 1;
   while((m - 1 - sizeof(Rep)) < tmem) m <<= 1;
   Rep *p = new (m - 1 - sizeof(Rep)) Rep;
   p->mem = m - 1 - sizeof(Rep); p->ref = 1; p->vulnerable = false;
   return p;
}

// create null string representation
void tstring::Rep::createNulRep() {
   nul = (Rep *)nul_mem;
   nul->len = 0;
   nul->mem = 0;
   nul->ref = 1; // never modify/delete static object
   nul->vulnerable = false;
   nul->terminate();
}

// create zero string representation
void tstring::Rep::createZeroRep() {
   zero = (Rep *)zero_mem;
   zero->len = 1;
   zero->mem = 1;
   zero->ref = 1; // never modify/delete static object
   zero->vulnerable = false;
   (*zero)[0] = '0';
   zero->terminate();
}
      
      
// non inline string implelentation

tstring::tstring(const char *s):rep(0) {
   if(s){
      int l = strlen(s);
      rep = Rep::create(l);
      rep->len = l;
      strcpy(rep->data(), s);
   } else rep = Rep::nulRep()->grab();
}


tstring::tstring(const char *s, size_t l):rep(0) {
   if(s && (l > 0)) {
      rep = Rep::create(l);
      rep->len = l;
      memcpy(rep->data(), s, l);      
      rep->terminate();
   } else rep = Rep::nulRep()->grab();
}


tstring::tstring(char c, size_t n):rep(0) {
   if(n) {
      rep = Rep::create(n);
      rep->len = n;
      if(n) memset(rep->data(), c, n);      
      rep->terminate();      
   } else rep = Rep::nulRep()->grab();
}


tstring::tstring(char c):rep(0) {
   rep = Rep::create(1); 
   rep->len = 1; 
   (*rep)[0] = c; 
   rep->terminate();
}


tstring::tstring(int i):rep((i==0)?(Rep::zeroRep()->grab()):(Rep::nulRep()->grab())) {
   if(i) sprintf("%d", i);
}


tstring::tstring(int i, const char *format):rep(Rep::nulRep()->grab()) {
   sprintf(format, i);
}


tstring::tstring(double d, const char *format):rep(Rep::nulRep()->grab()) {
   sprintf(format, d);
}



tstring operator + (const tstring& s1, const tstring& s2) {
   tstring r(s1); r += s2; return r; }
tstring operator + (const char *s1, const tstring& s2) {
   tstring r(s1); r += s2; return r; }
tstring operator + (const tstring& s1, const char *s2) {
   tstring r(s1); r += s2; return r; }
tstring operator + (char s1, const tstring& s2) {
   tstring r(s1); r += s2; return r; }
tstring operator + (const tstring& s1, char s2) {
   tstring r(s1); r += tstring(s2); return r; }

bool operator == (const tstring& s1, const tstring& s2) {return tstring::_string_equ(s1, s2);}
bool operator == (const tstring& s1, const char   *s2) {return (strcmp(s1.c_str(), s2)==0);}
bool operator == (const char   *s1, const tstring& s2) {return (strcmp(s1, s2.c_str())==0);}
bool operator != (const tstring& s1, const tstring& s2) {return !tstring::_string_equ(s1, s2);}
bool operator != (const tstring& s1, const char   *s2) {return (strcmp(s1.c_str(), s2)!=0);}
bool operator != (const char   *s1, const tstring& s2) {return (strcmp(s1, s2.c_str())!=0);}
bool operator <  (const tstring& s1, const tstring& s2) {return (tstring::_string_cmp(s1, s2) < 0);}
bool operator <  (const tstring& s1, const char   *s2) {return (strcmp(s1.c_str(), s2) < 0);}
bool operator <  (const char   *s1, const tstring& s2) {return (strcmp(s1, s2.c_str()) < 0);}
bool operator >  (const tstring& s1, const char   *s2) {return (strcmp(s1.c_str(), s2) > 0);}
bool operator >  (const char   *s1, const tstring& s2) {return (strcmp(s1, s2.c_str()) > 0);}
bool operator >  (const tstring& s1, const tstring& s2) {return (tstring::_string_cmp(s1, s2) > 0);}

/// append string
tstring& tstring::operator += (const tstring& a) {if(!a.empty()) {append(a.rep->data(), a.rep->len);} return *this;}
/// append cstring
tstring& tstring::operator += (const char *a) {if(a) append(a, strlen(a)); return *this;}
/// append cstring
tstring& tstring::operator += (char c) {detachResize(rep->len + 1); (*rep)[rep->len++]=c; (*rep)[rep->len]=0; return *this;}
/// append byte array a of length len
tstring& tstring::append(const char *a, int alen) {
   if(a) {
      detachResize(rep->len + alen);
      memcpy(rep->data() + rep->len, a, alen);
      rep->len += alen;
      rep->terminate();
   }
   return *this;
}
/// assign string a to this
tstring& tstring::operator = (const tstring& a) 
{if(&a != this) {rep->release(); rep = a.rep->grab();} return *this;}
/// direct character access: const/readonly
char tstring::operator [] (size_t i) const /* throw(IndexOutOfRange) */ {
   if(i <= rep->len) return (*rep)[i];
   else return 0;
}
/// direct character access: read/write
char& tstring::operator[](size_t i) {
   if(i < rep->len) {detach(); return (*rep)[i];}
   detachResize(i + 1);
   for(; rep->len <= i; rep->len++) (*rep)[rep->len] = 0;
   return (*rep)[i];
}

/// substring extraction (len=end-start)
tstring tstring::substr(size_t start, size_t end) const /* throw(InvalidRange) */ {
   if((end == npos) || (end > rep->len)) end = rep->len;
   if(start > rep->len) start = rep->len;
   if(start > end) start = end;
   return tstring(rep->data()+start, end-start); 
}

// compare helpers
int tstring::_string_cmp(const tstring& s1, const tstring& s2) {
   int r = memcmp(s1.rep->data(), s2.rep->data(), s1.rep->len <= s2.rep->len ? s1.rep->len : s2.rep->len);
   if(r) return r;
   if(s1.rep->len > s2.rep->len) return +1;
   if(s1.rep->len < s2.rep->len) return -1;
   return 0;
}

bool tstring::_string_equ(const tstring& s1, const tstring& s2) {
   if(s1.rep->len != s2.rep->len) return false;
   return memcmp(s1.rep->data(), s2.rep->data(), s1.rep->len)==0;
}

/// detach from string pool, you should never need to call this
void tstring::detach() { if(rep->ref > 1) { replaceRep(rep->clone()); } }
// no, there is *not* a dangling pointer here (ref > 1)
/** detach from string pool and make sure at least minsize bytes of mem are available
 (use this before the dirty version sprintf to make it clean)
 (use this before the clean version sprintf to make it fast)
 */
void tstring::detachResize(size_t minsize) {
   if((rep->ref==1) && (minsize <= rep->mem)) return;
   replaceRep(rep->clone(minsize));
}
/// detach from string pool and declare that string might be externally modified (the string has become vulnerable)
void tstring::invulnerableDetach() { detach(); rep->vulnerable = true; }

/// check for 0 in string (then its not a real cstring anymore)
bool tstring::containsNulChar() const {
   rep->terminate();
   if(strlen(rep->data()) != rep->len) 
     return true; 
   else 
     return false;
}


/// get a pointer to the at most max last chars (useful for printf)
const char *tstring::pSuf(size_t max) const {
   return rep->data()+((max>=rep->len)?0:(rep->len-max));
}


/// sprintf into this string
void tstring::sprintf(const char *format, ...) {
   va_list ap;
   int ret = -1;
   va_start(ap, format);
#if defined(__STRICT_ANSI__)
   // this is the unsecure and dirty but ansi compatible version
   detachResize(256);      
   ret = vsprintf(rep->data(), format, ap); // not secure! may write out of bounds!
#else
   // this is the clean version (never overflows)
   int s = 16/4;
   do { 
      if(ret <= s)
	s <<= 2; // fast increase, printf may be slow
      else 
	s = ret + 8; // C99 standard, after first iteration this should be large enough
      detachResize(s);
      ret = vsnprintf(rep->data(), s, format, ap); 
   } while((ret == -1) || (ret >= s));
#endif
   va_end(ap);
   rep->len = ret;
}


// returns true on success! returns value in bool_out!
bool tstring::toBool(bool& bool_out) const {
   char buf[7];
   int i;
   for(i=0; i<6; i++) {
      buf[i] = tolower((*rep)[i]);
      if((buf[i]==0) || isspace(buf[i])) break; 
   }
   buf[i]=0;
   switch(i) {
    case 1:
      if((buf[0]=='1')||(buf[0]=='t')) { bool_out = true;  return true; }
      if((buf[0]=='0')||(buf[0]=='f')) { bool_out = false; return true; }
      break;
    case 2:
      if(strcmp(buf,"on")==0)          { bool_out = true;  return true; }
      if(strcmp(buf,"no")==0)          { bool_out = false; return true; }
      break;
    case 3:
      if(strcmp(buf,"yes")==0)         { bool_out = true;  return true; }
      if(strcmp(buf,"off")==0)         { bool_out = false; return true; }
      break;
    case 4:
      if(strcmp(buf,"true")==0)        { bool_out = true;  return true; }
      break;
    case 5:
      if(strcmp(buf,"false")==0)       { bool_out = false; return true; }
      break;
   }   
   return false;
}


// returns true on success
bool tstring::toLong(long& long_out, int base) const {
   char *p = 0;
   long r = strtoul(rep->data(), &p, base);
   if(p == rep->data()) return false;
   if(*p) if(!isspace(*p)) return false;
   long_out = r;
   return true;
}


// returns true on success
bool tstring::toInt(int& int_out, int base) const {
   char *p = 0;
   int r = strtoul(rep->data(), &p, base);
   if(p == rep->data()) return false;
   if(*p) if(!isspace(*p)) return false;
   int_out = r;
   return true;
}


// returns true on success
bool tstring::toDouble(double& double_out) const {
   char *p = 0;
   double r = strtod(rep->data(), &p);
   if(p == rep->data()) return false;
   if(*p) if(!isspace(*p)) return false;
   double_out = r;
   return true;
}


tstring tstring::scanToken(size_t& scanner, int flags, 
		       const char *allow, const char *forbid,
		       bool allow_quoted) const 
{
   if(allow_quoted && (scanner < rep->len)) {
      char q = (*rep)[scanner];
      if((q=='\'')||(q=='\"')) {
	 int st(++scanner);
	 while((scanner < rep->len) && ((*rep)[scanner]!=q))
	   ++scanner;
	 tstring out = substr(st, scanner);	 
	 if(scanner < rep->len) ++scanner;
	 return out;
      }
   }
   size_t start(scanner);
   for(; (scanner < rep->len); ++scanner) {
      char c = (*rep)[scanner];
      if(forbid && strchr(forbid, c)) break; 
      if((flags&ALL                )) continue;
      if(allow  && strchr(allow , c)) continue; 
      if((flags&ALPHA) && isalpha(c)) continue;
      if((flags&DIGIT) && isdigit(c)) continue;
      if((flags&LOWER) && islower(c)) continue;
      if((flags&UPPER) && isupper(c)) continue;
      if((flags&PRINT) && isprint(c)) continue;
      if((flags&GRAPH) && isgraph(c)) continue;
      if((flags&CNTRL) && iscntrl(c)) continue;
      if((flags&SPACE) && isspace(c)) continue;
      if((flags&XDIGIT)&&isxdigit(c)) continue;
      if((flags&PUNCT) && ispunct(c)) continue;
      break;
   }
   return substr(start, scanner);
}


tstring tstring::shortFilename(size_t maxchar) const {
   if(rep->len <= maxchar) return *this;
   if(maxchar < 3) return "";
   return "..." + substr(rep->len - maxchar + 3);
}


void tstring::normalizePath() {
   // split path
   tvector<tstring> a = split(*this, "/", false, false);

   // delete nul dirs 
   for(tvector<tstring>::iterator i = a.begin(); i != a.end();) {
      if(i->empty() || (*i == ".")) i = a.erase(i);
      else i++;
   }
   
   // check for absolute
   if((*rep)[0]=='/') clear();
   else operator=(".");

   // delete '..'
   for(tvector<tstring>::iterator i = a.begin(); i != a.end();) {
      if((*i == "..") && (i != a.begin())) {
	 i--;
	 if(*i != "..") {
	    i = a.erase(i);
	    i = a.erase(i);
	 } else {
	    i++;
	    i++;
	 }
      } else i++;
   }
      
   // assemble string
   if((a.size() > 0) || (len() == 0))
     operator+=("/" + join(a, "/"));
}
void tstring::extractFilename() {
   const char *p = strrchr(rep->data(), '/');
   if(p) operator=(p+1);
}


void tstring::extractPath() {
   const char *p = strrchr(rep->data(), '/');
   if(p) {
      truncate((p - rep->data() + 1));
      removeDirSlash();
   }
   else clear();
}


void tstring::removeDirSlash() {
   if(*this == "/") return;
   while(lastChar() == '/') truncate(rep->len-1);   
}


void tstring::addDirSlash() {
   if(lastChar() != '/') operator += ("/");
}


void tstring::extractFilenameExtension() {
   extractFilename();  // get file name
   const char *p = strrchr(rep->data(), '.');
   if(p) {  // contains dot
      if(p > rep->data()) { // last dot not first char
	 operator=(p+1);    // get extension
	 return;
      }
   }
   clear(); // no extension
}


double tstring::binaryPercentage() const {
   double bin = 0;
   
   for(size_t i = 0; i < rep->len; i++) 
     if((!isprint((*rep)[i])) && (!isspace((*rep)[i]))) bin+=1.0;
   return (bin * 100.0) / double(rep->len);
}


bool tstring::isLower() const {
   if(rep->len == 0) return false;
   for(size_t i = 0; i < rep->len; i++) 
     if(isalpha((*rep)[i])) 
       if(isupper((*rep)[i])) 
	 return false;
   return true;
}


bool tstring::isUpper() const {
   if(rep->len == 0) return false;
   for(size_t i = 0; i < rep->len; i++) 
     if(isalpha((*rep)[i])) 
       if(islower((*rep)[i])) 
	 return false;
   return true;
}


bool tstring::isCapitalized() const {
   if(rep->len == 0) return false;
   if(isalpha((*rep)[0])) if(islower((*rep)[0])) return false;
   for(size_t i = 1; i < rep->len; i++)
     if(isalpha((*rep)[i])) 
       if(isupper((*rep)[i])) 
	 return false;
   return true;   
}


void tstring::lower() {
   detach();
   for(size_t i = 0; i < rep->len; i++) (*rep)[i] = tolower((*rep)[i]);
}


void tstring::upper() {
   detach();
   for(size_t i = 0; i < rep->len; i++) (*rep)[i] = toupper((*rep)[i]);
}


void tstring::capitalize() {
   lower();
   if(rep->len) (*rep)[0] = toupper((*rep)[0]);
}


static const char *bytesearch(const char *mem, int mlen,
			      const char *pat, int plen,
			      bool ignore_case, bool whole_words) {
   int i,j;   
   for(i=0; i <= mlen-plen; i++) {
      if(ignore_case) {
	 for(j=0; j<plen; j++) 
	   if(tolower(mem[i+j]) != tolower(pat[j])) break;
      } else {
	 for(j=0; j<plen; j++) 
	   if(mem[i+j] != pat[j]) break;
      }
      if(j==plen) { // found
	 if(!whole_words) return mem + i;
	 else {
	    bool left_ok = true;
	    bool right_ok = true;
	    if(i > 0) if(isalnum(mem[i-1]) || (mem[i-1]=='_')) 
	      left_ok = false;
	    if(i < mlen-plen) if(isalnum(mem[i+plen]) || (mem[i+plen]=='_')) 
	      right_ok = false;
	    if(left_ok && right_ok) return mem + i;
	 }
      }
   }
   return 0; // not found
}


int tstring::searchReplace(const tstring& tsearch, const tstring& replace_, 
			   bool ignore_case, bool whole_words,
			   bool preserve_case, int progress,
			   const tstring& pre_padstring, const tstring& post_padstring, tvector<int> *match_pos, int max_num) {
   // get new length and positions
   if(progress) { putc('S', stderr);fflush(stderr); }
   int num = search(tsearch, ignore_case, whole_words, progress);
   if(progress) { putc('R', stderr);fflush(stderr); }   
   if(num==0) {
      return 0;
   }
   if(num >= max_num) num = max_num;
   int newlen = rep->len + num*(replace_.rep->len-tsearch.rep->len + 
				pre_padstring.len()+post_padstring.len());

   // create new string 
   Rep *newrep = Rep::create(newlen);   
   const char *p = rep->data();  // read
   char *q =    newrep->data();  // write
   const char *r;                // found substring
   int mlen = rep->len;          // rest of read mem
   for(int i=0; i < num; i++) {
      if(progress>0) if((i%progress)==0) {putc('.', stderr);fflush(stderr);}
      r = bytesearch(p, mlen, tsearch.rep->data(), tsearch.rep->len, ignore_case, whole_words);
      memcpy(q, p, r-p); // add skipped part
      q += r-p;      
      if(match_pos) (*match_pos) += int(q-newrep->data()); // enter start
      memcpy(q, pre_padstring.rep->data(), pre_padstring.rep->len); // add pre pad
      q += pre_padstring.len();
      if(!preserve_case) { // add replaced part
	 memcpy(q, replace_.rep->data(), replace_.rep->len);
      } else {
	 tstring rr(preserveCase(tstring(r, tsearch.rep->len), replace_.rep->data()));
	 memcpy(q, rr.rep->data(), rr.rep->len);
      }
      q += replace_.rep->len;      
      memcpy(q, post_padstring.rep->data(), post_padstring.rep->len); // add post pad
      q += post_padstring.len();
      if(match_pos) (*match_pos) += int(q-newrep->data()); // enter end
      mlen -= r-p;
      mlen -= tsearch.rep->len;
      p = r + tsearch.rep->len;
   }
   memcpy(q, p, mlen); // add rest
   replaceRep(newrep);
   rep->len = newlen;
   rep->terminate();
   return num;
}


int tstring::search(const tstring& pat, bool ignore_case, bool whole_words, int progress, tvector<int> *match_pos) const {
   if(pat.empty()) return -1;
   int num=0;
   int mlen=rep->len;
   const char *q;		      		      
   for(const char *p = rep->data(); (q=bytesearch(p, mlen, pat.rep->data(), pat.rep->len,
					ignore_case, whole_words)); num++) {
      if(match_pos) (*match_pos) += int(q-rep->data());
      mlen -= q-p;
      mlen -= pat.rep->len;
      p = q + pat.rep->len;
      if(match_pos) (*match_pos) += int(p-rep->data());
      if(progress>0) if((num%progress)==0) {putc('.', stderr);fflush(stderr);}
   }
   return num;
}


/// replace substring
void tstring::replace(size_t start, size_t len_, const tstring &str) {
   if(start > length()) return;
   if(start + len_ > length()) return;
   if(str.length() > len_)
     detachResize(length() + str.length() - len_);
   else
     detach();
   if(str.length() != len_)
     memmove(rep->data() + start + str.length(), rep->data() + start + len_, length() - start - len_);
   // insert string
   memcpy(rep->data() + start, str.data(), str.length());
   // fix length
   rep->len += str.length() - len_;
   rep->terminate();
}


bool tstring::hasPrefix(const tstring& pref) const {
   if(pref.rep->len > rep->len) return false;
   return memcmp(rep->data(), pref.rep->data(), pref.rep->len)==0;
}


bool tstring::hasSuffix(const tstring& suf) const {
   if(suf.rep->len > rep->len) return false;
   return memcmp(rep->data() + (rep->len - suf.rep->len), 
		 suf.rep->data(), suf.rep->len)==0;
}


bool tstring::consistsOfSpace() const {
   for(size_t i = 0; i < rep->len; i++) {
      if(!isspace((*rep)[i])) return false;
   }
   return true;
}


void tstring::truncate(size_t max) {
   if(max < rep->len) {
      detach();
      rep->len = max;
      rep->terminate();
   }
}


void tstring::replaceUnprintable(bool only_ascii) {
   for(size_t i = 0; i < rep->len; i++) {
      unsigned char& c = (unsigned char &)(*rep)[i];
      if(!isprint(c)) {
	 if(c < ' ') {
	    c = '!';
	 } else if(only_ascii || (c < 0xa0)) {
	    c = '?';
	 }
      }
   }
}


void tstring::unquote(bool allow_bslash, bool crop_space) {
   detach();
   
   char *p=rep->data();
   char *q=rep->data();
   char quote=0;
   char *nonspace=rep->data();
   
   if(crop_space) while(isspace(*p)) p++;
   for(; *p; p++) {
      if(allow_bslash && *p=='\\') {
	 if(p[1] == quote) {
	    p++;
	    if(*p == 0) break;
	 }
      } else {
	 if(quote) {
	    if(*p == quote) {
	       quote = 0;
	       continue;
	    }
	 } else {
	    if((*p == '\'') || (*p == '\"')) {
	       quote = *p;
	       continue;
	    }
	 }	 
      }
      if(quote || (!isspace(*p))) nonspace = q;
      *(q++) = *p;
   }   
   *q = 0;
   if(crop_space) if(*nonspace) nonspace[1] = 0;
   rep->len = strlen(rep->data());   
}


tstring tstring::getFitWordsBlock(size_t max) {
   tstring r = getFitWords(max);
   size_t spaces;
   size_t fill = max - r.len();
   if(fill > 8) return r;
   size_t i,j;
      
   for(i = 0; i < r.len(); i++)
     if(r[i] != ' ') break;
   for(spaces = 0; i < r.len(); i++)
     if(r[i] == ' ') spaces++;
   if(fill > spaces) return r;
   tstring t;
   t.detachResize(max);
   for(i = 0, j = 0; i < r.len(); i++) {
      if(r[i] != ' ') break;
      (*(t.rep))[j++] = r[i];
   }
   for(; i < r.len(); i++) {
      if((fill > 0)&&(r[i] == ' ')) {
	 (*(t.rep))[j++] = ' ';
	 (*(t.rep))[j++] = ' ';
	 fill--;
      } else (*(t.rep))[j++] = r[i];
   }
   t.rep->len = j;
   t.rep->terminate();
   return t;
}


void tstring::cropSpaceEnd() {
   int e = rep->len;
   
   if(e == 0) return;
   else e--;
   while((e >= 0) && isspace((*rep)[e])) e--;
   truncate(e+1);               
}


tstring tstring::getFitWords(size_t max) {
   if(max < 1) return tstring();

   tstring r(*this); // return value
   
   // check for lf
   size_t lf = firstOccurence('\n');
   if((lf != npos) && (lf <= max)) {
      operator=(substr(lf + 1));
      r.truncate(lf);
      r.cropSpaceEnd();
      return r;
   }
   
   // string fits
   if(rep->len <= max) {
      clear();
      r.cropSpaceEnd();
      return r;
   }
   
   // find space
   size_t last_space = npos;
   for(size_t i = 0; i <= max; i++) {
      if((*rep)[i] == ' ') last_space = i;
   }
   if(last_space == npos) last_space = max;
   
   // return 
   r.truncate(last_space);
   while(isspace((*rep)[last_space])) last_space++;
   operator=(substr(last_space));
   r.cropSpaceEnd();
   return r;
}


void tstring::expandUnprintable(char quotes) {
   Rep *newrep = Rep::create(rep->len*4);
   char *q = newrep->data(); // write
   char *p = rep->data();    // read
   size_t l = 0;
   
   // expand each char
   for(size_t j = 0; j < rep->len; ++j, ++p) {
      if(isprint(*p) || (((unsigned char)*p) > 160)) { // printable --> print
	 if((*p=='\\') || (quotes && (*p==quotes))) { // backslashify backslash and quotes
	    *(q++) = '\\';	 
	    l++;	    
	 } 
	 *(q++) = *p;
	 l++;
      } else { // unprintable --> expand
	 *(q++) = '\\';	// leading backslash
	 l++;
	 switch(*p) {
	  case '\a':
	    *(q++) = 'a';
	    l++;
	    break;
	  case '\b':
	    *(q++) = 'b';
	    l++;
	    break;
	  case '\f':
	    *(q++) = 'f';
	    l++;
	    break;
	  case '\n':
	    *(q++) = 'n';
	    l++;
	    break;
	  case '\r':
	    *(q++) = 'r';
	    l++;
	    break;
	  case '\t':
	    *(q++) = 't';
	    l++;
	    break;
	  case '\v':
	    *(q++) = 'v';
	    l++;
	    break;
	  default: // no single char control
	    unsigned int i = (unsigned char)*p;
	    l += 3;
	    if(i < 32) {  // print lower control octal
	       if(isdigit(p[1])) {
		  q += ::sprintf(q, "%03o", i);
	       } else {
		  q += ::sprintf(q, "%o", i);
		  if(i>=8) --l;
		  else l-=2;
	       }
	    } else {    // print octal or hex
	       if(isxdigit(p[1])) {
		  q += ::sprintf(q, "%03o", i);
	       } else {
		  q += ::sprintf(q, "x%02x", i);
	       }
	    }
	 }
      }
   }
   
   // end
   replaceRep(newrep);
   rep->len = l;
   rep->terminate();
}


void tstring::backslashify() {
   Rep *newrep = Rep::create(rep->len*2);
   char *p = rep->data();
   char *q = newrep->data();
   int l = 0;
   
   // backslashify each char
   for(size_t i = 0; i < rep->len; i++, p++) {
      switch(*p) {
       case '\\':
	 *(q++) = '\\';
	 *(q++) = '\\';
	 l+=2;
	 break;
       case '\'':
	 *(q++) = '\\';
	 *(q++) = '\'';
	 l+=2;
	 break;
       case '\"':
	 *(q++) = '\\';
	 *(q++) = '\"';
	 l+=2;
	 break;
       default:
	 *(q++) = *p;
	 l++;
	 break;
      }
   }
   
   // end
   replaceRep(newrep);
   rep->len = l;
   rep->terminate();
}


void tstring::compileCString() {
   detach();

   char *p = rep->data(); // read
   char *q = rep->data(); // write
   char c;                // tmp char
   size_t l = 0;          // write
   size_t i = 0;          // read
   
   while(i < rep->len) {
      c = *(p++); // read char
      i++;
      if(c == '\\') { // compile char
	 if(i>=rep->len) break;
	 c = *(p++);
	 i++;
	 switch(c) {
	  case 'a':
	    c = '\a';
	    break;
	  case 'b':
	    c = '\b';
	    break;
	  case 'f':
	    c = '\f';
	    break;
	  case 'n':
	    c = '\n';
	    break;
	  case 'r':
	    c = '\r';
	    break;
	  case 't':
	    c = '\t';
	    break;
	  case 'v':
	    c = '\v';
	    break;
	  case 'x': // hex
	    char *qq;
	    c = strtol(p, &qq, 16);
	    i += qq-p;
	    p = qq;
	    break;	    
	  case '0': // octal
	  case '1':
	  case '2':
	  case '3':
	  case '4':
	  case '5':
	  case '6':
	  case '7':
	    char buf[4];
	    buf[0] = c;
	    buf[1] = *p;
	    buf[2] = (i < rep->len) ? p[1] : 0;
	    buf[3] = 0;
	    char *t;
	    c = strtol(buf, &t, 8);
	    i += (t-buf)-1;
	    p += (t-buf)-1;
	    break;	    
	 }	 
      } 
      *(q++) = c; // write char
      l++;
   }
   rep->len = l;
   rep->terminate();
}


void tstring::removeHTMLTags(int& level) {
   detach();

   char *p = rep->data(); // read
   char *q = rep->data(); // write
   size_t l = 0;          // write
   size_t i = 0;          // read
   
   while(i < rep->len) {
      switch(*p) {
       case '<': 
	 level++;
	 break;

       case '>':
	 if(level > 0) level--;
	 break;
	 
       default:
	 if(level == 0) {
	    *(q++) = *p;
	    l++;
	 }
      }      
      p++;
      i++;
   }
   
   rep->len = l;
   rep->terminate();
}


void tstring::cropSpace(void) {
   size_t first = rep->len;
   size_t last = 0;
   size_t i;
   
   // get first nonspace
   for(i = 0; i < rep->len; ++i) 
     if(!isspace((*rep)[i])) {
	first = i;
	break;
     }
   
   // full of spaces   
   if(first == rep->len) {
      clear();
      return;
   }
   
   // get last nonspace
   for(i = rep->len - 1; i >= first; --i) 
     if(!isspace((*rep)[i])) {
	last = i;
	break;
     }
   ++last;
   
   // truncate
   if(first == 0) {
      truncate(last);
      return;
   }
     
   // extract substring
   operator=(substr(first, last));   
}


void tstring::collapseSpace(void) {
   detach();
   
   char *p = rep->data(); // read
   char *q = rep->data(); // write
   char last_char = ' ';
   size_t l = 0;          // length
   char c;
   
   for(size_t i = 0; i < rep->len; ++i, ++p) {
      if((!isspace(*p)) || (!isspace(last_char))) {
	 c = *p;
	 if(isspace(c)) c=' ';
	 *(q++) = c;
	 last_char = c;
	 l++;
      }
   }
   if(isspace(last_char)&&(l>0)) --l;
   rep->len = l;
   rep->terminate();
}


void tstring::translateChar(char from, char to) {
   detach();   
   char *p = rep->data();   
   for(size_t i = 0; i < rep->len; ++i, ++p)
     if(*p == from) *p = to;
}


size_t tstring::firstOccurence(char c) const {
   size_t i;
   
   for(i = 0; (i < rep->len) && ((*rep)[i] != c); ++i) /* empty body */;
   if(i < rep->len) return i;
   else return npos;
}



// non member implementation


tvector<tstring> split(const tstring &s, const char *sep, bool allow_quoting, bool crop_space) {
   tvector<tstring> r;
   tstring buf;
   const char *p = s.c_str();
   p--; // bias
   
   do {
      // next chunk
      p++;	  
      
      // collect chars to buf
      while(*p) {
	 if(strchr(sep, *p)) {
	    break;
	 } else	if(!allow_quoting) {
	    buf += *(p++);	    
	 } else if(*p=='\\') {
	    p++;
	    if(strchr(sep, *p)==0) buf += '\\';
	    if(*p) buf += *(p++);
	 } else if(*p=='\'') {
	    buf += '\'';
	    for(p++; *p && *p!='\''; p++) {
	       if(*p=='\\') {
		  p++;
		  buf += '\\';
		  if(*p) buf += *p;
	       } else 
		 buf += *p;
	    }
	    buf += '\'';
	    if(*p=='\'') p++;
	 } else if(*p=='\"') {
	    buf += '\"';
	    for(p++; *p && *p!='\"'; p++) {
	       if(*p=='\\') {
		  p++;
		  buf += '\\';
		  if(*p) buf += *p;
	       } else 
		 buf += *p;
	    }
	    buf += '\"';
	    if(*p=='\"') p++;
	 } else {
	    buf += *(p++);
	 }
      }
      
      // put buf to r
      if(crop_space) buf.cropSpace();
      r.push_back(buf);

      // cleanup
      buf.clear();
   } while(*p);
   
   return r;
}


tstring join(const tvector<tstring>& a, const tstring& sep) {
   tstring r;
   
   if(a.empty()) return r;
   else r = a[0];   
   for(size_t i = 1; i < a.size(); i++) {
      r += sep;
      r += a[i]; 
   }
   return r;
}


tstring preserveCase(const tstring& from, const tstring& to) {
   tstring r(to);
   
   if(from.len() == to.len()) { 
      // same len
      for(size_t i = 0; i < r.len(); i++) {
	 if(islower(from[i])) r[i] = tolower(r[i]);
	 else if(isupper(from[i])) r[i] = toupper(r[i]);
      }
   } else {   
      // some heuristics
      if(from.isLower()) r.lower();
      if(from.isUpper()) r.upper();
      if(from.isCapitalized()) r.capitalize();
   }
   
   return r;
}


const char *progressBar(const char *message, unsigned int n, unsigned int max, int width) {
   // max size of a buffer
#define size 1024
   // number of static buffers (must be power of two)
#define numbuf 4
   static char tbuf[size * numbuf];
   static int tphase = 0;
   static int phase = 0;
   static char phasechar[] = "/-~-_-\\|";

   tphase++;
   tphase &= numbuf - 1;
   char *buf = tbuf + size * tphase;
   
   // limit width
   if(width >= size) width = size - 1;
   if(message == 0) {
      // clear line
      sprintf(buf, "%*s", width, "");
      return buf;
   }
   if(max == 0) {
      // open end progress
      if(phasechar[phase] == 0) phase = 0;
      sprintf(buf, "%.*s %11d %c", width - (11 - 3), message, n, phasechar[phase++]);
      return buf;
   }
   
   // proportional progress
    
   // get num chars for number and max
   int nlen = 0, i;
   for(i = max; i; i /= 10, nlen++) /* empty body */;
   
   int l = sprintf(buf, "%.*s %*d/%*d (%5.1f%%) ", width - (12 + 2 * nlen), message, nlen, n, nlen, max, double(n)/double(max)*100.0);
   int rest = width - l;
   if(rest <= 0) return buf;
   int done = int(double(n)/double(max)*double(rest));
   if(done > rest) done = rest;
   char *p = buf + l;
   for(i = 0; i < done; i++) *(p++) = '*';
   for(; i < rest; i++) *(p++) = '.';
   *p = 0;
   return buf;
#undef size
}


bool tstring::readLine(FILE *file) {
   char buf[1024];
   
   clear();
   for(;;) {	 
      buf[sizeof(buf)-2] = '\n';
      if(!fgets(buf, sizeof(buf), file)) break;
      operator+=(buf);
      if(buf[sizeof(buf)-2] == '\n') break;
   }
   if(rep->len) return true;
   else    return false;
}


size_t tstring::write(FILE *file) const {
   return fwrite(rep->data(), 1, rep->len, file);
}


size_t tstring::read(FILE *file, size_t l) {
   rep->release();
   rep = Rep::create(l);
   int r = fread(rep->data(), 1, l, file);
   rep->len = r;
   rep->terminate();
   return r;
}


int tstring::readFile(const char *filename) {
   struct stat buf;

   if(stat(filename, &buf)) return -1; // does not exist
   FILE *f=fopen(filename, "rb");
   if(f == 0) return -2;                 // no permission?
   int r = read(f, buf.st_size);
   fclose(f);
   if(r != buf.st_size) return -3;     // read error
   return 0;
}


int tstring::writeFile(const char *filename) {
   FILE *f = fopen(filename, "wb");
   if(f == 0) return -2;                 // no permission?
   int r = write(f);
   fclose(f);
   if(r != int(length())) return -3;     // write error
   return 0;
}


tvector<tstring> loadTextFile(const char *fname) {
   FILE *f = fopen(fname, "r");
   if(f==0) throw TFileOperationErrnoException(fname, "fopen(mode='r')", errno);
   tvector<tstring> r;
   for(size_t i = 0; r[i].readLine(f); i++) /* empty body */;
   fclose(f);
   r.pop_back();
   return r;
}


tvector<tstring> loadTextFile(FILE *file) {
   tvector<tstring> r;
   for(size_t i = 0; r[i].readLine(file); i++) /* empty body */;
   r.pop_back();
   return r;
}

