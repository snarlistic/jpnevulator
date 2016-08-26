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

static unsigned short crcTable[256];

static unsigned short calcCRC(unsigned short seed,unsigned short poly,unsigned short data) {
	unsigned short index;
	unsigned short crc=seed;
	for(index=8;index;index--) {
		if((data^crc)&0x0001) {
			crc=(crc>>1)^poly;
		} else {
			crc>>=1;
		}
		data>>=1;
	}
	return(crc);
}

void crc16TableCreate(unsigned short seed,unsigned short poly) {
	unsigned short index;
	for(index=0;index<256;index++) {
		crcTable[index]=calcCRC(seed,poly,index);
	}
}

static unsigned short crcAdd(unsigned short crc,unsigned char byte) {
	return(crc>>8)^crcTable[(crc&0xFF)^byte];
}

unsigned short crc16Calculate(unsigned char *data,int length) {
	unsigned short crc;
	int index;
	crc=0;
	for(index=0;index<length;index++) {
		crc=crcAdd(crc,data[index]);
	}
	return(crc);
}
