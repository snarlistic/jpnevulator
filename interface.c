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
#include <string.h>

#include "options.h"
#include "jpnevulator.h"
#include "interface.h"
#include "list.h"

void interfaceInitialize(void) {
	listInitialize(&_jpnevulatorOptions.interface);
}

static int cmpr(void *p,void *q) {
	return(!strcasecmp(((struct interface *)p)->name,(char *)q));
}

enum interfaceRtrn interfaceAdd(char *name,int (*interfaceOpen)(char *,int),int (*interfaceControlGet)(int,char *),void (*interfaceControlWrite)(FILE *,int),void (*interfaceClose)(int)) {
	struct interface *interface;
	int charactersPrinted;
	char error[1024];
	char *alias;
	/* Was an interface name given? */
	if(name==NULL) {
		/* Nope. Since the interface name is optional for certain types of interfaces, make sure
		 * we have some sort of empty value to work with. */
		name="";
	} else {
		/* Is this interface already present in our list of interfaces? */
		if(listSearch(&_jpnevulatorOptions.interface,cmpr,(void *)name)!=NULL) {
			return(interfaceRtrnDouble);
		}
	}
	interface=(struct interface *)malloc(sizeof(struct interface));
	if(interface==NULL) {
		return(interfaceRtrnMemory);
	}
	/* Did the user give us an alias for the interface name? */
	if((alias=strstr(name,_jpnevulatorOptions.aliasSeparator))!=NULL) {
		/* Yes, so split the alias from the name. */
		*alias='\0';
		/* Advance the alias to the real beginning of it. */
		alias+=strlen(_jpnevulatorOptions.aliasSeparator);
		/* And put it in our alias reference. */
		charactersPrinted=sprintf(interface->alias,"%.*s",(int)sizeof(interface->alias)-1,alias);
		if(charactersPrinted!=strlen(alias)) {
			fprintf(stderr,"%s: interface alias %s truncated to %d chars -> %s\n",PROGRAM_NAME,alias,charactersPrinted,interface->alias);
		}
	} else {
		/* No, empty the alias. */
		strcpy(interface->alias,"");
	}
	charactersPrinted=sprintf(interface->name,"%.*s",(int)sizeof(interface->name)-1,name);
	if(charactersPrinted!=strlen(name)) {
		fprintf(stderr,"%s: interface %s truncated to %d chars -> %s\n",PROGRAM_NAME,name,charactersPrinted,interface->name);
	}
	interface->fd=interfaceOpen(interface->name,sizeof(interface->name)-1);
	if(interface->fd==-1) {
		snprintf(error,sizeof(error)-1,"%s: Unable to open interface %s",PROGRAM_NAME,interface->name);
		perror(error);
		return(interfaceRtrnOpen);
	}
	/* Initialize the byte count. We have not received/send any bytes yet. */
	interface->byteCount=0UL;
	/* Put the control call-back in place and get the current state of the control bits if needed. */
	interface->controlGet=interfaceControlGet;
	interface->controlWrite=interfaceControlWrite;
	if(boolIsSet(_jpnevulatorOptions.control)) {
		interface->control=interfaceControlGet(interface->fd,interface->name);
	}
	/* Add our interface to the list of interfaces. */
	if(listAppend(&_jpnevulatorOptions.interface,(void *)interface)!=listRtrnOk) {
		fprintf(stderr,"%s: unable to add interface %s to the list of interfaces!\n",PROGRAM_NAME,interface->name);
		return(interfaceRtrnList);
	}
	/* Store a reference to the function to close this interface. */
	interface->close=interfaceClose;
	/* Advance to this next position. so all interfaces will be inline with the order the user gave us. */
	listNext(&_jpnevulatorOptions.interface);
	return(interfaceRtrnOk);
}

int interfaceControlGet(struct interface *interface) {
	return(interface->controlGet(interface->fd,interface->name));
}

void interfaceControlWrite(struct interface *interface,FILE *output,int control) {
	interface->controlWrite(output,control);
}

static void garbageCollect(void *data) {
	struct interface *interface;
	interface=(struct interface *)data;
	interface->close(interface->fd);
	free(interface);
}

void interfaceDestroy(void) {
	listDestroy(&_jpnevulatorOptions.interface,garbageCollect);
}
