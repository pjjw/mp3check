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

#include <sys/types.h>
#include <dirent.h>
#include "tfiletools.h"

#define COUNT_VERBOSE_STEP 1000

// TFile implementation
// 
TFile::FileType TFile::filetype() const {
    if(isregular()) return FT_REGULAR;
    else if(isdir()) return FT_DIR;
    else if(issymlink()) return FT_SYMLINK;
    else if(ischardev()) return FT_CHARDEV;
    else if(isblockdev()) return FT_BLOCKDEV;
    else if(isfifo()) return FT_FIFO;
    else if(issocket()) return FT_SOCKET;
    else return FT_UNKNOWN;
}

tstring TFile::filetypeLongStr() const {
    switch(filetype()) {
    case FT_SOCKET:   return "socket";      
    case FT_SYMLINK:  return "symbolic link";	
    case FT_REGULAR:  return "regular file";	
    case FT_BLOCKDEV: return "block device";	
    case FT_DIR:      return "directory";      
    case FT_CHARDEV:  return "character device";	
    case FT_FIFO:     return "fifo";
    default: { tstring t; t.sprintf("%#o", int(filetypebits())); return t; }
    }
}

char TFile::filetypeChar() const {
    switch(filetype()) {
    case FT_SOCKET:   return 's';      
    case FT_SYMLINK:  return 'l';	
    case FT_REGULAR:  return '-';	
    case FT_BLOCKDEV: return 'b';	
    case FT_DIR:      return 'd';      
    case FT_CHARDEV:  return 'c';	
    case FT_FIFO:     return 'p';
    default:          return '?';
    }
}

tstring TFile::filetypeStr7() const {
    switch(filetype()) {
    case FT_SOCKET:   return "socket ";
    case FT_SYMLINK:  return "symlink";	
    case FT_REGULAR:  return "regular";	
    case FT_BLOCKDEV: return "blockdv";	
    case FT_DIR:      return "dir    ";      
    case FT_CHARDEV:  return "chardev";	
    case FT_FIFO:     return "fifo   ";
    default:          return "unknown";
    }
}

void TFile::getstat() const {
    if(stated) return;
    if(name_.empty()) throw TNotInitializedException("TFile");
    TFile *t = const_cast<TFile*>(this);
    if(follow_links) {     
	if(stat(name_.c_str(), &(t->statbuf))) throw TFileOperationErrnoException(name_.c_str(), "stat");
    } else {
	if(lstat(name_.c_str(), &(t->statbuf))) throw TFileOperationErrnoException(name_.c_str(), "lstat");
    }
    t->stated = true;
    num_stated++;
}



// TDir implementation
// 
size_t TDir::numRecursive(bool low_mem, const char *verbose, bool count_files, bool count_dirs) const {
    size_t r = 0;
    if(count_files) r = numFiles();
    if(count_dirs) r += numDirs();
    if(verbose) { 
	verbose_num += r; 
	if(verbose_num > (old_verbose_num + COUNT_VERBOSE_STEP)) {
	    printf("counting %s in %s: %11u\r", count_files ? (count_dirs ? "files and dirs" : "files") : (count_dirs ? "dirs" : "nothing"), verbose, unsigned(verbose_num));
	    fflush(stdout); 
	    old_verbose_num = verbose_num;
	}
    } 
    for(size_t i = 0; i < numDirs(); i++) 
	r += dir(i).numRecursive(low_mem, verbose, count_files, count_dirs);
    if(low_mem) freeMem();
    return r;
}

void TDir::scan() const {
    if(scanned) return;
    assert(subTreeContext);
    if(name().empty()) throw TNotInitializedException("TDir");
    TDir *t = const_cast<TDir*>(this);
    // maximum depth already reached? if so treat directory as empty
    if(depth >= subTreeContext->max_depth)
    {
	t->scanned = true;
	return;
    }
    // prevent crossing filesystems?
    if(!subTreeContext->cross_filesystems) {
	if(subTreeContext->root->device() != device()) {
	    // consider directory empty because it is on a different filesystem than the subtree root
	    t->scanned = true;
	    return;
	}
    }
    // scan directory
    struct dirent *dire = 0;
    DIR *tdir = opendir(name().c_str());
    if(tdir == 0) throw TFileOperationErrnoException(name().c_str(), "opendir");
    size_t dirs_left = hardlinks() - 2;
    while((dire = readdir(tdir)) != 0) {
	if((dire->d_name[0] != '.') || (strcmp(".", dire->d_name) && strcmp("..", dire->d_name))) {
	    TFile f((name() + "/") + dire->d_name);
	    if((dirs_left || no_leaf_optimize) && f.isdir()) {
		t->name2dirid[dire->d_name] = dirs.size();
		t->dirs[dirs.size()] = TDir(f, *subTreeContext, depth + 1);
		dirs_left--;
	    } else {
		t->name2fileid[dire->d_name] = files.size();
		t->files[files.size()] = f;
	    }
	}
    }   
    closedir(tdir);
    t->scanned = true;
    // update access time
    invalidateStat();
}

size_t TDir::verbose_num = 0;
size_t TDir::old_verbose_num = 0;
bool TFile::follow_links = false;
size_t TFile::num_stated = 0;
#ifdef WIN32
bool TDir::no_leaf_optimize = true;
#else
bool TDir::no_leaf_optimize = false;
#endif


// global functions
tvector<tstring> findFilesRecursive(const TDir& dir) {
    // return value
    tvector<tstring> r;
    
    // add files
    for(size_t i = 0; i < dir.numFiles(); i++) 
	r.push_back(dir.file(i).name());
    
    // add dirs recursively
    for(size_t j = 0; j < dir.numDirs(); j++)
	r += findFilesRecursive(dir.dir(j));
    
    // return list
    return r;
}


tvector<tstring> filterExtensions(const tvector<tstring>& list, const tvector<tstring>& extensions, bool remove) {
    // create lookup map
    tmap<tstring,int> ext;
    for(size_t i = 0; i < extensions.size(); i++)
	ext[extensions[i]] = 1;
    
    // filter list
    tvector<tstring> r;
    for(size_t j = 0; j < list.size(); j++) {
	tstring e = list[j];
	e.extractFilenameExtension();
	if(ext.contains(e) != remove)
	    r.push_back(list[j]);
    }
    
    // return list
    return r;
}


void makeDirectoriesIncludingParentsIfNecessary(const tstring& dirname, bool verbose, bool dummy) {
    // check whether dirname already exists
    try {
	TFile f(dirname);
	// if it exists and is a directory we are done
	if(f.isdir()) return;
    }
    // else we need to create the parent directory and then dirname
    catch(const TFileOperationErrnoException& e) {
	if(e.err == ENOENT) {
	    // create parent dir
	    tstring parent = dirname;
	    parent.removeDirSlash();
	    parent.extractPath();
	    if(!parent.empty())
		makeDirectoriesIncludingParentsIfNecessary(parent, verbose, dummy);  // recurse
	    
	    // create dirname
	    if(verbose)
		printf("creating directory '%s'\n", dirname.c_str());
	    if(!dummy)
		if(mkdir(dirname.c_str(), S_IRWXU) == -1)
		    throw TFileOperationErrnoException(dirname.c_str(), "mkdir");
	    return;
	} else throw;
    }
    
    // if we get here dirname exists and is not a directory: error
    throw TFileOperationErrnoException(dirname.c_str(), "makeDirectoriesIncludingParentsIfNecessary", ENOTDIR);
}

