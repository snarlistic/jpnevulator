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

#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#include "jpnevulator.h"
#include "interface.h"
#include "pty.h"

static int ptyOpen(char *name,int length) {
	int fd;
	fd=posix_openpt(O_RDWR);
	if(fd!=-1) {
		char *pts;
		int charactersPrinted;
		grantpt(fd);
		unlockpt(fd);
		pts=ptsname(fd);
		fprintf(stderr,"%s: slave pts device is %s.\n",PROGRAM_NAME,pts);
		charactersPrinted=sprintf(name,"%.*s",length,pts);
		if(charactersPrinted!=strlen(name)) {
			fprintf(stderr,"%s: interface %s truncated to %d chars -> %s\n",PROGRAM_NAME,pts,charactersPrinted,name);
		}
	}
	return(fd);
}

static int ptyControlGet(int fd,char *name) {
	/* A pty does not have modem control bits, so we simply return 0. This way the interface plays nicely
	   according to general rules and never produces any output. */
	return(0);
}

static void ptyControlWrite(FILE *output,int control) {
	/* We don't do anything and cowardly refuse to write anything. */
}

static void ptyClose(int fd) {
	close(fd);
}

enum interfaceRtrn ptyAdd(char *name) {
	return(interfaceAdd(name,ptyOpen,ptyControlGet,ptyControlWrite,ptyClose));
}
