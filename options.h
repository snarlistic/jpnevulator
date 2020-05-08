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

#ifndef __OPTIONS_H
#define __OPTIONS_H

#include "list.h"
#include "misc.h"
#include "byte.h"

enum checksumType {
	checksumTypeNone=0,
	checksumTypeChecksum,
	checksumTypeCrc16,
	checksumTypeCrc8
};

enum actionType {
	actionTypeNone=0,
	actionTypeRead,
	actionTypeWrite
};

struct jpnevulatorOptions {
	enum checksumType checksum;
	unsigned int crc16Poly;
	unsigned char crc8Poly;
	bool_t checksumFuckup;
	char io[256];
	list_t interface;
	int size;
	bool_t send;
	bool_t print;
	unsigned long delayLine;
	unsigned long delayByte;
	enum actionType action;
	int width;
	bool_t timingPrint;
	unsigned long timingDelta;
	bool_t ascii;
	char *aliasSeparator;
	bool_t byteCountDisplay;
	bool_t pass;
	bool_t control;
	unsigned long controlPoll;
	int count;
	bool_t append;
	char *appendSeparator;
	enum byteBase base;
};

enum optionsRtrn {
	optionsRtrnOk=0,
	optionsRtrnUsage,
	optionsRtrnVersion,
	optionsRtrnImpossible
};

extern enum optionsRtrn optionsParse(int,char **);

#endif
