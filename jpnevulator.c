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
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <ctype.h>

#include "jpnevulator.h"
#include "byte.h"
#include "io.h"
#include "interface.h"
#include "checksum.h"
#include "crc16.h"
#include "crc8.h"
#include "misc.h"

struct jpnevulatorOptions _jpnevulatorOptions;

static void messageChecksumAdd(unsigned char *message,int *size) {
	unsigned short checksum;
	switch(_jpnevulatorOptions.checksum) {
		case checksumTypeCrc8: {
			checksum=crc8Calculate(message,*size);
			/* Really dirty trick to put a 0x0D (CR) at the end of the message. But
			 * it's Friday afternoon and frankly I don't care to code this once in
			 * a life time usage properly. */
			checksum|=0x0D00;
			break;
		}
		case checksumTypeCrc16: {
			checksum=crc16Calculate(message,*size);
			break;
		}
		default:
		case checksumTypeChecksum: {
			checksum=checksumCalculate(message,*size);
			break;
		}
	}
	message[(*size)++]=checksum&0xFF;
	message[(*size)++]=(checksum>>8)&0xFF;
}

/* Nice way of leaving no traces...
 * ...the more we know, the more we return. */
#define jpnevulatorGarbageCollect() { \
	interfaceDestroy(); \
	if(input!=NULL) { \
		ioClose(input); \
	} \
	if(message!=NULL) { \
		free(message); \
	} \
}
enum jpnevulatorRtrn jpnevulatorWrite(void) {
	int byte; 
	struct interface *interface;
	FILE *input=NULL;
	unsigned char *message=NULL;
	int index;
	int line;
	bool_t writingStart;

	/* Open our input file. */
	input=ioOpen("r");
	if(input==NULL) {
		perror(PROGRAM_NAME": Unable to open input");
		jpnevulatorGarbageCollect();
		return(jpnevulatorRtrnNoInput);
	}

	/* Allocate memory for the messages to send. */
	message=(unsigned char *)malloc(sizeof(message[0])*_jpnevulatorOptions.size);
	if(message==NULL) {
		perror(PROGRAM_NAME": Unable to allocate memory for message");
		jpnevulatorGarbageCollect();
		return(jpnevulatorRtrnNoMessage);
	}

	/* By default we do not expect to start writing because we have reached the
	 * maximum amount (--count) of bytes. */
	boolReset(writingStart);

	/* Collect and send the messages. We collect the messages byte by byte from the
	 * input and send them on the line when we receive an end-of-line. Take notice
	 * of this rather strange test. We need to test for writingStart too, since it's
	 * possible the counter equals zero and we still need to write some bytes on the
	 * line. I myself will probably not understand why within a couple of months, hence
	 * this special remark. Not too many details, since I like to keep myself puzzeled
	 * every once in a while. ;) */
	line=1;
	for(index=0;(boolIsSet(writingStart)||(_jpnevulatorOptions.count!=0))&&((byte=byteGet(input,_jpnevulatorOptions.base))!=byteRtrnEOF);) {
		if(boolIsSet(writingStart)) {
			byte=byteRtrnEOL;
		}
		switch(byte) {
			case byteRtrnEOL: {
				int n;

				/* Add a checksum to the message if requested. */
				if(_jpnevulatorOptions.checksum!=checksumTypeNone) {
					messageChecksumAdd(message,&index);
					if(boolIsSet(_jpnevulatorOptions.checksumFuckup)) {
						/* Subtract one from the last checksum byte of the message if the user
						 * request to fuck up the checksum. */
						message[index-1]-=1;
					}
				}

				/* Send the message on the line. */
				if(boolIsSet(_jpnevulatorOptions.send)) {
					if((interface=(struct interface *)listFirst(&_jpnevulatorOptions.interface))!=NULL) {
						do {
							/* Delay between bytes if requested. */
							if(_jpnevulatorOptions.delayByte>0) {
								int byteIndex;
								for(byteIndex=0;byteIndex<index;byteIndex++) {
									n=write(interface->fd,&(message[byteIndex]),1);
									if(n<0) {
										fprintf(stderr,"%s: %s: write of line %d byte %d failed(%d).\n",PROGRAM_NAME,interfacePrint(interface),line,byteIndex,n);
									}
									usleep(_jpnevulatorOptions.delayByte);
								}
							} else {
								n=write(interface->fd,message,sizeof(message[0])*index);
								if(n<0) {
									fprintf(stderr,"%s: %s: write of line %d failed(%d).\n",PROGRAM_NAME,interfacePrint(interface),line,n);
								}
							}
						} while((interface=(struct interface *)listNext(&_jpnevulatorOptions.interface))!=NULL);
					}
				}

				/* Print the message if requested. */
				if(boolIsSet(_jpnevulatorOptions.print)) {
					for(n=0;n<index;n++) {
						printf("%02X%c",message[n],n!=(index-1)?' ':'\n');
					}
				}

				/* Delay between messages if requested. */
				if(_jpnevulatorOptions.delayLine>0) {
					usleep(_jpnevulatorOptions.delayLine);
				}

				/* Make sure this is reset again, otherwise we will never stop reading from stdin even though we have
				 * already refused to write more data. :) */
				boolReset(writingStart);

				/* Start again with a new message and increase the line counter. */
				index=0;
				line++;
				break;
			}
			case byteRtrnUnknown: {
				/* Warn the user if we read invalid characters in the input file. We only give a warning and still
				 * send the message. The user might now what he or she is doing :-) */
				fprintf(stderr,"%s: invalid characters on input line %d. Message can be corrupted.\n",PROGRAM_NAME,line);
				break;
			}
			default: {
				/* Place the new input byte into the message. Check if there is enough room for
				 * this new byte and do leave some room(2 bytes) for the checksum if necessary.
				 * Nice trick ;-) */
				if(index<(_jpnevulatorOptions.size-(_jpnevulatorOptions.checksum*2))) {
					message[index++]=byte;
				} else {
					fprintf(stderr,"%s: Input line %d too big. Increase message size (--size).\n",PROGRAM_NAME,line);
				}
				/* Do we count the amount of bytes to write and if so are we finished writing? */
				if((_jpnevulatorOptions.count>0)&&(--_jpnevulatorOptions.count==0)) {
					boolSet(writingStart);
				}
				break;
			}
		}
	}

