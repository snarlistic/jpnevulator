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

#ifndef __LIST_H
#define __LIST_H

enum listRtrn {
	listRtrnOk=0,
	listRtrnMemory
};

struct listElement {
	void *data;
	struct listElement *previous;
	struct listElement *next;
};
typedef struct list {
	struct listElement *first;
	struct listElement *last;
	struct listElement *current;
	int elements;
} list_t;

/* Do notice the use of -> in listElements, so it conforms to all the
 * other routines in that you need to give the address of the list. */
#define listElements(x) ((x)->elements)
extern enum listRtrn listInitialize(list_t *);
extern enum listRtrn listAppend(list_t *,void *);
extern enum listRtrn listInsert(list_t *,void *);
extern void listRemove(list_t *,void (*)(void *));
extern void *listCurrent(list_t *);
/* These next two are routines to temporary freeze the current pointer in the list. This
 * way one can traverse the list within a traverse of the list itself. */
#define listCurrentPositionSave(x) ((x)->current)
#define listCurrentPositionLoad(x,y) ((x)->current=y)
extern void *listFirst(list_t *);
extern void *listNext(list_t *);
extern void *listPrevious(list_t *);
extern void *listLast(list_t *);
extern void *listSearch(list_t *,int (*)(void *,void *),void *);
extern void listDestroy(list_t *,void (*)(void *));

#endif
