/*GPL*START*
 *
 * tappconfig.cc - console application framework
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

#include <ctype.h>
#include <limits.h>
#include <float.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include "tappconfig.h"

// config:

// global email of author:
const char *TAPPCONFIG_EMAIL = "Johannes.Overmann@gmx.de";
// gpl message
const char *TAPPCONFIG_GPL = "this program is distributed under the terms of the GNU General Public License version 2";


// usage:
// ======
//
//    TAppConfig(const char *optionlist[], const char *listname,    /* option list (see below) */
//	      int argc, char *av[],                                 /* argc and argv from main or qt */
//	      const char *envstrname,                               /* environment variable for options (0=none) */
//	      const char *rcname,                                   /* name for config file (0=none) (use "foorc" to get ~/.foorc and /etc/foorc */
//	      const string& version);                               /* application version */
// 
// the name of the application is taken from argv[0] 
// the whole magic is in optionlist:
// optionlist is a list of c-strings (usually static string constants)
// which is terminated by the string "EOL" (end of list, three chars),
// each string may contain either an option item or a meta command
//
// meta commands: (order and position does not matter)
// (all meta commands begin with a '#' char)
// ---------------------------------------------------
// #usage="string"		       /* set usage, printed on top of --help */
// #trailer="string"		       /* set trailer, printed at the bottom of --help */
// #onlycl   			       /* this application does not use config files */
// #stopat--			       /* stop option parsing after a doubledash (--) */
// #remove--			       /* remove the first doubledash from parameterlist (--) */
// #ignore_negnum		       /* something like -20 or -.57 is not treated as option */
// #commonheadline		       /* headline on top of --help */
//
// the usage and the trailer strings may contain the following variable names:
// %n      is replaced by the application name
// %v      is replaced by the application version
// %e      is replaved by the authors email address
// %gpl    is replaced by a one line version of the GNU General Public License
//
//
// option item:
// ------------
// an option item string contains a comma separated list of tags. some of
// the tags take an argument (commas must be quoted by a backslash or 
// quotes). some tags are optional, others required.
// each such line defines an option item (i.e. a command line options)
// 
// required tags:
// --------------
// name="name"			       /* the long option name and also the internal name of that option */
// type=type:			       /* type of the option */
//      string      expected parameter is a character string of arbitrary length, a fallback type for all kinds of parameters
//      int         expected parameter is an integer
//      double      expected parameter is a double precision floating point value
//      switch      no parameter is expected: this is more like a switch (like -l in ls -l)
//      bool        expected parameter is a truth value: 1/t/on/yes/true for 'true' and 0/f/off/no/false for 'false' (case insensitive)
// help="help"                         /* the help message (for --help), the text will be formatted correctly at runtime by TAppConfig */
//                                     /* so do not enter newlines unless you really want to end a paragraph */
//
// optional tags:
// --------------
// char=c			       /* the short option name (like -c) */
// param=PAR			       /* what to print after an option that takes an argument in --help (i.e. --width=NUM) */
// headline="foo options:"	       /* headline, printed before this option in --help */
// default=val			       /* default value, if option item is not specified by teh user. this should be of correct type */
//                                     /* this default to empty string/0/0.0/false, not possible for type=switch */
// upper=val			       /* upper bound (inclusive) of numeric value (only int/double), default +INF */
// lower=val			       /* lower bound (inclusive) of numeric value (only int/double), default -INF */
// alias=aliaslist		       /* comma separated list (in quotes!) of aliases for this option (use - for single char alias) */
// musthave			       /* this option item is required (so it's not really an option, but a named parameter) */
// shouldhave			       /* this option prints a warning, if it is not specified (it is recommended)  */
// string-mode-append		       /* for string type option items, multiple parameters are concatenated (see string-append-separator) */
// string-mode-once		       /* for string type option items, only one parameter is allowed (else an error message is printed) */
// string-append-separator=string      /* this string separates multiple parameters for string-mode-append */
// onlycl			       /* this option may only be specified on the command line */
// save				       /* this option item and its value is saved in the config file if save() is called */
// optional_param                      /* the parameter for this option is optional, if it is missing the default is used */
// hide,hidden                         /* do not print option in --help, --hhelp will print it but --hhelp itself is hidden */
// onlyapp			       /* for internal use only: application private variable (like application version/name) */
// configopt			       /* for internal use only: application private option item (like --create-rc) */
//
// example of a small option list:
#if 0
const char *options[] = {
   "#usage='Usage: %n [OPTIONS and FILES] -- [FILES]\n\n"   // no comma here!
   "this program does very strange things\n",
   "#trailer='\n%n version %v *** (C) 1997 by Johannes Overmann\ncomments, bugs and suggestions welcome: %e\n%gpl'",
   "#onlycl", // only command line options
   "#stopat--", // stop option scanning after a -- parameter
   // options
   "name=recursive        , type=switch, char=r, help=recurse directories, headline=file options:",
   "name=fileext          , type=string, char=e, help=process only files with extension in comma separated LIST, string-mode-append, string-append-separator=',', param=LIST",
   
   "name=ignore-case      , type=switch, char=i, help=ignore case, headline=matching options:",
   "name=lower-case       , type=switch,         help=lower replacements",
   "name=upper-case       , type=switch,         help=upper replacements",

   "name=dummy-mode       , type=switch, char=0, help='do *not* write/change anything, just test', headline=common options:",
   "name=quiet            , type=switch, char=q, help=quiet execution\\, do not say anything",
   "name=progress         , type=int,    char=P, param=NUM, default=0, lower=0, help='progress indicator, useful with large files, print one dot per event",
   // --help and --version will appear automatically here
   "EOL" // end of list
};
#endif
// 
// notes:
// ------
// to make it possible for the application to process control characters during runtime every 
// string is compiled from a c-string format (with \n, \xff and such things) at runtime.
// if you enter the option list as string constants, then the c compiler already compiles
// those strings. so you have to protect the backslash sequences by backslashes. example: (not worst case, but ugly)
// to get a single backslash in the output (i.e. in --help) the string must contain two backslashes (\\), but
// you must protect them in your string constant -- so the source looks like this: \\\\   <ugh> :)
// (of course, if you want to print this help with printf you have to write \\\\\\\\, tricky eh? :) 
// to make things more complicated, you have to protect commas in your text by quotes or backslashes which must
// also be protected from the compiler to ... oh no.
// just take a look at the example above (dummy-mode and quiet), not that worse, isnt it? :)



// history:
// ========
// 1997
// 01:45 Jun 11 stop on '--' feature added (929 lines)
// 02:00 Jul  3 string once/append feature added (968 lines)
// 12:00 Aug  7 application set/get support started (1103 lines)
// 13:00 Aug  7 application set/get support finished (untested) (1164 lines)
// 13:00 Aug  7 autosave support started
// 21:00 Aug  7 autosave canceled: save support instead
// 23:30 Aug  7 save support finished (1327 lines)
// ???          get default/ranges
// 1998
// 17:00 Jan 10 improved rc file parsing (string scan tools)
// 18:47 Aug 04 fixed %s bug (printf->puts) 
// 22:48 Aug 05 added %e for authors email
// 21:00 Oct 01 documentation (1595)
// 21:29 Nov 12 allow %n in help (application name) (1607)
// 20:51 Dec 01 ignore_negnum added (1617)
// 1999
// 13:58 Jan 01 remove-- added (1623)
// 15:01 Jan 01 type==STRING/OVERRIDE, rc-file/cmdline not set bug fixed (1625)
// 23:00 Feb 03 single char alias added
// 01:16 Feb 11 help fit terminal width added (1650)
// 15:08 Feb 15 tobject type info removed
// 12:29 Apr 02 optional_param added
// 22:55 Jun 04 string_append bug fixed
// 19:00 Sep 19 environment NULL pointer segfault fixed
// 20:58 Sep 20 hide option added
// 21:56 Sep 29 --hhelp added (oh how I like this ... :), hidden=hide (203/1677)
// 2000:
// 01:35 Jul 08 misc bugfixes
// 2001: 
// 22:58 Jun 11 userErrorExitStatus added (273/1946)
// 2007:
//       Jan 28 wasSetByUser() added
//       Jan 29 print warning for unknown rc-file parameters instead of error (1977 lines)


// global data:

/// global TAppConfig pointer to the only instance TAppConfig
TAppConfig *tApp = 0;
// global application name (pseudo invisible to the outside)
const char *terrorApplicationName;
// standard options
static const char *self_conflist[] = {
   // standard options
   "type=switch, name=help, char=h, onlycl, help='print this help message, then exit successfully'",
   "type=switch, name=hhelp, onlycl, hide, help='like help but also show hidden options'",
   "type=switch, name=version, onlycl, help='print version, then exit successfully'",
   
   // configuration options
   "type=switch, name=verbose-config, onlycl, configopt, help='print all options, values and where they were specified, then exit', headline='config file options:'",
   "type=string, name=rc-file, onlycl, configopt, param=FILE, help='use FILE as rcfile'",
   "type=string, name=create-rc, onlycl, configopt, param=FILE, help='create a FILE with default values for all options'",
   
   // pseudovariables only visible to the application, not to the user
   "type=string, name=application-version, onlyapp",
   "type=string, name=application-name, onlyapp",
   "EOL"
};
/// global exit status on userError()
static int userErrorExitStatus = 1;

