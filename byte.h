/* jpnevulator - serial reader/writer
 * Copyright (C) 2006-2016 Freddy Spierenburg
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

#ifndef __BYTE_H
#define __BYTE_H

enum byteRtrn {
	byteRtrnEOF=-128,
	byteRtrnEOL,
	byteRtrnUnknown
};

#define BASES \
	base2 \
	base16

#define base2 BASE(2,byteBaseBinary,8)
#define base16 BASE(16,byteBaseHexadecimal,2)

enum byteBase {
#define BASE(base,name,width) name=base,
	BASES
#undef BASE
};

extern int byteGet(FILE *,enum byteBase);
extern void bytePut(FILE *,enum byteBase,unsigned char);

#endif
