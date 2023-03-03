#include "skiplist.h"
#include <math.h>
/* Create a skiplist node with the specified number of levels.
 * The SDS string 'ele' is referenced by the node after the call. */
ps_ptr zslCreateNode(int level, double score, ps_ptr ele) {
    ps_ptr ps_zn;
    // 
    ps_zn = psmalloc(sizeof(zskiplistNode)+level*sizeof(struct zskiplistLevel));
    zskiplistNode *zn = (zskiplistNode *)void_ptr(ps_zn);
    zn->score = score;
    zn->ele = ele;
    return ps_zn;
}

/* Create a new skiplist. */
ps_ptr zslCreate(void) {
    int j;
    zskiplist *zsl;
    ps_ptr cursor;

    cursor = psmalloc(sizeof(zskiplist));
    zsl = (zskiplist *)void_ptr(cursor);
    // zsl = zmalloc(sizeof(*zsl));
    zsl->level = 1;
    zsl->length = 0;
    zsl->header = zslCreateNode(ZSKIPLIST_MAXLEVEL,0,NULL);
    zskiplistNode *header = (zskiplistNode *)void_ptr(zsl->header);

    for (j = 0; j < ZSKIPLIST_MAXLEVEL; j++) {
        header->level[j].forward = 0;
        header->level[j].span = 0;
        // zsl->header->level[j].forward = NULL;
        // zsl->header->level[j].span = 0;
    }
    header->backward = 0;
    // zsl->header->backward = NULL;
    zsl->tail = 0;
    return cursor;
}

/* Free the specified skiplist node. The referenced SDS string representation
 * of the element is freed too, unless node->ele is set to NULL before calling
 * this function. */
void zslFreeNode(ps_ptr node) {
    zskiplistNode* zn = (zskiplistNode *)void_ptr(node);
    psfree(zn->ele);
    psfree(node);
    // sdsfree(node->ele);
    // zfree(node);
}

/* Free a whole skiplist. */
void zslFree(ps_ptr ps_zsl) {
    zskiplist* zsl = (zskiplist *)void_ptr(ps_zsl);
    zskiplistNode *header = (zskiplistNode *)void_ptr(zsl->header), *node;
    ps_ptr ps_node = header->level[0].forward, ps_next;

    psfree(zsl->header);
    while(ps_node) {
        node = (zskiplistNode *)void_ptr(ps_node);
        ps_next = node->level[0].forward;
        zslFreeNode(ps_node);
        ps_node = ps_next;
    }
    psfree(ps_zsl);
}

/* Returns a random level for the new skiplist node we are going to create.
 * The return value of this function is between 1 and ZSKIPLIST_MAXLEVEL
 * (both inclusive), with a powerlaw-alike distribution where higher
 * levels are less likely to be returned. */
int zslRandomLevel(void) {
    static const int threshold = ZSKIPLIST_P*RAND_MAX;
    int level = 1;
    while (random() < threshold)
        level += 1;
    return (level<ZSKIPLIST_MAXLEVEL) ? level : ZSKIPLIST_MAXLEVEL;
}

/* Insert a new node in the skiplist. Assumes the element does not already
 * exist (up to the caller to enforce that). The skiplist takes ownership
 * of the passed SDS string 'ele'. */
zskiplistNode *zslInsert(zskiplist *zsl, double score, ps_ptr ele) {
    zskiplistNode *update[ZSKIPLIST_MAXLEVEL], *x, *forward, *header;
    ps_ptr ps_x, ps_backward;
    unsigned long rank[ZSKIPLIST_MAXLEVEL];
    int i, level;

    assert(!isnan(score));
    ps_x = zsl->header;
    header = (zskiplistNode *)void_ptr(zsl->header);
    for (i = zsl->level-1; i >= 0; i--) {
        /* store rank that is crossed to reach the insert position */
        rank[i] = i == (zsl->level-1) ? 0 : rank[i+1];
        while(ps_x){
            x = (zskiplistNode *)void_ptr(ps_x);
            if(x->level[i].forward==0) break;
            forward = (zskiplistNode *)void_ptr(x->level[i].forward);

            if(forward->score >= score) break;

            if(strcmp((char*)void_ptr(forward->ele),(char*)void_ptr(ele)) >= 0) break;

            rank[i] += x->level[i].span;

            if(i==0) ps_backward = ps_x;
            ps_x = x->level[i].forward;
        }
        
        update[i] = x;
        
    }
    /* we assume the element is not already inside, since we allow duplicated
     * scores, reinserting the same element should never happen since the
     * caller of zslInsert() should test in the hash table if the element is
     * already inside or not. */
    level = zslRandomLevel();
    if (level > zsl->level) {
        for (i = zsl->level; i < level; i++) {
            rank[i] = 0;
            update[i] = header;
            update[i]->level[i].span = zsl->length;
        }
        zsl->level = level;
    }
    ps_x = zslCreateNode(level,score,ele);
    for (i = 0; i < level; i++) {
        x = (zskiplistNode *)void_ptr(ps_x);
        x->level[i].forward = update[i]->level[i].forward;
        update[i]->level[i].forward = ps_x;

        /* update span covered by update[i] as x is inserted here */
        x->level[i].span = update[i]->level[i].span - (rank[0] - rank[i]);
        update[i]->level[i].span = (rank[0] - rank[i]) + 1;
    }

    /* increment span for untouched levels */
    for (i = level; i < zsl->level; i++) {
        update[i]->level[i].span++;
    }

    x->backward = (ps_backward == zsl->header) ? 0 : ps_backward;
    if (x->level[0].forward) {
        ((zskiplistNode *)void_ptr(x->level[0].forward))->backward = ps_x;
    } else {
        zsl->tail = ps_x;
    }
        
    zsl->length++;
    return x;
}