// helpers

template<class T> 
inline bool tOutOfRange(const T& value, const T& lower, const T& upper) {
   if((value<lower) || (value>upper)) return true; else return false;}


// file tools

bool fisdir(const char *fname) {
   struct stat buf;   
   if(stat(fname, &buf)) return false;
   if(S_ISDIR(buf.st_mode)) return true;
   else return false;
}

bool fisregular(const char *fname) {
   struct stat buf;   
   if(stat(fname, &buf)) return false;
   if(S_ISREG(buf.st_mode)) return true;
   else return false;
}

bool fexists(const char *fname) {
   struct stat buf;   
   if(stat(fname, &buf)) return false;
   else return true;
}

bool fissymlink(const char *fname) {
#ifdef __STRICT_ANSI__
   fname = fname;
   return false;
#else
   struct stat buf;   
   if(lstat(fname, &buf)) return false;
   if(S_ISLNK(buf.st_mode)) return true;
   else return false;
#endif
}

off_t flen(const char *fname) {
   struct stat buf;   
   if(stat(fname, &buf)) throw TFileOperationErrnoException(fname, "stat");
   return buf.st_size;
}

off_t flen(int fdes) {
   struct stat buf;   
   if(fstat(fdes, &buf)) throw TFileOperationErrnoException("<anon>", "fstat");
   return buf.st_size;
}

off_t flen(FILE *file) {
#ifdef __STRICT_ANSI__
   long c = ftell(file);
   if(c == -1) throw TFileOperationErrnoException("<anon>", "ftell");
   if(fseek(file, 0, SEEK_END)) throw TFileOperationErrnoException("<anon>", "fseek");
   long r = ftell(file);
   if(r == -1) throw TFileOperationErrnoException("<anon>", "ftell");
   if(fseek(file, c, SEEK_SET)) throw TFileOperationErrnoException("<anon>", "fseek");
   return (off_t)r;
#else
   struct stat buf;   
   if(fstat(fileno(file), &buf)) throw TFileOperationErrnoException("<anon>", "fstat");
   return buf.st_size;
#endif
}

// errors

   
void userWarning(const char *message, ...) {
   va_list ap;
   
   va_start(ap, message);
   if(terrorApplicationName) fprintf(stderr, "\r%s: warning: ", terrorApplicationName);
   else fprintf(stderr, "\rwarning: ");
   vfprintf(stderr, message, ap);
   va_end(ap);
}

// returns old status
int setUserErrorExitStatus(int status) {
   int old = userErrorExitStatus;
   userErrorExitStatus = status;
   return old;
}

void userError(const char *message, ...) {
   va_list ap;
   
   va_start(ap, message);
   if(terrorApplicationName) fprintf(stderr, "\r%s: ", terrorApplicationName);
   else fprintf(stderr, "\rerror: ");
   vfprintf(stderr, message, ap);
   va_end(ap);
   exit(userErrorExitStatus);
}


#ifdef CHECKPOINTS
#ifdef KEYWORD_CHECKPOINTS
#undef for
#undef do
#undef else
#undef switch
#undef break
#undef return
#endif
#define MAX_CP 64
const char *checkpoint_file[MAX_CP];
const char *checkpoint_func[MAX_CP];
int checkpoint_line[MAX_CP];
static int num_cp = -1;

int addCheckpoint(const char *file, int line, const char *func) {
   fprintf(stderr, "%s:%d:%s%s checkpoint\n", file, line, func?func:"", func?":":"");
   if(num_cp < 0) {
      // initialize
      memset(checkpoint_file, 0, sizeof(checkpoint_file));
      memset(checkpoint_line, 0, sizeof(checkpoint_line));
      memset(checkpoint_func, 0, sizeof(checkpoint_func));
      num_cp = 0;
   } else {
      checkpoint_file[num_cp] = file;
      checkpoint_line[num_cp] = line;
      checkpoint_func[num_cp] = func;
      num_cp++;
      if(num_cp >= MAX_CP) num_cp = 0;
   }
   return 1;
}

static void printCheckpoints() {
   if(num_cp < 0) {
      fprintf(stderr, "\ncheckpoint list not yet initialized\n");
      return;
   }
   int n = num_cp - 1;
   for(int i = 0; i < MAX_CP; i++, n--) {
      if(n < 0) n += MAX_CP;
      if(checkpoint_file[n])
	fprintf(stderr, "%s:%d:%s%s checkpoint\n", checkpoint_file[n], checkpoint_line[n],
		checkpoint_func[n]?checkpoint_func[n]:"", checkpoint_func[n]?":":"");
   }
}
#endif

#ifdef __GNUC__
#define fatalError(format) fatalError_func(__FILE__,__LINE__,__PRETTY_FUNCTION__,format)
#define fatalError1(format,a) fatalError_func(__FILE__,__LINE__,__PRETTY_FUNCTION__,format, a)
#define fatalError2(format,a,b) fatalError_func(__FILE__,__LINE__,__PRETTY_FUNCTION__,format, a, b)
#define fatalError3(format,a,b,c) fatalError_func(__FILE__,__LINE__,__PRETTY_FUNCTION__,format, a, b, c)
#define fatalError4(format,a,b,c,d) fatalError_func(__FILE__,__LINE__,__PRETTY_FUNCTION__,format, a, b, c, d)
#else
#define fatalError(format) fatalError_func(__FILE__,__LINE__,0,format)
#define fatalError1(format,a) fatalError_func(__FILE__,__LINE__,0,format, a)
#define fatalError2(format,a,b) fatalError_func(__FILE__,__LINE__,0,format, a, b)
#define fatalError3(format,a,b,c) fatalError_func(__FILE__,__LINE__,0,format, a, b, c)
#define fatalError4(format,a,b,c,d) fatalError_func(__FILE__,__LINE__,0,format, a, b, c, d)
#endif
static void fatalError_func(const char *file, int line, const char *function, const char *message, ...)
#ifdef __GNUC__
  __attribute__ ((noreturn,format(printf,4,5)))
#endif
    ;
static void fatalError_func(const char *file, int line, const char *function, const char *message, ...)
{
#ifdef CHECKPOINTS
   printCheckpoints();
#endif
   if(function) fprintf(stderr, "\n%s:%d: fatal error in function '%s':\n", file, line, function);
   else         fprintf(stderr, "\n%s:%d: fatal error:\n", file, line);
   va_list ap;   
   va_start(ap, message);
   vfprintf(stderr, message, ap);
   va_end(ap);
   exit(1);
}







// TAppConfigItem implementation

TAppConfigItem::TAppConfigItem():
must_have(false),
should_have(false),
only_cl(false),
configopt(false),
only_app(false),
save(false),
optional_param(false),
hide(false),
type(TACO_TYPE_NONE),
set_in(NEVER),
string_mode(OVERRIDE),
string_sep(""),
double_value(0), double_upper(0), double_lower(0), double_default(0),
int_value(0), int_upper(0), int_lower(0), int_default(0),
bool_value(false), bool_default(false),
printed(false),
name(),
context(),
help(),
headline(),
char_name(),
par(),
alias(),
type_str(),
upper(), lower(), def(),
string_value(), string_default()
{}


TAppConfigItem::TAppConfigItem(const char *str, const char *line_context, 
			       bool privat):
must_have(false),
should_have(false),
only_cl(false),
configopt(false),
only_app(false),
save(false),
optional_param(false),
hide(false),
type(TACO_TYPE_NONE),
set_in(NEVER),
string_mode(OVERRIDE),
string_sep(""),
double_value(0), double_upper(0), double_lower(0), double_default(0),
int_value(0), int_upper(0), int_lower(0), int_default(0),
bool_value(false), bool_default(false),
printed(false),
name(),
context(line_context),
help(),
headline(),
char_name(),
par(),
alias(),
type_str(),
upper(), lower(), def(),
string_value(), string_default()
{
   tvector<tstring> comp = split(str, ",", true);
   for(size_t i = 0; i < comp.size(); i++) {
      setComp(split(comp[i], "=", true, true), privat);
   }
   validate(line_context);
}


inline static bool isalphaorul(char c) {
   if(isalpha(c) || c=='_') return true;
   else return false;
}


static tstring Range2Str(int lower, int upper) {
   if((lower != INT_MIN) || (upper != INT_MAX)) {
      tstring lstr;
      tstring ustr;
      if(lower != INT_MIN) lstr = tstring(lower);
      if(upper != INT_MAX) ustr = tstring(upper);
      return "[" + lstr + ".." + ustr + "]";
   } else {
      return "";
   }
}

