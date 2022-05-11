/*================================================
Copyright © 2016-2022 Sequitur Labs Inc. All rights reserved.

The information and software contained in this package is proprietary property of
Sequitur Labs Incorporated. Any reproduction, use or disclosure, in whole
or in part, of this software, including any attempt to obtain a
human-readable version of this software, without the express, prior
written consent of Sequitur Labs Inc. is forbidden.
================================================*/
#ifndef _SEQ_LIST_H
#define _SEQ_LIST_H

//Helper macros to allow quick cross platform implementation.
#define SEQ_LIST_MALLOC malloc
#define SEQ_LIST_FREE free

//Predefined to allow pointer type.
typedef struct seq_entry_t SeqEntry;
struct seq_entry_t {
	void *data;
	SeqEntry *next;
	SeqEntry *prev;
};


typedef struct seq_list_t{
	SeqEntry *head;
	SeqEntry *tail;
} SeqList;

typedef int (*SeqIterFunc)(SeqEntry *item, void* data);

SeqList *seq_new_list( void );
void seq_append_entry(SeqList *list,void *data);
void seq_delete_entry(SeqList *list, SeqEntry *entry);
void seq_free_list(SeqList *list, SeqIterFunc freefunc);
int seq_iterate_list(SeqList *list, SeqEntry *start, SeqIterFunc callback, void *data);
SeqEntry *seq_search_list(SeqList *list, SeqEntry *start, SeqIterFunc compback, void *data);
int seq_get_list_count(SeqList *list);

#endif /*SEQ_LIST_H*/
