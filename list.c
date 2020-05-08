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

#include <stdlib.h>

#include "list.h"

/* This routine initializes our list meta data. */
enum listRtrn listInitialize(list_t *list) {
	list->first=list->last=list->current=NULL;
	list->elements=0;
	return(listRtrnOk);
}

/* This routine appends one element to the list. It will
 * be appended just after the current element. */
enum listRtrn listAppend(list_t *list,void *data) {
	struct listElement *p;
	/* Allocate memory for the new element. */
	p=(struct listElement *)malloc(sizeof(p[0]));
	if(p!=NULL) {
		/* Assign the data to the element. */
		p->data=data;
		/* Do any elements exist already in the list? */
		if(list->current!=NULL) {
			/* Yes, we will append this one after the current one. */
			/* Is the current one the last one in the list? */
			if(list->current->next!=NULL) {
				/* No, so we have to let the one after the current know
				 * that we will be the previous element from now on. */
				list->current->next->previous=p;
			} else {
				/* Yes, so we tell the meta data that we are the last in the
				 * list right now. */
				list->last=p;
			}
			/* Our next one will be the next one of the current element. */
			p->next=list->current->next;
			/* Our previous one will be the current element. */
			p->previous=list->current;
			/* And of course we should let the current element know that
			 * we come next in the list. */
			list->current->next=p;
		} else {
			/* No, there do not exist any elements in the list yet so we
			 * are the first, last and current. */
			p->previous=p->next=NULL;
			list->first=list->last=list->current=p;
		}
		/* Increase the amount of elements known to the list. */
		list->elements++;
		return(listRtrnOk);
	} else {
		return(listRtrnMemory);
	}
}

/* This routine insert one element to the list. It will
 * be inserted just before the current element. */
enum listRtrn listInsert(list_t *list,void *data) {
	struct listElement *p;
	/* Allocate memory for the new element. */
	p=(struct listElement *)malloc(sizeof(p[0]));
	if(p!=NULL) {
		/* Assign the data to the element. */
		p->data=data;
		/* Do any elements exist already in the list? */
		if(list->current!=NULL) {
			/* Yes, we will insert this one before the current one. */
			/* Our previous one will be the one before the current element. */
			p->previous=list->current->previous;
			/* We will be the previous one for the current element. */
			list->current->previous=p;
			/* Our next one will be the current element. */
			p->next=list->current;
			/* Are we now the first one in the list? */
			if(p->previous!=NULL) {
				/* No, so we have to let the one before us know that we
				 * will be the next element from now on. */
				p->previous->next=p;
			} else {
				/* Yes, so we tell the meta data that we are the first in the
				 * list right now. */
				list->first=p;
			}
		} else {
			/* No, there do not exist any elements in the list yet so we
			 * are the first, last and current. */
			p->previous=p->next=NULL;
			list->first=list->last=list->current=p;
		}
		/* Increase the amount of elements known to the list. */
		list->elements++;
		return(listRtrnOk);
	} else {
		return(listRtrnMemory);
	}
}

/* This routine removes the current element from the list. If none available
 * it does nothing. */
void listRemove(list_t *list,void (*garbageCollect)(void *)) {
	/* Any elements in the list available? */
	if(list->current!=NULL) {
		struct listElement *p;
		/* Yes, let's remove the current one. */
		p=list->current;
		/* Is it the first element in the list? */
		if(p->previous!=NULL) {
			/* No, so let the element before know what the new element after
			 * it will be. */
			list->current->previous->next=p->next;
		} else {
			/* Yes, so update the first meta data. */
			list->first=p->next;
		}
		/* Is it the last element in the list? */
		if(p->next!=NULL) {
			/* No, so let the element after it know what the new element
			 * before it will be. */
			p->next->previous=p->previous;
		} else {
			/* Yes, so update the last meta data. */
			list->last=p->previous;
		}
		/* Now we need to update the current meta data. Does an element
		 * after this element exist? */
		if(p->next!=NULL) {
			/* Yes, this next element will be the new current element. */
			list->current=p->next;
		} else {
			/* No, the element before this one will be the new current element. */
			list->current=p->previous;
		}
		/* Run the garbage collector for this item if required. */
		if(garbageCollect!=NULL) {
			garbageCollect(p->data);
		}
		/* Return system resources. */
		free(p);
		/* Decrease the amount of elements known to the list. */
		list->elements--;
	}
}

/* This routine returns the data of the current element or NULL if
 * none is available. */
void *listCurrent(list_t *list) {
	return(list->current!=NULL?list->current->data:NULL);
}

/* This routine returns the first element in the list or NULL if
 * none is available. It also sets the current meta data to be the
 * first element in the list. */
void *listFirst(list_t *list) {
	list->current=list->first;
	return(listCurrent(list));
}

/* This routine returns the next element in the list or NULL if
 * none is available. It also advances the current meta data to the
 * next element. */
void *listNext(list_t *list) {
	if((list->current!=NULL)&&(list->current->next!=NULL)) {
		list->current=list->current->next;
		return(listCurrent(list));
	} else {
		return(NULL);
	}
}

/* This routine returns the previous element in the list or NULL if
 * none is available. It also sets the current meta data to the
 * previous element. */
