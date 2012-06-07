/*GPL*START*
 * 
 * tstring - NUL byte tolerant sophisticated string class
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

#ifndef _ngw_tstring_h_
#define _ngw_tstring_h_

#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include "tvector.h"
#include "texception.h"

using namespace std;

/**@name null tolerant string class */
/*@{*/
/// null tolerant string class
class tstring {
 public:
   // invalid iterator
   static const size_t npos = static_cast<size_t>(-1);
   // flags for scanToken()
   enum {ALPHA=1, NUM=2, DIGIT=2, LOWER=4, UPPER=8, PRINT=16, XDIGIT=32, 
      SPACE=64, ALNUM=1|2, PUNCT=128, CNTRL=256, GRAPH=1024,
      ALL=2048, NONE=0};
   /// case flags for modify case
   enum {NOT=0, CAPITALIZE=-1};
 private:
   // internal string representation
   class Rep {
    public:
      size_t len; // length without term 0 byte
      size_t mem; // allocated mem without term 0 byte
      int ref; // reference count (>=1)
      bool vulnerable; // true == always grab by clone, never by reference
      //                  (the string has become vulnerable to the outside)
      // char data[mem+1]; string data follows (+1 for term 0 byte)
      
      // return pointer to string data
      char *data() {return (char *)(this + 1);} // 'this + 1' means 'the byte following this object'
      // character access
      char& operator[] (size_t i) {return data()[i];}
      // reference
      Rep* grab() {if(vulnerable) return clone(); ++ref; return this;}
      // dereference
      void release() {if(--ref == 0) delete this;}
      // copy this representation
      Rep *clone(size_t minmem = 0);
      // terminate string with 0 byte
      void terminate() {*(data()+len) = 0;} // set term 0 byte
      
      // static methods
      // operator new for this class
      static void * operator new (size_t size, size_t tmem) {
	 return ::operator new (size + tmem + 1);}
      static void operator delete (void *p, size_t) {
	 ::operator delete (p); }
      
      // create a new representation
      static Rep *create(size_t tmem);
            
      // return pointer to the null string representation
      static Rep * nulRep() {if(nul == 0) createNulRep(); return nul;}

      // return pointer to the zero string representation (string conatining a literal 0: "0" (and not "\0"))
      static Rep * zeroRep() {if(zero == 0) createZeroRep(); return zero;}
	 
      // create null string representation
      static void createNulRep();
      
      // create zero string representation
      static void createZeroRep();

    private:
      // static null string ("") representation
      static Rep* nul;
      static char nul_mem[];
      // static zero string ("0") representation
      static Rep* zero;
      static char zero_mem[];
      
      // forbid assignement
      Rep& operator=(const Rep&);
   };
   
