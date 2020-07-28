#ifndef __JPNEVULATOR_H
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

#define __JPNEVULATOR_H

#include "options.h"

#define PROGRAM_NAME "jpnevulator"
#define PROGRAM_VERSION "2.3.6"

enum jpnevulatorRtrn {
	jpnevulatorRtrnOk=0,
	jpnevulatorRtrnOptions,
	jpnevulatorRtrnNoTTY,
	jpnevulatorRtrnNoInput,
	jpnevulatorRtrnNoOutput,
	jpnevulatorRtrnNoMessage,
	jpnevulatorRtrnNoAscii
};

extern struct jpnevulatorOptions _jpnevulatorOptions;

extern enum jpnevulatorRtrn jpnevulatorWrite(void);
extern enum jpnevulatorRtrn jpnevulatorRead(void);

#endif
