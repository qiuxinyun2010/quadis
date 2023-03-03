#ifndef __DICT_H
#define __DICT_H
#endif

#include <stdint.h>
#ifndef PSMALLOC
#define PSMALLOC
#include "psmallocv4.h"
#endif

#define META_SUFFIX "meta"
#define DICT_OK 0
#define DICT_ERR 1
#define DICT_HT_INITIAL_EXP      2
#define DICT_HT_INITIAL_SIZE     (1<<(DICT_HT_INITIAL_EXP))
#define DICTHT_SIZE_MASK(exp) ((exp) == -1 ? 0 : (DICTHT_SIZE(exp))-1)
/* ------------------------------- Macros ------------------------------------*/
#define UNUSED(V) ((void) V)
#define dictIsRehashing(d) ((d)->rehashidx != -1)
#define dictSize(d) ((d)->ht_used[0]+(d)->ht_used[1])
#define DICTHT_SIZE(exp) ((exp) == -1 ? 0 : (unsigned long)1<<(exp))
#define dictSlots(d) (DICTHT_SIZE((d)->ht_size_exp[0])+DICTHT_SIZE((d)->ht_size_exp[1]))
#define dictSetVal(d, entry, _val_) do { \
        (entry)->v.val = (_val_); \
} while(0)

#define dictSetKey(d, entry, _key_) do { \
        (entry)->key = (_key_); \
} while(0)

/* If our unsigned long type can store a 64 bit number, use a 64 bit PRNG. */
#if ULONG_MAX >= 0xffffffffffffffff
#define randomULong() ((unsigned long) genrand64_int64())
#else
#define randomULong() random()
#endif

typedef enum {
    DICT_RESIZE_ENABLE,
    DICT_RESIZE_AVOID,
    DICT_RESIZE_FORBID,
} dictResizeEnable;

typedef struct dictMeta {
    ps_ptr entry;
    int encoding;
    uint32_t dict_id; 
} dictMeta;

typedef struct dictEntry {
    ps_ptr key;
    union {
        ps_ptr val;
        uint64_t u64;
        int64_t s64;
        double d;
    } v;

    // struct dictEntry *next;    
    ps_ptr next;
} dictEntry;

typedef struct dict dict;
struct dict {
    ps_ptr ht_table[2];
    //ps_ptr<ps_ptr*> ht_table[2]
    // ps_ptr *ht_table[2];
    // dictEntry **ht_table[2];

    unsigned long ht_used[2];

    long rehashidx; /* rehashing not in progress if rehashidx == -1 */

    /* Keep small vars at end for optimal (minimal) struct padding */
    int16_t pauserehash; /* If >0 rehashing is paused (<0 indicates coding error) */
    int16_t direct_val;
    signed char ht_size_exp[2]; /* exponent of size. (size = 1<<exp) */
};

dict *dictLoad(const char* path);
// int dictAdd(dict *d, void *key, void *val);
int dictAdd(dict *d, ps_ptr key, ps_ptr val);
// dictEntry *dictAddRaw(dict *d, void *key, dictEntry **existing);
dictEntry *dictAddRaw(dict *d, ps_ptr key, dictEntry **existing);
int dictExpand(dict *d, unsigned long size);
int dictRehash(dict *d, int n);
dictEntry *dictFind(dict *d, const void *key);
dictEntry *dictGetRandomKey(dict *d);
int dictDelete(dict *ht, const void *key);
void dictRelease(dict *d);