 public:
   /**@name constructor & destructor */
   /*@{*/
   /// default construction
   tstring(): rep(Rep::nulRep()->grab()) {}
   /// copy construction
   tstring(const tstring& a):rep(a.rep->grab()) {}
   /// init from cstring
   tstring(const char *s);
   /// extract bytearray s of length len 
   tstring(const char *s, size_t len);
   /// create string of chars c with length n
   explicit tstring(char c, size_t n);
   /// char to string conversion
   explicit tstring(char c);
   /// int to string conversion
   explicit tstring(int i);
   /// int to string conversion with format
   explicit tstring(int i, const char *format);
   /// double to string conversion
   explicit tstring(double d, const char *format = "%g");
   /// destructor
   ~tstring() {rep->release();}
   /*@}*/
      
   
   /**@name main interface */
   /*@{*/
   /// return length in bytes
   size_t len() const {return rep->len;}
   /// return length in bytes
   size_t length() const {return rep->len;}
   /// return length in bytes
   size_t size() const {return rep->len;}
   /// clear string
   void clear() {replaceRep(Rep::nulRep()->grab());}
   /// explicit conversion to c string
   // const char *operator*() const {return rep->data();}
   /// explicit conversion to c string
   const char *c_str() const {return rep->data();}
   /// explicit conversion to c string
   const char *data() const { return rep->data();}
   /// direct raw data access: user with caution
   char *rawdata() { invulnerableDetach(); return rep->data(); }
   /// return true if string is empty, else false
   bool empty() const {return rep->len == 0;}
   /// append string
   tstring& operator += (const tstring& a);
   /// append cstring
   tstring& operator += (const char *a);
   /// append cstring
   tstring& operator += (char c);
   /// append byte array a of length len
   tstring& append(const char *a, int alen);
   /// assign string a to this
   tstring& operator = (const tstring& a);
   /// direct character access: const/readonly
   char operator [] (size_t i) const;
   /// direct character access: read/write
   char& operator [] (size_t i);
   /// substring extraction (len=end-start)
   tstring substr(size_t start, size_t end = npos) const;
   /// ASCII to number conversion
   bool toLong(long& long_out, int base = 0) const;
   bool toInt(int& int_out, int base = 0) const;
   int getInt(int base = 0) const { int i = 0; toInt(i, base); return i; }
   bool toDouble(double& double_out) const;
   bool toBool(bool& bool_out) const;
   /*@}*/
   
      
   /**@name scanning */
   /*@{*/
   /// return a scanned token with scanner
   tstring scanToken(size_t& scanner, int flags, 
		  const char *allow=0, const char *forbid=0, 
		  bool allow_quoted=false) const;
   /// scan a token or quoted string to out with scanner
   tstring scanString(size_t& scanner, int flags, 
		  const char *allow=0, const char *forbid=0) const {
		     return scanToken(scanner, flags, allow, forbid, true);}
   /// scan a token up to char upto
   tstring scanUpTo(size_t& scanner, char upto) const {
      int start(scanner);
      while((scanner < rep->len)&&((*rep)[scanner]!=upto)) ++scanner;
      return substr(start, scanner);}
   /// scan a token to out up to chars upto
   tstring scanUpTo(size_t& scanner, const char *upto) const {
      int start(scanner);
      while((scanner < rep->len)&&(strchr(upto, (*rep)[scanner])==0))
	++scanner;
      return substr(start, scanner);}
   /// return the rest of the scanned string
   tstring scanRest(size_t& scanner) const {if(scanner < rep->len) {
      int start(scanner);scanner=rep->len;return substr(start, scanner);
   } return tstring();}   
   /// skip spaces
   void skipSpace(size_t& scanner) const
   {while((scanner < rep->len)&&isspace((*rep)[scanner]))++scanner;}
   /// perhaps skip one char c
   void perhapsSkipOneChar(size_t& scanner, char c) const 
   {if((scanner < rep->len)&&((*rep)[scanner]==c)) ++scanner;}
   /// return true if the end of string (eos) is reached
   bool scanEOS(size_t scanner) const
   {if(scanner >= rep->len) return true; else return false;}
   
   
   /// return the last character in the string or 0 if empty
   char lastChar() const {return rep->len?(*rep)[rep->len-1]:0;}
   /// return the first character in the string or 0 if empty
   char firstChar() const {return (*rep)[0];}
   /// return true if entire string consists of whitespace
   bool consistsOfSpace() const;
   /// return true if string has prefix 
   bool hasPrefix(const tstring& prefix) const;
   /// return true if string has suffix 
   bool hasSuffix(const tstring& suffix) const;
   /// return index of first occurence of char c or npos if not found
   size_t firstOccurence(char c) const;
   /// check whether char is contained or not
   bool contains(char c) const { return firstOccurence(c) != npos; }
   /// remove whitespace at beginning and end 
   void cropSpace();
   /// remove whitespace at end
   void cropSpaceEnd();
   /// collapse whitespace 
   void collapseSpace();
   /// replace char from with char to
   void translateChar(char from, char to);
   /// expand unprintable chars to C-style backslash sequences
   void expandUnprintable(char quotes = 0);
   /// backslashify backslash and quotes 
   void backslashify();
   /// compile C-style backslash sequences back to unprintable chars
   void compileCString();
   /// truncate to maximal length max
   void truncate(size_t max);
   /// replace unprintable characters for safe printing
   void replaceUnprintable(bool only_ascii = true);
   /**
    remove quotes
    @param allow_bslash true == backslashing allowed to protect quotes
    @param crop_space   true == remove leading/trailing spaces not protected by quotes
    */
   void unquote(bool allow_bslash = true, bool crop_space = true);
   /// return and remove the first words that fit into a string of length max
   tstring getFitWords(size_t max); // throw(InvalidWidth);
   /// remove the first words that fit into a string of length max and return in block format
   tstring getFitWordsBlock(size_t max); // throw(InvalidWidth);
   /// remove html tags (level == number of open brakets before call, init:0)
   void removeHTMLTags(int& level);
   /*@}*/
      
   /**@name search/replace */
   /*@{*/
   /// replace substring search with replace, return number of replacements (not regexp, use TRegEx to match regular expressions)
   int searchReplace(const tstring& search, const tstring& replace,
		     bool ignore_case=false, bool whole_words=false, 
		     bool preserve_case=false, int progress=0,
		     const tstring& pre_padstring=tstring(), 
		     const tstring& post_padstring=tstring(), tvector<int> *match_pos=0, int max_num = INT_MAX);
   /// return number of occurences of pat (not regexp) returns -1 on empty pat
   int search(const tstring& pat, 
	      bool ignore_case=false, bool whole_words=false,
	      int progress=0, tvector<int> *match_pos=0) const; // throw(StringIsEmpty);
   /// replace substring
   void replace(size_t start, size_t len, const tstring &str);
   /*@}*/
      
   /**@name file I/O */
   /*@{*/
   /// read line from file like fgets, no line length limit
   bool readLine(FILE *file);
   /// write string to file, return number of bytes written
   size_t write(FILE *file) const;
   /// read len bytes from file to string, return bytes read
   size_t read(FILE *file, size_t len); // throw(InvalidWidth);
   /// read whole file into one string, return 0 on success -x on error
   int readFile(const char *filename);
   /// write string into file, return 0 on success -x on error
   int writeFile(const char *filename);
   /*@}*/
   
