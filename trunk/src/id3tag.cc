/*GPL*START*
 *
 * id3tag.cc - id3 tag detection and analysis
 * 
 * Copyright (C) 2000 by Jean Delvare <delvare@ensicaen.ismra.fr>
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
 *
 * *GPL*END*/  

#include "string.h"
#include "id3tag.h"

/************************
** Auxiliary functions **
************************/

// Returns wether or not a field complies with the specification.
bool Tagv1::valid_tag_field_strict(const unsigned char *p, const int len)
{
	int i;
	bool filling;
	for(i=0, filling=false; i<len; i++)
	{
		if(p[i]==0)
			filling=true; // found a zero byte
		else if(p[i]<32)
			return(false); // found a non-printable char
		else if(filling==true)
			return(false); // found a non-zero byte after a zero byte
	}
	return(true);
}

// Returns wether or not the pointed zone could be a tag field.
bool Tagv1::valid_tag_field_loose(const unsigned char *p, const int len)
{
	for(int i=0; i<len; i++)
	{
		if(p[i]==0)
			return(true); // found a zero byte
		else if(p[i]<32)
			return(false); // found a non-printable char
	}
	return(true);
}

// Detects a space (instead of zero) filled field.
// To be used on prepared fields, *not* on original zone.
bool Tagv1::spacefilled_tag_field(const char *p, const unsigned int len)
{
	return((strlen(p)==len)&&(p[len-2]==' ')&&(p[len-1]==' '));
}

// Returns next position of an id3 v1 tag, or -1 if not found.
int Tagv1::find_next_tag(const unsigned char *p, int len)
{
	int i;
	Tagv1* tag;

	tag=new Tagv1();
	for(i=0; i < len-127; i++)
	{
		tag->setTarget(p+i);
		if(tag->isValidGuess())
		{
			delete tag;
			return(i);
		}
	}
	delete tag;

	return(-1); // not found
}

/****************
** Tagv1 class **
****************/

// Constructors and destructor
Tagv1::Tagv1() { target=NULL; al=false; }
Tagv1::Tagv1(const unsigned char *p) { target=(unsigned char *)p; al=false; }
Tagv1::~Tagv1() { if(al&&target) delete [] target; }

// Version of the tag, 1.0 or 1.1 so far. The value is return as a 16-bits
// unsigned integer. The method for detecting the v1.1 tags was found in
// [1].
short unsigned int Tagv1::version() const
{
	if(target[125]==0&&target[126]!=0)
		return(0x0101);
	else
		return(0x0100);
}

// First level of validity. We only check if the tag begins with 'TAG'.
// It is enough if the tag is at the very end of the file.
bool Tagv1::isValid() const
{
	return(!(bool)memcmp(target,"TAG",3));
}

// Second level of validity. We want to know if what we have has chances to be
// a tag. This verification is looser than isValidSpecs(), since we stop
// reading the fields after the first binary zero.
bool Tagv1::isValidGuess() const
{
	// The tag must begin with 'TAG'.
	if(memcmp(target,"TAG",3))
		return(false);
	// Next 30 bytes: title, filled with binary zeroes.
	if(!Tagv1::valid_tag_field_loose(target+3,30)) return(false);
	// Next 30 bytes: artist, filled with binary zeroes.
	if(!Tagv1::valid_tag_field_loose(target+33,30)) return(false);
	// Next 30 bytes: album, filled with binary zeroes.
	if(!Tagv1::valid_tag_field_loose(target+63,30)) return(false);
	// Next 4 bytes: year.
	if(!Tagv1::valid_tag_field_loose(target+93,4)) return(false);
    // Next 30 chars: comment, filled with binary zeroes.
	// In fact, the 30th char is the track number for v1.1, we can't assume
	// anything on it.
	if(!Tagv1::valid_tag_field_loose(target+97,29)) return(false);
	// Last one char: byte-coded genre.
	// We accept all values here, in case some new genres have been added.
	// All tests successful.
	return(true);
}

// Third level of validity. This time, we want to know if the tag complies
// with the specifications as found in [1] (see comments). Note that we
// treat the year field as a normal string.
bool Tagv1::isValidSpecs() const
{
	// The tag must begin with 'TAG'. This has probably already been checked
	// by either isValid() or isValidGuess(), but maybe not.
	if(memcmp(target,"TAG",3))
		return(false);
	// Next 30 bytes: title, filled with binary zeroes.
	if(!Tagv1::valid_tag_field_strict(target+3,30)) return(false);
	// Next 30 bytes: artist, filled with binary zeroes.
	if(!Tagv1::valid_tag_field_strict(target+33,30)) return(false);
	// Next 30 bytes: album, filled with binary zeroes.
	if(!Tagv1::valid_tag_field_strict(target+63,30)) return(false);
	// Next 4 bytes: year.
	if(!Tagv1::valid_tag_field_strict(target+93,4)) return(false);
    // Next 30 chars: comment, filled with binary zeroes.
	// In fact, the 30th char is the track number for v1.1, we can't assume
	// anything on it.
	if(!Tagv1::valid_tag_field_strict(target+97,29)) return(false);
	// Last one char: byte-coded genre.
	if((target[127]>=Tagv1::genres_count)&&(target[127]!=0xff)) return(false);
	// All tests successful.
	return(true);
}

