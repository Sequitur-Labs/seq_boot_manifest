/*================================================
Copyright © 2016-2019 Sequitur Labs Inc. All rights reserved.

The information and software contained in this package is proprietary property of
Sequitur Labs Incorporated. Any reproduction, use or disclosure, in whole
or in part, of this software, including any attempt to obtain a
human-readable version of this software, without the express, prior
written consent of Sequitur Labs Inc. is forbidden.
================================================*/

#include <common.h>
#include <malloc.h>
#include <linux/types.h>
#include <seq/seq_list.h>

static int free_func(SeqEntry *item, void *data)
{
	if (data) {
		SeqIterFunc freefunc=(SeqIterFunc)data;
		freefunc(item,0);
	}
	free(item);
	return 0;
}


static int count_func(SeqEntry *e, void *data)
{
	*(int*)data+=1;
	return 0;
}

//-----------------------------------------------


SeqList *seq_new_list()
{
	SeqList *res=(SeqList*)malloc(sizeof(SeqList));
	res->head=0;
	res->tail=0;
	return res;
}

void seq_append_entry(SeqList *list,void *data)
{
	SeqEntry *entry=(SeqEntry*)malloc(sizeof(SeqEntry));
	if (entry) {
		entry->data=data;
		entry->next=0;
		entry->prev=0;
		if (list->head==0 || list->tail==0) {
			list->head=entry;
			list->tail=entry;
		} else {
			list->tail->next=entry;
			entry->prev=list->tail;
			list->tail=entry;
		}
	} else {
		printf("Failed to allocate memory for adding item to list!!!\n");
	}
}

void seq_delete_entry(SeqList *list,SeqEntry *entry)
{
	if(!entry) {
		return;
	}

	if (entry->prev) {
		entry->prev->next=entry->next;
	}

	if (entry->next) {
		entry->next->prev=entry->prev;
	}

	if(!list) {
		return;
	}

	if(entry == list->head) {
		list->head = entry->next;
	}
	if(entry == list->tail) {
		list->tail = entry->prev;
	}
}

void seq_free_list(SeqList *list, SeqIterFunc freeback)
{
	if(!list) {
		return;
	}

	seq_iterate_list(list,0,free_func,freeback);
	free(list);
}


int seq_iterate_list(SeqList *list, SeqEntry *start, SeqIterFunc callback, void *data)
{
	SeqEntry *ptr=start;
	int cbackres=0;

	if(!list) {
		return cbackres;
	}

	if (ptr==0) {
		ptr=list->head;
	}
	
	while (ptr) {
		SeqEntry *next=ptr->next;
		cbackres=callback(ptr,data);
		if (cbackres) {
			break;
		}
		ptr=next;
	}
	return cbackres;
}

SeqEntry *seq_search_list(SeqList *list, SeqEntry *start, SeqIterFunc compback, void *data)
{
	SeqEntry *ptr=start;
	SeqEntry *res=0;
	int cbackres=0;

	if(!list) {
		return res;
	}

	if (ptr==0) {
		ptr=list->head;
	}
	
	while (ptr) {
		SeqEntry *next=ptr->next;
		cbackres=compback(ptr,data);
		if (cbackres) {
			res=ptr;
			break;
		}
		ptr=next;
	}
	return res;
}

int seq_get_list_count(SeqList *list)
{
	int res=0;
	if(!list) {
		return res;
	}

	seq_iterate_list(list, 0, count_func, &res);
	return res;
}
