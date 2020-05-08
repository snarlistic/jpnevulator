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

#include "jpnevulator.h"
#include "options.h"

int main(int argc,char **argv) {
	int returnValue;
	if(optionsParse(argc,argv)!=optionsRtrnOk) {
		returnValue=jpnevulatorRtrnOptions;
	} else {
		switch(_jpnevulatorOptions.action) {
			case actionTypeRead: {
				returnValue=jpnevulatorRead();
				break;
			}
			case actionTypeWrite: {
				returnValue=jpnevulatorWrite();
				break;
			}
			case actionTypeNone:
			default: {
				/* Should be impossible. :-) */
				returnValue=optionsRtrnImpossible;
				break;
			}
		}
	}
	return(returnValue);
}