void Tagv1::setTarget(const unsigned char *p)
{
	target=(unsigned char *)p;
}

bool Tagv1::store()
{
	if(!al)
	{
		unsigned char *p=new unsigned char[128];
		memcpy(p,target,128);
		target=p;
		al=true;
		return(true);
	}
	else
		return(false);
}

bool Tagv1::restore(unsigned char *p)
{
	if(al)
	{
		memcpy(p,target,128);
		delete [] target;
		target=p;
		al=false;
		return(true);
	}
	else
		return(false);
}

void Tagv1::copyStringField(char *dest, const unsigned char *src, int len)
{
	memcpy(dest,src,len);
	dest[len]='\0';
	for(int i=0;i<len;i++)
	{
		if((dest[i]>0) && (dest[i]<32))
			dest[i]='!';
		else if(dest[i] & 128)
			dest[i]='?';
	}
}

void Tagv1::fillFields()
{
	fields_version=version();
	fields_spacefilled=false;
	Tagv1::copyStringField(field_title,target+3,30);
	Tagv1::copyStringField(field_artist,target+33,30);
	Tagv1::copyStringField(field_album,target+63,30);
	Tagv1::copyStringField(field_year,target+93,4);
	fields_spacefilled|=Tagv1::spacefilled_tag_field(field_title,30);
	fields_spacefilled|=Tagv1::spacefilled_tag_field(field_artist,30);
	fields_spacefilled|=Tagv1::spacefilled_tag_field(field_album,30);
	fields_spacefilled|=Tagv1::spacefilled_tag_field(field_year,4);
	field_genre=target[127];
	if((fields_version&0xff)==1)
	{
		field_track=target[126];
		Tagv1::copyStringField(field_comment,target+97,28);
		fields_spacefilled|=Tagv1::spacefilled_tag_field(field_comment,28);
	}
	else
	{
		field_track=0;
		Tagv1::copyStringField(field_comment,target+97,30);
		fields_spacefilled|=Tagv1::spacefilled_tag_field(field_comment,30);
	}
}

// the following table comes from xmms/mpeg123
const char * const Tagv1::id3_genres[] =
{
	"Blues", "Classic Rock", "Country", "Dance",
	"Disco", "Funk", "Grunge", "Hip-Hop",
	"Jazz", "Metal", "New Age", "Oldies",
	"Other", "Pop", "R&B", "Rap", "Reggae",
	"Rock", "Techno", "Industrial", "Alternative",
	"Ska", "Death Metal", "Pranks", "Soundtrack",
	"Euro-Techno", "Ambient", "Trip-Hop", "Vocal",
	"Jazz+Funk", "Fusion", "Trance", "Classical",
	"Instrumental", "Acid", "House", "Game",
	"Sound Clip", "Gospel", "Noise", "Alt",
	"Bass", "Soul", "Punk", "Space",
	"Meditative", "Instrumental Pop",
	"Instrumental Rock", "Ethnic", "Gothic",
	"Darkwave", "Techno-Industrial", "Electronic",
	"Pop-Folk", "Eurodance", "Dream",
	"Southern Rock", "Comedy", "Cult",
	"Gangsta Rap", "Top 40", "Christian Rap",
	"Pop/Funk", "Jungle", "Native American",
	"Cabaret", "New Wave", "Psychedelic", "Rave",
	"Showtunes", "Trailer", "Lo-Fi", "Tribal",
	"Acid Punk", "Acid Jazz", "Polka", "Retro",
	"Musical", "Rock & Roll", "Hard Rock", "Folk",
	"Folk/Rock", "National Folk", "Swing",
	"Fast-Fusion", "Bebob", "Latin", "Revival",
	"Celtic", "Bluegrass", "Avantgarde",
	"Gothic Rock", "Progressive Rock",
	"Psychedelic Rock", "Symphonic Rock", "Slow Rock",
	"Big Band", "Chorus", "Easy Listening",
	"Acoustic", "Humour", "Speech", "Chanson",
	"Opera", "Chamber Music", "Sonata", "Symphony",
	"Booty Bass", "Primus", "Porn Groove",
	"Satire", "Slow Jam", "Club", "Tango",
	"Samba", "Folklore", "Ballad", "Power Ballad",
	"Rhythmic Soul", "Freestyle", "Duet",
	"Punk Rock", "Drum Solo", "A Cappella",
	"Euro-House", "Dance Hall", "Goa",
	"Drum & Bass", "Club-House", "Hardcore",
	"Terror", "Indie", "BritPop", "Negerpunk",
	"Polsk Punk", "Beat", "Christian Gangsta Rap",
	"Heavy Metal", "Black Metal", "Crossover",
	"Contemporary Christian", "Christian Rock",
	"Merengue", "Salsa", "Thrash Metal",
	"Anime", "JPop", "Synthpop"
};

const int Tagv1::genres_count = sizeof(id3_genres) / sizeof(*id3_genres);