static tstring Range2Str(double lower, double upper) {
   if((lower!=-DBL_MAX) || (upper!=DBL_MAX)) {
      tstring lstr;
      tstring ustr;
      if(lower != -DBL_MAX) lstr = tstring(lower);
      if(upper !=  DBL_MAX) ustr = tstring(upper);
      return "[" + lstr + ".." + ustr + "]";
   } else {
      return "";
   }
}

void TAppConfigItem::validate(const char *line_context) {
   // name
   if(name.len()<2) 
     fatalError2("%s: name too short! (was %d, must have a least two chars)\n", line_context, int(name.len()));
   if(!isalphaorul(name[0])) 
     fatalError2("%s: name should begin with alpha char or '_'! (was %c, must have a least two chars)\n", line_context, name[0]);
   if(char_name.len() > 1) 
     fatalError1("%s: only one character may be specified as char name!\n", line_context);

   // help, alias and flags
   if((help=="")&&(!only_app)&&(!hide))
     fatalError1("%s: you must give a help for each non nidden option!\n", line_context);
   for(size_t i = 0; i < alias.size(); i++) 
     if(alias[i].len() < 1)
       fatalError1("%s: alias must not be empty!\n", line_context);
   if(only_app&&only_cl) 
     fatalError1("%s: only one out of [onlyapp,onlycl] may be specified!\n", line_context);
   if(should_have&&must_have) 
     fatalError1("%s: only one out of [shouldhave,musthave] may be specified!\n", line_context);
   if((!def.empty()) && must_have && only_cl)
     fatalError1("%s: default value for required command line option makes no sense!\n", line_context);

   // type
   if(type_str=="bool") type=BOOL;
   else if(type_str=="int") type=INT;
   else if(type_str=="switch") type=SWITCH;
   else if(type_str=="double") type=DOUBLE;
   else if(type_str=="string") type=STRING;
   else fatalError2("%s: illegal/unknown type '%s'!\n", line_context, type_str.c_str());

   // string mode
   if((string_mode!=OVERRIDE) && (type!=STRING))
     fatalError1("%s: string-mode-... makes only sense with strings!\n", line_context);
   if((!string_sep.empty()) && (type!=STRING))
     fatalError1("%s: string-append-separator makes only sense with strings!\n", line_context);
   if((!string_sep.empty()) && (string_mode!=APPEND))
     fatalError1("%s: string-append-separator makes only sense with string-mode-append!\n", line_context);
        
   // each type
   switch(type) {
    case SWITCH:
      if(must_have||should_have) fatalError1("%s: switch can't be reqired/recommended!\n", line_context);
      if(!def.empty()) fatalError1("%s: switch can't have a defaut (is always false)!\n", line_context);
      if(!upper.empty()) fatalError1("%s: switch can't have an upper limit!\n", line_context);
      if(!lower.empty()) fatalError1("%s: switch can't have a lower limit!\n", line_context);
      bool_value = false;
      break;
    case BOOL:
      if(!def.empty()) {
	 if(!def.toBool(bool_default)) 
	   fatalError2("%s: illegal/unknown bool value for default '%s'! (can be [true|yes|on|t|1|false|no|off|f|0])\n", line_context, def.c_str());
      }
      else bool_default = false;
      bool_value = bool_default;
      if(!upper.empty()) fatalError1("%s: bool can't have an upper limit!\n", line_context);
      if(!lower.empty()) fatalError1("%s: bool can't have a lower limit!\n", line_context);
      break;      
    case INT:
      if(!def.empty()) {
	 if(!def.toInt(int_default, 0)) 
	   fatalError2("%s: illegal chars for default value in '%s'!\n", line_context, def.c_str());
      }
      else int_default = 0;
      int_value = int_default;
      if(!upper.empty()) {
	 if(!upper.toInt(int_upper, 0)) 
	   fatalError2("%s: illegal chars for upper bound in '%s'!\n", line_context, upper.c_str());
      }
      else int_upper = INT_MAX;
      if(!lower.empty()) {
	 if(!lower.toInt(int_lower, 0))
	   fatalError2("%s: illegal chars for lower bound in '%s'!\n", line_context, lower.c_str());
      }
      else int_lower = INT_MIN;
      if(tOutOfRange(int_default, int_lower, int_upper)) 
	fatalError3("%s: default value out of range! (%d not in %s)!\n", line_context, int_default, Range2Str(int_lower, int_upper).c_str());
      break;
    case DOUBLE:
      if(!def.empty()) {
	 if(!def.toDouble(double_default)) 
	   fatalError2("%s: illegal chars for default value in '%s'!\n", line_context, def.c_str());
      }
      else double_default = 0.0;
      double_value = double_default;
      if(!upper.empty()) {
	 if(!upper.toDouble(double_upper))
	   fatalError2("%s: illegal chars for upper bound in '%s'!\n", line_context, upper.c_str());
      }
      else double_upper = DBL_MAX;
      if(!lower.empty()) {
	 if(!lower.toDouble(double_lower)) 
	   fatalError2("%s: illegal chars for lower bound in '%s'!\n", line_context, lower.c_str());
      }
      else double_lower = -DBL_MAX;
      if(tOutOfRange(double_default, double_lower, double_upper)) 
	fatalError3("%s: default value out of range! (%g not in %s)!\n", line_context, double_default, Range2Str(double_lower, double_upper).c_str());
      break;
     case STRING: 
      // all strings are valid: most generic type!
      string_default = def;
      string_value = string_default;
      break;
    case TACO_TYPE_NONE:
      fatalError("internal error! (invalid type id)\n");
   }
   set_in = DEFAULT;
}

void TAppConfigItem::setComp(const tvector<tstring>& a, bool privat) {
   if(a.size() > 2)
     fatalError2("%s: syntax error near component '%s'! (too many '=')\n", 
		context.c_str(), join(a,"=").c_str());
   if(a.size() == 0) 
     fatalError2("%s: syntax error near component '%s'! (no component)\n",
		context.c_str(), join(a,"=").c_str());
   if((a.size()==1) && (a[0].consistsOfSpace())) return;
   tstring comp = a[0];
   tstring param;
   if(a.size()>1) {
      param=a[1];
      param.unquote();
   }
   if(comp=="name") { name = param;
   } else if(comp=="help") { help = param;
   } else if(comp=="headline") { headline = param;
   } else if(comp=="param") { par = param;
   } else if(comp=="char") { char_name = param; 
   } else if(comp=="alias") { alias = split(param, "; ,", true, true);
   } else if(comp=="type") { type_str = param;
   } else if(comp=="default") { def = param;
   } else if(comp=="upper") { upper = param; 
   } else if(comp=="lower") { lower = param;
   } else if(comp=="musthave") { must_have = true;
   } else if(comp=="shouldhave") { should_have = true;
   } else if(comp=="string-mode-append") { string_mode = APPEND;
   } else if(comp=="string-mode-once") { string_mode = ONCE;
   } else if(comp=="string-append-separator") { string_sep = param;
   } else if(privat && comp=="configopt") { configopt = true;
   } else if(comp=="save") { save = true;
   } else if(comp=="optional_param") { optional_param = true;
   } else if(comp=="hide") { hide = true;
   } else if(comp=="hidden") { hide = true;
   } else if(comp=="onlycl") { only_cl = true;
   } else if(privat && comp=="onlyapp") { only_app = true;
   } else fatalError2("%s: unknown component '%s'!\n", context.c_str(), comp.c_str());
}

tstring TAppConfigItem::getWasSetStr(TACO_SET_IN setin, const tstring& env, const tstring& rcfile) const {
   switch(setin) {
    case DEFAULT:
      return "by default";
    case COMMAND_LINE:
      return "on command line";
    case ENVIRONMENT:
      return "in environment variable '" + env + "'";
    case RC_FILE:
      return "in rc file '" + rcfile + "'";
    default:
      fatalError1("illegal value %d in setin! (0==NEVER)\n", setin);
#ifndef __GNUC__
      return "";
#endif
   }
}

tstring TAppConfigItem::getWasSetStr(const tstring& env, const tstring& rcfile) const {
   return getWasSetStr(set_in, env, rcfile);
}

void TAppConfigItem::printValue(const tstring& env, const tstring& rcfile) const {
   switch(type) {
    case SWITCH:
      if(bool_value) 
	printf("switch %s was set %s\n", name.c_str(), getWasSetStr(env, rcfile).c_str());
      else
	printf("switch %s was not set\n", name.c_str());	
      break;
      
    case BOOL:
      printf("bool   %s was set to %s %s\n", name.c_str(), bool_value?"true":"false", getWasSetStr(env, rcfile).c_str());
      break;
      
    case INT:
      printf("int    %s was set to %d %s\n", name.c_str(), int_value, getWasSetStr(env, rcfile).c_str());
      break;
      
    case DOUBLE:
      printf("double %s was set to %g %s\n", name.c_str(), double_value, getWasSetStr(env, rcfile).c_str());
      break;
      
    case STRING:
      {
	 tstring s(string_value);
	 s.expandUnprintable('"');
	 printf("string %s was set to \"%s\" %s\n", name.c_str(), s.c_str(), getWasSetStr(env, rcfile).c_str());
      }
      break;
      
    default:
      fatalError1("printValue(): Illegal value %d in type! (0==TACO_TYPE_NONE)\n", type);
   }
}

