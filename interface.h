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

#ifndef __INTERFACE_H
#define __INTERFACE_H

#define INTERFACE_NAME_LENGTH 512

struct interface {
	char name[INTERFACE_NAME_LENGTH+1];
	char alias[INTERFACE_NAME_LENGTH+1];
	int fd;
	unsigned long byteCount;
	int control;
	void (*close)(int);
	int (*controlGet)(int,char *);
	void (*controlWrite)(FILE *,int);
};

enum interfaceRtrn {
	interfaceRtrnOk=0,
	interfaceRtrnDouble,
	interfaceRtrnMemory,
	interfaceRtrnOpen,
	interfaceRtrnList
};

#define interfacePrint(x) (strlen((x)->alias)>0?(x)->alias:(x)->name)
extern void interfaceInitialize(void);
extern enum interfaceRtrn interfaceAdd(char *,int (*)(char *,int),int (*)(int,char *),void (*)(FILE *,int),void (*)(int));
extern int interfaceControlGet(struct interface *);
extern void interfaceControlWrite(struct interface *,FILE *,int);
extern void interfaceDestroy(void);

#endif
