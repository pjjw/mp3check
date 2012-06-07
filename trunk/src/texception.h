/*GPL*START*
 * 
 * texception - basic exceptions
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

#ifndef _ngw_texception_h_
#define _ngw_texception_h_

extern "C" {
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
}

// history:
// 1999:
// 17:02 04 Jun derived from terror.h
// 2000:
// 11:10 09 Jul tbaseexception and texception merged, message() and name() added
// 00:50 09 Jul internal error added

#define TExceptionN(n) public: virtual const char *name()  const { return #n; }
#define TExceptionM(m) public: virtual const char *message() const { return m; }
#define TExceptionM1(m,a) public: virtual const char *message() const { char *buf; asprintf(&buf, m, a); return buf; }
#define TExceptionM2(m,a,b) public: virtual const char *message() const { char *buf; asprintf(&buf, m, a,b); return buf; }
#define TExceptionM3(m,a,b,c) public: virtual const char *message() const { char *buf; asprintf(&buf, m, a,b,c); return buf; }
#define TExceptionM4(m,a,b,c,d) public: virtual const char *message() const { char *buf; asprintf(&buf, m, a,b,c,d); return buf; }

// base class of all exceptions 
class TException {
   TExceptionN(TException);
   virtual ~TException() {}
   TExceptionM("(no message available)");
#if 0    
#ifndef __USE_GNU
   static void asprintf(char **strp, const char *format, ...) {
      va_list ap;
      va_start(ap, format);
      *strp = new char[1024];
      vsprintf(*strp, format, ap);
      va_end(ap);
   }
#endif
#if !(defined HAVE_STRDUP)
   static char *strdup(const char *str) { char *buf; vasprintf(&buf, "%s", str); return buf; }
#endif
#endif
};


// general exceptions, also base classes
class TIndexOutOfRangeException: public TException {
   TExceptionN(TIndexOutOfRangeException);
   TIndexOutOfRangeException(int lower_, int index_, int upper_): 
   lower(lower_), index(index_), upper(upper_) {}
   TExceptionM3("index %d not in [%d..%d]", index, lower, upper);
   int lower, index, upper;
};


class TZeroBasedIndexOutOfRangeException: public TIndexOutOfRangeException {
   TExceptionN(TZeroBasedIndexOutOfRangeException);
   TZeroBasedIndexOutOfRangeException(int index_, int total_num_): TIndexOutOfRangeException(0, index_, total_num_-1) {}
};


class TErrnoException: public TException {
   TExceptionN(TErrnoException);
   TErrnoException(int error = -1): err(error) { if(err < 0) err = errno; }
   const char *str() const { if(err >= 0) return strerror(err); else return "(no error)"; }
   int err;
   TExceptionM2("%s (errno #%d)", str(), err);
};


class TOperationErrnoException: public TErrnoException {
   TExceptionN(TOperationErrnoException);
   TOperationErrnoException(const char *operation_, int error = -1): TErrnoException(error), operation(operation_) {}
   const char *operation;
   TExceptionM3("%s: %s (errno #%d)", operation, str(), err);
};


class TNotFoundException: public TException {
   TExceptionN(TNotFoundException);
};


class TFileOperationErrnoException: public TErrnoException {
   TExceptionN(TFileOperationErrnoException);
   TFileOperationErrnoException(const char *filename_, const char *operation_, int err_ = -1):
   TErrnoException(err_), filename(strdup(filename_)), operation(strdup(operation_)) {}
//   virtual ~TFileOperationErrnoException() { free(filename); free(operation); }
   const char *filename;
   const char *operation;
   TExceptionM3("%s: %s (during %s)", filename, TErrnoException::message(), operation);
//   TExceptionM2("%s: %s (during )", filename, operation);
//   TExceptionM("toll");
};


class TInternalErrorException: public TException {
   TExceptionN(TInternalErrorException);
   TInternalErrorException(const char *error_ = "unspecific error"): error(strdup(error_)) {}
//   ~TInternalErrorException() { free(error); }
   char *error;
   TExceptionM1("internal error: %s", error);
};

class TNotInitializedException: public TInternalErrorException {
   TExceptionN(TNotInitializedException);
   TNotInitializedException(const char *type_name_): type_name(type_name_) {}
   TExceptionM1("internal_error: object of type '%s' is not initialized", type_name ? type_name : "<unknown>");
   const char *type_name;
};


#endif /* texception.h */