tstring TAppConfigItem::getParamStr() const {
   if(!par.empty()) return par;
   switch(type) {
    case BOOL:
      return "<bool>";
    case INT:
      return "<int>";
    case DOUBLE:
      return "<double>";
    case STRING:
      return "<string>";
    case SWITCH:
      return "";
    default:
      fatalError1("getParamStr(): Illegal value %d in type! (0==TACO_TYPE_NONE)\n", type);
#ifndef __GNUC__
      return "";
#endif
   }
}


tstring TAppConfigItem::getTypeStr() const {
   switch(type) {
    case BOOL:
      return "bool";
    case INT:
      return "int";
    case DOUBLE:
      return "double";
    case STRING:
      return "string";
    case SWITCH:
      return "switch";
    default:
      fatalError1("getTypeStr(): Illegal value %d in type! (0==TACO_TYPE_NONE)\n", type);
#ifndef __GNUC__
      return "";
#endif
   }
}

int TAppConfigItem::getOptLen() const {
   int l = getParamStr().len();
   if(l) l++;
   if(l) if(optional_param) l += 2;
   return name.len() + l;
}

tstring TAppConfigItem::getFlagsStr(const tstring& optprefix, 
				   bool globalonlycl) const 
{
   tstring h;   
   tstring range;
   switch(type) {
    case DOUBLE:
      range = Range2Str(double_lower, double_upper);
      break;
    case INT:
      range = Range2Str(int_lower, int_upper);
      break;
    default:
      break;
   }
   tstring defval;
   tstring string_mod;
   switch(type) {
    case DOUBLE:
      if(double_default!=0.0) defval = def;
      break;
    case INT:
      if(int_default!=0) defval = def;
      break;
    case STRING:
      {
	 tstring s(string_default);
	 s.expandUnprintable('"');
	 if(!s.empty()) defval = "\"" + s + "\"";
      }
      switch(string_mode) {
       case APPEND: string_mod = "multiple allowed"; break;
       case ONCE:   string_mod = "only once"; break;
       default: break;
      }
      break;
    case BOOL:
      if(!def.empty()) defval = def; // take string to allow: yes true on 1 ...
      break;
    default:
      break;
   }				    
   if((only_cl && (!globalonlycl)) || must_have || should_have ||
      (alias.size()>0) || (!range.empty()) || (!defval.empty()) || (!string_mod.empty()) || optional_param || hide) {
      h += "(";
      if(only_cl && (!globalonlycl)) h += "only command line";
      if(optional_param) {
	 if(h.lastChar()!='(') h+= ", ";
	 h += "parameter is optional";
      }	
      if(must_have) {
	 if(h.lastChar()!='(') h+= ", ";
	 h += "required";
      }	
      if(should_have) {
	 if(h.lastChar()!='(') h+= ", ";
	 h += "recommended";
      }	
      if(hide) {
	 if(h.lastChar()!='(') h+= ", ";
	 h += "hidden";
      }	
      if(alias.size()==1) {
	 if(h.lastChar()!='(') h+= ", ";
	 if((alias[0].len()==2) && (alias[0][0]=='-')) h += "alias=" + alias[0];
	 else h += "alias=" + optprefix + alias[0];
      }
      if(alias.size()>1) {
	 if(h.lastChar()!='(') h+= ", ";	 
	 h += "aliases={";
	 for(size_t i=0; i < alias.size(); i++) {
	    if((alias[i].len()==2) && (alias[i][0]=='-')) h += alias[i];
	    else h += optprefix + alias[i];
	    if(i < alias.size()-1) h+=", ";
	 }
	 h += "}";
      }
      if(!range.empty()) {
	 if(h.lastChar()!='(') h+= ", ";
	 h += "range=" + range;
      }
      if(!defval.empty()) {
	 if(h.lastChar()!='(') h+= ", ";
	 h += "default=" + defval;
      }
      if(!string_mod.empty()) {
	 if(h.lastChar()!='(') h+= ", ";
	 h += string_mod;
      }
      h += ")";
   }
   return h;
}
   
void TAppConfigItem::printItemToFile(FILE *f) const {
   if(!headline.empty()) {    // print headline
      fprintf(f, "\n# %.76s\n# %.76s\n\n", headline.c_str(), tstring('=', headline.len()).c_str());
   }
   tstring h(help + "\n");
   if(type == SWITCH) {
      h += "uncomment next line to activate switch";
   } else {
      h += "parameter is of type " + getTypeStr();
   }  
   h += " " + getFlagsStr("", false);
   while(!h.empty()) {
      fprintf(f, "# %s\n", h.getFitWords(80 - 2 - 1).c_str());
   }
   tstring defval;
   switch(type) {
    case DOUBLE:
      defval = tstring(double_default);
      break;
    case INT:
      defval = tstring(int_default);
      break;
    case STRING:
      {
	 tstring s(string_default);
	 s.expandUnprintable('"');
	 defval = "\"" + s + "\"";
      }
      break;
    case BOOL:
      if(!def.empty()) defval = def; // take string to allow yes true on 1 ...
      else defval = bool_default?"true":"false";
      break;
    default:
      break;
   }
   if(type == SWITCH) {
      fprintf(f, "#%s\n", name.c_str());
   } else {
      fprintf(f, "%s = %s\n", name.c_str(), defval.c_str());
   }
   fprintf(f, "\n");
}


void TAppConfigItem::printCurItemToFile(FILE *f, bool simple) const {
   if(!simple) {
      if(!headline.empty()) {    // print headline
	 fprintf(f, "\n# %.76s\n# %.76s\n\n", headline.c_str(), tstring('=', headline.len()).c_str());
      }
      tstring h(help + "\n");
      if(type==SWITCH) h += "parameter is a switch";
      else {
	 h += "parameter is of type " + getTypeStr();
	 h += " " + getFlagsStr("", false);
      }
      while(!h.empty()) {
	 fprintf(f, "# %s\n", h.getFitWords(80 - 2 - 1).c_str());
      }
   }
   tstring val;
   switch(type) {
    case DOUBLE:
      val = tstring(double_value);
      break;
    case INT:
      val = tstring(int_value);
      break;
    case STRING:
      {
	 tstring s(string_value);
	 s.expandUnprintable('"');
	 val = "\"" + s + "\"";
      }
      break;
    case BOOL:
      val = bool_value?"true":"false";
      break;
    default:
      break;
   }
   if(type == SWITCH) {
      if(bool_value)
	fprintf(f, "%s\n", name.c_str());
      else
	fprintf(f, "#%s\n", name.c_str());
   } else {
      fprintf(f, "%s = %s\n", name.c_str(), val.c_str());
   }     
   if(!simple) fprintf(f, "\n");
}


void TAppConfigItem::printHelp(int max_optlen, bool globalonlycl) const {
   int hcol = max_optlen + 5 + 2;
   char buf[256];
   int width = 80;
#ifndef NOWINSIZE
   struct winsize win;
   if(ioctl(1, TIOCGWINSZ, &win) == 0) {
      width = win.ws_col;
   }
#endif
   if(width < hcol + 8) width = hcol + 8;
   if(!headline.empty()) {
      printf("\n%s\n", headline.c_str());
   }
   sprintf(buf, "%c%c --%s%s%s%s", (!char_name.empty())?'-':' ', (!char_name.empty())?char_name[0]:' ', 
	   name.c_str(), optional_param?"[":"", type==SWITCH?"":("=" + getParamStr()).c_str(), optional_param?"]":"");
   tstring h(help);
   h += " " + getFlagsStr("--", globalonlycl);
   printf("%*s%s\n", -hcol, buf, h.getFitWords(width - hcol - 1).c_str());
   while(!h.empty()) {
      printf("%*s%s\n", -hcol, "", h.getFitWords(width - hcol - 1).c_str());
   } 
}


