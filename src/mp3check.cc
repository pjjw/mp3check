/*GPL*START*
 * mp3check - check mp3 file for consistency and print infos
 * 
 * Copyright (C) 1998-2005,2008-2009 by Johannes Overmann <Johannes.Overmann@gmx.de>
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


#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "tappconfig.h"
#include "crc16.h"
#include "id3tag.h"
#include "tfiletools.h"



#define ONLY_MP3 "mp3,MP3,Mp3,mP3"

// please update also HISTORY
#define VERSION "0.8.7"


const char *options[] = {
   "#usage='Usage: [OPTIONS, FILES AND DIRECTORIES] [--] [FILES AND DIRECTORIES]\n\n"
     "this program checks audio mpeg layer 1,2 and 3 (*.mp3) files for\n"       
     "consistency (headers and crc) and anomalies'",
   "#trailer='\n%n version %v *** (C) 1998-2003,2005,2008-2009,2012 by Johannes Overmann\ncomments, bugs and suggestions welcome: %e\n%gpl'",
   "#stopat--",
   "#onlycl",					 
   // options
   "name=list             , type=switch, char=l, help='list parameters by examining the first valid header and size', headline=mode:",
   "name=compact-list     , type=switch, char=c, help='list parameters of one file per line in a very compact format: "
     "version (l=1.0, L=2.0), layer, sampling frequency [kHz] (44=44.1), bitrate [kbit/s], mode (js=joint stereo, st=stereo, sc=single channel, dc=dual channel), "
     "emphasis (n=none, 5=50/15 usecs, J=CCITT J.17), COY (has [C]rc, [O]riginal, cop[Y]right), length [min:sec], filename (poss. truncated)'",
   "name=error-check      , type=switch, char=e, help='check crc and headers for consistency and print several error messages'",
   "name=max-errors       , type=int   , char=m, param=N, lower=0, help='with -e: set maximum number of errors N to print per file (default 0==infinity)'",
   "name=anomaly-check    , type=switch, char=a, help='report all differences from these parameters: layer 3, 44.1kHz, 128kB, joint stereo, no emphasis, has crc'",
   "name=dump-header      , type=switch, char=d, help='dump all possible header with sync=0xfff'",
   "name=dump-tag         , type=switch, char=t, help='dump all possible tags of known version'",
   "name=raw-list         , type=switch,       , help='list parameters in raw output format for use with external programs'",
   "name=raw-elem-sep     , type=string,       , default=0x09, param=N, help='separate elements in one line by char N (numerical ASCII code)'",
   "name=raw-line-sep     , type=string,       , default=0x0a, param=N, help='separate lines by char N (numerical ASCII code)'",
   "name=edit-frame-b     , type=string,       , param=P, help='modify a single byte of a specific frame at a specific offset; B has the format \\'frame,offset,byteval\\', (use 0xff for hex or 255 for dec or 0377 for octal); this mode operates on all given files and is useful for your own experiment with broken streams or while testing this tool ;-)'",
				       
   "name=cut-junk-start   , type=switch,       , help='remove junk before first frame', headline='fix errors:'",
   "name=cut-junk-end     , type=switch,       , help='remove junk after last frame'",
   "name=cut-tag-end      , type=switch,       , help='remove trailing tag'",
   "name=fix-headers      , type=switch,       , help='fix invalid headers (prevent constant parameter switching), implies -e, use with care'",
   "name=fix-crc          , type=switch,       , help='fix crc (set crc to the calculated one), implies -e, use with care\n(note: it is not possible to add crc to files which have been created without crc)'",
   "name=add-tag          , type=switch,       , help='add ID3 v1.1 tag calculated from filename and path using simple heuristics if a file does not already have a tag'",

   "name=ign-tag128       , type=switch, char=G, help='ignore 128 byte TAG after last frame', headline='disable error messages for -e --error-check:'",
   "name=ign-resync       , type=switch, char=Y, help='ignore synchronization errors (invalid frame header/frame too long/short)'",
   "name=ign-junk-end     , type=switch, char=E, help='ignore junk after last frame'",
   "name=ign-crc-error    , type=switch, char=Z, help='ignore crc errors'",
   "name=ign-non-ampeg    , type=switch, char=N, help='ignore non audio mpeg streams'",
   "name=ign-truncated    , type=switch, char=T, help='ignore truncated last frames'",
   "name=ign-junk-start   , type=switch, char=S, help='ignore junk before first frame'",
   "name=ign-bitrate-sw   , type=switch, char=B, help='ignore bitrate switching and enable VBR support'",
   "name=ign-constant-sw  , type=switch, char=W, help='ignore switching of constant parameters, such as sampling frequency'",
   "name=show-valid       , type=switch,       , help='print the message \\'valid audio mpeg stream\\' for all files which appear to be error free (after ignoring errors)",
					 
   "name=any-crc          , type=switch, char=C, help='ignore crc anomalies', headline='disable anomaly messages for -a --anomaly-check'",
   "name=any-mode         , type=switch, char=M, help='ignore mode anomalies'",
   "name=any-layer        , type=switch, char=L, help='ignore layer anomalies'",
   "name=any-bitrate      , type=switch, char=K, help='ignore bitrate anomalies'",
   "name=any-version      , type=switch, char=I, help='ignore version anomalies'",
   "name=any-sampling     , type=switch, char=F, help='ignore sampling frequency anomalies'",
   "name=any-emphasis     , type=switch, char=P, help='ignore emphasis anomalies'",
   
   "name=recursive        , type=switch, char=r, help='process any given directories recursively (the default is to ignore all directories specified on the command line)', headline='file options:'",
   "name=filelist         , type=string, char=f, param=FILE, help='process all files specified in FILE (one filename per line) in addition to the command line'",
   "name=accept           , type=string, char=A, param=LIST, help='process only files with filename extensions specified by comma separated LIST'",
   "name=reject           , type=string, char=R, param=LIST, help='do not process files with a filename extension specified by comma separated LIST'",
   "name=only-mp3         , type=switch, char=3, help='same as --accept " ONLY_MP3 "'",
#ifndef __CYGWIN__
   "name=xdev             , type=switch,         help='do not descend into other filesystems when recursing directories'",
#endif
   "name=print-files      , type=switch,         help='just print all filenames without processing them, then exit (for debugging purposes, also useful to create files for --filelist)'",
     
   "name=single-line      , type=switch, char=s, help='print one line per file and message instead of splitting into several lines', headline='output options:'",
   "name=no-summary       , type=switch,       , help='suppress the summary printed below all messages if multiple files are given'",
   "name=log-file         , type=string, char=g, param=FILE, help='print names of erroneous files to FILE, one per line'",
   "name=quiet            , type=switch, char=q, help='quiet mode, hide messages about directories, non-regular or non-existing files'",
   "name=color            , type=switch, char=o, help='colorize output with ANSI sequences'",
   "name=alt-color        , type=switch, char=b, help='colorize: do not use bold ANSI sequences'",
   "name=ascii-only       , type=switch,         help='replace the range of ASCII chars 160-255 (which is usually printable: e.g. ISO-8859) by \\'?\\''",
   "name=progress         , type=switch, char=p, help='show progress information on stderr'", 
   "name=verbose          , type=switch, char=v, help='be more verbose'",
   "name=no-mmap          , type=switch,       , help='do not use mmap (e.g. when you get \\'mmap: No such device\\')'",
   "name=dummy            , type=switch, char=0, help='do not write/modify anything other than the logfile', headline=common options:",
   "EOL" // end of list     
};

// default value for systems which do not define O_BINARY
#ifndef O_BINARY
#define O_BINARY 0
#endif

					 

// color config:
// 30 black
// 31 red
// 32 green
// 33 yellow
// 34 blue
// 35 magenta
// 36 cyan
// 37 white
const char *cfil = "\033[1;37m";
const char *cano = "\033[1;33m";
const char *cerror = "\033[1;31m";
const char *cval = "\033[1;34m";
const char *cok  = "\033[0;32m";
const char *cnor = "\033[0m";

const char *c_fil = "\033[37m";
const char *c_ano = "\033[33m";
const char *c_err = "\033[31m";
const char *c_val = "\033[34m";
const char *c_ok  = "\033[32m";
const char *c_nor = "\033[0m";

// minimum number of sequential valid and constant frame headers to validate header
const int MIN_VALID = 6;
const int LIST_MAX_HEADER_SEARCH = 1024*1024; // search max 1MB for --list --compact-list --raw-list

// global data
bool progress = false;
bool single_line = false;
bool dummy = false;
int max_errors = 0;
unsigned int columns = 0;
bool show_valid_files = false;
bool quiet = false;
bool only_ascii = false;

bool ano_any_crc   = false;
bool ano_any_bit   = false;
bool ano_any_emp   = false;
bool ano_any_rate  = false;
bool ano_any_mode  = false;
bool ano_any_layer = false;
bool ano_any_ver   = false;

bool ign_crc   = false;
bool ign_start = false;
bool ign_end   = false;
bool ign_tag   = false;
bool ign_bit   = false;
bool ign_const = false;
bool ign_trunc = false;
bool ign_noamp = false;
bool ign_sync  = false;


// header info
					 
int layer_tab[4]= {0, 3, 2, 1};

const int FREEFORMAT = 0;
const int FORBIDDEN = -1;
int bitrate1_tab[16][3] = {   
   {FREEFORMAT, FREEFORMAT, FREEFORMAT},
   {32, 32, 32},
   {64, 48, 40},
   {96, 56, 48},
   {128, 64, 56},
   {160, 80, 64},
   {192, 96, 80},
   {224, 112, 96},
   {256, 128, 112},
   {288, 160, 128},
   {320, 192, 160},
   {352, 224, 192},
   {384, 256, 224},
   {416, 320, 256},
   {448, 384, 320},
   {FORBIDDEN, FORBIDDEN, FORBIDDEN}
};
// 
int bitrate2_tab[16][3] = {   
   {FREEFORMAT, FREEFORMAT, FREEFORMAT},
   { 32,  8,  8},
   { 48, 16, 16},
   { 56, 24, 24},
   { 64, 32, 32},
   { 80, 40, 40},
   { 96, 48, 48},
   {112, 56, 56},
   {128, 64, 64},
   {144, 80, 80},
   {160, 96, 96},
   {176,112,112},
   {192,128,128},
   {224,144,144},
   {256,160,160},
   {FORBIDDEN, FORBIDDEN, FORBIDDEN}
};


double sampd1_tab[4]={44.1, 48.0, 32.0, 0.0};
int samp_1_tab[4]={44100, 48000, 32000, 50000};
double sampd2_tab[4]={22.05, 24.0, 16.0, 0.0};
int samp_2_tab[4]={22050, 24000, 16000, 50000};

const unsigned int CONST_MASK = 0xffffffff;
struct Header {
#ifdef WORDS_BIGENDIAN
   unsigned int
     syncword: 12,         // fix must 0xfff
     ID: 1,                // fix 1==mpeg1.0 0==mpeg2.0
     layer_index: 2,       // fix 0 reserved
     protection_bit: 1,    // fix
     bitrate_index: 4,     //    15 forbidden
     sampling_frequency: 2,// fix 3 reserved
     padding_bit: 1,       //
     private_bit: 1,       //
     mode: 2,              // fix
     mode_extension: 2,    //      (not fix!)
     copyright: 1,         // fix
     original: 1,          // fix
     emphasis: 2;          // fix 2 reserved
#else                  
   unsigned int
     emphasis: 2,          // fix 2 reserved
     original: 1,          // fix
     copyright: 1,         // fix
     mode_extension: 2,    //      (not fix!)
     mode: 2,              // fix
     private_bit: 1,       //
     padding_bit: 1,       //
     sampling_frequency: 2,// fix 3 reserved
     bitrate_index: 4,     //    15 forbidden
     protection_bit: 1,    // fix
     layer_index: 2,       // fix 0 reserved
     ID: 1,                // fix 1==mpeg1.0 0==mpeg2.0
     syncword: 12;         // fix must 0xfff
#endif  // ifdef BIGENDIAN
   
   bool isValid() const {
      if(syncword!=0xfff) return false;
      if(ID==1) { // mpeg 1.0
	 if((layer_index!=0) && (bitrate_index!=15) &&
	    (sampling_frequency!=3) && (emphasis!=2)) return true;
	 return false;
      } else {    // mpeg 2.0
	 if((layer_index!=0) && (bitrate_index!=15) &&
	    (sampling_frequency!=3) && (emphasis!=2)) return true;
	 return false;
      }
   }
   
   bool sameConstant(Header h) const {
      const unsigned int *p1 = (unsigned int*)this;
      const unsigned int *p2 = (unsigned int*)(&h);
      if(*p1 == *p2) return true;
      if((syncword          ==h.syncword          ) &&
	 (ID                ==h.ID                ) &&
	 (layer_index       ==h.layer_index       ) &&
	 (protection_bit    ==h.protection_bit    ) &&
//	 (bitrate_index     ==h.bitrate_index     ) &&
	 (sampling_frequency==h.sampling_frequency) &&
	 (mode              ==h.mode              ) &&
//	 (mode_extension    ==h.mode_extension    ) &&
	 (copyright         ==h.copyright         ) &&
	 (original          ==h.original          ) &&
	 (emphasis          ==h.emphasis          ) &&
	 1) return true;
      else return false;
   }
   
   int bitrate() const {
      if(ID)
	return bitrate1_tab[bitrate_index][layer()-1];
      else
	return bitrate2_tab[bitrate_index][layer()-1];
   }
   int layer() const {return layer_tab[layer_index];}
   
   tstring print() const {
      tstring s;
      
      s.sprintf("(%03x,ID%d,l%d,prot%d,%2d,%4.1fkHz,pad%d,priv%d,mode%d,ext%d,copy%d,orig%d,emp%d)", 
		syncword, ID, layer(), protection_bit, bitrate_index, 
		samp_rate(), padding_bit, private_bit, mode,
		mode_extension, copyright, original, emphasis);
      return s;
   }
   double version() const {
      if(ID) return 1.0;
      else   return 2.0;
   }
   enum {STEREO, JOINT_STEREO, DUAL_CHANNEL, SINGLE_CHANNEL};
   const char *mode_str() const {
      switch(mode) {
       case STEREO:         return "stereo";
       case JOINT_STEREO:   return "joint stereo";
       case DUAL_CHANNEL:   return "dual channel";
       case SINGLE_CHANNEL: return "single chann";
      }
      return 0;
   }
   const char *short_mode_str() const {
      switch(mode) {
       case STEREO:         return "st";
       case JOINT_STEREO:   return "js";
       case DUAL_CHANNEL:   return "dc";
       case SINGLE_CHANNEL: return "sc";
      }
      return 0;
   }
   enum {emp_NONE, emp_50_15_MICROSECONDS, emp_RESERVED, emp_CCITT_J_17};
   const char *emphasis_str() const {
      switch(emphasis) {
       case emp_NONE:               return "no emph";
       case emp_50_15_MICROSECONDS: return "50/15us";
       case emp_RESERVED:           return "reservd";
       case emp_CCITT_J_17:         return "C. J.17";
      }
      return 0;
   }
   const char *short_emphasis_str() const {
      switch(emphasis) {
       case emp_NONE:               return "n";
       case emp_50_15_MICROSECONDS: return "5";
       case emp_RESERVED:           return "!";
       case emp_CCITT_J_17:         return "J";
      }
      return 0;
   }
   double samp_rate() const {
      if(ID)
	return sampd1_tab[sampling_frequency];
      else
 	return sampd2_tab[sampling_frequency];
   }
   int samp_int_rate() const {
      if(ID)
	return samp_1_tab[sampling_frequency];
      else
 	return samp_2_tab[sampling_frequency];
   }
   // this should be not affected by endianess
   int get_int() const {return *((const int *)this);}
};


// get header from pointer
inline Header get_header(const unsigned char *p) {
   Header h;
   unsigned char *q = (unsigned char *)&h;
#ifdef WORDS_BIGENDIAN
      q[0]=p[0];
      q[1]=p[1];
      q[2]=p[2];
      q[3]=p[3];   
#else
      q[0]=p[3];
      q[1]=p[2];
      q[2]=p[1];
      q[3]=p[0];   
#endif   
   return h;
}


// set header to pointer
inline void set_header(unsigned char *p, Header h) {
   unsigned char *q = (unsigned char *)&h;
#ifdef WORDS_BIGENDIAN
      p[0]=q[0];
      p[1]=q[1];
      p[2]=q[2];
      p[3]=q[3];
#else
      p[0]=q[3];
      p[1]=q[2];
      p[2]=q[1];
      p[3]=q[0];
#endif   
}


// set header from header
// preserves padding bit and mode extension (under conditions),
// among other things
void set_header(Header &to, Header from) {
   if(to.mode!=from.mode)
   {
      to.mode            = from.mode;
      to.mode_extension  = from.mode_extension;
   }
   to.ID                 = from.ID;
   to.layer_index        = from.layer_index;
   to.protection_bit     = from.protection_bit;
   to.sampling_frequency = from.sampling_frequency;
   to.copyright          = from.copyright;
   to.original           = from.original;
   to.emphasis           = from.emphasis;
}


// set crc to pointer
inline bool set_crc_value(unsigned char *p, unsigned short c) {
    p[1] = (unsigned char) c;
    p[0] = (unsigned char) (c>>8);
    return true;
}


// return length of frame in bytes
inline int frame_length(Header h) {
   if(h.version() == 1.0) {
      switch(h.layer()) {
       case 1:
	 return (((12000*h.bitrate()) / h.samp_int_rate()) + h.padding_bit) * 4;
       default:
	 return ((144000*h.bitrate()) / h.samp_int_rate()) + h.padding_bit;
      }
   } else {  
      switch(h.layer()) {
       case 1:
	 return (((6000*h.bitrate()) / h.samp_int_rate()) + h.padding_bit) * 4;
       default:
	 return ((72000*h.bitrate()) / h.samp_int_rate()) + h.padding_bit;
      }
   }
}

					 
// return duration of frame in ms
inline double frame_duration(Header h) {
   return (((double)frame_length(h)*8) / h.bitrate());
}


// return next pos of min_valid sequential valid and constant header 
// or -1 if not found
inline int find_next_header(const unsigned char *p, int len, int min_valid) {
   int i;
   const unsigned char *q = p;
   const unsigned char *t;   
   int rest, k, l;
   Header h, h2;
   
   for(i=0; i < len-3; i++, q++) {
      if(*q==255) {
	 h = get_header(q);
	 l = frame_length(h);
	 if(h.isValid() && (l>=21)) {
	    t = q + l;
	    rest = len - i - l;
	    for(k=1; (k < min_valid) && (rest >= 4); k++) {
	       h2 = get_header(t);
	       if(!h2.isValid()) break;
	       if(!h2.sameConstant(h)) break;
	       l = frame_length(h2);
	       if(l < 21) break;
	       t += l;
	       rest -= l;
	    }
	    if(k == min_valid) return i;
	 }
      }
   }
   
   return -1;  // not found
}

// return pointer to beginning of nth frame from start (0 is start) or 0 if frame not found
const unsigned char *skip_n_frames(const unsigned char *start, int len, int n) {
   while(1) {
      // skip invalid data
      int s = find_next_header(start, len, MIN_VALID);
      if(s < 0) return 0;
      start += s;
      len -= s;
      Header h = get_header(start);
      while(1) {
	 if(len < 4) return 0;
	 h = get_header(start);
	 if(h.isValid()) {
	    if(n <= 0) return start;
	    int l = frame_length(h);
	    start += l;
	    len -= l;
	    n--;
	 } else {
	    break;
	 }
      }
   }
}


#ifdef __GNUC__
void fmes(const char *name, const char *format, ...) __attribute__ ((format(printf,2,3)));
#endif

void fmes(const char *name, const char *format, ...) {
   if(quiet) return;
   va_list ap;
   static tstring lastname;
   tstring pname = name;
   pname.replaceUnprintable(only_ascii);
   
   if(progress) putc('\r', stderr);
   if(strcmp(format, "\n") == 0) {
      printf("%s%s%s:\n", cfil, pname.c_str(), cnor);
      return;
   }     
   if(single_line) {
      printf("%s%s%s: ", cfil, pname.c_str(), cnor);
   } else {
      if(name != lastname) {
	 lastname = name;	 
	 tstring s = pname.shortFilename(columns-1);
	 printf("%s%s%s:\n", cfil, s.c_str(), cnor);	 
      }
   }
   va_start(ap, format);
   vprintf(format, ap);
   va_end(ap);
}


// returns true on error
bool error_check(const char *name, const unsigned char *stream, int len, CRC16& crc, bool fix_headers, bool fix_crc) {
   int errors = 0;
   const unsigned char *p = stream;
   int start = find_next_header(p, len, MIN_VALID);
   int rest = len;
   int frame=0;
   double time=0.0;
   int l=0,s;
   Tagv1* tag;
   
   if(start<0) {
      if(!ign_noamp) {
	 fmes(name, "%s%s%s\n", cerror, (len?"not an audio mpeg stream":"empty file"), cnor);
	 errors++;
      }
   } else {
      
      // check for junk at beginning
      if(start>0) {
         int pos=0;
	 if(!ign_start) {
	    fmes(name, "%s%d%s %sbyte%s of junk before first frame header%s\n", 
		 cval, start, cnor, cerror, (start>1)?"s":"", cnor);
	    errors++;
	    // check for possible id3 tags within the junk
	    while(start-pos >= 128)
	    {
	    	int offset=Tagv1::find_next_tag(p+pos, start-pos);
		if(offset!=-1) {
		   pos+=offset;
		   tag=new Tagv1(p+pos);
	           fmes(name, "in leading junk: %spossible %s id3 tag v%u.%u%s at %s0x%08x%s\n",
		        cerror, (tag->isValidSpecs()?"valid":"invalid"),
			(tag->version())>>8, (tag->version())&0xff, cnor,
			cval, pos, cnor);
		   delete tag;
		   pos+=3;
		} else {
		   pos=start;
		}
	    }  
	 }
      }
      
      // check for TAG trailers
      // Note that we emit a warning if we found more than one tag, even
      // if ign_tag is set, unless ign_end is also set.
      tag=new Tagv1(p+rest-128);
      int tag_counter=0;
      while((rest>=128)&&tag->isValid()) {
	 tag_counter++;
	 if((!ign_tag)||((tag_counter>1)&&!ign_end)) {
	    fmes(name, "%s%s%s id3 tag trailer v%u.%u found%s\n", 
	         cerror, (tag_counter>1?"another ":""), (tag->isValidSpecs()?"valid":"invalid"),
		 (tag->version())>>8, (tag->version())&0xff, cnor);
	    errors++;
	 }
	 rest-=128;
	 tag->setTarget(p+rest-128);
      } 
      delete tag;
      
      // check whole file
      rest -= start;
      p += start;
      Header head = get_header(p);
      while(rest>=4) {
	 Header h = get_header(p);
	 if(progress) {
	    if((frame%1000)==0) {
	       putc('.', stderr);
	       fflush(stderr);
	    }
	 }
	 if(!(h.isValid()&&(frame_length(h)>=21))) {
	    // invalid header 
	    
	    // search for next valid header
	    if(!l) {
	       printf("ERROR! Invalid header with no previous frame. Needs debuging.\n");
	    }
	    // search within previous frame
	    p-=(l-4);
	    start-=(l-4);
	    rest+=(l-4);
	    
	    // first look for any isolated frame with the same header
	    s = find_next_header(p, rest, 1);
	    if(s<0) { // error: junk at eof
	       p+=(l-4);
	       start+=(l-4);
	       rest-=(l-4);
	       break;
	    }
	    
	    // else look for a regular stream
      	    h = get_header(p+s);
      	    if(!head.sameConstant(h))
	       s = find_next_header(p, rest, MIN_VALID);
	    if(s<0) { // error: junk at eof
	       p+=(l-4);
	       start+=(l-4);
	       rest-=(l-4);
	       break;
	    }
      	    
	    if(!ign_sync) {
	       if(s<l-4) {
		  fmes(name, "frame %s%5d%s/%s%2u:%02u%s: %ssync error (frame too short)%s at %s0x%08x%s, %s%d%s byte%s mising\n", 
		       cval, frame - 1, cnor,
		       cval, (unsigned int)(time/1000)/60, (unsigned int)(time/1000)%60, cnor,
		       cerror, cnor, cval, start - 4, cnor,
		       cval, l-4-s, cnor, (l-4-s>1)?"s":"");
		  errors++;
	       } else {
		  fmes(name, "frame %s%5d%s/%s%2u:%02u%s: %ssync error (frame too long)%s at %s0x%08x%s, skipping %s%d%s byte%s at %s0x%08x%s\n",
		       cval, frame - 1, cnor,
		       cval, (unsigned int)(time/1000)/60, (unsigned int)(time/1000)%60, cnor,
		       cerror, cnor, cval, start - 4, cnor,
		       cval, s-l+4, cnor, (s-l+4>1)?"s":"",
		       cval, start - 4 + l, cnor);
		  errors++;
	       }
	    }	    

	    // try to fix header including sync information
	    if(fix_headers && (s>l-4)) {
	       unsigned int old_padding_bit = head.padding_bit;
	       head.padding_bit = 0;
	       if(s-l+4 == frame_length(head)) {
		  fmes(name, "frame %s%5d%s/%s%2u:%02u%s: %sfixing header (including sync)%s\n",
		       cval, frame, cnor,
		       cval, (unsigned int)(time/1000)/60, (unsigned int)(time/1000)%60, cnor,
		       cerror, cnor);
		  set_header((unsigned char *)(p+l-4), head);
		  frame++; // we just created a new frame
		  time+=frame_duration(head);
		  l = s-l+4;
	       } else {
		  head.padding_bit = 1;
		  if(s-l+4 == frame_length(head)) {
		     fmes(name, "frame %s%5d%s/%s%2u:%02u%s: %sfixing header (including sync)%s\n",
			  cval, frame, cnor,
			  cval, (unsigned int)(time/1000)/60, (unsigned int)(time/1000)%60, cnor,
			  cerror, cnor);
		     set_header((unsigned char *)(p+l-4), head);
		     frame++; // we just created a new frame
		     time+=frame_duration(head);
		     l = s-l+4;
		  } else {
		     // prevent possible side effect
		     head.padding_bit = old_padding_bit;
		  } 		  
	       }
	    } 

	    // position on next frame
	    p += s;
	    rest -= s;
	    start += s;
	 } else {
	    // valid header
	    
	    // check for constant parameters
	    if(!head.sameConstant(h)) {
	       if(!ign_const) {
		  fmes(name, "frame %s%5d%s/%s%2u:%02u%s: %sconstant parameter switching%s at %s0x%08x%s (%s0x%08x%s -> %s0x%08x%s)\n",
		       cval, frame, cnor,
		       cval, (unsigned int)(time/1000)/60, (unsigned int)(time/1000)%60, cnor,
		       cerror, cnor,
		       cval, start, cnor,
		       cval, head.get_int()&CONST_MASK, cnor,
		       cval, h.get_int()&CONST_MASK, cnor);
		  if(h.ID!=head.ID)
		     printf("frame %s%5d%s/%s%2u:%02u%s:   %sMPEG version switching%s (MPEG %s%1.1f%s -> MPEG %s%1.1f%s)\n",
		            cval, frame, cnor,
		            cval, (unsigned int)(time/1000)/60, (unsigned int)(time/1000)%60, cnor,
		            cerror, cnor,
			    cval, head.version(), cnor,
			    cval, h.version(), cnor);
		  if(h.layer_index!=head.layer_index)
		     printf("frame %s%5d%s/%s%2u:%02u%s:   %sMPEG layer switching%s (layer %s%1d%s -> layer %s%1d%s)\n",
		            cval, frame, cnor,
		            cval, (unsigned int)(time/1000)/60, (unsigned int)(time/1000)%60, cnor,
		            cerror, cnor,
			    cval, head.layer(), cnor,
			    cval, h.layer(), cnor);
		  if(h.samp_rate()!=head.samp_rate())
		     printf("frame %s%5d%s/%s%2u:%02u%s:   %ssampling frequency switching%s (%s%f%skHz -> %s%f%skHz)\n",
		            cval, frame, cnor,
		            cval, (unsigned int)(time/1000)/60, (unsigned int)(time/1000)%60, cnor,
		            cerror, cnor,
			    cval, head.samp_rate(), cnor,
			    cval, h.samp_rate(), cnor);
		  if(h.mode!=head.mode)
		     printf("frame %s%5d%s/%s%2u:%02u%s:   %smode switching%s (%s%s%s -> %s%s%s)\n",
		            cval, frame, cnor,
		            cval, (unsigned int)(time/1000)/60, (unsigned int)(time/1000)%60, cnor,
		            cerror, cnor,
			    cval, head.mode_str(), cnor,
			    cval, h.mode_str(), cnor);
		  if(h.protection_bit!=head.protection_bit)
		     printf("frame %s%5d%s/%s%2u:%02u%s:   %sprotection bit switching%s (%s%s%s -> %s%s%s)\n",
		            cval, frame, cnor,
		            cval, (unsigned int)(time/1000)/60, (unsigned int)(time/1000)%60, cnor,
		            cerror, cnor,
			    cval, head.protection_bit?"no crc":"crc", cnor,
			    cval, h.protection_bit?"no crc":"crc", cnor);
		  if(h.copyright!=head.copyright)
		     printf("frame %s%5d%s/%s%2u:%02u%s:   %scopyright bit switching%s (%s%s%s -> %s%s%s)\n",
		            cval, frame, cnor,
		            cval, (unsigned int)(time/1000)/60, (unsigned int)(time/1000)%60, cnor,
		            cerror, cnor,
			    cval, head.copyright?"copyright":"no copyright", cnor,
			    cval, h.copyright?"copyright":"no copyright", cnor);
		  if(h.original!=head.original)
		     printf("frame %s%5d%s/%s%2u:%02u%s:   %soriginal bit switching%s (%s%s%s -> %s%s%s)\n",
		            cval, frame, cnor,
		            cval, (unsigned int)(time/1000)/60, (unsigned int)(time/1000)%60, cnor,
		            cerror, cnor,
			    cval, head.original?"original":"not original", cnor,
			    cval, h.original?"original":"not original", cnor);
		  errors++;
	       }
	       if(fix_headers) {
		  fmes(name, "frame %s%5d%s/%s%2u:%02u%s: %sfixing header%s\n", 
		       cval, frame, cnor,
		       cval, (unsigned int)(time/1000)/60, (unsigned int)(time/1000)%60, cnor,
		       cerror, cnor);
		  // fix only what should be
		  set_header(h, head);
		  set_header((unsigned char *)p, h);
	       } 
	    }
	    if(head.bitrate_index != h.bitrate_index) {
	       if(!ign_bit) {
		  fmes(name, "frame %s%5d%s/%s%2u:%02u%s: %sbitrate switching%s (%s%d%s -> %s%d%s)\n", 
		       cval, frame, cnor,
		       cval, (unsigned int)(time/1000)/60, (unsigned int)(time/1000)%60, cnor,
		       cerror, cnor, 
		       cval, head.bitrate(), cnor,
		       cval, h.bitrate(), cnor);
		  errors++;
		  if(fix_headers) {
		     fmes(name, "frame %s%5d%s/%s%2u:%02u%s: %sfixing header%s\n", 
			  cval, frame, cnor,
			  cval, (unsigned int)(time/1000)/60, (unsigned int)(time/1000)%60, cnor,
			  cerror, cnor);
		     // fix only what should be
		     h.bitrate_index=head.bitrate_index;
		     set_header((unsigned char *)p, h);
		  } 
	       }
	    }
	    head = h;
	    	    
	    // check crc16
	    if((!ign_crc)&&(h.protection_bit==0)&&(rest>=32+6)) {
	       // reset crc checker
	       crc.reset(0xffff);
	       // get length of side info
	       s = 0;
	       if(h.version()==1.0) { // mpeg 1.0
		  switch(h.layer()) { 
		   case 3:            // layer 3
		     if(h.mode==Header::SINGLE_CHANNEL) s = 17;
		     else s = 32;
		     break;
		     
		   case 1:            // layer 1
		     switch(h.mode) {
		      case Header::SINGLE_CHANNEL: s = 16; break;
		      case Header::DUAL_CHANNEL:   s = 32; break;
		      case Header::STEREO:         s = 32; break;
		      case Header::JOINT_STEREO:   s = 18+h.mode_extension*2; break;
		     }
		     break;
		     
		   default:
		     s = 0; // mpeg 1.0 layer 2 not yet supported
		     break;		     
		  } 
	       } else {               // mpeg 2.0 or 2.5
		  if(h.layer()==3) {  // layer 3
		     if(h.mode==Header::SINGLE_CHANNEL) s = 9; 
		     else s = 17;
		  } else {
		     s = 0; // mpeg 2.0 or 2.5 layer 1 and 2 not yet supported
		  }
	       }
	       if(s) {
		  // calc crc
		  crc.add(p[2]);
		  crc.add(p[3]);
		  for(int i=0; i < s; i++) crc.add(p[i+6]);
		  // check crc
		  unsigned short c = p[5] | ((unsigned short)(p[4])<<8);
		  int fixed_crc = 0;
		  if(c != crc.crc()) {
		     if(fix_crc) {
			fixed_crc = set_crc_value((unsigned char *) &(p[4]), crc.crc());
		     }
		     fmes(name, "frame %s%5d%s/%s%2u:%02u%s: %scrc error%s (%s0x%04x%s!=%s0x%04x%s)%s\n",
			  cval, frame, cnor,
			  cval, (unsigned int)(time/1000)/60, (unsigned int)(time/1000)%60, cnor,
		          cerror, cnor, 
			  cval, c, cnor, cval, crc.crc(), cnor, fixed_crc ? " fixed" : "");
		     errors++;
		  }
//		  printf("frame=%d, pos=%d, s=%d, c=%04x crc.crc()=%04x\n", frame, start, s, c, crc.crc());
	       }
	    }
	    
	    // skip to next frame
	    l = frame_length(h);
	    p += l;
	    rest -= l;
	    start += l;
	    frame++;
	    time+=frame_duration(h);
	 }
	 
	 // maximum number of error reached?
	 if(max_errors && (errors >= max_errors))
	 {
	    fmes(name, "frame %s%5d%s/%s%2u:%02u%s: %smaximum number of errors exceeded%s\n", 
		 cval, frame, cnor,
		 cval, (unsigned int)(time/1000)/60, (unsigned int)(time/1000)%60, cnor,
		 cerror, cnor);
	    rest = 0;
	    break;
	 }
      }
      
      // check for truncated file
      if(rest < 0) {
	 if(!ign_trunc) {
	    fmes(name, "frame %s%5d%s/%s%2u:%02u%s: %sfile truncated%s, %s%d%s byte%s missing for last frame\n", 
		 cval, frame, cnor,
		 cval, (unsigned int)(time/1000)/60, (unsigned int)(time/1000)%60, cnor,
		 cerror, cnor, cval, -rest, cnor, (-rest)>1?"s":"");	    
	    errors++;
	 }
      }
      
      // check for trailing junk
      if(rest > 0) {
	 if(!ign_end) {
	    fmes(name, "frame %s%5d%s/%s%2u:%02u%s: %s%d%s %sbyte%s of junk after last frame%s at %s0x%08x%s\n", 
		 cval, frame, cnor,
		 cval, (unsigned int)(time/1000)/60, (unsigned int)(time/1000)%60, cnor,
		 cval, rest, cnor, cerror, (rest>1)?"s":"", cnor, cval, start, cnor);
	    errors++;
	    // check for possible id3 tags within the junk
	    while(rest >= 128)
	    {
	    	int offset=Tagv1::find_next_tag(p, rest);
		if(offset!=-1) {
		   start+=offset;
		   p+=offset;
		   tag=new Tagv1(p);
	           fmes(name, "in trailing junk: %spossible %s id3 tag v%u.%u%s at %s0x%08x%s\n",
		        cerror, (tag->isValidSpecs()?"valid":"invalid"),
				(tag->version())>>8, (tag->version())&0xff, cnor,
			cval, start, cnor);
		   delete tag;
		   p+=3;
		   start+=3;
		   rest-=(offset+3);
		} else {
		   start+=rest;
		   p+=rest;
		   rest=0;
		}
	    }
	 }
      }      
   }   
   if(progress) {
      fputs("\r                                                                              \r", stderr);
      fflush(stderr);
   }
   if((errors == 0) && show_valid_files)
      fmes(name, "%svalid audio mpeg stream%s\n", cok, cnor);
   return errors > 0;
}


// returns true on anomaly
bool anomaly_check(const char *name, const unsigned char *p, int len, bool err_check, int& err) {
   bool had_ano = false;
   int start = find_next_header(p, len, MIN_VALID);
   if(start>=0) {
      Header h = get_header(p+start);
      if(!ano_any_ver) {
	 if(h.version()!=1.0) {
	    fmes(name, "%sanomaly%s: audio mpeg version %s%3.1f%s stream\n", 
		 cano, cnor, cval, h.version(), cnor);	 
	    had_ano = true;
	 } 
      }
      if(!ano_any_layer) {
	 if(h.layer()!=3) {
	    fmes(name, "%sanomaly%s: audio mpeg %slayer %d%s stream\n", 
		 cano, cnor, cval, h.layer(), cnor);	 
	    had_ano = true;
	 } 
      }
      if(!ano_any_rate) {
	 if(h.samp_rate()!=44.1) {
	    fmes(name, "%sanomaly%s: sampling rate %s%4.1fkHz%s\n", 
		 cano, cnor, cval, h.samp_rate(), cnor); 
	    had_ano = true;
	 }
      }
      if(!ano_any_bit) {
	 if(h.bitrate()!=128) {
	    fmes(name, "%sanomaly%s: bitrate %s%3dkbit/s%s\n", 
		 cano, cnor, cval, h.bitrate(), cnor); 
	    had_ano = true;
	 }
      }
      if(!ano_any_mode) {
	 if(h.mode!=Header::JOINT_STEREO) {
	    fmes(name, "%sanomaly%s: mode %s%s%s\n", 
		 cano, cnor, cval, h.mode_str(), cnor);  
	    had_ano = true;
	 }
      }
      if(!ano_any_crc) {
	 if(h.protection_bit==1) {
	    fmes(name, "%sanomaly%s: %sno crc%s\n", 
		 cano, cnor, cval, cnor);	    
	    had_ano = true;
	 }
      }
      if(!ano_any_emp) {
	 if(h.emphasis!=Header::emp_NONE) {
	    fmes(name, "%sanomaly%s: emphasis %s%s%s\n", 
		 cano, cnor, cval, h.emphasis_str(), cnor); 
	    had_ano = true;
	 }
      }
   } else {
      if((!err_check)&&(!ign_noamp)) {
	 fmes(name, "%s%s%s\n", cerror, (len?"not an audio mpeg stream":"empty file"), cnor);
	 ++err;
      }
   }
   return had_ano;
}
					

// returns the stream duration in ms
// also returns minimum, maximum and average bitrates if told to
unsigned int stream_duration(const unsigned char *p, int len, unsigned short int *minbr, unsigned short int *maxbr, unsigned short int *avgbr) {
   int next = find_next_header(p, len, MIN_VALID);
   int rest = len - next;
   double duration = 0.0;
   unsigned short int min = 2048, max = 0;
   unsigned long int bytes = 0;

   if(next<0) return 0;

   while(rest>=4) {
      Header h=get_header(p+next);
      if(!h.isValid()) {
      	 int old = next;
	 next = find_next_header(p+old, rest, MIN_VALID);
	 if(next<0) break;
	 rest -= next;
	 next += old;
      }
      else
      {
	 int l=frame_length(h);
	 int br=h.bitrate();
	 if(br>max) max=br;
	 if(br<min) min=br;
	 bytes+=l;
	 next+=l;
	 rest-=l;
	 duration+=frame_duration(h);
      }
   }

   if(minbr!=NULL) { *minbr = min; }
   if(maxbr!=NULL) { *maxbr = max; }
   if(avgbr!=NULL) { *avgbr = (bytes * 8) / (unsigned int)duration; }
   return (unsigned int)duration;
}


// return true if junk was found and cut
bool cut_junk_end(const char *name, const unsigned char *p, int len, const unsigned char *free_p, int fd, int& err) {
// this is basically the error_check routine which treats only the trainling junk case
// (implemented by Pollyanna Lindgren <jtlindgr@cs.helsinki.fi>)
   int start = find_next_header(p, len, MIN_VALID);
   int rest = len;
   int frame=0;
   int l,s;
   Tagv1 *tag = 0;
   bool have_a_tag = false;

   if(start<0) {
      if(!ign_noamp) {
	 fmes(name, "%s%s%s\n", cerror, (len?"not an audio mpeg stream":"empty file"), cnor);
	 err++;
      }
      return false;
   } else {
      
      // check for TAG trailer
      if(rest >= 128) {
         tag = new Tagv1(p + rest - 128);
	 if(tag->isValid()) {
	    if(progress) putc('\r', stderr);
	    if(tag->store())
	    {
	       fmes(name, "%scut-junk-end: %s id3 tag trailer v%u.%u found, protecting it%s\n",
		  cok, (tag->isValidSpecs()?"valid":"invalid"),
		  (tag->version())>>8, (tag->version())&0xff, cnor);
	       have_a_tag = true;
	    }
	    else
	    {
	       fmes(name, "%scut-junk-end: %s id3 tag trailer v%u.%u found, could not protect it, sorry%s\n",
		  cok, (tag->isValidSpecs()?"valid":"invalid"),
		  (tag->version())>>8, (tag->version())&0xff, cnor);
	    }
	 }
      } 

      // check whole file
      rest -= start;
      p += start;
      Header head = get_header(p);
      l = frame_length(head);
      p += l;
      rest -= l;
      start += l;
      while(rest>=4) {
	 Header h = get_header(p);
	 frame++;
	 if(progress) {
	    if((frame%1000)==0) {
	       putc('.', stderr);
	       fflush(stderr);
	    }
	 }
	 if(!(h.isValid()&&(frame_length(h)>=21))) {
	    // invalid header
	    
	    // search for next valid header
	    s = find_next_header(p, rest, MIN_VALID);
	    
	    if(s<0) break; // error: junk at eof
	    
	    // skip s invalid bytes
	    p += s;
	    rest -= s;
	    start += s;
	 } else {
	    // valid header
	    
	    head = h;
	    
	    // skip to next frame
	    l = frame_length(h);
	    p += l;
	    rest -= l;
	    start += l;
	 }
      }
      
      // in case we had found a tag
      if((rest >= 128) && have_a_tag)
      {
      	rest-=128;
      }
      
      // remove incomplete last frames
      //if((rest < 0) && remove_truncated_last_frame)
      //{
	// rest += l;
      //}
      
      // remove trailing junk
      if(rest > 0) {
	 fmes(name, "%scut-junk-end: removing last %s%d%s byte%s, %sretrying%s\n",
	      cok, cval, rest, cok, (rest>1)?"s":"", dummy?"not (due to dummy) ":"", cnor);
	 if(!dummy) {
	    // rewrite tag
	    if(have_a_tag)
	    {
	       if(tag->restore((unsigned char*)p)) {
	          fmes(name, "%scut-junk-end: tag successfully restored%s\n", cok, cnor);
	       } else {
	          fmes(name, "%scut-junk-end: unable to restore tag, sorry%s\n", cok, cnor);
	       }
	       delete tag;
		     
	    }
	    // unmap file
	    if(munmap((char*)free_p, len)) {
	       perror("munmap");
	       userError("can't unmap file '%s'!\n", name);
	    }
	    // truncate file and close
	    len-=rest;
#ifndef __STRICT_ANSI__
	    if(ftruncate(fd, len) < 0) {
		perror("ftruncate");
	    }
#else
	    userError("cannot truncate file since this executable was compiled with __STRICT_ANSI__ defined!\n");
#endif
	    close(fd);      
	 }
	 return true;
      } else {
	 fmes(name, "%scut-junk-end: no junk found%s\n", cok, cnor);	    
	 return false;
      }
   }
}
					 
   
// return true if trailing tag was found and cut
bool cut_tag_end(const char *name, const unsigned char *p, int len, int fd, int& err) {
// this is basically the cut_junk_end routine that only looks for a 128 bytes trailing tag
// (implemented by Jean Delvare <delvare@ensicaen.ismra.fr>)
   int start = find_next_header(p, len, MIN_VALID);
   Tagv1* tag;

   if(start<0) {
      if(!ign_noamp) {
	 fmes(name, "%s%s%s\n", cerror, (len?"not an audio mpeg stream":"empty file"), cnor);
	 err++;
      }
      return false;
   } else {
      
      // check for TAG trailer
      if(len>=128) {
         tag=new Tagv1(p+len-128);
	 if(tag->isValid()) {
	    if(progress) putc('\r', stderr);
	    fmes(name, "%scut-tag-end: %s id3 tag trailer v%u.%u found and removed, %sretrying%s\n",
		 cok, (tag->isValidSpecs()?"valid":"invalid"),
		 (tag->version())>>8, (tag->version())&0xff, dummy?"not (due to dummy) ":"", cnor);
	    if(!dummy) {

	       // unmap file
	       if(munmap((char*)p, len)) {
	          perror("munmap");
	          userError("can't unmap file '%s'!\n", name);
	       }
	          
	       // truncate file and close
	       len-=128;
#ifndef __STRICT_ANSI__
	       if(ftruncate(fd, len) < 0) {
		  perror("ftruncate");
	       }
#else
	       userError("cannot truncate file since this executable was compiled with __STRICT_ANSI__ defined!\n");
#endif
	       close(fd);      
	    }
	    delete tag;
	    return true;
	 }
         delete tag;
      }
      fmes(name, "%scut-tag-end: no tag found%s\n", cok, cnor);
      return false;
   }
}

// check for ID3 v1.x tag
bool checkForID3V1(const unsigned char *p, size_t len)
{
    if(len < 128)
	return false;
    return memcmp(p + len - 128, "TAG", 3) == 0;
}

// check for ID3 v2.x tag
bool checkForID3V2(const unsigned char *p, size_t len)
{
    if(len < 10)
	return false;
    return (memcmp(p + len - 10, "3DI", 3) == 0) || (memcmp(p, "ID3", 3) == 0);
}

// check for plausible string
bool isValidStr(const unsigned char *p, size_t len)
{
    for(size_t i = 0; (i < len) && (i < 30); i++)
    {
	if(p[i] == 0)
	    return i > 2;
	if(!isprint(p[i]))
	    return false;
    }
	return true;
}

// split artist and title
void splitArtistTitle(const tstring& title, tstring &artistOut, tstring &titleOut)
{
    tstring origtitle = title;
    const char *start = title.c_str();
    int patlen = 3;
    const char *p = strstr(start, " - ");
    if(p == 0)
    {
	patlen = 1;
	p = strstr(start, "-");
    }
    if(p)
    {
	size_t len = p - start;
	artistOut = title.substr(0, len);
	titleOut = title.substr(len + patlen);
	if((strcasecmp(titleOut.substr(0, 3).c_str(), "Vol") == 0) ||
	   (strcasecmp(titleOut.substr(0, 3).c_str(), "CD1") == 0) ||
	   (strcasecmp(titleOut.substr(0, 3).c_str(), "CD3") == 0) ||
	   (strcasecmp(titleOut.substr(0, 3).c_str(), "CD4") == 0) ||
	   (strcasecmp(titleOut.substr(0, 3).c_str(), "CD2") == 0))
	{
	    titleOut = origtitle;
	    artistOut = "";
	}
	if(artistOut == "Various")
	    artistOut = "";
	if(artistOut == "Sampler")
	    artistOut = "";
    }
    else
    {
	artistOut = "";
	titleOut = origtitle;
    }
}


// check for possible tags more sloppy
bool checkForTagsSloppy(const unsigned char *p, size_t len)
{
    if(len < 10)
	return false;
    size_t max = 1024*1024*1024;
    if(max > len - 3)
	max = len - 3;
    for(size_t i = 0; i <= max; i++)
    {
	if(
	   ((memcmp(p + i, "TAG", 3) == 0) && (isValidStr(p + i + 3, len - i - 3))) ||
	   (
	    (
	     (memcmp(p + i, "ID3", 3) == 0) ||
	     (memcmp(p + i, "3DI", 3) == 0)
	     ) && 
	    (p[i + 3] < 10) && (p[i + 4] < 10) && ((p[i + 5] & 0xf) == 0) && 
	    (p[i + 6] < 128) && (p[i + 7] < 128) && (p[i + 8] < 128) && (p[i + 9] < 128)))
	{
	    printf("%swarning: possible %c%c%c tag found at offset %d (%d from end) (%02x %02x %02x %02x %02x %02x %02x)%s\n", 
		   cano, p[i+0], p[i+1], p[i+2], int(i), int(len - 3 - i), p[i+3]&255, p[i+4]&255, p[i+5]&255, 
		   p[i+6]&255, p[i+7]&255, p[i+8]&255, p[i+9]&255, cnor);
	    return true;
	}
    }
    return false;
}

// capitalize string
void capitalize(tstring &str)
{
    bool lastAlpha = false;
    for(size_t i = 0; i < str.length(); i++)
    {
	if((!lastAlpha) && islower(str[i]))
	    str[i] = toupper(str[i]);
	lastAlpha = isalpha(str[i]);
    }
}

// main
int main(int argc, char *argv[]) {      

   // get parameters
   TAppConfig ac(options, "options", argc, argv, 0, 0, VERSION);
   
   // get the terminal width if available
   struct winsize win;
   if(ioctl(1, TIOCGWINSZ, &win) == 0) 
       columns = win.ws_col;
   const char* scolumns=getenv("COLUMNS");
   if(scolumns)
      columns=atoi(scolumns)-1;
   if(columns<30)
      columns=79;

   // setup options
   quiet = ac("quiet");
   dummy = ac("dummy");
   progress = ac("progress");
   max_errors = ac.getInt("max-errors");
   show_valid_files = ac("show-valid");
   bool nommap = ac("no-mmap");
   int opt=0;
   // alt mode
   if(ac("error-check")) opt=1; 
   if(ac("fix-headers")) opt=1;
   if(ac("fix-crc")) opt=1;
   if(ac("add-tag")) opt=1;
   if(ac("anomaly-check")) opt=1; 
   if(ac("cut-junk-start")) opt=1; 
   if(ac("cut-junk-end")) opt=1;
   if(ac("cut-tag-end")) opt=1;
   if(!ac.getString("edit-frame-b").empty()) opt=1;
   // main mode
   if(ac("dump-header")) opt++; 
   if(ac("dump-tag")) opt++;
   if(ac("list")) opt++; 
   if(ac("compact-list")) opt++; 
   if(ac("raw-list")) opt++; 
   if(ac("print-files")) opt=1;
   // check for mode
   if(opt==0) 
     userError("you must specify the mode of operation!  (try --help for more info)\n");
   if(opt>1) 
     userError("incompatible modes specified!  (try --help for more info)\n");
   // color
   if(!ac("color"))
     cval = cnor = cano = cerror = cfil = cok = "";
   if(ac("alt-color")) {
      cval = c_val;
      cano = c_ano;
      cerror = c_err;
      cfil = c_fil;
      cnor = c_nor;
      cok = c_ok;
   }
   tstring rawsepstr = ac.getString("raw-elem-sep");
   char rawsep = strtol(rawsepstr.c_str(), 0, 0);
   if(!rawsepstr.empty()) if(!isdigit(rawsepstr[0])) rawsep = rawsepstr[0];
   tstring rawlinesepstr = ac.getString("raw-line-sep");
   char rawlinesep = strtol(rawlinesepstr.c_str(), 0, 0);     
   if(!rawlinesepstr.c_str()) if(!isdigit(rawlinesepstr[0])) rawsep = rawlinesepstr[0];
   if(ac("raw-list")) {
      quiet=true;
      progress=false;
   }
   bool edit_frame_byte = !ac.getString("edit-frame-b").empty();
   int efb_value = 0;
   int efb_offset = 0;
   int efb_frame = 0;
   if(edit_frame_byte) {
      tvector<tstring> a = split(ac.getString("edit-frame-b"), ",");
      if((a.size() != 3) || !a[0].toInt(efb_frame) || !a[1].toInt(efb_offset) || !a[2].toInt(efb_value))
	userError("format of parameter for --edit-frame-b is 'frame,offset,byteval'!\n");
   }
   bool recursive = ac("recursive");
#ifdef __CYGWIN__
   bool cross_filesystems = true;
#else
   bool cross_filesystems = !ac("xdev");
#endif
   tstring extensions = ac.getString("accept");
   tstring reject_extensions = ac.getString("reject");
   if(ac("only-mp3")) {
      if(extensions.empty()) extensions = ONLY_MP3;
      else                   extensions += "," ONLY_MP3;
   }
   single_line = ac("single-line");
   only_ascii = ac("ascii-only");
   
   
   // check params
   if(ac.numParam()==0) 
     userError("need at least one file or directory! (try --help for more info)\n");
   
   // setup ignores/anys
   ano_any_crc   = ac("any-crc");
   ano_any_mode  = ac("any-mode");
   ano_any_layer = ac("any-layer");
   ano_any_bit   = ac("any-bitrate");
   ano_any_emp   = ac("any-emphasis");
   ano_any_rate  = ac("any-sampling");
   ano_any_ver   = ac("any-version");
   ign_crc   = ac("ign-crc-error");
   ign_start = ac("ign-junk-start");
   ign_end   = ac("ign-junk-end");
   ign_tag   = ac("ign-tag128");
   ign_bit   = ac("ign-bitrate-sw");
   ign_const = ac("ign-constant-sw");
   ign_trunc = ac("ign-truncated");
   ign_noamp = ac("ign-non-ampeg");
   ign_sync  = ac("ign-resync");

   // get file list
   tvector<tstring> filelist;
   // from command line (perhaps recurse directories)
   for(size_t i = 0; i < ac.numParam(); i++) {
      if(recursive) {
	 try {
	    TFile f(ac.param(i));
	    if(f.isdir()) {
	       TSubTreeContext context(cross_filesystems);
	       TDir d(f, context);
	       filelist += findFilesRecursive(d);
	       continue;
	    }
	 }
	 catch(...) {}
      } 
      filelist.push_back(ac.param(i));
   }     
   // read filenames from text file
   tstring filelistfile = ac.getString("filelist");
   if(!filelistfile.empty()) {
      try {
	 filelist += loadTextFile(filelistfile.c_str());
      }
      catch(...) {
	 userError("cannot open file '%s' for reading!\n", filelistfile.c_str());
      }      
   }
   // filter filenames
   if(!extensions.empty())
     filelist = filterExtensions(filelist, split(extensions, ",;:"));
   if(!reject_extensions.empty())
     filelist = filterExtensions(filelist, split(reject_extensions, ",;:"), true);

   // print filelist
   if(ac("print-files")) {
      for(size_t i = 0; i < filelist.size(); i++) 
	printf("%s\n", filelist[i].c_str());
      exit(0);
   }
   
   // check all files
   int err=0;
   int checked=0;
   int num_ano=0;
   int num_tagsadded = 0;
   CRC16 crc(CRC16::CRC_16);   
   FILE *log = NULL;
   if(!ac.getString("log-file").empty()) {
      log = fopen(ac.getString("log-file").c_str(), "a");
      if(log==NULL)
	userError("can't open logfile '%s'!\n", ac.getString("log-file").c_str());
   }
   for(size_t i = 0; i < filelist.size(); i++) {
      const char *name = filelist[i].c_str();
      // ignore all files starting with ._ which are apple metafiles
      {
	  tstring t = name;
	  t.extractFilename();
	  if((t[0] == '.') && (t[1] == '_'))
	      continue;
      }
       
      // check for file
      struct stat buf;
      if(stat(name, &buf)) {
	 fmes(name, "%scan't stat file (dangling symbolic link?)%s\n", cerror, cnor);
	 continue;
      }
      if(S_ISDIR(buf.st_mode)) {
	 fmes(name, "%signoring directory%s\n", cerror, cnor);
	 continue;
      }
      if(!S_ISREG(buf.st_mode)) {
	 fmes(name, "%signoring non regular file%s\n", cerror, cnor);
	 continue;
      }
      off_t len = buf.st_size;
      
      // open file
      int flags = O_RDONLY;
      int prot = PROT_READ;
      if(!dummy) {
	 if(ac("fix-headers")||ac("cut-junk-start")||ac("fix-crc")||ac("cut-junk-end")||ac("cut-tag-end")||edit_frame_byte) {
	    flags = O_RDWR;
	    prot |= PROT_WRITE;
	    if(nommap)
		 userError("option --no-mmap does not yet support options which may modify a file (e.g --fix-crc)!\n");
	 }
      }
      flags |= O_BINARY;
      int fd = open(name, flags);
      if(fd==-1) {
	 perror("open");
	 userError("can't open file '%s' for reading!\n", name);      
      }
       
      // mmap or read file
      const unsigned char *p;
      const unsigned char *free_p = 0;
      if(nommap) {
	  // read file
	  free_p = p = new unsigned char[len];
	  if(read(fd, (void*)p, len) != len) {
	      perror("read");
	      userError("error while reading file '%s'!\n", name);
	  }
      } else {
	  // mmap file
	  if(len) {
	      free_p = p = (const unsigned char *) mmap(0, len, prot, MAP_SHARED, fd, 0);
	  } else {
	      p = NULL;
	  }
	  if(p==(const unsigned char *)MAP_FAILED) {
	      perror("mmap");
	      userError("can't map file '%s'!\n", name);
	  }
      }

      // edit single byte of a frame
      if(edit_frame_byte) {
	 if(progress) {
	    tstring s = tstring(name).shortFilename(79);
	    fprintf(stderr, "%-79.79s\r", s.c_str());
	    fflush(stderr);
	 }	 
	 unsigned char *pp = const_cast<unsigned char *>(skip_n_frames(free_p, len, efb_frame));
	 if(pp) {
	    if(!dummy) pp[efb_offset] = efb_value;
	 } else {
	    fmes(name, "%sframe %s%d%s not found%s\n", cerror, cval, efb_frame, cerror, cnor);
	    err++;
	 }	 	 
      }      
            
      // list
      if(ac("list")||ac("compact-list")||ac("raw-list")) {
	 // speed up list of very large files (like *.wav)
	 int maxl = LIST_MAX_HEADER_SEARCH;
	 int start = find_next_header(p, len<maxl?len:maxl, MIN_VALID);
	 if(start<0) {
	    if(!ign_noamp) {
	       if(ac("raw-list")) {
		  printf("%s%c%s%c%c", (len?"invalid_stream":"empty_stream"), rawsep, name, rawsep, rawlinesep);
	       } else if(ac("compact-list")) {
		  printf("%s%-25s%s%s %s%s%s\n", cerror, (len?"not an audio mpeg stream":"empty file"), cnor, (columns>=82?"  ":""), cfil, name, cnor);
	       } else {	       
		  fmes(name, "%s%s%s\n", cerror, (len?"not an audio mpeg stream":"empty file"), cnor);
	       }
	       err++;
	    }
	 } else {
	    Header h = get_header(p+start);
	    unsigned short int minbr = 0, maxbr = 0, avgbr;
	    unsigned int l_min = (ign_bit?stream_duration(p, len, &minbr, &maxbr, &avgbr):len/(h.bitrate()/8));
	    unsigned int l_mil = l_min%1000;
	    l_min/=1000;
	    unsigned int l_sec = l_min%60;
	    l_min/=60;
	    tstring l_str;
	    if(l_min >= 60)
	      l_str.sprintf("%2u:%02u", l_min/60, l_min%60);
	    else 
	      l_str.sprintf("   %2u", l_min);
	    Tagv1 *tag=new Tagv1(p+len-128);
	    unsigned short int tag_version=0;
	    if(tag->isValid())
	    	tag_version=tag->version();
	    if(ac("list")) {
	       unsigned int xwidth = 0;
	       tstring n = single_line?tstring(name):tstring(name).shortFilename(columns-1);
	       fmes(name, "mpeg %s%3.1f%s layer %s%d%s %s%2.1f%skHz %s%3d%skbps",
		      h.version()==1.0?cval:cano, h.version(), cnor, 
		      h.layer()==3?cval:cano, h.layer(), cnor, 
		      h.samp_rate()==44.1?cval:cano, h.samp_rate(), cnor, 
		      (h.bitrate()==128&&minbr==maxbr)?cval:cano, minbr!=maxbr?avgbr:h.bitrate(), cnor);
	       if(ign_bit && columns>=83) {
	          printf(" %s%s%s",
		         minbr!=maxbr?cano:cval,minbr!=maxbr?"VBR":"CBR",cnor);
		  xwidth+=4;
	       } 
	       printf(" %s%-12.12s%s %s%-7.7s%s %s%s%s %s%s%s %s%s%s %s%s:%02u.%02u%s",
		      h.mode==Header::JOINT_STEREO?cval:cano, h.mode_str(), cnor,
		      h.emphasis==Header::emp_NONE?cval:cano, h.emphasis_str(), cnor,
		      h.protection_bit?cano:cval, h.protection_bit?"---":"crc", cnor, 
		      h.original?cval:cano, h.original?"orig":"----", cnor,
		      cval, h.copyright?"copy":"----", cnor,
		      cval, l_str.c_str(), l_sec, l_mil/10, cnor);
	       if(columns>=87+xwidth) {
	         if(tag_version)
		   printf(" id3 %s%1u.%1u%s", cval, tag_version>>8,
		          tag_version&0xff,cnor);
//		 xwidth+=8;
	       }
	       printf("\n");
	    } else if(ac("compact-list")) {
	       unsigned int xwidth = 0;
	       printf("%s%c%s%s%d%s %s%2.0f%s %s%3d%s",
		      h.version()==1.0?cval:cano, h.version()==1.0?'l':'L', cnor, 
		      h.layer()==3?cval:cano, h.layer(), cnor, 
		      h.samp_rate()==44.1?cval:cano, h.samp_rate(), cnor, 
		      (h.bitrate()==128&&minbr==maxbr)?cval:cano, minbr!=maxbr?avgbr:h.bitrate(), cnor);
	       if(ign_bit && columns>=80) {
	          printf("%s%c%s",
		         minbr==maxbr?cval:cano, minbr==maxbr?' ':'V', cnor);
		  xwidth+=1;
	       }
	       printf(" %s%s%s %s%s%s %s%s%s%s%s%s%s%s%s",
		      h.mode==Header::JOINT_STEREO?cval:cano, h.short_mode_str(), cnor,
		      h.emphasis==Header::emp_NONE?cval:cano, h.short_emphasis_str(), cnor,
		      h.protection_bit?cano:cval, h.protection_bit?"-":"C", cnor, 
		      h.original?cval:cano, h.original?"O":"-", cnor,
		      cval, h.copyright?"Y":"-", cnor);
	       if(columns>=81+xwidth) {
	         if(tag_version)
		   printf(" %s%1u%s",cval,tag_version>>8,cnor);
		 else
		   printf(" %s-%s",cval,cnor);
	         xwidth+=2;
	       }
	       tstring n = tstring(name).shortFilename(columns-(26+xwidth));
	       n.replaceUnprintable(only_ascii);
	       printf(" %s%3u:%02u%s %s%s%s\n",
		      cval, l_min, l_sec, cnor, cfil, n.c_str(), cnor);  
	    } else if(ac("raw-list")) {	       
	       printf("valid_stream%c%.1f%c%d%c%.1f%c%d%c%s%c%s%c%s%c%s%c%s%c%s%c%u%c%u%c%u%c%s%c%c",
		      rawsep,
		      h.version(), rawsep,
		      h.layer(), rawsep, 
		      h.samp_rate(), rawsep,
		      minbr!=maxbr?avgbr:h.bitrate(), rawsep,
		      ign_bit?(minbr!=maxbr?"VBR":"CBR"):"?", rawsep,
		      h.mode_str(), rawsep,
		      h.emphasis_str(), rawsep,
		      h.protection_bit?"---":"crc", rawsep,
		      h.original?"orig":"copy", rawsep,
		      h.copyright?"cprgt":"-----", rawsep,
		      l_min, rawsep, 
		      l_sec, rawsep, 
		      l_mil, rawsep,
		      name, rawsep, rawlinesep);
	    }
	    delete tag;
	 }
      }
      
      // cut-junk-start
      if(ac("cut-junk-start")) {
	 int start = find_next_header(p, len, MIN_VALID);
	 if(start<0) {
	    fmes(name, "%s%s%s\n", cerror, (len?"not an audio mpeg stream":"empty file"), cnor);
	    err++;
	 } else if(start==0) {
	    fmes(name, "%scut-junk-start: no junk found%s\n", cok, cnor);	    
	 } else {
	    fmes(name, "%scut-junk-start: removing first %s%d%s byte%s, %sretrying%s\n",
		 cok, cval, start, cok, (start>1)?"s":"", dummy?"not (due to dummy) ":"", cnor);
	    if(!dummy) {
	       // move start to begining and truncate the file
	       memmove((char*)free_p, free_p + start, len - start);
	       if(munmap((char*)free_p, len)) {
		  perror("munmap");
		  userError("can't unmap file '%s'!\n", name);
	       }
	       len-=start;
#ifndef __STRICT_ANSI__
	       if(ftruncate(fd, len) < 0) {
		  perror("ftruncate");
	       }
#else
	       userError("cannot truncate file since this executable was compiled with __STRICT_ANSI__ defined!\n");
#endif
	       close(fd);      
	       
	       // retry this file 
	       --i;
	       continue;
	    }
	 }
      }

      // cut-tag-end
      if(ac("cut-tag-end")) {
	 // lots of side effects: perhaps unmaps and closes file
	 if(cut_tag_end(name, p, len, fd, err)) {
	    // retry this file if not dummy
	    if(!dummy) {
	       --i;
	       continue;
	    }
	 }
      }

      // cut-junk-end
      if(ac("cut-junk-end")) {
	 // lots of side effects: perhaps unmaps and closes file
	 if(cut_junk_end(name, p, len, free_p, fd, err)) {
	    // retry this file if not dummy
	    if(!dummy) {
	       --i;
	       continue;	   
	    }	   
	 }
      }
      
      // check for errors
      if(ac("error-check") || ac("fix-headers") || ac("fix-crc")) {
	 if(progress) {
	    tstring s = tstring(name).shortFilename(79);
	    fprintf(stderr, "%-79.79s\r", s.c_str());
	    fflush(stderr);
	 }
	 if(error_check(name, p, len, crc, ac("fix-headers"), ac("fix-crc"))) {
	    if(log) fprintf(log, "%s\n", name);
	    ++err;
	 }
      }

      // check for anomalies
      if(ac("anomaly-check")) {
	 if(progress) {
	    tstring s = tstring(name).shortFilename(79);
	    fprintf(stderr, "%-79.79s\r", s.c_str());
	    fflush(stderr);
	 }
	 if(anomaly_check(name, p, len, ac("error-check"), err)) ++num_ano;
      }      
            
      // dump header
      if(ac("dump-header")) {
	 fmes(name, "\n");
	 for(int k=0; k<len-3; p++, k++) {
	    if(*p==255) {
	       Header h;	       
	       h = get_header(p);
	       if(h.syncword==0xfff) {
		  tstring s=h.print();
		  printf("%7d %s\n", k, s.c_str());
		  int l = frame_length(h);
		  if(l>=21) {
		     p+=l;
		     p--;
		     k+=l;
		     k--;
		  }
	       }
	    }
	 }
      }      
      
      // dump tag
      if(ac("dump-tag")) {
         unsigned int err_thisfile=0;
	 fmes(name, "\n");
	 Tagv1 *tag=new Tagv1;
	 for(int k=0; k<len-127; k++) {
      	    tag->setTarget(p+k);
	    if(tag->isValidGuess()) {
	       printf("  Found at: %s0x%08x%s (%s%s%s)\n", cval, k, cnor,
	              (k==len-128?cok:cerror),
		      ((k==len-128)||!(++err_thisfile)?"end":"in the stream"), cnor);
	       tag->fillFields();
	       printf("  Version: %s%u.%u%s\n", cval, tag->fields_version>>8,
	              tag->fields_version&0xff, cnor);
	       printf("  Conforms to specification: %s%s%s\n",
	              (tag->isValidSpecs()&&!tag->fields_spacefilled?cok:cerror),
	              (tag->isValidSpecs()||!(++err_thisfile)?(tag->fields_spacefilled?"space filled":"yes"):"no"),
		      cnor);
	       printf("  Title: \"%s%s%s\"\n", cval, tag->field_title, cnor);
	       printf("  Artist: \"%s%s%s\"\n", cval, tag->field_artist, cnor);
	       printf("  Album: \"%s%s%s\"\n", cval, tag->field_album, cnor);
	       printf("  Year: \"%s%s%s\"\n", cval, tag->field_year, cnor);
	       printf("  Comment: \"%s%s%s\"\n", cval, tag->field_comment, cnor);
	       if(tag->field_genre==0xff) // not set
	          printf("  Genre: not set\n");
	       else
	       {
	          printf("  Genre: %s%s%s\n", ((tag->field_genre>=Tagv1::genres_count)?cerror:cval),
		         ((tag->field_genre<Tagv1::genres_count)?(Tagv1::id3_genres[tag->field_genre]):"unknown"),
			 cnor);
	       }
	       if(tag->field_track)
	          printf("  Track: %s%u%s\n", cval, tag->field_track, cnor);
	       k+=2;
	    }
      	 }
	 delete tag;
         if(err_thisfile) ++err;
      }
       
       // --add-id3
       bool addTag = false;
       if(ac("add-tag"))
       {
	   if(progress) {
	       tstring s = tstring(name).shortFilename(79);
	       fprintf(stderr, "%-79.79s\r", s.c_str());
	       fflush(stderr);
	   }
	   // check for existing tag
	   if(checkForID3V1(p, len))
	   {
	       if(ac("verbose"))
		   fmes(name, "id3 tag v1.x found, not adding anything\n");
	   }
	   else if(checkForID3V2(p, len))
	   {
	       if(ac("verbose"))
		   fmes(name, "id3 tag v2.x found, not adding anything\n");
	   }
#if 0	   
	   // this wqs just here to make sure we do not mis something
	   else if(checkForTagsSloppy(p, len))
	   {
	       fmes(name, "some tag found\n");	       
	   }
#endif	   
	   else
	   {
	       // no tag found: add tag
	       addTag = true;
	   }
       }
      
      if(nommap) {
	  // free mem
	  delete[] free_p;
      } else {
	  // unmap file and close
	  if((free_p!=NULL)&&munmap((char*)free_p, len)) {
	      perror("munmap");
	      userError("can't unmap file '%s'!\n", name);
	  }
      }
       // close file
      close(fd);       
       
       // add tag? (see above)
       if(addTag)
       {	   
	   // extract data from filename
	   tstring title;  // 30
	   tstring artist; // 30
	   tstring album;  // 30
	   tstring comment;// 28
	   char track = 0;     // 1
	   tstring fname = name;
	   fname.translateChar('_', ' ');
	   fname.searchReplace("cd 1", "cd1");
	   fname.searchReplace("cd 2", "cd2");
	   fname.searchReplace("cd 3", "cd3");
	   fname.searchReplace("cd 4", "cd4");
	   fname.searchReplace(" - ms/disc1/", "/cd1 - ");
	   fname.searchReplace(" - ms/disc2/", "/cd2 - ");
	   fname.searchReplace("/cd1/", " - cd1/");
	   fname.searchReplace("/cd2/", " - cd2/");
	   fname.searchReplace("/cd3/", " - cd3/");
	   fname.searchReplace("/cd4/", " - cd4/");
	   fname.searchReplace("zance - a decade of dance from ztt", "zance decade of dance from ztt");
	   fname.searchReplace("/captain future soundtrack - ", "/");
	   fname.searchReplace("-ms/", "/");
	   fname.collapseSpace();
	   title = fname;
	   album = fname;
	   comment = fname;
	   title.extractFilename();
	   album.extractPath();
	   album.removeDirSlash();
	   album.extractFilename();
	   comment.extractPath();
	   comment.removeDirSlash();
	   comment.extractPath();
	   comment.removeDirSlash();
	   comment.extractFilename();

	   // title
	   // remove album
	   if(title != album)
	       title.searchReplace(album, "");
	   // remove .mp3
	   if((title.length() >= 4) && strcasecmp(title.c_str() + title.length(), ".mp3"))
	       title.truncate(title.length() - 4);
	   // check for cd1 - 
	   if((strcasecmp(title.substr(0, 3).c_str(), "CD1") == 0) ||
	      (strcasecmp(title.substr(0, 3).c_str(), "CD1") == 0) ||
	      (strcasecmp(title.substr(0, 3).c_str(), "CD2") == 0))
	   {
	       album += " " + title.substr(0,3);
	       title = title.substr(3);
	       // skip separator
	       while(strchr(" -.", title.c_str()[0])) 
		   title = title.substr(1);
	   }
	   // check for AA-TT format
	   if(isdigit(title[0]) && isdigit(title[1]) && isdigit(title[3]) && isdigit(title[4]) && (title[2] == '-'))
	   {
	       album += " cd" + title.substr(1,2);
	       title = title.substr(3);
	   }
	   // check for 'audio '
	   tstring ltitle = title;
	   ltitle.lower();
 	   if(ltitle.substr(0, 5) == "audio")
	       title = title.substr(5) + " " + title.substr(0, 5);
 	   else if(ltitle.substr(0, 10) == "track cd -")
	   {
	       title = title.substr(10);
	       album += " track cd";
	   }
 	   else if(ltitle.substr(0, 8) == "mix cd -")
	   {
	       title = title.substr(8);
	       album += " mix cd";
	   }
 	   else if(ltitle.substr(0, 6) == "track-")
	       title = title.substr(6) + " " + title.substr(0, 6);
 	   else if(ltitle.substr(0, 5) == "track")
	       title = title.substr(5) + " " + title.substr(0, 5);
	   title.cropSpace();
	   title.collapseSpace();
	   
	   // scan track
	   size_t pos = 0;
	   while(isdigit(title.c_str()[pos])) pos++;
	   if((pos >= 1) && (pos <=2))
	   {
	       int t = 0;
	       title.substr(0, pos).toInt(t, 10);
	       track = t;	       
	   }
	   else if(pos > 2)
	       pos = 0;
	   // skip separator
	   while(strchr(" -.", title.c_str()[pos])) pos++;
	   title = title.substr(pos);
	   // split artist - title
	   splitArtistTitle(title, artist, title);
	   title.cropSpace();

	   // album
	   tstring ar;
	   // split artist - album
	   if((album == "alben") ||
	      (album == "cdrom2") ||
	      (album == "cdrom") ||
	      (album == "misc1") ||
	      (album == "mp3") ||
	      (album == "ov") ||
	      (album == "all"))
	       album = "";
	   splitArtistTitle(album, ar, album);
	   if((!artist.empty()) && (!ar.empty()))
	   {
	       if(artist != ar)
	       {
		   title = artist + "-" + title;
		   artist = ar;
	       }
	   }
	   else if(artist.empty())
	   {
	       artist = ar;
	       ar = "";
	   }
	   if(artist.empty())
	   {
	       artist = album;
	       album = "";
	   }
	   album.cropSpace();
	   artist.cropSpace();
	   if(artist == "diverse")
	       artist = "";
	   
	   // comment
	   comment.cropSpace();
	   if((comment == "alben") ||
	      (comment == "cdrom2") ||
	      (comment == "cdrom") ||
	      (comment == "mp3") ||
	      (comment == "ov") ||
	      (comment == "chr music") ||
	      (comment == "mp3 dl") ||
	      (comment == "diverse") ||
	      (comment.substr(0, 2) == "M0") ||
	      (comment.substr(0, 7) == "Unknown") ||
	      (comment.substr(0, 7) == "Various") ||
	      (comment == "all"))
	       comment = "";

	   // capitalize strings
	   capitalize(title);
	   capitalize(artist);
	   capitalize(album);
	   capitalize(comment);
	   
	   // move rest of long strings into comment
	   if(title.length() > 30)
	       title.searchReplace(" - ", "-");
	   if(artist.length() > 30)
	       artist.searchReplace(" - ", "-");
	   if(album.length() > 30)
	       album.searchReplace(" - ", "-");
	   if(title.length() > 30)
	   {
	       comment = "T:" + title.substr(30);
	       title.truncate(30);
	   }
	   if(artist.length() > 30)
	   {
	       comment = "R:" + artist.substr(30);
	       artist.truncate(30);
	   }
	   if(album.length() > 30)
	   {
	       comment = "L:" + album.substr(30);
	       album.truncate(30);
	   }

	   // print and check tag
//	   fmes(name, "appending id3 tag v1.1:\ntitle=  '%s%s%s'\nartist= '%s%s%s'\nalbum=  '%s%s%s'\ncomment='%s%s%s'\ntrack=  %s%d%s\n", 
//		cval, title.c_str(), cnor, cval, artist.c_str(), cnor, cval, album.c_str(), cnor, cval, comment.c_str(), cnor, cval, track, cnor);
	   fmes(name, "'%s%-30.30s%s' '%s%-30.30s%s' '%s%-30.30s%s' '%s%-28.28s%s' %s%d%s\n",
		cval, title.c_str(), cnor, cval, artist.c_str(), cnor, cval, album.c_str(), cnor, cval, comment.c_str(), cnor, cval, track, cnor);
	   if(title.length() > 30)
	       printf("%swarning%s: title > 30 chars\n", cerror, cnor);
	   if(artist.length() > 30)
	       printf("%swarning%s: artist > 30 chars\n", cerror, cnor);
	   if(album.length() > 30)
	       printf("%swarning%s: album > 30 chars\n", cerror, cnor);
	   if(comment.length() > 28)
	       printf("%swarning%s: comment > 28\n", cerror, cnor);
	   if(track == 0)
	       printf("%swarning%s: no track\n", cerror, cnor);
	   
	   // write tag
	   char tag[128];
	   memset(tag, 0, 128);
	   tag[0] = 'T';
	   tag[1] = 'A';
	   tag[2] = 'G';
	   strncpy(tag + 3, title.c_str(), 30);
	   strncpy(tag + 3 + 30, artist.c_str(), 30);
	   strncpy(tag + 3 + 60, album.c_str(), 30);
	   strncpy(tag + 3 + 90 + 4, comment.c_str(), 28);
	   tag[126] = track;
	   tag[127] = -1;
	   num_tagsadded++;
#if 1
	   if(!dummy)
	   {
	       // append to file
	       FILE *f = fopen(name, "ab");
	       if(f == 0)
		   userError("can't open file '%s' for writing!\n", name);
	       if(fwrite(tag, 1, 128, f) != 128)
		   userError("error while writing to file '%s'\n", name);
	       fclose(f);
	   }
#endif
       }
       
      ++checked;
   } // for all params

   // print final statistics
   if((filelist.size()>1)&&(!ac("raw-list")) && (!ac("no-summary")) && (!quiet)) {
      printf("--                                                                             \n"
	     "%s%d%s file%s %s, %s%d%s erroneous file%s found\n", 
	     cval, checked, cnor, checked==1?"":"s", 
	     ac("list")?"listed":"checked", cval, err, cnor, err==1?"":"s");
      if(num_ano) printf("(%s%d%s %sanomal%s%s found)\n", 
			 cval, num_ano, cnor, cano, num_ano==1?"y":"ies", cnor);
       if(num_tagsadded)
	   printf("(%s%d%s tags added)\n", 
		  cval, num_tagsadded, cnor);
   }
   
   // end
   if(log!=NULL) fclose(log);
   
   return (err || num_ano)?1:0;
}

