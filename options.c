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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "options.h"
#include "jpnevulator.h"
#include "io.h"
#include "crc16.h"
#include "crc8.h"
#include "list.h"
#include "interface.h"
#include "tty.h"
#include "pty.h"
#include "byte.h"

static void usage(void) {
	printf(
		"Usage: %s [--version] [--help] [--checksum] [--crc16=poly]\n"
		"         [--crc8=poly] [--fuck-up] [--file=file] [--no-send]\n"
		"         [--delay-line=microseconds] [--delay-byte=microseconds]\n"
		"         [--print] [--size=size] [--tty=tty] [--pty [=alias]] [--width] [--pass]\n"
		"         [--read] [--write] [--timing-print] [--timing-delta=microseconds]\n"
		"         [--ascii] [--alias-separator=separator] [--byte-count]\n"
		"         [--append] [--append-separator=separator] [--control]\n"
		"         [--control-poll=microseconds] [--count=bytes] [--base] <file>\n",
		PROGRAM_NAME
	);
}

static void optionsDefault(void) {
	/* Don't add a checksum by default. */
	_jpnevulatorOptions.checksum=checksumTypeNone;

	/* Use the crc16 x^16+x^15+x^2+1 polynomial by default. */
	_jpnevulatorOptions.crc16Poly=0xA001;
	crc16TableCreate(0,_jpnevulatorOptions.crc16Poly);

	/* Use the crc8 x^8+x^2+x+1 polynomial by default. */
	_jpnevulatorOptions.crc8Poly=0x07;
	crc8PolyInit(_jpnevulatorOptions.crc8Poly);

	/* Don't fuck up the checksum(wrongly named crc) by default. */
	boolReset(_jpnevulatorOptions.checksumFuckup);

	/* Read/Write standard input/output by default, but use some black magic to find out if a user selects
	 * multiple I/O sources. Which we of course neglect anyway. */
	sprintf(_jpnevulatorOptions.io,"%.*s",(int)sizeof(_jpnevulatorOptions.io)-1,ioMAGIC);

	/* Initialize the list where we store our interfaces. We do not yet add
	 * the default interface. We postpone that up untill we know for sure the user
	 * did not gave us any interface. */
	interfaceInitialize();

	/* By default we expect to send/receive Cham2 messages. */
	_jpnevulatorOptions.size=22;

	/* By default we send our messages on the serial port... */
	boolSet(_jpnevulatorOptions.send);

	/* ...but we probably don't want to see the messages on the console. Or do we? :-) */
	boolReset(_jpnevulatorOptions.print);

	/* Do not delay between messages by default. */
	_jpnevulatorOptions.delayLine=0L;

	/* Do not delay between bytes by default. */
	_jpnevulatorOptions.delayByte=0L;

	/* Action type is mandatory (not by getopts, but by our own mechanism). */
	_jpnevulatorOptions.action=actionTypeNone;

	/* By default we display 16 bytes on one line. */
	_jpnevulatorOptions.width=16;

	/* By default we do not display timing information. */
	boolReset(_jpnevulatorOptions.timingPrint);

	/* By default we couple bytes that arrive within 100 miliseconds. */
	_jpnevulatorOptions.timingDelta=100000UL;

	/* By default we do not display ascii data in read mode. */
	boolReset(_jpnevulatorOptions.ascii);

	/* By default the alias separator is the ':' character. */
	_jpnevulatorOptions.aliasSeparator=":";

	/* Do not show the byte count by default. */
	boolReset(_jpnevulatorOptions.byteCountDisplay);

	/* Do not pass bytes between interfaces by default. */
	boolReset(_jpnevulatorOptions.pass);

	/* Do not poll modem control bits by default. */
	boolReset(_jpnevulatorOptions.control);

	/* Poll every milisecond by default (should suffice for 9600bps). */
	_jpnevulatorOptions.controlPoll=1000UL;

	/* By default we read/write endlessly up untill the end of time. */
	_jpnevulatorOptions.count=-1;

	/* By default, overwrite the existing output file */
	boolReset(_jpnevulatorOptions.append);

	/* By default the append separator is a simple newline. */
	_jpnevulatorOptions.appendSeparator="\n";

	/* By default, read/write hex byte values. */
	_jpnevulatorOptions.base=byteBaseHexadecimal;
}