	/* Free allocated memory and close files opened. */
	jpnevulatorGarbageCollect();

	return(jpnevulatorRtrnOk);
}
#undef jpnevulatorGarbageCollect

static void asciiWrite(FILE *output,char *ascii,int asciiSize,int *bytesWritten,bool_t fill) {
	if((*bytesWritten)!=0) {
		if(boolIsSet(_jpnevulatorOptions.ascii)) {
			if(boolIsSet(fill)) {
				int index;
				for(index=*bytesWritten;index<_jpnevulatorOptions.width;index++) {
					switch(_jpnevulatorOptions.base) {
#define BASE(base,name,width) \
						case name: { \
							fprintf(output,"%s",SPACES(width+1)); \
							break; \
						}
						BASES
#undef BASE
					}
				}
			}
			fprintf(output,"\t%s",ascii);
			memset(ascii,'\0',asciiSize);
		}
		fprintf(output,"\n");
		*bytesWritten=0;
	}
}

static void headerWrite(
	FILE *output,
	char *ascii,int asciiSize,
	int *bytesWritten,
	struct interface *interfaceReader,char *interfaceNameCopy,int interfaceNameCopySize,
	struct timeval *timeCurrent,struct timeval *timeLast
) {
	struct tm *time;
	*timeLast=*timeCurrent;
	gettimeofday(timeCurrent,NULL);
	if(
		boolIsSet(_jpnevulatorOptions.timingPrint)&&
		((memcmp(interfaceNameCopy,interfaceReader->name,interfaceNameCopySize)!=0)||
		(((((timeCurrent->tv_sec-timeLast->tv_sec)*1000000L)+timeCurrent->tv_usec)-timeLast->tv_usec)>_jpnevulatorOptions.timingDelta))
	) {
		asciiWrite(output,ascii,asciiSize,bytesWritten,boolTrue);
		time=localtime(&(timeCurrent->tv_sec));
		fprintf(
			output,
			"%04d-%02d-%02d %02d:%02d:%02d.%06ld:",
			time->tm_year+1900,time->tm_mon+1,time->tm_mday,
			time->tm_hour,time->tm_min,time->tm_sec,timeCurrent->tv_usec
		);
		/* If more than one interface is given we want it always to
		 * be displayed as part of the printing of the timing. It's
		 * way to confusing otherwise. */
		if(listElements(&_jpnevulatorOptions.interface)>1) {
			fprintf(output," %s",interfacePrint(interfaceReader));
		}
		memcpy(interfaceNameCopy,interfaceReader->name,interfaceNameCopySize);
		fprintf(output,"\n");
	} else {
		if(
			(listElements(&_jpnevulatorOptions.interface)>1)&&
			(memcmp(interfaceNameCopy,interfaceReader->name,interfaceNameCopySize)!=0)
		) {
			asciiWrite(output,ascii,asciiSize,bytesWritten,boolTrue);
			fprintf(output,"%s\n",interfacePrint(interfaceReader));
			memcpy(interfaceNameCopy,interfaceReader->name,interfaceNameCopySize);
		}
	}
}

