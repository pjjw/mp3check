/*GPL*START*
 *
 * tfilesync.h - file and directory handling tolls header file
 * 
 * Copyright (C) 2000 by Johannes Overmann <Johannes.Overmann@gmx.de>
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

#ifndef _ngw_tfiletools_h_
# define _ngw_tfiletools_h_

# include <sys/stat.h>
# include <sys/types.h>
# include <unistd.h>
# include <assert.h>
# include "tstring.h"
# include "texception.h"
# include "tmap.h"
# include "tvector.h"

// history:
// 2001:
// 25 Jun 2001: created (Dir and File taken from filesync.cc)
// 
// 2007 24 Oct: removed __STRICT_ANSI__ support, fixed dev_t and made it more robust, fixed operator < for TFileInstance


// own device type which is a simple 64 bit unsigned integer
typedef unsigned long long mydev_t;
inline mydev_t dev_t2mydev_t(const dev_t& dev) {
# ifdef __APPLE__
    assert(sizeof(dev_t) == 4);
    return *(reinterpret_cast<const unsigned int*>(&dev));
# else
    assert(sizeof(dev_t) == 8);
    return *(reinterpret_cast<const mydev_t*>(&dev));
# endif
}
inline dev_t mydev_t2dev_t(mydev_t dev) {
# ifdef __APPLE__
    assert(sizeof(dev_t) == 4);
    unsigned int tmp = dev;
    return *(reinterpret_cast<const dev_t*>(&tmp));
# else
    assert(sizeof(dev_t) == 8);
    return *(reinterpret_cast<const dev_t*>(&dev));
# endif
}


// file instance information (needed for hardlink structure)
struct TFileInstance {
    TFileInstance(mydev_t d = 0, ino_t i = 0): device(d), inode(i) {}
    mydev_t device;
    ino_t inode;
};
inline bool operator < (const TFileInstance& d1, const TFileInstance& d2) {
    if(d1.device < d2.device) return true;
    if(d1.device > d2.device) return false;
    return d1.inode < d2.inode;
}
inline bool operator == (const TFileInstance& d1, const TFileInstance& d2) {
    return (d1.inode == d2.inode) && (d1.device == d2.device);
}
inline bool operator != (const TFileInstance& d1, const TFileInstance& d2) {
    return (d1.inode != d2.inode) || (d1.device != d2.device);
}

class TFile {
public:
    // cons & des
    TFile(const tstring& fname = tstring()): name_(fname), stated(false) {}
    ~TFile() {}
    void invalidateStat() const { const_cast<TFile*>(this)->stated = false; }
    
    // read only interface
   
    // file types 
    enum FileType { FT_UNKNOWN, FT_REGULAR, FT_DIR, FT_SYMLINK, FT_CHARDEV, FT_BLOCKDEV, FT_FIFO, FT_SOCKET };
    
    // name
    const tstring& name() const {if(name_.empty()) throw TNotInitializedException("TFile"); return name_;}
    tstring filename() const {tstring r= name_; r.extractFilename(); return r;}
    tstring pathname() const {tstring r= name_; r.extractPath(); return r;}
    
    // stat fields
    mydev_t device() const {getstat(); return dev_t2mydev_t(statbuf.st_dev);}
    ino_t inode() const {getstat(); return statbuf.st_ino;}
    nlink_t hardlinks() const {getstat(); return statbuf.st_nlink;}
    uid_t userid() const {getstat(); return statbuf.st_uid;}
    gid_t groupid() const {getstat(); return statbuf.st_gid;}
    mydev_t devicetype() const {getstat(); return dev_t2mydev_t(statbuf.st_rdev);}
    off_t size() const {getstat(); return statbuf.st_size;}
    time_t atime() const {getstat(); return statbuf.st_atime;}
    time_t mtime() const {getstat(); return statbuf.st_mtime;}
    time_t ctime() const {getstat(); return statbuf.st_ctime;}
    // mode fields
    mode_t protection() const { getstat(); return statbuf.st_mode & (S_ISUID|S_ISGID|S_ISVTX|S_IRWXU|S_IRWXG|S_IRWXO); }
    bool isdir() const { getstat(); return S_ISDIR(statbuf.st_mode); }
    bool isregular() const { getstat(); return S_ISREG(statbuf.st_mode); }
    bool issymlink() const { getstat(); return S_ISLNK(statbuf.st_mode); }
    bool ischardev() const { getstat(); return S_ISCHR(statbuf.st_mode); }
    bool isblockdev() const { getstat(); return S_ISBLK(statbuf.st_mode); }
    bool isfifo() const { getstat(); return S_ISFIFO(statbuf.st_mode); }
    bool issocket() const { getstat(); return S_ISSOCK(statbuf.st_mode); }
    bool devicetypeApplies() const { return ischardev() || isblockdev(); }
    mode_t filetypebits() const { return statbuf.st_mode & S_IFMT; }
    TFileInstance instance() const { return TFileInstance(device(), inode()); }
    FileType filetype() const;
    tstring filetypeLongStr() const;
    char filetypeChar() const;
    tstring filetypeStr7() const;
    static void followLinks(bool follow = true) { follow_links = follow; }
    static size_t numStated() { return num_stated; }

private:
    // private helpers
    void getstat() const;
    
    // private data
    tstring name_;
    struct stat statbuf;
    bool stated;
    
    // private static data
    static bool follow_links;
    static size_t num_stated;
    
    // forbid implicit comparison
    bool operator==(const TFile&);
};


class TDir;

class TSubTreeContext {
public:
    TSubTreeContext(bool cross_filesystems, size_t max_depth = 0xffffffff): root(0), cross_filesystems(cross_filesystems), max_depth(max_depth) {}
    TDir *root;
    bool cross_filesystems;
    size_t max_depth;
};


class TDir: public TFile {
public:
    TDir(): TFile(), scanned(false), subTreeContext(0), depth(0) {}
    TDir(const tstring& fname, TSubTreeContext& subTreeContext_, size_t depth = 0): TFile(fname), scanned(false), subTreeContext(&subTreeContext_), depth(depth) { init(); }
    TDir(const TFile& file_, TSubTreeContext& subTreeContext_, size_t depth = 0): TFile(file_), scanned(false), subTreeContext(&subTreeContext_), depth(depth) { init(); }
    ~TDir() {}
    
    // read only interface
    const TFile& file(const tstring& fname) const { scan(); if(name2fileid.contains(fname)) return files[name2fileid[fname]]; else throw TNotFoundException(); }
    const TDir & dir (const tstring& fname) const { scan(); if(name2dirid .contains(fname)) return dirs [name2dirid [fname]]; else throw TNotFoundException(); }
    const TFile& file(size_t index) const { scan(); if(index >= files.size()) throw TZeroBasedIndexOutOfRangeException(index, files.size()); return files[index];}
    const TDir & dir (size_t index) const { scan(); if(index >= dirs .size()) throw TZeroBasedIndexOutOfRangeException(index, dirs .size()); return dirs [index];}
    size_t numFiles() const { scan(); return files.size();}
    size_t numDirs () const { scan(); return dirs .size();}
    bool containsFile(const tstring& fname) const { scan(); return name2fileid.contains(fname); }
    bool containsDir (const tstring& fname) const { scan(); return name2dirid .contains(fname); }
    bool contains    (const tstring& fname) const { return containsDir(fname) || containsFile(fname); }
    bool isEmpty() const { return dirs.empty() && files.empty(); }
    void invalidateContents() { freeMem(); }
    size_t numRecursive(bool low_mem = true, const char *verbose = 0, bool count_files = true, bool count_dirs = true) const;
    void freeMem() const { if(scanned) { TDir *t = const_cast<TDir*>(this); t->dirs.clear(); t->files.clear(); t->name2fileid.clear(); t->name2dirid.clear(); t->scanned = false; } }
    static void resetVerboseNum() { old_verbose_num = verbose_num = 0; };
    static void noLeafOptimize(bool no_opt) {
	no_leaf_optimize = no_opt;
# ifdef WIN32
	no_leaf_optimize = true;
# endif      
    }
    
private:
    // private helpers
    void scan() const;
    void init() { assert(subTreeContext); if(subTreeContext->root == 0) subTreeContext->root = this; }
    
    // private data
    bool scanned;
    tmap<tstring,int> name2fileid;
    tmap<int,TFile> files;
    tmap<tstring,int> name2dirid;
    tmap<int,TDir> dirs;
    TSubTreeContext *subTreeContext;
    size_t depth;
    
    // private static data
    static size_t verbose_num;
    static size_t old_verbose_num;
    static bool no_leaf_optimize;
    
    // forbid implicit comparison
    bool operator==(const TDir&);
};

// global functions
tvector<tstring> findFilesRecursive(const TDir& dir);
tvector<tstring> filterExtensions(const tvector<tstring>& list, const tvector<tstring>& extensions, bool remove = false);
void makeDirectoriesIncludingParentsIfNecessary(const tstring& dirname, bool verbose = false, bool dummy = false);


#endif /* _ngw_tfiletools_h_ */