void TAppConfigItem::setValue(const tstring& parameter, TACO_SET_IN setin, 
			      bool verbose, const tstring& env, 
			      const tstring& rcfile, const tstring& line_context) {
   if(only_app)       
     userError("internal parameter name '%s' is private to the application may not be set by the user!\n", name.c_str());
   if(only_cl && (!setin==COMMAND_LINE))       
     userError("parameter name '%s' is only available on the command line!\n", name.c_str());

   // remember verbatim value for non string parameters in string_value
   if(type != STRING) string_value = parameter;
   
   // prepare option string
   tstring option;
   if(setin==COMMAND_LINE) {
      option = "option -" + char_name + ", --" + name;
   } else {
      option = "option '" + name + "'";
   }
  
   if(set_in!=DEFAULT) {
      if(!((type==STRING)&&(string_mode!=OVERRIDE))) {	    
	 if(verbose) 
	   printf("warning: %s (set %s) is overridden %s\n", 
		  option.c_str(), getWasSetStr(setin, env, rcfile).c_str(),
		  getWasSetStr(env, rcfile).c_str());
	 return;
      }
   }
  
   switch(type) {
    case SWITCH:
      if(!parameter.empty())
	userError("%s: %s is a switch and takes no parameter!\n", line_context.c_str(), option.c_str());
      bool_value = true;
      break;
      
    case BOOL:
      if(!parameter.toBool(bool_value)) 
	userError("%s: illegal/unknown bool value '%s' for %s!\n(can be [true|yes|on|t|1|false|no|off|f|0])\n", line_context.c_str(), parameter.c_str(), option.c_str());
      break;

    case INT:
      if(!parameter.toInt(int_value)) 
	userError("%s: illegal int value format for %s in '%s'!\n", line_context.c_str(), option.c_str(), parameter.c_str());
      if(tOutOfRange(int_value, int_lower, int_upper))
	userError("%s: value out of range for %s! (%d not in %s)!\n", line_context.c_str(), option.c_str(), int_value, Range2Str(int_lower, int_upper).c_str());
      break;
      
    case DOUBLE:
      if(!parameter.toDouble(double_value)) 
	userError("%s: illegal double value format for %s in '%s'!\n", line_context.c_str(), option.c_str(), parameter.c_str());
      if(tOutOfRange(double_value, double_lower, double_upper))
	userError("%s: value out of range for %s! (%g not in %s)!\n", line_context.c_str(), option.c_str(), double_value, Range2Str(double_lower, double_upper).c_str());
      break;
      
    case STRING:
      switch(string_mode) {
       case ONCE:
	 if(set_in != DEFAULT) 
	   userError("%s: %s may only be set once! (was already set to '%s' and was tried to set to '%s')\n", line_context.c_str(), option.c_str(), string_value.c_str(), parameter.c_str());
	 string_value = parameter;
	 break;
       case APPEND:
	 if((set_in != DEFAULT)||(!string_value.empty()))
	   string_value += string_sep;
	 string_value += parameter;
	 break;
       case OVERRIDE:
	 string_value = parameter; // overwrite old value, the default
	 break;
      }
      break;
      
    default:
      fatalError1("setValue(): Illegal value %d in type! (0==TACO_TYPE_NONE)\n", type);
   }
   set_in = setin;
}


void TAppConfigItem::setValueFromApp(bool parameter) {
   if(only_app)       
     fatalError1("internal parameter name '%s' is private to tapplication may not be set by the user!\n", name.c_str());
   if(only_cl)       
     fatalError1("parameter name '%s' is only available on the command line!\n", name.c_str());
   set_in = APPLICATION;
   if((type==SWITCH)||(type==BOOL)) {
      bool_value=parameter;
   } else {
      fatalError1("setValueFromApp(bool/switch): type mismatch: type was %s\n", getTypeStr().c_str());
   }
}


// returns true if value is valid, else false
bool TAppConfigItem::setValueFromApp(int i) {
   if(only_app)       
     fatalError1("internal parameter name '%s' is private to tapplication may not be set by the user!\n", name.c_str());
   if(only_cl)       
     fatalError1("parameter name '%s' is only available on the command line!\n", name.c_str());
   set_in = APPLICATION;
   if(type==INT) {
      if(i > int_upper) {
	 int_value = int_upper;
	 return false;
      }
      if(i < int_lower) {
	 int_value = int_lower;
	 return false;
      }
      int_value = i;
      return true;
   } else {
      fatalError1("setValueFromApp(int): type mismatch: type was %s\n", getTypeStr().c_str());
#ifndef __GNUC__
      return false;
#endif
   }
}


// returns true if value is valid, else false
bool TAppConfigItem::setValueFromApp(double d) {
   if(only_app)       
     fatalError1("internal parameter name '%s' is private to tapplication may not be set by the user!\n", name.c_str());
   if(only_cl)       
     fatalError1("parameter name '%s' is only available on the command line!\n", name.c_str());
   set_in = APPLICATION;
   if(type==DOUBLE) {
      if(d > double_upper) {
	 double_value = double_upper;
	 return false;
      }
      if(d < double_lower) {
	 double_value = double_lower;
	 return false;
      }
      double_value = d;
      return true;
   } else {
      fatalError1("setValueFromApp(double): type mismatch: type was %s\n", getTypeStr().c_str());
#ifndef __GNUC__
      return false;
#endif
   }
}


void TAppConfigItem::setValueFromApp(const tstring& str) {
   if(only_app)       
     fatalError1("internal parameter name '%s' is private to tapplication may not be set by the user!\n", name.c_str());
   if(only_cl)       
     fatalError1("parameter name '%s' is only available on the command line!\n", name.c_str());
   set_in = APPLICATION;
   if(type==STRING) {
      string_value = str;
   } else {
      fatalError1("setValueFromApp(string): type mismatch: type was %s\n", getTypeStr().c_str());
   }
}


// returns true if value is valid, else false
// sets value from string according to any type (switch == bool here)
bool TAppConfigItem::setValueFromAppFromStr(const tstring& parameter) {
   if(only_app)
     fatalError1("internal parameter name '%s' is private to tapplication may not be set by the user!\n", name.c_str());
   if(only_cl)       
     fatalError1("parameter name '%s' is only available on the command line!\n", name.c_str());
   set_in = APPLICATION;
   switch(type) {
    case SWITCH:
    case BOOL:
      if(!parameter.toBool(bool_value)) 
	fatalError2("SetValueFromAppFromStr: illegal/unknown bool value '%s' for %s!\n(can be [true|yes|on|t|1|false|no|off|f|0])\n", parameter.c_str(), name.c_str());
      return true;

    case INT:
      if(!parameter.toInt(int_value)) 
	fatalError2("SetValueFromAppFromStr: illegal int value format for %s in '%s'!\n", name.c_str(), parameter.c_str());
      return setValueFromApp(int_value);
      
    case DOUBLE:
      if(!parameter.toDouble(double_value)) 
	fatalError2("SetValueFromAppFromStr: illegal double value format for %s in '%s'!\n", name.c_str(), parameter.c_str());
      return setValueFromApp(double_value);
      
    case STRING:
      string_value = parameter;
      return true;
      
    default:
      fatalError1("SetValueFromStrFromApp(): Illegal value %d in type! (0==TACO_TYPE_NONE)\n", type);
#ifndef __GNUC__
      return false;
#endif
   }
}



// TAppConfig implementation

TAppConfig::TAppConfig(const char *conflist[], const char *listname,
		       int argc, char *argv[],
		       const char *envstrname,
		       const char *rcname,
		       const tstring& version): 
_params(),
name(),
opt(),
alias(),
envname(),
rc_name(),
rc_str(rcname?rcname:""),
verbose_conf(false),
onlycl(false),
stopatdd(false),
removedd(false),
ignore_negnum(false),
usage(),
trailer(),
commonhead()
{
   if(tApp)
     fatalError("only exactly one instance of TAppConfig is allowd!\n");
   for(int i = 0; i < 256; i++) char2index[i] = -1;
   stopatdd = onlycl = verbose_conf = false;
   addConfigItems(conflist, listname, false);
   addConfigItems(self_conflist, "self_conflist", true);
   doCommandLine(argc, argv, version);
   for(size_t i=0; i<opt.size(); i++) 
     opt[i].help.searchReplace("%n", getString("application-name"));     
   if(getBool("help")) printHelp(false);
   if(getBool("hhelp")) printHelp(true);
   if(getBool("version")) printf("%s version %s\n", getString("application-name").c_str(),
				 getString("application-version").c_str());
   tstring creatercfile;
   if(!onlycl) {
      creatercfile = getString("create-rc");
      verbose_conf = getBool("verbose-config");
      if(!creatercfile.empty()) createRCFile(creatercfile, rcname);
      if(envstrname) doEnvironVar(envstrname);
      if(rcname) doRCFile(rcname, getString("rc-file"));
      if(verbose_conf) printValues();   
   }
   if(verbose_conf || getBool("help") || getBool("hhelp") || getBool("version") || (!creatercfile.empty()))
     exit(0);
   for(size_t i = 0; i < opt.size(); i++) {
      if(opt[i].must_have && (opt[i].set_in==TAppConfigItem::DEFAULT))
	userError("error: option '%s' must be specified somewhere!\n", 
		  opt[i].name.c_str());
      if(opt[i].should_have && (opt[i].set_in==TAppConfigItem::DEFAULT))
	userWarning("warning: option '%s' should be specified somewhere\n",
	       opt[i].name.c_str());
   }
   // set global data
   tApp = this;
   terrorApplicationName = new char[getString("application-name").len()+1];
   strcpy((char *)terrorApplicationName, getString("application-name").c_str());
}


TAppConfig::~TAppConfig() {
   tApp = 0; // avoid dangling pointer to global application configuration;
   delete[] terrorApplicationName; // delete application name in terror
}