static void controlHandle(
	FILE *output,
	char *ascii,int asciiSize,
	int *bytesWritten,
	struct interface *interfaceReader,char *interfaceNameCopy,int interfaceNameCopySize,
	struct timeval *timeCurrent,struct timeval *timeLast
) {
	int control;
	control=interfaceControlGet(interfaceReader);
	if(control!=interfaceReader->control) {
		/* We need this explicit call to asciiWrite, even though headerWrite will call asciiWrite() itself probably. Yes, the probably
		 * means exactly what it says probably. It's possible that controlHandle() gets called and headerWrite() does not think it
		 * needs to write a new header and so no need to write the ascii data, but new control data will get written before the ascii
		 * data is written. That is, if the modem control bits change within the timing delta on an interface that has just received
		 * data. Blam, nasty output! This explicit call to asciiWrite() fixes that. */
		asciiWrite(output,ascii,asciiSize,bytesWritten,boolTrue);
		headerWrite(output,ascii,asciiSize,bytesWritten,interfaceReader,interfaceNameCopy,interfaceNameCopySize,timeCurrent,timeLast);
		interfaceControlWrite(interfaceReader,output,control);
		interfaceReader->control=control;
	}
}

/* Nice way of leaving no traces...
 * ...the more we know, the more we return. */
#define jpnevulatorGarbageCollect() { \
	interfaceDestroy(); \
	if(output!=NULL) { \
		ioClose(output); \
	} \
	if(message!=NULL) { \
		free(message); \
	} \
	if(ascii!=NULL) { \
		free(ascii); \
	} \
}
enum jpnevulatorRtrn jpnevulatorRead(void) {
	FILE *output=NULL;
	unsigned char *message=NULL;
	char *ascii=NULL;
	int asciiSize;
	ssize_t bytesRead;
	int bytesWritten;
	struct timeval timeCurrent,timeLast,*timeoutPtr,timeout;
	unsigned long *timeoutReference;
	int timeoutDelta,timeoutCount;
	fd_set readfdsReal,readfdsCopy;
	struct interface *interfaceReader,*interfaceWriter;
	char interfaceNameCopy[sizeof(interfaceReader->name)];
	int nfds;

	/* Open our output file. */
	output=ioOpen(boolIsSet(_jpnevulatorOptions.append)?"a":"w");
	if(output==NULL) {
		perror(PROGRAM_NAME": Unable to open output");
		jpnevulatorGarbageCollect();
		return(jpnevulatorRtrnNoOutput);
	}
	/* In append mode we first check if the file is empty. If not we
	 * first append the given append separator. */
	if(boolIsSet(_jpnevulatorOptions.append)) {
		struct stat outputStat;
		fstat(fileno(output),&outputStat);
		if(outputStat.st_size>0) {
			char *index;
			/* We need to parse the append separator a little bit and search for
			 * the special newline sequence. Otherwise we simply put the found
			 * character in place. */
			for(index=_jpnevulatorOptions.appendSeparator;*index!='\0';index++) {
				if((*index=='\\')&&(*(index+1)=='n')) {
					fprintf(output,"\n");
					index++;
				} else {
					fprintf(output,"%c",*index);
				}
					
			}
		}
	}

	/* Allocate memory for the messages to receive. */
	message=(unsigned char *)malloc(sizeof(message[0])*_jpnevulatorOptions.size);
	if(message==NULL) {
		perror(PROGRAM_NAME": Unable to allocate memory for message");
		jpnevulatorGarbageCollect();
		return(jpnevulatorRtrnNoMessage);
	}

	/* Allocate memory for the ascii data to print if desired. */
	if(boolIsSet(_jpnevulatorOptions.ascii)) {
		asciiSize=(sizeof(ascii[0])*_jpnevulatorOptions.width)+1;
		ascii=(char *)malloc(asciiSize);
		if(ascii==NULL) {
			perror(PROGRAM_NAME": Unable to allocate memory for ascii data");
			jpnevulatorGarbageCollect();
			return(jpnevulatorRtrnNoAscii);
		}
		memset(ascii,'\0',asciiSize);
	} else {
		/* No real reason to do this, but it makes the compiler happy. If this next line is not
		 * here the compiler generates this warning below:
		 *
		 * jpnevulator.c:407:19: warning: ‘asciiSize’ may be used uninitialized in this function [-Wuninitialized]
		 *
		 * So let's initialize asciiSize anyway. */
		asciiSize=0;
	}

