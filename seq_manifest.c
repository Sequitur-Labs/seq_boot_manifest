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
#include <linux/string.h>
#include <asm/string.h>
#include "seq_list.h"
#include "seq_manifest.h"

#define NAME_SIZE 32
#define ALIGNMENT_BYTES 8

static uint8_t MAGIC[8]={0x73,0x65,0x71,0x6c,0x61,0x62,0x73,0x00};

#define PARAM_PADDING(s) ((ALIGNMENT_BYTES-((s)%ALIGNMENT_BYTES))%ALIGNMENT_BYTES)

typedef struct writedata {
	uint8_t *ptr;
	uint32_t numentries;
	uint32_t totalbytes;
} SeqWriteDataType;

/*
 static char* seq_strrchr(char* str,int character)
 {
	char* res=0;
	char* ptr=str+(strlen(str)-1);
	do
	{
		if (*ptr==character)
		{
			res=ptr;
			break;
		}
		ptr--;
	}
	while (ptr>=str);
	return res;
 }
 */
	
//-----------------------------------------------
// iterators
static int free_param(SeqEntry *e,void *data)
{
	SeqParamKey *key=NULL;
	if(!e) {
		return 0;
	}
	key = (SeqParamKey*)e->data;
	if(key) {
		free(key);
	}
	return 0;
}


static int find_proc(SeqEntry *e,void *data)
{
	int res=0;
	char *compstr=(char*)data;
	SeqParamKey *key=NULL;
	if(!e) {
		return res;
	}
	key = (SeqParamKey*)e->data;

	if(key && key->key && compstr) {
		if (!strcmp(key->key,compstr)) {
			res=1;
		}
	}
	return res;
}


static int size_proc(SeqEntry *e, void *data)
{
	size_t *accumulator=(size_t*)data;
	SeqParamKey *key=NULL;
	if(!e) {
		return 0;
	}
	key = (SeqParamKey*)e->data;
	if(!key || !data) {
		return 0;
	}

	*accumulator+=NAME_SIZE; // name
	*accumulator+=sizeof(uint32_t); // type
	*accumulator+=sizeof(uint32_t); // size

	int padding=PARAM_PADDING(key->size);
	
	*accumulator+=key->size;
	*accumulator+=padding;

	return 0;
}


static int write_proc(SeqEntry *e, void *data)
{
	SeqWriteDataType *transfer=(SeqWriteDataType*)data;
	SeqParamKey *key=NULL;
	if(!e) {
		return -1;
	}
	key = (SeqParamKey*)e->data;
	if(!key || !data) {
		return -1;
	}

	memset(transfer->ptr,0,NAME_SIZE);
	strncpy((char*)transfer->ptr,key->key,strlen(key->key));
	transfer->ptr+=NAME_SIZE;
	
	memcpy(transfer->ptr,&key->type,sizeof(uint32_t));
	transfer->ptr+=sizeof(uint32_t);

	memcpy(transfer->ptr,&key->size,sizeof(uint32_t));
	transfer->ptr+=sizeof(uint32_t);

	int padding=PARAM_PADDING(key->size);

	memset(transfer->ptr,0,key->size+padding);
	memcpy(transfer->ptr,key->value,key->size);

	transfer->ptr+=key->size+padding;
	transfer->numentries++;
	transfer->totalbytes+=(NAME_SIZE+sizeof(uint32_t)+sizeof(uint32_t)+key->size+padding);

	return 0;
}

static int entry_proc(SeqEntry *e, void *data)
{
	return (e && e->data==data);
}

static int exist_proc(SeqEntry *e, void *data)
{
	char *section=NULL;
	char *newsection=(char*)data;
	if(!e || !e->data || !data){
		return 0;
	}

	section = (char*)e->data;

	int res=strcmp(section,newsection) ? 0 : 1;
	return res;
}