// save some items
// return rcname in rc_name_out
bool TAppConfig::save(tstring *rc_name_out) {
   if(rc_name_out)
	*rc_name_out = "$(HOME)/." + rc_str;
   char *home = getenv("HOME");
   if(home==0) {
      fprintf(stderr, "can't save config: no HOME variable defined!\n");
      return false;
   }
   tstring realname(tstring(home) + "/." + rc_str);
   if(rc_name_out)
	*rc_name_out = realname;
   tstring tmpname(realname + ".tmp");
   FILE *tmpf = fopen(tmpname.c_str(), "w");
   if(tmpf==0) {
      fprintf(stderr, "can't save config: can't open '%s' for writing!\n", tmpname.c_str());
      return false;
   }

   // init
   for(size_t i = 0; i < opt.size(); i++) opt[i].printed = false;
   int pri = 0;
   
   // update old config
   FILE *oldf = fopen(realname.c_str(), "r");
   if(oldf) {
      if(verbose_conf) 
	printf("updating items in '%s'\n", realname.c_str());
      char buf[1024];
      while(fgets(buf, sizeof(buf), oldf)) {
	 size_t j;
	 tstring line(buf);
	 if(strchr(";\\!", buf[0]) || line.consistsOfSpace()) { // copy comment
	    fputs(buf, tmpf);
	 } else if(buf[0]=='#') { // check for commented out switches
	    tstring sw(buf+1);
	    sw.cropSpace();
	    if(alias.contains(sw)||name.contains(sw)) {
	       line = sw;
	       goto upd;
	    } else {
	       fputs(buf, tmpf);
	    }
	 } else { // change parameter
	    j = line.firstOccurence('=');
	    if(j == tstring::npos) {
	       line.cropSpace();
	       for(j = 0; j < line.len() && (!isspace(line[j])); j++) /*empty body*/;
	    }
	    line.truncate(j);
	    upd:
	    line.cropSpace();
	    if(alias.contains(line)) line = opt[alias[line]].name;
	    if(name.contains(line)) { // valid name found
	       TAppConfigItem& a(opt[name[line]]);
	       if((!a.only_app)&&(!a.only_cl)) {
		  if(a.save) {
		     a.printCurItemToFile(tmpf, true);
		     a.printed = true;
		  } else {
		     fputs(buf, tmpf);
		  }
	       }
	    } else {
	       fprintf(stderr, "discarding invalid line from file '%s' :%s", realname.c_str(), buf);
	    }
	 }	 
	 pri++;
      }
      fclose(oldf);
   }
   
   // write any remaining items?
   int rem=0;
   for(size_t i = 0; i < opt.size(); i++) { 
      const TAppConfigItem& a(opt[i]);
      if(a.only_app) continue;
      if(a.only_cl) continue;
      if(a.printed) continue;
      if(!a.save) continue;
      rem++;
   }
   if(rem) {
      if(verbose_conf) 
	printf("writing %d remaining items to '%s'\n", rem, realname.c_str());
      if(pri) {
	 fprintf(tmpf, "\n\n\n# the following items have been appended by '%s' V%s\n", getString("application-name").c_str(), getString("application-version").c_str());
      } else {
	 fprintf(tmpf, "# this file was created by '%s' V%s\n", getString("application-name").c_str(), getString("application-version").c_str());
	 fprintf(tmpf, "# feel free to edit this file\n");
      }
      fprintf(tmpf, "\n\n");
      for(size_t i = 0; i < opt.size(); i++) { 
	 const TAppConfigItem& a(opt[i]);
	 if(a.only_app) continue;
	 if(a.only_cl) continue;
	 if(a.printed) continue;
	 if(!a.save) continue;
	 a.printCurItemToFile(tmpf, false);
      }
   }
   
   // finalize
   fclose(tmpf);
   if(::rename(tmpname.c_str(), realname.c_str())) {
      fprintf(stderr, "can't save config: can't move '%s' to '%s'!\n", tmpname.c_str(), realname.c_str());
      return false;
   }
   return true;
}


void TAppConfig::createRCFile(const tstring& fname, const tstring& rcname) const {
   FILE *f;
   
   f = fopen(fname.c_str(), "w");
   if(f==NULL) 
     userError("can't open '%s' for writing!\n", fname.c_str());
   fprintf(f, "# this is a machine generated rcfile for %s version %s\n",
	   getString("application-name").c_str(), getString("application-version").c_str());
   fprintf(f, "# delete these lines and edit this file as soon as possible since all options\n");
   fprintf(f, "# are set to default values and all switches are disabled (commented out)\n\n\n\n");
   for(size_t i = 0; i < opt.size(); i++) { 
      const TAppConfigItem& a(opt[i]);
      if(a.only_app) continue;
      if(a.only_cl) continue;
      a.printItemToFile(f);
   }
   fclose(f);
   tstring globalname = "/etc/" + rcname;
   tstring localname;
   char *home = getenv("HOME");
   if(home) {
      localname = tstring(home) + "/." + rcname;
   } else {
      localname = "($HOME)/." + rcname;
   }
   printf("you should now customize the rcfile '%s' and\nsave it as '%s' (or '%s' as root)\n",
	  fname.c_str(), localname.c_str(), globalname.c_str());
}


tstring TAppConfig::getName(const tstring& str, const tstring& context1, 
			   const tstring& optprefix, bool errorIfNotFound) const {
   tstring context(context1);
   if(!context.empty()) context += ": ";
   if(str.len()==0) 
     fatalError1("%sempty option name!\n", context.c_str());
   if(alias.contains(str)) return opt[alias[str]].name; // alias match
   if((str.len()==1)&&(optprefix!="--")) { // char name
      if(char2index[(unsigned char)str[0]] < 0)
      {
       if (errorIfNotFound)
       {
	   userError("%sunknown char option name '%s'! (try --help)\n", context.c_str(), str.c_str());
       }
       else
       {
	   userWarning("%sunknown char option name '%s'!\n", context.c_str(), str.c_str());
	   return "";
       }
      }
      return opt[char2index[(unsigned char)str[0]]].name;
   }     
   if(name.contains(str)) return str; // exact match
   tvector<tstring> found; 
   for(size_t i = 0; i < opt.size(); i++) { // search for prefixes
      if(opt[i].name.hasPrefix(str) && (!opt[i].only_app)) 
	found += opt[i].name;
   }
   if(found.size()==0)
   {
       if (errorIfNotFound)
       {
	   userError("%sunknown option '%s'! (try --help)\n", context.c_str(), (optprefix+str).c_str());
       }
       else
       {
	   userWarning("%sunknown option '%s'!\n", context.c_str(), (optprefix+str).c_str());
	   return "";
       }
   }
   if(found.size() != 1) {
      tstring option = optprefix + str;
      tstring list = optprefix + join(found, (", "+optprefix));
      userError("%sambiguous option '%s' matches '%s'!\n", context.c_str(), option.c_str(), list.c_str());
   }			      
   return found[0]; // uniq abbrevation
}


bool TAppConfig::getBool(const tstring &n) const {
   if(name.contains(n)) {
      const TAppConfigItem& a(opt[name[n]]);
      if((a.type==TAppConfigItem::BOOL) || (a.type==TAppConfigItem::SWITCH)) return a.bool_value;
      else fatalError2("type mismatch in call for %s '%s' as bool!\n", a.getTypeStr().c_str(), n.c_str());
   } else fatalError1("unknown parameter name '%s'\n", n.c_str());
#ifndef __GNUC__
   return false;
#endif
}


void TAppConfig::setValue(const tstring &n, bool b) {
   if(name.contains(n)) {
      opt[name[n]].setValueFromApp(b);
   } else fatalError1("unknown parameter name '%s'\n", n.c_str());
}


void TAppConfig::setValue(const tstring &n, const tstring& str) {
   if(name.contains(n)) {
      opt[name[n]].setValueFromApp(str);
   } else fatalError1("unknown parameter name '%s'\n", n.c_str());
}


bool TAppConfig::setValue(const tstring &n, int i) {
   if(name.contains(n)) {
      return opt[name[n]].setValueFromApp(i);
   } else fatalError1("unknown parameter name '%s'\n", n.c_str());
#ifndef __GNUC__
   return false;
#endif
}


bool TAppConfig::setValue(const tstring &n, double d) {
   if(name.contains(n)) {
      return opt[name[n]].setValueFromApp(d);
   } else fatalError1("unknown parameter name '%s'\n", n.c_str());
#ifndef __GNUC__
   return false;
#endif
}


bool TAppConfig::setValueFromStr(const tstring &n, const tstring& str) {
   if(name.contains(n)) {
      return opt[name[n]].setValueFromAppFromStr(str);
   } else fatalError1("unknown parameter name '%s'\n", n.c_str());
#ifndef __GNUC__
   return false;
#endif
}


const tstring& TAppConfig::getString(const tstring &n) const {
   if(name.contains(n)) {
      const TAppConfigItem& a(opt[name[n]]);
      return a.string_value;
   } else fatalError1("unknown parameter name '%s'\n", n.c_str());
#ifndef __GNUC__
   static tstring s;
   return s;
#endif
}


