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
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>

#include "jpnevulator.h"
#include "interface.h"
#include "tty.h"

static int ttyOpen(char *name,int length) {
	return(open(name,O_RDWR));
}

static int ttyControlGet(int fd,char *name) {
	int control;
	control=0;
	if(ioctl(fd,TIOCMGET,&control)) {
		char error[256];
		sprintf(error,"%s: Can't get the modem control bits of interface %s",PROGRAM_NAME,name);
		perror(error);
	}
	return control;
}

static void ttyControlWrite(FILE *output,int control) {
	fprintf(
		output,
		"le=%d, dtr=%d, rts=%d, st=%d, sr=%d, cts=%d, cd=%d, ri=%d, dsr=%d\n",
		control&TIOCM_LE?1:0,
		control&TIOCM_DTR?1:0,
		control&TIOCM_RTS?1:0,
		control&TIOCM_ST?1:0,
		control&TIOCM_SR?1:0,
		control&TIOCM_CTS?1:0,
		control&TIOCM_CD?1:0,
		control&TIOCM_RI?1:0,
		control&TIOCM_DSR?1:0
	);
	fflush(output);
}

static void ttyClose(int fd) {
	close(fd);
}

enum interfaceRtrn ttyAdd(char *name) {
	return(interfaceAdd(name,ttyOpen,ttyControlGet,ttyControlWrite,ttyClose));
}