void *listPrevious(list_t *list) {
	if((list->current!=NULL)&&(list->current->previous!=NULL)) {
		list->current=list->current->previous;
		return(listCurrent(list));
	} else {
		return(NULL);
	}
}

/* This routine returns the last element in the list or NULL if
 * none is available. It also sets the current meta data to be the
 * last element in the list. */
void *listLast(list_t *list) {
	list->current=list->last;
	return(listCurrent(list));
}

/* This routine search the list for a specific element given by needle and
 * returns that element or NULL if it's not found. */
void *listSearch(list_t *list,int (*compare)(void *,void *),void *needle) {
	void *data;
	int found;
	/* At first we of course didn't find anything and especially when
	   we do not receive a list we make sure we did not found anything
	   as of yet. */
	found=0;
	/* Does any element exist in the list? */
	if((data=listFirst(list))!=NULL) {
		do {
			/* Yes, so search for our element... */
			if((found=compare(data,needle))) break;
			/* ...untill none left. */
		} while((data=listNext(list))!=NULL);
	}
	/* If we found our data return it, otherwise return NULL. */
	return(found?data:NULL);
}

/* This routine gracefully returns all system resources and empty the
 * list. */
void listDestroy(list_t *list,void (*garbageCollect)(void *)) {
	struct listElement *p;
	/* Search untill no elements are left... */
	while(list->first!=NULL) {
		/* First save a reference to our next element. */
		p=list->first->next;
		/* Run the garbage collector for this item if required. */
		if(garbageCollect!=NULL) {
			garbageCollect(list->first->data);
		}
		/* Then return resources for the current one. */
		free(list->first);
		/* And at last advance to our next element. */
		list->first=p;
	}
	/* Reset all our meta data. */
	list->first=list->last=list->current=NULL;
	list->elements=0;
}

#if 0
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

int cmpr(void *p,void *q) {
	return(!strcmp((char *)p,(char *)q));
}

void garbageCollect(void *data) {
	printf("Taking down: %s\n",(char *)data);
}

int main(int argc,char **argv) {
	list_t list;
	int index;
	char *arg;
	int elements;
	/* Initialize our list. */
	listInitialize(&list);
	/* Fill our list with all command line arguments given, but skip the
	 * first argument since that's our needle (see below -> listSearch()). */
	for(index=2;index<argc;index++) {
		/* Append the argument to our list. */
		if(listAppend(&list,(void *)argv[index])!=listRtrnOk) {
			fprintf(stderr,"Can't create item!\n");
			break;
		}
		/* Advance to this next position. By default listAppend() does not
		 * advance our current pointer to the appended element. */
		listNext(&list);
	}
	/* Print all our arguments from the first to the last. */
	printf("First to last:\n");
	/* Does any argument exist? */
	if((arg=(char *)listFirst(&list))!=NULL) {
		/* Yes, we print it and... */
		do {
			printf("[%s]\n",arg);
			/* ...advance to the next argument, untill none left. */
		} while((arg=(char *)listNext(&list))!=NULL);
	} else {
		printf("empty list\n");
	}
	/* Print all our arguments from the last to the first. */
	printf("Last to first:\n");
	/* Does any argument exist? */
	if((arg=(char *)listLast(&list))!=NULL) {
		/* Yes, we print it and... */
		do {
			printf("[%s]\n",arg);
			/* ...advance to the next argument, untill none left. */
		} while((arg=(char *)listPrevious(&list))!=NULL);
	} else {
		printf("empty list\n");
	}
	/* If there do exist enough arguments... */
	if(argc>=2) {
		/* ...search for the first one given... */
		arg=listSearch(&list,cmpr,(void *)argv[1]);
		/* ...and tell if it's found. */
		printf("searching for [%s]... %sfound\n",argv[1],arg!=NULL?"":"not ");
	}
	/* Randomly remove half of the arguments. */
	srandom(getpid());
	for(elements=listElements(&list)/2;elements>0;elements--) {
		int element=(random()%(argc-2))+2;
		arg=listSearch(&list,cmpr,(void *)argv[element]);
		printf("Removing [%s]... ",argv[element]);
		if(arg!=NULL) {
			listRemove(&list,NULL);
			printf("ok\n");
		} else {
			printf("not in the list!\n");
		}
	}
	/* Print all our arguments from the first to the last. */
	printf("First to last:\n");
	/* Does any argument exist? */
	if((arg=(char *)listFirst(&list))!=NULL) {
		/* Yes, we print it and... */
		do {
			printf("[%s]\n",arg);
			/* ...advance to the next argument, untill none left. */
		} while((arg=(char *)listNext(&list))!=NULL);
	} else {
		printf("empty list\n");
	}
	/* Print all our arguments from the last to the first. */
	printf("Last to first:\n");
	/* Does any argument exist? */
	if((arg=(char *)listLast(&list))!=NULL) {
		/* Yes, we print it and... */
		do {
			printf("[%s]\n",arg);
			/* ...advance to the next argument, untill none left. */
		} while((arg=(char *)listPrevious(&list))!=NULL);
	} else {
		printf("empty list\n");
	}

	/* Destroy the list and all its elements. */
	listDestroy(&list,garbageCollect);
	return(0);
}
#endif