int TAppConfig::getInt(const tstring &n) const {
   if(name.contains(n)) {
      const TAppConfigItem& a(opt[name[n]]);
      if(a.type==TAppConfigItem::INT) return a.int_value;
      else fatalError2("type mismatch in call for %s '%s' as int!\n", a.getTypeStr().c_str(), n.c_str());
   } else fatalError1("unknown parameter name '%s'\n", n.c_str());
#ifndef __GNUC__
   return 0;
#endif
}


int TAppConfig::intUpper(const tstring &n) const {
   if(name.contains(n)) {
      const TAppConfigItem& a(opt[name[n]]);
      if(a.type==TAppConfigItem::INT) return a.int_upper;
      else fatalError2("type mismatch in call for %s '%s' as int!\n", a.getTypeStr().c_str(), n.c_str());
   } else fatalError1("unknown parameter name '%s'\n", n.c_str());
#ifndef __GNUC__
   return 0;
#endif
}


int TAppConfig::intLower(const tstring &n) const {
   if(name.contains(n)) {
      const TAppConfigItem& a(opt[name[n]]);
      if(a.type==TAppConfigItem::INT) return a.int_lower;
      else fatalError2("type mismatch in call for %s '%s' as int!\n", a.getTypeStr().c_str(), n.c_str());
   } else fatalError1("unknown parameter name '%s'\n", n.c_str());
#ifndef __GNUC__
   return 0;
#endif
}


int TAppConfig::intDefault(const tstring &n) const {
   if(name.contains(n)) {
      const TAppConfigItem& a(opt[name[n]]);
      if(a.type==TAppConfigItem::INT) return a.int_default;
      else fatalError2("type mismatch in call for %s '%s' as int!\n", a.getTypeStr().c_str(), n.c_str());
   } else fatalError1("unknown parameter name '%s'\n", n.c_str());
#ifndef __GNUC__
   return 0;
#endif
}


double TAppConfig::getDouble(const tstring &n) const {
   if(name.contains(n)) {
      const TAppConfigItem& a(opt[name[n]]);
      if(a.type==TAppConfigItem::DOUBLE) return a.double_value;
      else fatalError2("type mismatch in call for %s '%s' as double!\n", a.getTypeStr().c_str(), n.c_str());
   } else fatalError1("unknown parameter name '%s'\n", n.c_str());
#ifndef __GNUC__
   return 0;
#endif
}


double TAppConfig::doubleUpper(const tstring &n) const {
   if(name.contains(n)) {
      const TAppConfigItem& a(opt[name[n]]);
      if(a.type==TAppConfigItem::DOUBLE) return a.double_upper;
      else fatalError2("type mismatch in call for %s '%s' as double!\n", a.getTypeStr().c_str(), n.c_str());
   } else fatalError1("unknown parameter name '%s'\n", n.c_str());
#ifndef __GNUC__
   return 0;
#endif
}


double TAppConfig::doubleLower(const tstring &n) const {
   if(name.contains(n)) {
      const TAppConfigItem& a(opt[name[n]]);
      if(a.type==TAppConfigItem::DOUBLE) return a.double_lower;
      else fatalError2("type mismatch in call for %s '%s' as double!\n", a.getTypeStr().c_str(), n.c_str());
   } else fatalError1("unknown parameter name '%s'\n", n.c_str());
#ifndef __GNUC__
   return 0;
#endif
}


double TAppConfig::doubleDefault(const tstring &n) const {
   if(name.contains(n)) {
      const TAppConfigItem& a(opt[name[n]]);
      if(a.type==TAppConfigItem::DOUBLE) return a.double_default;
      else fatalError2("type mismatch in call for %s '%s' as double!\n", a.getTypeStr().c_str(), n.c_str());
   } else fatalError1("unknown parameter name '%s'\n", n.c_str());
#ifndef __GNUC__
   return 0;
#endif
}


tstring TAppConfig::stringDefault(const tstring &n) const {
  if(name.contains(n)) {
     const TAppConfigItem& a(opt[name[n]]);
     if(a.type==TAppConfigItem::STRING) return a.string_default;
     else fatalError2("type mismatch in call for %s '%s' as string!\n", a.getTypeStr().c_str(), n.c_str());
  } else fatalError1("unknown parameter name '%s'\n", n.c_str());
#ifndef __GNUC__
   static tstring s;
   return s;
#endif
}


bool TAppConfig::boolDefault(const tstring &n) const {
  if(name.contains(n)) {
     const TAppConfigItem& a(opt[name[n]]);
     if(a.type==TAppConfigItem::BOOL) return a.bool_default;
     else fatalError2("type mismatch in call for %s '%s' as bool!\n", a.getTypeStr().c_str(), n.c_str());
  } else fatalError1("unknown parameter name '%s'\n", n.c_str());
#ifndef __GNUC__
   return false;
#endif
}


bool TAppConfig::getSwitch(const tstring &n) const {
   return getBool(n);
}


bool TAppConfig::operator() (const tstring &n) const {
   return getBool(n);
}


TAppConfigItem::TACO_SET_IN TAppConfig::wasSetIn(const tstring &n) const {
   if(name.contains(n)) {
      const TAppConfigItem& a(opt[name[n]]);
      return a.set_in;
   } else fatalError1("unknown parameter name '%s'\n", n.c_str());
#ifndef __GNUC__
   // will never come here, avoid warning
   return TAppConfigItem::NEVER;
#endif
}


bool TAppConfig::wasSetByUser(const tstring &n) const 
{
    TAppConfigItem::TACO_SET_IN set = wasSetIn(n);
    return (set == TAppConfigItem::COMMAND_LINE) || (set == TAppConfigItem::RC_FILE);
}


void TAppConfig::setFromStr(const tstring& str, const tstring& context, 
			    TAppConfigItem::TACO_SET_IN setin) {
   // prepare name n and parameter par
   tstring n;
   tstring par;
   size_t scanner=0;
   str.skipSpace(scanner);
   n = str.scanUpTo(scanner, " \t\n=");
   str.skipSpace(scanner);
   str.perhapsSkipOneChar(scanner, '=');
   str.skipSpace(scanner);
   par = str.scanRest(scanner);
   par.cropSpace();
   
   // set value
   if(!n.empty()) {
       // get name, ignore unknown parameters in config files with a warning
       n = getName(n, context, "", setin != TAppConfigItem::RC_FILE); 
       if (n.empty())
	   return; // ignore unknown parameters (warning already printed)
      TAppConfigItem& a(opt[name[n]]);
      if(a.type == TAppConfigItem::SWITCH) { // need no param
	 if(str.contains('=')) par = "t"; // force error
      } else { // get param
	 if(par.consistsOfSpace()) 
	   userError("%s: missing parameter for option '%s'!\n",
		     context.c_str(), n.c_str());
      }
      if(a.type == TAppConfigItem::STRING) {
	 par.unquote();
	 par.compileCString();
      }
      a.setValue(par, setin, verbose_conf, envname, rc_name, context);
   } else {
      userError("%s: syntax error, missing option name in '%s'!\n", 
		context.c_str(), str.c_str());
   }
}

void TAppConfig::doEnvironVar(const char *env) {
   envname = env;
   char *p = getenv(env);
   if(p) {
      if(tstring(p).consistsOfSpace()) {
	 if(verbose_conf)
	   printf("environment variable '%s' is empty\n", env);
	 return;
      }
      if(verbose_conf)
	printf("parsing environment variable '%s'\n", env);
      tvector<tstring> a = split(p, ",", true, true);
      for(size_t i=0; i<a.size(); i++) {
	 setFromStr(a[i], "environment '" + tstring(env) + "'", TAppConfigItem::ENVIRONMENT);
      }
   } else {
      if(verbose_conf)
	printf("environment variable '%s' not set\n", env);
   }
}

void TAppConfig::doRCFile(const tstring& rcfile, const tstring& clrcfile) {
   tvector<tstring> a;
   
   // which file ?
   if(!clrcfile.empty()) {
      if(fexists(clrcfile.c_str())) {
	 if(verbose_conf)
	   printf("parsing rc-file '%s'\n", clrcfile.c_str());
	 rc_name = clrcfile;
      } else {
	 userError("can't load rc-file '%s'!\n", clrcfile.c_str());
      }
   } else {
      tstring globalname = "/etc/" + rcfile;
      tstring localname;
      char *home = getenv("HOME");
      if(home) {
	 localname = tstring(home) + "/." + rcfile;
      }
      if(fexists(localname.c_str())) {
	 if(verbose_conf)
	   printf("parsing local rc-file '%s'\n", localname.c_str());
	 rc_name = localname;
      } else if(fexists(globalname.c_str())) {
	 if(verbose_conf)
	   printf("parsing global rc-file '%s'\n", globalname.c_str());
	 rc_name = globalname;
      } else {
	 if(verbose_conf)
	   printf("neither '%s' nor '%s' exist: no rc-file found\n",
		  localname.c_str(), globalname.c_str());
	 return;
      }
   }
   
   // load file
   a = loadTextFile(rc_name.c_str());
   
   // parse rc-file
   for(size_t i = 0; i < a.size(); i++) {
      if(!strchr("#;!/", a[i][0])) { // ignore comment
	 if(!a[i].consistsOfSpace()) 
	     setFromStr(a[i], "rc-file '" + rc_name + "' (line " + tstring(int(i + 1)) + ")", TAppConfigItem::RC_FILE);
      }
   }
}

