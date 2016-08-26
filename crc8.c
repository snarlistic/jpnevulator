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

static int _poly=0x07;

void crc8PolyInit(unsigned char poly) {
	_poly=poly;
}

unsigned char crc8Calculate(unsigned char *mssg,int size) {
	unsigned char crc=0;
	unsigned char crcReversed=0;
	int index;
	for(index=0;index<size*8;index++) {
		if((crc>>7)^((mssg[index>>3]>>(index&7))&1)) {
			crc=(crc<<1)^_poly;
		} else {
			crc<<=1;
		}
	}
	for(index=0;index<8;index++) {
		crcReversed=(crcReversed<<1)|((crc>>index)&1);
	}
	return(crcReversed);
}