	/* Initialize our last time to be far enough from the current time. Far
	 * enough is a little bit more than --timing-delta away from it. This way
	 * our first data will always gets it's timing information if requested. Take
	 * notice that I use timeCurrent here, since that one will be assigned to
	 * timeLast before gettimeofday() in the loop. */
	gettimeofday(&timeCurrent,NULL);
	timeCurrent.tv_sec-=(_jpnevulatorOptions.timingDelta/1000000L)+1;

	/* Setup our set of read file descriptors to watch. We set up the copy
	 * so we don't have to parse our list of interfaces every time we iterate. */
	FD_ZERO(&readfdsCopy);
	nfds=0;
	if((interfaceReader=(struct interface *)listFirst(&_jpnevulatorOptions.interface))!=NULL) {
		do {
			FD_SET(interfaceReader->fd,&readfdsCopy);
			nfds=max(nfds,interfaceReader->fd);
		} while((interfaceReader=(struct interface *)listNext(&_jpnevulatorOptions.interface))!=NULL);
	} else {
		fprintf(stderr,"%s: No available interface to read from\n",PROGRAM_NAME);
		jpnevulatorGarbageCollect();
		return(jpnevulatorRtrnNoTTY);
	}

	/* Clear our copy of the interface name, so if multiple interfaces are
	 * given it will print the first one and only on a change the name
	 * of the interface will be printed. */
	memset(interfaceNameCopy,'\0',sizeof(interfaceNameCopy));