static void optionsIOWrite(char *file) {
	int charactersPrinted;
	charactersPrinted=sprintf(_jpnevulatorOptions.io,"%.*s",(int)sizeof(_jpnevulatorOptions.io)-1,file);
	if(charactersPrinted!=strlen(file)) {
		fprintf(stderr,"%s: Filename truncated to %d chars\n",PROGRAM_NAME,charactersPrinted);
	}
}

static void optionsBaseWrite(char *base) {
	switch(atoi(base)) {
#define BASE(xbase,name,width) \
		case xbase: { \
			_jpnevulatorOptions.base=name; \
			break; \
		}
		BASES
#undef BASE
		default: {
			fprintf(stderr,"%s: Unsupported base unit selected, cowardly using default base(=%d) unit.\n",PROGRAM_NAME,_jpnevulatorOptions.base);
			break;
		}
	}
}

enum optionsRtrn optionsParse(int argc,char **argv) {
	int finished;
	optionsDefault();
	for(finished=0;!finished;) {
		int option,option_index;
		static struct option long_options[]={
			{"append",no_argument,NULL,'A'},
			{"ascii",no_argument,NULL,'a'},
			{"byte-count",no_argument,NULL,'b'},
			{"base",required_argument,NULL,'B'},
			{"checksum",no_argument,NULL,'c'},
			{"control",no_argument,NULL,'C'},
			{"control-poll",required_argument,NULL,'D'},
			{"delay-line",required_argument,NULL,'d'},
			{"timing-delta",required_argument,NULL,'e'},
			{"file",required_argument,NULL,'f'},
			{"timing-print",no_argument,NULL,'g'},
			{"help",no_argument,NULL,'h'},
			{"width",required_argument,NULL,'i'},
			{"fuck-up",no_argument,NULL,'j'},
			{"delay-byte",required_argument,NULL,'k'},
			{"alias-separator",required_argument,NULL,'l'},
			{"no-send",no_argument,NULL,'n'},
			{"count",required_argument,NULL,'o'},
			{"pass",no_argument,NULL,'P'},
			{"print",no_argument,NULL,'p'},
			{"pty",optional_argument,NULL,'q'},
			{"read",no_argument,NULL,'r'},
			{"size",required_argument,NULL,'s'},
			{"append-separator",required_argument,NULL,'S'},
			{"tty",required_argument,NULL,'t'},
			{"version",no_argument,NULL,'v'},
			{"write",no_argument,NULL,'w'},
			{"crc16",optional_argument,NULL,'y'},
			{"crc8",optional_argument,NULL,'z'},
			{NULL,no_argument,NULL,0}
		};
		option=getopt_long(argc,argv,"aAbB:cCd:D:e:f:ghi:jk:l:no:pPq:rs:S:t:vwy:z:",long_options,&option_index);
		switch(option) {
			case -1: {
				finished=!finished;
				break;
			}
			case 'a': {
				boolSet(_jpnevulatorOptions.ascii);
				break;
			}
			case 'A': {
				boolSet(_jpnevulatorOptions.append);
				break;
			}
			case 'b': {
				boolSet(_jpnevulatorOptions.byteCountDisplay);
				break;
			}
			case 'B': {
				optionsBaseWrite(optarg);
				break;
			}
			case 'c': {
				_jpnevulatorOptions.checksum=checksumTypeChecksum;
				break;
			}
			case 'C': {
				boolSet(_jpnevulatorOptions.control);
				break;
			}
			case 'd': {
				unsigned long delay;
				delay=atol(optarg);
				if(delay>0) {
					_jpnevulatorOptions.delayLine=delay;
				} else {
					fprintf(stderr,"%s: Discarding line delay. It should be bigger than zero.\n",PROGRAM_NAME);
				}
				break;
			}
			case 'D': {
				unsigned long poll;
				poll=atol(optarg);
				if(poll>0) {
					_jpnevulatorOptions.controlPoll=poll;
				} else {
					fprintf(stderr,"%s: Discarding control poll. It should be bigger than zero.\n",PROGRAM_NAME);
				}
				break;
			}
			case 'e': {
				_jpnevulatorOptions.timingDelta=atol(optarg);
				break;
			}
			case 'f': {
				optionsIOWrite(optarg);
				break;
			}
			case 'g': {
				boolSet(_jpnevulatorOptions.timingPrint);
				break;
			}
			case 'h': {
				usage();
				return(optionsRtrnUsage);
			}
			case 'i': {
				int width;
				width=atoi(optarg);
				if(width>0) {
					_jpnevulatorOptions.width=width;
				} else {
					fprintf(stderr,"%s: Discarding width. It should be bigger than zero.\n",PROGRAM_NAME);
				}
				break;
			}
			case 'j': {
				boolSet(_jpnevulatorOptions.checksumFuckup);
				break;
			}
			case 'k': {
				unsigned long delay;
				delay=atol(optarg);
				if(delay>0) {
					_jpnevulatorOptions.delayByte=delay;
				} else {
					fprintf(stderr,"%s: Discarding byte delay. It should be bigger than zero.\n",PROGRAM_NAME);
				}
				break;
			}
			case 'l': {
				_jpnevulatorOptions.aliasSeparator=optarg;
				break;
			}
			case 'n': {
				boolReset(_jpnevulatorOptions.send);
				break;
			}
			case 'o': {
				int count;
				count=atoi(optarg);
				if(count>0) {
					_jpnevulatorOptions.count=count;
				} else {
					fprintf(stderr,"%s: Discarding count. It should be bigger than zero.\n",PROGRAM_NAME);
				}
				break;
			}
			case 'p': {
				boolSet(_jpnevulatorOptions.print);
				break;
			}
			case 'P': {
				boolSet(_jpnevulatorOptions.pass);
				break;
			}
			case 'q': {
				ptyAdd(optarg);
				break;
			}
			case 'r': {
				if(_jpnevulatorOptions.action!=actionTypeNone) {
					fprintf(stderr,"%s: Use --read or --write, but not both. Performing a read this time.\n",PROGRAM_NAME);
				}
				_jpnevulatorOptions.action=actionTypeRead;
				break;
			}
			case 's': {
				int size;
				size=atoi(optarg);
				if(size>0) {
					_jpnevulatorOptions.size=size;
				} else {
					fprintf(stderr,"%s: Discarding size. It should be bigger than zero.\n",PROGRAM_NAME);
				}
				break;
			}
			case 'S': {
				_jpnevulatorOptions.appendSeparator=optarg;
				break;
			}
			case 't': {
				ttyAdd(optarg);
				break;
			}
			case 'v': {
				printf(
					"%s version %s\n"
					"Copyright (C) 2006-2015 Freddy Spierenburg <freddy@snarl.nl>\n"
					"This is free software.  You may redistribute copies of it under the terms of\n"
					"the GNU General Public License <http://www.gnu.org/licenses/gpl.html>.\n"
					"There is NO WARRANTY, to the extent permitted by law.\n",
					PROGRAM_NAME,PROGRAM_VERSION
				);
				return(optionsRtrnVersion);
			}
			case 'w': {
				if(_jpnevulatorOptions.action!=actionTypeNone) {
					fprintf(stderr,"%s: Use --read or --write, but not both. Performing a write this time.\n",PROGRAM_NAME);
				}
				_jpnevulatorOptions.action=actionTypeWrite;
				break;
			}
			case 'y': {
				_jpnevulatorOptions.checksum=checksumTypeCrc16;
				if(optarg) {
					printf("[%s]\n",optarg);
					_jpnevulatorOptions.crc16Poly=strtol(optarg,NULL,16);
					crc16TableCreate(0,_jpnevulatorOptions.crc16Poly);
				}
				break;
			}
			case 'z': {
				_jpnevulatorOptions.checksum=checksumTypeCrc8;
				if(optarg) {
					_jpnevulatorOptions.crc8Poly=strtol(optarg,NULL,16);
					crc8PolyInit(_jpnevulatorOptions.crc8Poly);
				}
				break;
			}
		}
	}

	/* As mentioned more early, we read/write standard input/output by default and warn the user if he
	 * or she tries to use multiple sources. */
	if(optind<argc) {
		if(strcmp(_jpnevulatorOptions.io,ioMAGIC)!=0) {
			fprintf(stderr,"%s: Overriding io file given before...\n",PROGRAM_NAME);
		}
		optionsIOWrite(argv[optind]);
	} else {
		if(strcmp(_jpnevulatorOptions.io,ioMAGIC)==0) {
			optionsIOWrite("-");
		}
	}

	if(_jpnevulatorOptions.action==actionTypeNone) {
		fprintf(stderr,"%s: Use of --read or --write mandatory. Defaulting to write this time.\n",PROGRAM_NAME);
		_jpnevulatorOptions.action=actionTypeWrite;
	}

	/* If the user did not mentioned any interface we will by default
	 * open the /dev/ttyS0 device. */
	if(listElements(&_jpnevulatorOptions.interface)==0) {
		ttyAdd("/dev/ttyS0");
	}

	return(optionsRtrnOk);
}
