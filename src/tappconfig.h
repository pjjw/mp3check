/*GPL*START*
 *
 * tappconfig.h - console application framework header file
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

#ifndef _ngw_tappconfig_h_
#define _ngw_tappconfig_h_

#include "tmap.h"
#include "tvector.h"
#include "tstring.h"


// global application pointer
class TAppConfig;
extern TAppConfig *tApp;

// should be a private subclass of TAppConfig
class TAppConfigItem {
 public:
   // types
enum TACO_TYPE {TACO_TYPE_NONE, STRING, INT, DOUBLE, BOOL, SWITCH};
enum TACO_STRING_MODE {OVERRIDE, APPEND, ONCE};
enum TACO_SET_IN {NEVER, DEFAULT, COMMAND_LINE, RC_FILE, ENVIRONMENT, APPLICATION};

   // cons
   TAppConfigItem();
   TAppConfigItem(const char *confitem, const char *context, bool privat);
   
   // interface
   void printItemToFile(FILE *f) const; // print default
   void printCurItemToFile(FILE *f, bool simple) const; // print current
   void printValue(const tstring& env, const tstring& rcfile) const;
   void printHelp(int maxoptlen, bool globalonlycl) const;
   int getOptLen() const;
   tstring getTypeStr() const;
   void setValue(const tstring& par, TACO_SET_IN setin, 
		 bool verbose=false, const tstring& env="", 
		 const tstring& rcfile="",
		 const tstring& context="command line");
   
   // set from application methods:
   
   // returns true if value is valid, else false
   // sets value from string according to any type (switch == bool here)
   bool setValueFromAppFromStr(const tstring& parameter);
   void setValueFromApp(const tstring& str);
   // returns true if value is valid, else false
   bool setValueFromApp(double d);
   // returns true if value is valid, else false
   bool setValueFromApp(int i);
   void setValueFromApp(bool b);
   



 private: 
   // private methods
   void setComp(const tvector<tstring>& a, bool privat);
   void validate(const char *context);
   void privateInit();
   tstring getParamStr() const;
   tstring getWasSetStr(const tstring& env, const tstring& rcfile) const;
   tstring getWasSetStr(TACO_SET_IN setin, const tstring& env, const tstring& rcfile) const;
   tstring getFlagsStr(const tstring& optprefix, bool globalonlycl) const;
   

 public:   
   // data
   bool must_have;
   bool should_have;
   bool only_cl;
   bool configopt;
   bool only_app;
   bool save;
   bool optional_param;
   bool hide;
   
   TACO_TYPE type;
   TACO_SET_IN set_in;
   TACO_STRING_MODE string_mode;
   tstring string_sep;
   double double_value, double_upper, double_lower, double_default;
   int int_value, int_upper, int_lower, int_default;
   bool bool_value, bool_default;
   
   // temp flag
   bool printed;

   tstring name;
   tstring context;
   tstring help;
   tstring headline;
   tstring char_name;
   tstring par;
   tvector<tstring> alias;
   tstring type_str;
   tstring upper, lower, def;
   tstring string_value, string_default;
};


// this is the main class of this h file
class TAppConfig {
 public:
   // ctor & dtor
   TAppConfig(const char *conflist[], const char *listname,
	      int argc, char *av[],
	      const char *envstrname,
	      const char *rcname,
	      const tstring& version);
   ~TAppConfig();
      
   // main interface
   void printHelp(bool show_hidden = false) const;
   void printValues() const;
   bool save(tstring *rc_name_out = 0); // save items with item.save==true
   
   // typed options:
   const tstring& getString(const tstring& par) const;
   double getDouble(const tstring& par) const;
   int getInt(const tstring& par) const;
   bool getBool(const tstring& par) const;        // bool + switch
   bool getSwitch(const tstring& par) const;      // bool + switch
   bool operator() (const tstring& par) const; // bool + switch
   
   // untyped parameters:
   tstring param(int i) const { return _params[i]; }
   size_t numParam() const { return _params.size(); }
   const tvector<tstring>& params() const { return _params; }
   
   // set values: 
   // returns true if value is valid, else false
   // sets value from string according to any type (switch == bool here)
   bool setValueFromStr(const tstring &n, const tstring& str);
   void setValue(const tstring &n, const tstring& str);
   bool setValue(const tstring &n, double d);
   bool setValue(const tstring &n, int i);
   void setValue(const tstring &n, bool b);

   // return the upper and lower bounds and defaults for the type
   int intUpper(const tstring &n) const;
   int intLower(const tstring &n) const;
   int intDefault(const tstring &n) const;
   double doubleUpper(const tstring &n) const;
   double doubleLower(const tstring &n) const;
   double doubleDefault(const tstring &n) const;
   tstring stringDefault(const tstring &n) const;
   bool   boolDefault(const tstring &n) const;

   // return location where parameter was set
   TAppConfigItem::TACO_SET_IN wasSetIn(const tstring& n) const;
   bool wasSetByUser(const tstring& n) const;
   
 private:
   // readonly public data
   tvector<tstring> _params;
   
   // private data
   tmap<tstring,int> name;  // get index of long name
   int char2index[256];           // get index of short name
   tvector<TAppConfigItem> opt;    // all options in the order of conflist
   tmap<tstring,int> alias; // aliases for options
   tstring envname;    // name of env var
   tstring rc_name;    // name of loaded rc file
   tstring rc_str;     // namestr of rc file
   bool verbose_conf; // verbose configuration: warnings and values
   bool onlycl;       // true== dont use rcfile and envvar, only command line
   bool stopatdd;     // stop option scanning after --
   bool removedd;     // remove first -- param
   bool ignore_negnum;// something like -20 or -.57 is not trated as options
   tstring usage;      // usage string: printed before help
   tstring trailer;    // trailer string: printed after help
   tstring commonhead; // headline for common options
   
   int getMaxOptLen(bool show_hidden) const;
   void doMetaChar(const tstring& str, const tstring& context);
   void setComp(const tvector<tstring>& a, const tstring& context);
   void addConfigItems(const char **list, const char *listname, bool privat);
   void doCommandLine(int ac, char *av[], const tstring& version);
   void doEnvironVar(const char *envvar);
   void doRCFile(const tstring& rcfile, const tstring& clrcfile);
   void setFromStr(const tstring& str, const tstring& context, TAppConfigItem::TACO_SET_IN setin);
   tstring getName(const tstring& str, const tstring& context, const tstring& optprefix="", bool errorIfNotFound = true) const;
   void createRCFile(const tstring& fname, const tstring& rcname) const;
};



// global helper functions neede by tappconfig and other applications

// file tools

// return true if file is a directory
bool fisdir(const char *fname);

// return true if file is a regular file
bool fisregular(const char *fname);

// return true if file is a symbolic link
bool fissymlink(const char *fname);

// return false if nothing with this name exists
bool fexists(const char *fname);

// return length of file or -1 if file does not exist
off_t flen(const char *fname);
off_t flen(int fdes);
off_t flen(FILE *file);


// errors

void userWarning(const char *message, ...)
#ifdef __GNUC__
  __attribute__ ((format(printf,1,2)))
#endif
    ;
void userError(const char *message, ...) 
#ifdef __GNUC__
  __attribute__ ((noreturn,format(printf,1,2)))
#endif
    ;
int setUserErrorExitStatus(int status);

// checkpoint macros

//#define CHECKPOINTS
#define KEYWORD_CHECKPOINTS

#ifdef CHECKPOINTS

# ifdef __GNUC__
#  define CP() addCheckpoint(__FILE__,__LINE__,__PRETTY_FUNCTION__)
# else
#  define CP() addCheckpoint(__FILE__,__LINE__)
# endif
int addCheckpoint(const char *file, int line, const char *func = 0);

// keyword checkpoint
# ifdef KEYWORD_CHECKPOINTS
#  define for if(CP())for
// #define while if(0);else while
#  define do if(CP())do
// #define if if(0);else if
#  define else else if(CP())
#  define switch if(CP())switch
#  define break if(CP())break
#  define return if(CP())return
# endif

// internal checkpoint data
extern const char *checkpoint_file[];
extern const char *checkpoint_func[];
extern int checkpoint_line[];

#endif


#endif /* tappconfig.h */
