#ifndef __SKIPLIST_H
#define __SKIPLIST_H

#include "dict.h"
#define ZSKIPLIST_MAXLEVEL 32 /* Should be enough for 2^64 elements */
#define ZSKIPLIST_P 0.25      /* Skiplist P = 1/4 */

typedef struct zslMeta {
    ps_ptr entry;
    int encoding;
    uint32_t zsl_id; 
} zslMeta;

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


ps_ptr zslCreateNode(int level, double score, ps_ptr ele);
// ps_ptr zslCreate(void);
zskiplist* zslLoad(const char* path);
void zslFreeNode(ps_ptr node);
void zslFree(ps_ptr ps_zsl);
int zslRandomLevel(void);
ps_ptr zslInsert(ps_ptr ps_zsl, double score, ps_ptr ele);
ps_ptr zslInsert(zskiplist *zsl, double score, ps_ptr ele);
int zslDelete(zskiplist *zsl, double score, const char* ele, zskiplistNode **node);
#endif