void TAppConfig::doCommandLine(int ac, char *av[], const tstring& version) {
   // one of these chars follows a signgle dash '-' to make it a numeric arg
   // in the sense of 'ignore_negnum'
   const char *validnegnumchars="0123456789.";
   
   // set version and name
   opt[name["application-name"]].string_value = av[0];
   opt[name["application-name"]].string_value.extractFilename();
   opt[name["application-version"]].string_value = version;
   
   // parse command line
   bool nomoreoptions = false; // -- reached = false 
   for(int i=1; i<ac; i++) {
      if((av[i][0]=='-') && 
	 (!nomoreoptions) && 
	 ((!ignore_negnum)||(strchr(validnegnumchars, av[i][1])==0))) {
	 switch(av[i][1]) {
	  case 0: // simple parameter '-'
	    _params += av[i];
	    break;
	    
	  case '-': // long name parameter
	    if(av[i][2]==0) { // simple parameter '--'
	       if(stopatdd) {
		  nomoreoptions = true; // suppress option scanning
		  if(!removedd) _params += av[i];
	       }
	    } else {
	       char *p = strchr(av[i], '=');	       
	       tstring n(&av[i][2]);
	       if(p) n.truncate(p - &av[i][2]);
	       if(!n.empty()) {
		  n = getName(n, onlycl?"":"command line", "--");
		  TAppConfigItem& a(opt[name[n]]);
		  tstring par;
		  if(a.type==TAppConfigItem::SWITCH) { // need no param
		     if(p) par = "t"; // force error
		  } else { // get param
		     if(p) par = &av[i][p-av[i]+1];
		     else {
			if(a.optional_param) {
			   par = a.def;
			} else {
			   i++;
			   if(i<ac) par = av[i];
			   else userError("missing parameter for long option '--%s'! (try --help)\n", n.c_str());
			}
		     }
		  }		     
		  par.compileCString();
		  a.setValue(par, TAppConfigItem::COMMAND_LINE);
	       } else {
		  userError("syntax error: missing name in long option '%s'!\n", av[i]);
	       }
	    }
	    break;
	    
	  default:  // short name parameter
	    for(const char *p = &av[i][1]; *p; p++) {
	       if(char2index[(unsigned char)*p] >= 0) { // valid short option
		  TAppConfigItem& a(opt[char2index[(unsigned char)*p]]);
		  tstring par;
		  if(a.type != TAppConfigItem::SWITCH) { // get param
		     if(p[1]) {
			par = &p[1];
		     } else {
			i++;
			if(i<ac) par = av[i];
			else userError("%smissing parameter for option '-%s'! (try --help)\n",
				       onlycl?"":"command line: ", p);
		     }
		  } 
		  a.setValue(par, TAppConfigItem::COMMAND_LINE);
		  if(a.type != TAppConfigItem::SWITCH) break;
	       } else {
		  userError("%sunknown option '-%c'! (try --help)\n", 
			    onlycl?"":"command line: ", *p);
	       }
	    }
	    break;
	 }
      } else {
	 _params += av[i]; // add simple parameter
      }
   }
}

int TAppConfig::getMaxOptLen(bool show_hidden) const {
   int max = 0;
   for(size_t i=0; i<opt.size(); i++) {
     if((!opt[i].only_app) && (!(opt[i].configopt && onlycl)) && (show_hidden || (!opt[i].hide))) {
	int len = opt[i].getOptLen();
	if(len>max) max = len;
     }
   }
   return max;
}


void TAppConfig::printHelp(bool show_hidden) const {
   tstring ausage(usage);
   tstring atrailer(trailer);
   ausage.searchReplace("%n", getString("application-name"));
   ausage.searchReplace("%v", getString("application-version"));
   ausage.searchReplace("%e", TAPPCONFIG_EMAIL);
   ausage.searchReplace("%gpl", TAPPCONFIG_GPL);
   atrailer.searchReplace("%n", getString("application-name"));
   atrailer.searchReplace("%v", getString("application-version"));
   atrailer.searchReplace("%e", TAPPCONFIG_EMAIL);
   atrailer.searchReplace("%gpl", TAPPCONFIG_GPL);   
   puts(ausage.c_str());
   int max = getMaxOptLen(show_hidden);
   for(size_t i=0; i<opt.size(); i++)
     if((!opt[i].only_app) && (!(opt[i].configopt && onlycl)) && (show_hidden || (!opt[i].hide))) {
	opt[i].printHelp(max, onlycl);
     }
   puts(atrailer.c_str());
}


void TAppConfig::printValues() const {
   for(size_t i=0; i<opt.size(); i++) 
     if((!opt[i].only_app) && (!opt[i].only_cl))
       opt[i].printValue(envname, rc_name);
}


void TAppConfig::setComp(const tvector<tstring>& a, const tstring& context) {
   if((a.size()==1) && (a[0].consistsOfSpace())) return;
   tstring comp = a[0];
   tstring parameter;
   if(a.size()>1) {
      parameter=a[1];
      parameter.unquote();
   }
   if(a.size()>2) {
      tstring s = join(a, "=");
      s.expandUnprintable('\'');
      fatalError2("%s: syntax error near component '%s'! (too many '=')\n", context.c_str(), s.c_str());
   }
   if(comp=="usage") { usage = parameter;
   } else if(comp=="commonheadline") { commonhead = parameter;
   } else if(comp=="trailer") { trailer = parameter;
   } else if(comp=="onlycl") { onlycl = true;
   } else if(comp=="stopat--") { stopatdd = true;
   } else if(comp=="remove--") { removedd = true;
   } else if(comp=="ignore_negnum") { ignore_negnum = true;
   } else fatalError2("%s: unknown component '%s'!\n", context.c_str(), comp.c_str());
}


void TAppConfig::doMetaChar(const tstring& str, const tstring& context) {
   tvector<tstring> a = split(str, ",", true);
   for(size_t i=0; i < a.size(); i++) {
      setComp(split(a[i], "=", true, true), context);
   }   
}

void TAppConfig::addConfigItems(const char **list, const char *listname,
				bool privat) {
   for(int line=1; ; line++, list++) {
      // context string for errors
      tstring context("conflist " + tstring(listname) + " (line " + tstring(line) + ")");

      // test for end of list (EOL)
      if(*list == 0) 
	fatalError1("%s: list not terminated! (0 pointer, should be \"EOL\")\n", context.c_str());
      if(strcmp(*list, "EOL")==0) break;
      if(strcmp(*list, "#EOL")==0) break;
      if(strcmp(*list, "")==0) break;
      if(strlen(*list)<7) 
	fatalError2("%s: line corrupt (too short) perhaps listitem or list not terminated!\n (was '%s') check listitem (must have at least type and name)\nor terminate list with \"EOL\"\n",
		   context.c_str(), *list);
      
      // meta char?
      if(*list[0] == '#') {
	 doMetaChar((*list)+1, context);
	 continue;
      }
      
      // get config item
      TAppConfigItem a(*list, context.c_str(), privat);
      
      // skip useless options
      if(onlycl && a.configopt) continue;

      // register name
      if(name.contains(a.name)) 
	fatalError3("%s: duplicate name '%s' (previoulsy defined in %s)!\n",
		   context.c_str(), a.name.c_str(), opt[name[a.name]].context.c_str());
      name[a.name] = opt.size();

      // register char name
      if(!a.char_name.empty()) {
	 if(char2index[(unsigned char)a.char_name[0]]>=0)
	   fatalError3("%s: duplicate char name '%c' (previoulsy defined in %s)!\n",
		      context.c_str(), a.char_name[0], opt[char2index[(unsigned char)a.char_name[0]]].context.c_str());
	 char2index[(unsigned char)a.char_name[0]] = opt.size();
      }

      // register aliases 
      for(size_t i=0; i < a.alias.size(); i++) {
	 if((a.alias[i].len() == 2) && (a.alias[i][0] == '-')) { // char alias
	    if(char2index[(unsigned char)a.alias[i][1]]>=0)
	      fatalError3("%s: duplicate char name for char alias '%c' (previoulsy defined in %s)!\n",
			 context.c_str(), a.alias[i][1], opt[char2index[(unsigned char)a.alias[i][1]]].context.c_str());
	    char2index[(unsigned char)a.alias[i][1]] = opt.size();
	 } else { // name alias
	    if(alias.contains(a.alias[i]))
	      fatalError2("%s: duplicate alias '%s'!\n", context.c_str(), a.alias[i].c_str());
	    alias[a.alias[i]] = opt.size();
	 }
      }

      // set headline for help option
      if(a.name == "help") 
	a.headline = commonhead;
      
      // add config item
      opt += a;
   }
}