	/* Do we need a timeout? We only need this when we also display the ASCII
	 * values for the received bytes. In that case we use the timeout to display
	 * the ASCII values automatically, otherwise they will never appear when less
	 * then the line length bytes are received and no more bytes are coming.
	 *
	 * And of course we need the timeout too when we poll for modem control bits. */
	timeoutCount=0;
	timeoutDelta=0;
	if(boolIsSet(_jpnevulatorOptions.ascii)||boolIsSet(_jpnevulatorOptions.control)) {
		timeoutPtr=&timeout;
		/* What timeout shall we use? If only one of the two is activated use
		 * that one and otherwise use the smallest of the two. */
		if(boolIsSet(_jpnevulatorOptions.ascii)&&boolIsSet(_jpnevulatorOptions.control)) {
			if(_jpnevulatorOptions.timingDelta<_jpnevulatorOptions.controlPoll) {
				timeoutReference=&_jpnevulatorOptions.timingDelta;
				timeoutDelta=(_jpnevulatorOptions.controlPoll/_jpnevulatorOptions.timingDelta)+1;
			} else {
				timeoutReference=&_jpnevulatorOptions.controlPoll;
				timeoutDelta=(_jpnevulatorOptions.timingDelta/_jpnevulatorOptions.controlPoll)+1;
			}
		} else if(boolIsSet(_jpnevulatorOptions.ascii)) {
			timeoutReference=&_jpnevulatorOptions.timingDelta;
		} else {
			timeoutReference=&_jpnevulatorOptions.controlPoll;
		}
	} else {
		timeoutPtr=NULL;
	}
	/* Receive our messages. */
	bytesWritten=0;
	for(;_jpnevulatorOptions.count!=0;) {
		int index;
		int rtrn;
		/* Restore our set of read file descriptors. */
		readfdsReal=readfdsCopy;
		if(timeoutPtr!=NULL) {
			timeoutPtr->tv_sec=(*timeoutReference)/1000000L;
			timeoutPtr->tv_usec=(*timeoutReference)%1000000L;
		}
		/* Wait and see if anything flows in. */
		rtrn=select(nfds+1,&readfdsReal,NULL,NULL,timeoutPtr);
		if(rtrn==-1) {
			/* Forgotten why, but we do not do anything here. I once must have had a
			 * very good reason, but I can't recall anymore. Let's just put in
			 * another funny bullshit comment to give this all some kind of meaning. */
		} else if(rtrn) {
			/* If we have received some data, we obviously need to reset out timeout counter. Otherwise it would
			 * be considered cheating. Forgetting to reset this counter results in a slight different interpretation
			 * of the --timing-delta option. Nice BUG, luckily found it myself. */
			timeoutCount=0;
			/* Walk through all our interfaces and see what needs to be done. */
			if((interfaceReader=(struct interface *)listFirst(&_jpnevulatorOptions.interface))!=NULL) {
				do {
					if(FD_ISSET(interfaceReader->fd,&readfdsReal)) {
						int size;
						/* How many bytes should we read? */
						if((_jpnevulatorOptions.count>0)&&(_jpnevulatorOptions.count<_jpnevulatorOptions.size)) {
							/* We need less bytes than the input buffer can handle, so only take what needed. */
							size=_jpnevulatorOptions.count;
						} else {
							/* Take as many as possible. No limit set or not yet within reach. */
							size=_jpnevulatorOptions.size;
						}
						bytesRead=read(interfaceReader->fd,message,size);
						if(bytesRead>0) {
							/* Are we counting bytes and if so subtract the amount just read. */
							if(_jpnevulatorOptions.count>0) {
								_jpnevulatorOptions.count-=bytesRead;
							}
							headerWrite(output,ascii,asciiSize,&bytesWritten,interfaceReader,interfaceNameCopy,sizeof(interfaceNameCopy),&timeCurrent,&timeLast);
							for(index=0;index<bytesRead;index++) {
								if(bytesWritten>=_jpnevulatorOptions.width) {
									asciiWrite(output,ascii,asciiSize,&bytesWritten,boolFalse);
								} else if(bytesWritten!=0) {
									fprintf(output," ");
								}
								if((bytesWritten==0)&&boolIsSet(_jpnevulatorOptions.byteCountDisplay)) {
									fprintf(output,"%08lX\t",interfaceReader->byteCount);
								}
								bytePut(output,_jpnevulatorOptions.base,message[index]);
								/* Increase the byte count for this interface. */
								interfaceReader->byteCount++;
								if(boolIsSet(_jpnevulatorOptions.ascii)) {
									ascii[bytesWritten]=isprint(message[index])?message[index]:'.';
								}
								bytesWritten++;
							}
							/* Does the user want to pass the data between all the interfaces? */
							if(boolIsSet(_jpnevulatorOptions.pass)) {
								struct listElement *interfaceListPosition;
								/* Save the current position in the interface list, since we are about to traverse
								 * it again. */
								interfaceListPosition=listCurrentPositionSave(&_jpnevulatorOptions.interface);
								/* Traverse the interface list again in search for all the other (not this read) interface. For
								 * every interface found write the read message to it. */
								if((interfaceWriter=(struct interface *)listFirst(&_jpnevulatorOptions.interface))!=NULL) {
									do {
										if(interfaceWriter->fd!=interfaceReader->fd) {
											ssize_t n;
											n=write(interfaceWriter->fd,message,bytesRead);
											if(n<0) {
												fprintf(stderr,"%s: %s: write of %ld bytes failed(%ld).\n",PROGRAM_NAME,interfacePrint(interfaceWriter),bytesRead,n);
											}
										}
									} while((interfaceWriter=(struct interface *)listNext(&_jpnevulatorOptions.interface))!=NULL);
								}
								/* Restore the current position for the interface list again. */
								listCurrentPositionLoad(&_jpnevulatorOptions.interface,interfaceListPosition);
							}
							fflush(output);
						}
					}
					/* See if we need to write some control data. */
					if(boolIsSet(_jpnevulatorOptions.control)) {
						controlHandle(output,ascii,asciiSize,&bytesWritten,interfaceReader,interfaceNameCopy,sizeof(interfaceNameCopy),&timeCurrent,&timeLast);
					}
				} while((_jpnevulatorOptions.count!=0)&&((interfaceReader=(struct interface *)listNext(&_jpnevulatorOptions.interface))!=NULL));
			}
		} else {
			/* Another timeout! Do we already need to write our ASCII data? */
			if(timeoutCount>=timeoutDelta) {
				asciiWrite(output,ascii,asciiSize,&bytesWritten,boolTrue);
				timeoutCount=0;
			} else {
				timeoutCount++;
			}
			/* See if we need to write some control data. */
			if(boolIsSet(_jpnevulatorOptions.control)) {
				if((interfaceReader=(struct interface *)listFirst(&_jpnevulatorOptions.interface))!=NULL) {
					do {
						controlHandle(output,ascii,asciiSize,&bytesWritten,interfaceReader,interfaceNameCopy,sizeof(interfaceNameCopy),&timeCurrent,&timeLast);
					} while((interfaceReader=(struct interface *)listNext(&_jpnevulatorOptions.interface))!=NULL);
				}
			}
		}
	}

	/* Might we possibly still need to write our ASCII data? */
	asciiWrite(output,ascii,asciiSize,&bytesWritten,boolTrue);
	
	/* And if we didn't wrote our ASCII data we most probably need a newline. */
	if(bytesWritten!=0) {
		fprintf(output,"\n");	
	}

	/* Close files opened. */
	jpnevulatorGarbageCollect();

	return(jpnevulatorRtrnOk);
}
#undef jpnevulatorGarbageCollect
