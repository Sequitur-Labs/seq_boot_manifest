/*================================================
Copyright © 2016-2019 Sequitur Labs Inc. All rights reserved.

The information and software contained in this package is proprietary property of
Sequitur Labs Incorporated. Any reproduction, use or disclosure, in whole
or in part, of this software, including any attempt to obtain a
human-readable version of this software, without the express, prior
written consent of Sequitur Labs Inc. is forbidden.
================================================*/
#ifndef _SEQ_MANIFEST_H
#define _SEQ_MANIFEST_H

#include "seq_list.h"

//#define DEBUG_BUILD 1

//SEQ is for Sequitur Params
#define SEQ_TYPE_UNKNOWN 0

#define SEQ_TYPE_UINT8   1
#define SEQ_TYPE_UINT16  2
#define SEQ_TYPE_UINT32  3
#define SEQ_TYPE_UINT64  4

#define SEQ_TYPE_INT8    5
#define SEQ_TYPE_INT16   6
#define SEQ_TYPE_INT32   7
#define SEQ_TYPE_INT64   8

#define SEQ_TYPE_STRING  9
#define SEQ_TYPE_BINARY  10

/*
 * Values for SeqParamKey  : flags.
 */
#define SEQ_FLAG_VALUE_DYNAMIC 1 //The 'value' has been dynamically allocated.
#define SEQ_FLAG_KEY_DYNAMIC 2   //The 'key' has been dynamically allocated.

typedef struct {
	uint8_t magic[8];
	uint32_t numentries;
	uint32_t size;
} SeqManifestHeader;


typedef struct seqp_key {
	char *key;
	uint32_t type;
	uint32_t size;
	void *value;
	uint32_t flags;
} SeqParamKey;


typedef struct seq_manifest {
	uint8_t *raw;
	uint32_t size;
	uintptr_t nvm;
	SeqList *params;
} SeqManifest;

SeqManifest *seq_load_manifest(uintptr_t where);
SeqManifest *seq_new_manifest( void );
SeqManifest *seq_init_manifest(uint8_t *raw, uint32_t size);
void seq_free_manifest(SeqManifest *params);

SeqParamKey *seq_find_param(SeqManifest *params,const char *section, const char *name);
void seq_delete_param_by_name(SeqManifest *params, const char *section, const char *name);
void seq_delete_param_by_key(SeqManifest *params, SeqParamKey *key);
void seq_add_param(SeqManifest *params, SeqParamKey *key);
SeqParamKey *seq_new_param(const char *section, const char *name, int type);

void seq_write_params(SeqManifest *params, uintptr_t dest);
uint8_t *seq_get_binary_manifest(SeqManifest *params, int *size);
SeqList *seq_manifest_sections(SeqManifest *params);
void seq_free_manifest_sections(SeqList *sectionlist);
SeqList * seq_manifest_section_keys(SeqManifest *params, const char *section);
void seq_free_manifest_section_keys(SeqList *keylist);

//Get the 'section' part of the key
char * seq_get_key_section(SeqParamKey *key);
//Get the 'name' part of the key
char * seq_get_key_name(SeqParamKey *key);

// value convenience functions
#define INT_CONVENIENCE_DECL(TYPE) TYPE seq_value_##TYPE(SeqParamKey *key);
INT_CONVENIENCE_DECL(uint8_t)
INT_CONVENIENCE_DECL(uint16_t)
INT_CONVENIENCE_DECL(uint32_t)
INT_CONVENIENCE_DECL(uint64_t)


/*
 * Returns either a char * or uint8_t * copy of the value for the key.
 * The caller is responsible for the memory allocated.
 */
#define PTR_CONVENIENCE_DECL(LABEL,TYPE) TYPE seq_value_##LABEL(SeqParamKey *key);
PTR_CONVENIENCE_DECL(string, char*)
PTR_CONVENIENCE_DECL(binary, uint8_t*)

/*
 * Returns the character string 'value' of the key at 'section:keyname'.
 * The caller is responsible for the memory allocated.
 */
char *seq_get_keyval_string(SeqManifest *slip, const char *section, const char *keyname);

/*
 * Returns the uint32_t 'value' of the key at 'section:keyname'.
 */
uint32_t seq_get_keyval_uint32(SeqManifest *slip, const char *section, const char *keyname);

/*
 * Returns the uint64_t 'value' of the key at 'section:keyname'.
 */
uint64_t seq_get_keyval_uint64(SeqManifest *slip, const char *section, const char *keyname);

#endif /*SEQ_PARAMS_H*/
