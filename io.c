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

#include <string.h>

#include "io.h"
#include "options.h"
#include "jpnevulator.h"

#define ioIsStdio() ((strcmp(_jpnevulatorOptions.io,ioMAGIC)==0)||strcmp(_jpnevulatorOptions.io,"-")==0)

FILE *ioOpen(char *mode) {
	if(ioIsStdio()) {
		FILE *ioHandle;
		if(strstr(mode,"r")!=NULL) {
			ioHandle=stdin;
		} else {
			ioHandle=stdout;
		}
		return(ioHandle);
	} else {
		return(fopen(_jpnevulatorOptions.io,mode));
	}
}

void ioClose(FILE *fd) {
	if(!ioIsStdio()) {
		fclose(fd);
	}
}
