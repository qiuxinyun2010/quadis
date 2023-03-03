#ifndef __SKIPLIST_H
#define __SKIPLIST_H
#endif
#include "dict.h"


#define ZSKIPLIST_MAXLEVEL 32 /* Should be enough for 2^64 elements */
#define ZSKIPLIST_P 0.25      /* Skiplist P = 1/4 */

/* ZSETs use a specialized version of Skiplists */
struct zskiplistLevel {
    // struct zskiplistNode *forward;
    ps_ptr forward;
    unsigned long span;
} ;
typedef struct zskiplistNode {
    // char* ele;
    ps_ptr ele;
    double score;
    // struct zskiplistNode *backward;
    ps_ptr backward;
    struct zskiplistLevel level[];
} zskiplistNode;

typedef struct zskiplist {
    // struct zskiplistNode *header, *tail;
    ps_ptr header, tail;
    unsigned long length;
    int level;
} zskiplist;

// typedef struct zset {
//     dict *dict;
//     zskiplist *zsl;
// } zset;

typedef struct zsetMeta {
    ps_ptr dict_entry;
    ps_ptr zsl_entry;
} zsetMeta;