static int section_proc(SeqEntry *e, void *data)
{
	SeqList *seclist=(SeqList*)data;
	SeqParamKey *key=NULL;
	if(!e) {
		return 0;
	}
	key = (SeqParamKey*)e->data;
	if(!key || !data) {
		return 0;
	}

	char *testkey=(char*)malloc(strlen(key->key)+1);
	if(!testkey) {
		return 0;
	}

	memset(testkey, 0, strlen(key->key)+1);
	strncpy(testkey, key->key, strlen(key->key));

	char *point=strrchr(testkey,'_');
	if (point) {
		*point=0;
		SeqEntry *existing=seq_search_list(seclist, 0, exist_proc, testkey);
		if (!existing) {
			char *actual=(char*)malloc(strlen(testkey)+1);
			if(actual) {
				memset(actual,0,strlen(testkey)+1);
				strncpy(actual,testkey,strlen(testkey));
				seq_append_entry(seclist,actual);
			}
		}
	}

	free(testkey);
	return 0;
}

static int free_section_proc(SeqEntry *e, void *data)
{
	char *section=NULL;
	if(e) {
		section = (char*)e->data;
	}
	if(section) {
		free(section);
	}
	return 0;
}

static int paramkeysproc(SeqEntry *e, void *data)
{
	struct paramlistdata *plistdata=(struct paramlistdata*)data;
	SeqList *paramlist=NULL;
	const char *section=NULL;
	SeqParamKey *key=NULL;
	char *testkey=NULL;
	char *point=NULL;


	if(!data || !e){
		return -1;
	}

	paramlist = plistdata->paramlist;
	section=plistdata->section;
	key=(SeqParamKey*)e->data;

	if(!paramlist || !section || !key){
		return -2;
	}

	testkey=(char*)malloc(strlen(key->key)+1);
	if(!testkey) {
		return -3;
	}

	memset(testkey,0,strlen(key->key)+1);
	memcpy(testkey,key->key,strlen(key->key));

	point=strchr(testkey,'_');

	if (point) {
		*point=0;
		if (!strcmp(testkey,section))
			seq_append_entry(paramlist,e->data);
	}

	free(testkey);
	return 0;
}

//-----------------------------------------------
// private
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
static void fill_params(SeqManifest *params)
{
	uint8_t *ptr=params->raw;
	uint8_t *end=ptr+params->size;

	while (ptr<end)
	{
		SeqParamKey *key=(SeqParamKey*)malloc(sizeof(SeqParamKey));
		if(!key) {
			break;
		}
		key->key=(char*)ptr;

		ptr+=NAME_SIZE;
		
		key->type=*(uint32_t*)ptr;
		ptr+=sizeof(uint32_t);
		
		key->size=*(uint32_t*)ptr;
		ptr+=sizeof(uint32_t);

		key->value=(void*)ptr;
		key->flags = 0;

		int padding=PARAM_PADDING(key->size);

		ptr+=key->size+padding;

		seq_append_entry(params->params,key);
	}
}
#pragma GCC diagnostic pop