   /**@name filename manipulation */
   /*@{*/
   /// remove leading path from filename
   void extractFilename();
   /// remove part after last slash
   void extractPath();   
   /// add a slash at the end if it is missing
   void addDirSlash();
   /// remove last char if last char is a slash
   void removeDirSlash();      
   /// extract part after the last dot (empty string if no extension, leading dot is ignored)
   void extractFilenameExtension();
   /// make paths comparable (kill multislash, dots and resolve '..')
   void normalizePath();
   /// check for absolute path
   bool isAbsolutePath() const {if((*rep)[0]=='/') return true; return false;}
   /// get truncated filename (for printing puroses)
   tstring shortFilename(size_t maxchar) const;
   /*@}*/
   
   /**@name misc */
   /*@{*/
   /// get percentage of nonprintable and nonspace chars (0.0 .. 100.0)
   double binaryPercentage() const;
   /// check for 0 in string (then its not a real cstring anymore)
   bool containsNulChar() const;
   /// get a pointer to the at most max last chars (useful for printf)
   const char *pSuf(size_t max) const;
   /// sprintf into this string
   void sprintf(const char *format, ...);
   /*@}*/
   
   /**@name case */
   /*@{*/
   /// convert to lower case
   void lower();
   /// convert to upper case
   void upper();
   /// convert to lower case, first char upper case
   void capitalize();
   /// check for lower case, empty string returns false      
   bool isLower() const;
   /// check for upper case, empty string returns false      
   bool isUpper() const;
   /// check for capitalized case, empty string returns false      
   bool isCapitalized() const;
   /*@}*/
      
 public:
   /**@name detach methods */
   /*@{*/
   /// detach from string pool, you should never need to call this
   void detach();
   // no, there is *not* a dangling pointer here (ref > 1)
   /** detach from string pool and make sure at least minsize bytes of mem are available
    (use this before the dirty version sprintf to make it clean)
    (use this before the clean version sprintf to make it fast)
    */
   void detachResize(size_t minsize);
   /// detach from string pool and declare that string might be externally modified (the string has become vulnerable)
   void invulnerableDetach();
   /*@}*/
   
 private:
   // hidden string representation
   Rep *rep;
   
   // private methods
   void replaceRep(Rep *p) {rep->release(); rep = p;}

 public:
   // compare helpers
   static int _string_cmp(const tstring& s1, const tstring& s2);
   static bool _string_equ(const tstring& s1, const tstring& s2);
};




/**@name concat operators */
/*@{*/
///
tstring operator + (const tstring& s1, const tstring& s2);
///
tstring operator + (const char *s1, const tstring& s2);
///
tstring operator + (const tstring& s1, const char *s2);
///
tstring operator + (char s1, const tstring& s2);
///
tstring operator + (const tstring& s1, char s2);
/*@}*/



/**@name compare operators */
/*@{*/
///
bool operator == (const tstring& s1, const tstring& s2);
///
bool operator == (const tstring& s1, const char   *s2);
///
bool operator == (const char   *s1, const tstring& s2);
///
bool operator != (const tstring& s1, const tstring& s2);
///
bool operator != (const tstring& s1, const char   *s2);
///
bool operator != (const char   *s1, const tstring& s2);
///
bool operator <  (const tstring& s1, const tstring& s2);
///
bool operator <  (const tstring& s1, const char   *s2);
///
bool operator <  (const char   *s1, const tstring& s2);
///
bool operator >  (const tstring& s1, const char   *s2);
///
bool operator >  (const char   *s1, const tstring& s2);
///
bool operator >  (const tstring& s1, const tstring& s2);
/*@}*/


/**@name misc friends and nonmembers */
/*@{*/
/// split string into pieces by characters in c-str separator
tvector<tstring> split(const tstring& s, const char *separator,
		     bool allow_quoting=false,
		     bool crop_space=false);

/// join, reverse the effect of split
tstring join(const tvector<tstring>& a, const tstring& separator);

/// try to preserve case from 'from' to 'to' and return altered 'to' with case from 'from'
tstring preserveCase(const tstring& from, const tstring& to);

/// modify case 
inline tstring modifyCase(const tstring& s, int _case) {
   tstring r(s);
   switch(_case) {
    case tstring::UPPER:      r.upper(); break;
    case tstring::LOWER:      r.lower(); break;
    case tstring::CAPITALIZE: r.capitalize(); break;
    default: break;      
   }
   return r;
}

/// Create progress bar
const char *progressBar(const char *message = 0, unsigned int n = 0, unsigned int max = 0, int width = 79);

/// load text file to array of strings
tvector<tstring> loadTextFile(const char *fname);
/// load text file to array of strings
tvector<tstring> loadTextFile(FILE *file);

/*@}*/
/*@}*/

#endif /* _ngw_tstring_h_ */
