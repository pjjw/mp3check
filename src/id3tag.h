/*GPL*START*
 *
 * id3tag.h - id3 tag detection and analysis header file
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


/*COMMENTS
 *
 * Reference docs are :
 * [1] Id3 made easy, http://www.id3.org/id3v1.html
 * [2] Xmms sources (xmms-1.2.3/Input/mpg123/mpg123.c)
 *
 * The specification [1] is somehow incomplete, so I'd like to add:
 * 1- Most software don't fill the fields with binary zeroes but with spaces
 *    (Ox20). This doesn't really go against the specification.
 * 2- The specification doesn't say what characters may be used in an id3 tag.
 *    I've considered that the use of non-printable chars (0x01 to 0x1f) is
 *    prohibited.
 * 3- As far as the year is concerned, the specification doesn't say wether
 *    only digits must be used of not. I suggest that the only two accepted
 *    possibilities are: not set (and thus filled with binary zeroes) or
 *    set to a four-digit year. But we also will accept four spaces as a
 *    correct value, since most software fill blanks with spaces. When only
 *    guessing, we treat that field exactly like any other, however.
 * 4- As seen in [2], there are more than 80 genres as seen in [1]. Also,
 *    the value 0xff seems to be used when genre is not set. Thus, we will
 *    accept all these values (0x00 to 0x93, plus 0xff).
 *
 * *COMMENTS*END*/

#ifndef _id3tag_h_
#define _id3tag_h_

class Tagv1
{

public:

	// Default constructor.
	Tagv1();
	
	// Constructor from pointer. Note that it is really only a pointer, the
	// data is not duplicated. This is probably not the safest way, but the
	// fastest.
	Tagv1(const unsigned char *p);
	
	// Destructor, doing nothing.
	~Tagv1();

	// Methods.
	short unsigned int version() const;
	bool isValid() const;
	bool isValidSpecs() const;
	bool isValidGuess() const;
	void setTarget(const unsigned char *p);
	bool store();
	bool restore(unsigned char *p);
	void fillFields();
	
	// Static functions.
	static bool valid_tag_field_strict(const unsigned char *p, const int len);
	static bool valid_tag_field_loose(const unsigned char *p, const int len);
	static int find_next_tag(const unsigned char *p, int len);
	static void copyStringField(char *dest, const unsigned char *src, int len);
	unsigned short int fields_version;
	bool fields_spacefilled;
	char field_title[31];
	char field_artist[31];
	char field_album[31];
	char field_year[5];
	char field_comment[31];
	unsigned char field_genre;
	unsigned char field_track;
	
	static const char * const id3_genres[];
	static const int genres_count;

private:

	static bool spacefilled_tag_field(const char *p, const unsigned int len);

	unsigned char *target;
	bool al; // wether memory is allocated or not
};

#endif