static size_t calculate_size(SeqManifest *params)
{
	size_t res=0;
	seq_iterate_list(params->params,0,size_proc,&res);
	return res;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
static uint8_t *create_binary_params(SeqManifest *params)
{
	uint8_t *res=0;
	SeqManifestHeader *header=0;
	struct writedata transfer;
	size_t buffersize=calculate_size(params);

	res=(uint8_t*)malloc(sizeof(SeqManifestHeader)+buffersize);
	if(!res) {
		return 0;
	}

	header=(SeqManifestHeader*)res;

	memcpy(header->magic,MAGIC,8);
	header->numentries=42;
	header->size=buffersize;

	memset(&transfer,0,sizeof(SeqWriteDataType));
	transfer.ptr=res+sizeof(SeqManifestHeader);

	seq_iterate_list(params->params,0,write_proc,&transfer);

	header->numentries=transfer.numentries;
	return res;
}
#pragma GCC diagnostic pop


static char *create_key_name(const char *section,const char *name)
{
	int clen = 0;
	char *ptr=NULL;
	char *res=(char*)malloc(NAME_SIZE);
	if(!res) {
		return NULL;
	}

	ptr=res;
	memset(res, 0, NAME_SIZE);
	clen = strlen(section);
	if(clen > NAME_SIZE) {
		clen=NAME_SIZE;
	}
	strncpy(ptr,section,strlen(section));
	ptr=ptr+strlen(ptr);

	if(clen < NAME_SIZE) {
		strncpy(ptr,"_",1);
		ptr++;
		if(clen + 1 + strlen(name) > NAME_SIZE) {
			clen = NAME_SIZE - clen - 1;
		} else {
			clen = strlen(name);
		}

		strncpy(ptr,name,clen);
	}

	return res;
}

//-----------------------------------------------
// public
SeqManifest *seq_load_manifest(uintptr_t where)
{
	SeqManifest *res=0;
	SeqManifestHeader header;

	/*
	Header is defined here and the information is copied because
	we can't be guaranteed that the offset into the update
	package (ddr) will be properly aligned.
	*/
	memcpy(&header, (void*)where, sizeof(SeqManifestHeader));
	
	if (memcmp(header.magic,MAGIC,8)==0)
	{
		uint8_t *raw=(uint8_t*)malloc(header.size);
		if(!raw) {
			printf("Failed to allocate memory. Returning NULL.\n");
			return NULL;
		}
		//printf("Raw: %p\n", raw);
		memcpy(raw,(uint8_t*)(where+sizeof(SeqManifestHeader)),header.size);
		res=seq_init_manifest(raw,header.size);
	}
	else {
		printf("NO MAGIC\n");
	}
	
	return res;
}

SeqManifest *seq_new_manifest( void ){
	SeqManifest *res=(SeqManifest*)malloc(sizeof(SeqManifest));
	if(!res) {
		return res;
	}

	memset(res,0,sizeof(SeqManifest));

	res->raw=NULL;
	res->size=0;
	res->params=seq_new_list();

	return res;
}

SeqManifest *seq_init_manifest(uint8_t *raw,uint32_t size)
{
	SeqManifest *res=(SeqManifest*)malloc(sizeof(SeqManifest));
	if(!res) {
		return res;
	}

	memset(res,0,sizeof(SeqManifest));

	res->raw=raw;
	res->size=size;
	res->params=seq_new_list();

	fill_params(res);

	return res;
}


void seq_free_manifest(SeqManifest *params)
{
	if(!params) {
		return;
	}
	if(params->params) {
		seq_free_list(params->params,free_param);
	}
	if(params->raw) {
		free(params->raw);
	}
	free(params);
}

SeqParamKey *seq_find_param(SeqManifest *params, const char *section, const char *name)
{
	SeqParamKey *res=0;
	SeqEntry *entry = NULL;
	char *keyname=create_key_name(section,name);
	if(!keyname) {
		return res;
	}

	entry=seq_search_list(params->params,0,find_proc,keyname);

	if (entry) {
		res=(SeqParamKey*)entry->data;
	}
	
	free(keyname);
	return res;
}


void seq_delete_param_by_name(SeqManifest *params, const char *section, const char *name)
{
	SeqParamKey *key=seq_find_param(params,section,name);
	if (key) {
		seq_delete_param_by_key(params,key);
		free(key);
		key=0;
	}
}

void seq_delete_param_by_key(SeqManifest *params, SeqParamKey *key)
{
	SeqEntry *delentry=NULL;

	if(params && params->params && key) {
		delentry = seq_search_list(params->params,0,entry_proc,key);
	}
	if (delentry) {
		if((key->flags & SEQ_FLAG_KEY_DYNAMIC) == SEQ_FLAG_KEY_DYNAMIC && key->key){
			free(key->key);
			key->key = 0;
		}
		if((key->flags & SEQ_FLAG_VALUE_DYNAMIC) == SEQ_FLAG_VALUE_DYNAMIC && key->value){
			free(key->value);
			key->value = 0;
		}

		//Removes entry from list but does not free memory. 'key' still needs to be freed.
		seq_delete_entry(params->params,delentry);
		free(delentry);
	}
}

void seq_add_param(SeqManifest *params, SeqParamKey *key)
{
	if(params->params && key ) {
		seq_append_entry(params->params,key);
	}
}

SeqParamKey *seq_new_param(const char *section, const char *name, int type)
{
	SeqParamKey *res=NULL;
	char *keyname = NULL;
	if(!section || !name) {
		return res;
	}

	res=(SeqParamKey*)malloc(sizeof(SeqParamKey));
	if(!res) {
		return res;
	}

	keyname = create_key_name(section,name);
	memset(res,0,sizeof(SeqParamKey));
	res->key=keyname;
	res->type=type;
	res->flags |= SEQ_FLAG_KEY_DYNAMIC;
	return res;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
uint8_t *seq_get_binary_manifest(SeqManifest *params, int *size)
{
	uint8_t *parambuffer=NULL;
	SeqManifestHeader *header=NULL;
	size_t totalsize=0;

	if(!params || !size) {
		return parambuffer;
	}

	parambuffer = create_binary_params(params);
	if(parambuffer) {
		header = (SeqManifestHeader*)parambuffer;
		totalsize=header->size+sizeof(SeqManifestHeader);
		*size=totalsize;
	}

	return parambuffer;
}
#pragma GCC diagnostic pop


SeqList *seq_manifest_sections(SeqManifest *params)
{
	SeqList *res=NULL;
	if(!params || !params->params) {
		return res;
	}

	res = seq_new_list();
	if( !res ){
		return res;
	}
	seq_iterate_list(params->params, 0, section_proc, res);
	return res;
}


void seq_free_manifest_sections(SeqList *sectionlist)
{
	if(sectionlist) {
		seq_free_list(sectionlist,free_section_proc);
	}
}

SeqList * seq_manifest_section_keys(SeqManifest *params, const char *section)
{
	SeqList *res=SeqNewList();
	struct paramlistdata pdata;
	pdata.paramlist=res;
	pdata.section=section;
	seq_iterate_list(params->params,0,paramkeysproc,&pdata);
	return res;
}


void seq_free_manifest_section_keys(SeqList *keylist)
{
	seq_free_list(keylist,0);
}

char * seq_get_key_section(SeqParamKey *key){
	char* res=NULL;
	char *point = strrchr(key->key, '_');
	if(point) {
		size_t len = (point-key->key);
		res = (char*)MALLOC(len+1);
		if(res) {
			memset(res, 0, len+1);
			memcpy(res, key->key, len);
		}
	}
	return res;
}

char * seq_get_key_name(SeqParamKey *key){
	char* res=NULL;
	char *point = strrchr(key->key, '_');
	if(point){
		size_t len = strlen(point);
		res = (char*)MALLOC(len);
		if(res) {
			memset(res, 0, len);
			memcpy(res, point+1, len-1);
		}
	}
	return res;
}

#define INT_CONVENIENCE(TYPE,ID) TYPE seq_value_##TYPE(SeqParamKey *key)	\
	{ \
		TYPE res=0;	\
		if (key && key->type==ID) {\
			memcpy(&res,key->value,key->size); \
		} \
		return res; \
	}

INT_CONVENIENCE(uint8_t,SEQ_TYPE_UINT8)
INT_CONVENIENCE(uint16_t,SEQ_TYPE_UINT16)
INT_CONVENIENCE(uint32_t,SEQ_TYPE_UINT32)
INT_CONVENIENCE(uint64_t,SEQ_TYPE_UINT64)

/* INT_CONVENIENCE(int8_t,SEQ_TYPE_INT8) */
/* INT_CONVENIENCE(int16_t,SEQ_TYPE_INT16) */
/* INT_CONVENIENCE(int32_t,SEQ_TYPE_INT32) */
/* INT_CONVENIENCE(int64_t,SEQ_TYPE_INT64) */

#define PTR_CONVENIENCE(LABEL,TYPE,ID) TYPE seq_value_##LABEL(SeqParamKey *key) \
	{ \
		TYPE res=0; \
		if (key && key->type==ID) \
		{ \
			res=(TYPE)malloc(key->size); \
			if(res && key->value) { \
				memcpy(res,key->value,key->size); \
			} \
		} \
		return res; \
}

PTR_CONVENIENCE(string,char*,SEQ_TYPE_STRING)
PTR_CONVENIENCE(binary,uint8_t*,SEQ_TYPE_BINARY)

char* seq_get_keyval_string(SeqManifest *slip, const char *section, const char *keyname)
{
	SeqParamKey *key=seq_find_param(slip,section,keyname);
	if (key) {
		return seq_value_string(key);
	}

	return NULL;
}

uint32_t seq_get_keyval_uint32(SeqManifest *slip, const char *section, const char *keyname)
{
	uint32_t res=0;
	SeqParamKey *key=seq_find_param(slip,section,keyname);
	if (key) {
		res=(uint32_t)seq_value_uint32_t(key);
	}

	return res;
}

uint64_t seq_get_keyval_uint64(SeqManifest *slip, const char *section, const char *keyname)
{
	uint32_t res=0;
	SeqParamKey *key=seq_find_param(slip,section,keyname);
	if (key) {
		res=(uint64_t)seq_value_uint64_t(key);
	}

	return res;
}
