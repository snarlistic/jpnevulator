/* jpnevulator - serial reader/writer
 * Copyright (C) 2006-2020 Freddy Spierenburg
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

#define isBinaryDigit(digit) ((digit=='0')||(digit=='1'))
#define bitGet(digit) (digit-'0')

enum byteStateBinary {
	byteStateBinaryComplete=0,
	byteStateBinaryBitsIncomplete,
	byteStateBinaryError
};
int byteBaseBinaryGet(FILE *fd) {
	int byte;
	int bitsRead;
	enum byteStateBinary state;

	/* No real reason to do this, but it makes the compiler happy. If this next line is not
	 * here the compiler generates this warning below:
	 *
	 *   warning: ‘byte’ may be used uninitialized in this function [-Wuninitialized]
	 *
	 * So let's initialize byte anyway. */
	byte=0;

	for(bitsRead=0,state=byteStateBinaryBitsIncomplete;state!=byteStateBinaryComplete;) {
		static int character;
		switch(state) {
			case byteStateBinaryBitsIncomplete: {
				if(bitsRead<8) {
					character=fgetc(fd);
					if(isBinaryDigit(character)) {
						byte=(byte<<1)|bitGet(character);
						bitsRead++;
					} else if(isblank(character)) {
						if(bitsRead) {
							state=byteStateBinaryComplete;
						}
					} else {
						if(bitsRead) {
							if(ungetc(character,fd)!=EOF) {
								state=byteStateBinaryComplete;
							} else {
								state=byteStateBinaryError;
							}
						} else {
							state=byteStateBinaryError;
						}
					}
				} else {
					state=byteStateBinaryComplete;
				}
				break;
			}
			case byteStateBinaryError: {
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
				state=byteStateBinaryComplete;
				break;
			}
			case byteStateBinaryComplete: {
				/* Do nothing, since it's impossible to reach this stage. Just
				 * make the compiler happy. */
				break;
			}
		}
	}
	return(byte);
}

/* Convert an ASCII character into a binary form. */
static char nibbleGet(char c) {
	return(isdigit(c)?c-'0':toupper(c)-'A'+0xA);
}

enum byteStateHexadecimal {
	byteStateHexadecimalComplete=0,
	byteStateHexadecimalNibbleFirst,
	byteStateHexadecimalHexNotation,
	byteStateHexadecimalNibbleSecond,
	byteStateHexadecimalError
};
int byteBaseHexadecimalGet(FILE *fd) {
	int byte;
	enum byteStateHexadecimal state;
	/* No real reason to do this, but it makes the compiler happy. If this next line is not
	 * here the compiler generates this warning below:
	 *
	 *   warning: ‘byte’ may be used uninitialized in this function [-Wuninitialized]
	 *
	 * So let's initialize byte anyway. */
	byte=0;

	for(state=byteStateHexadecimalNibbleFirst;state!=byteStateHexadecimalComplete;) {
		static int character;
		switch(state) {
			case byteStateHexadecimalNibbleFirst: {
				character=fgetc(fd);
				if(isxdigit(character)) {
					byte=nibbleGet(character);
					if(character=='0') {
						state=byteStateHexadecimalHexNotation;
					} else {
						state=byteStateHexadecimalNibbleSecond;
					}
				} else if(!isblank(character)) {
					state=byteStateHexadecimalError;
				}
				break;
			}
			case byteStateHexadecimalHexNotation: {
				character=fgetc(fd);
				if(tolower(character)=='x') {
					state=byteStateHexadecimalNibbleFirst;
				} else {
					if(ungetc(character,fd)!=EOF) {
						state=byteStateHexadecimalNibbleSecond;
					} else {
						state=byteStateHexadecimalError;
					}
				}
				break;
			}
			case byteStateHexadecimalNibbleSecond: {
				character=fgetc(fd);
				if(isxdigit(character)) {
					byte=(byte<<4)+nibbleGet(character);
					state=byteStateHexadecimalComplete;
				} else {
					state=byteStateHexadecimalError;
				}
				break;
			}
			case byteStateHexadecimalError: {
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
				state=byteStateHexadecimalComplete;
				break;
			}
			case byteStateHexadecimalComplete: {
				/* Do nothing, since it's impossible to reach this stage. Just
				 * make the compiler happy. */
				break;
			}
		}
	}
	return(byte);
}

int byteGet(FILE *fd,enum byteBase base) {
	int byte;
	/* No real reason to do this, but it makes the compiler happy. If this next line is not
	 * here the compiler generates this warning below:
	 *
	 *   warning: ‘byte’ may be used uninitialized in this function [-Wuninitialized]
	 *
	 * So let's initialize byte anyway. */
	byte=0;

	switch(base) {
#define BASE(base,name,width) \
		case name: { \
			byte=name##Get(fd); \
			break; \
		}
		BASES
#undef BASE
	}
	return(byte);
}

static void byteBaseBinaryPut(FILE *fd,unsigned char byte) {
	char bits[(sizeof(byte)*8)+1];
	char *p;
	for(*(p=bits+sizeof(bits)-1)='\0';p--!=bits;byte>>=1) {
		*p='0'+(byte&0x01);
	}
	fprintf(fd,"%s",bits);
}

static void byteBaseHexadecimalPut(FILE *fd, unsigned char byte) {
	fprintf(fd,"%02X",byte);
}

void bytePut(FILE *fd,enum byteBase base,unsigned char byte) {
	switch(base) {
#define BASE(base,name,width) \
		case name: { \
			name##Put(fd,byte); \
			break; \
		}
		BASES
#undef BASE
	}
}
