/* jpnevulator - serial reader/writer
 * Copyright (C) 2006-2015 Freddy Spierenburg
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <stdio.h>
#include <stdlib.h>
#ifndef __USE_ISOC99
#define __USE_ISOC99 /* for newly introduced isblank() */
#endif
#include <ctype.h>

#include "byte.h"

/* Convert an ASCII character into a binary form. */
static char nibbleGet(char c) {
	return(isdigit(c)?c-'0':toupper(c)-'A'+0xA);
}

enum byteState {
	byteStateComplete=0,
	byteStateNibbleFirst,
	byteStateHexNotation,
	byteStateNibbleSecond,
	byteStateError
};
int byteGet(FILE *fd) {
	int byte;
	enum byteState state;
	/* No real reason to do this, but it makes the compiler happy. If this next line is not
	   here the compiler generates this warning below:

	   byte.c:76:16: warning: ‘byte’ may be used uninitialized in this function [-Wuninitialized]

	   So let's initialize byte anyway.
	*/
	byte=0;

	for(state=byteStateNibbleFirst;state!=byteStateComplete;) {
		static int character;
		switch(state) {
			case byteStateNibbleFirst: {
				character=fgetc(fd);
				if(isxdigit(character)) {
					byte=nibbleGet(character);
					if(character=='0') {
						state=byteStateHexNotation;
					} else {
						state=byteStateNibbleSecond;
					}
				} else if(!isblank(character)) {
					state=byteStateError;
				}
				break;
			}
			case byteStateHexNotation: {
				character=fgetc(fd);
				if(tolower(character)=='x') {
					state=byteStateNibbleFirst;
				} else {
					if(ungetc(character,fd)!=EOF) {
						state=byteStateNibbleSecond;
					} else {
						state=byteStateError;
					}
				}
				break;
			}
			case byteStateNibbleSecond: {
				character=fgetc(fd);
				if(isxdigit(character)) {
					byte=(byte<<4)+nibbleGet(character);
					state=byteStateComplete;
				} else {
					state=byteStateError;
				}
				break;
			}
			case byteStateError: {
				switch(character) {
					case EOF: {
						byte=byteRtrnEOF;
						break;
					}
					case '\n': {
						byte=byteRtrnEOL;
						break;
					}
					default: {
						byte=byteRtrnUnknown;
						break;
					}
				}
				state=byteStateComplete;
				break;
			}
			case byteStateComplete: {
				/* Do nothing, since it's impossible to reach this stage. Just
				 * make the compiler happy. */
				break;
			}
		}
	}
	return(byte);
}
