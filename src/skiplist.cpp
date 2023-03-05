#include "skiplist.h"
#include <math.h>

#ifdef PRINT_DEBUG
#include <stdio.h> 
#endif

#include "util.h"
#include <sys/mman.h> 
#include <fcntl.h>
#include <unistd.h> 

#define ZSL_NODE(x) (zskiplistNode *)void_ptr(x)

/* Create a skiplist node with the specified number of levels.
 * The SDS string 'ele' is referenced by the node after the call. */
ps_ptr zslCreateNode(int level, double score, const char* ele) {
    #ifdef PRINT_DEBUG
    printf("[DEBUG] level:%d\n", level);
    #endif
    ps_ptr ps_zn;
    // 
    ps_zn = psmalloc(sizeof(zskiplistNode)+level*sizeof(struct zskiplistLevel));
    zskiplistNode *zn = ZSL_NODE(ps_zn);
    zn->score = score;

    zn->ele = ps_strcpy(ele);
    return ps_zn;
}

static zskiplist* _zslCreate(const char* path) {

    zslMeta* meta = (zslMeta*)create_and_map(path, sizeof(zslMeta));
    ps_ptr ps_zsl = psmalloc(sizeof(zskiplist));
    meta->entry = ps_zsl;
    munmap(meta,sizeof(zslMeta));

    zskiplist* zsl = (zskiplist *)void_ptr(ps_zsl);
    zsl->level = 1;
    zsl->length = 0;

    zsl->header = zslCreateNode(ZSKIPLIST_MAXLEVEL,0,nullptr);

    zskiplistNode *header = ZSL_NODE(zsl->header);

    for (int j = 0; j < ZSKIPLIST_MAXLEVEL; j++) {
        header->level[j].forward = 0;
        header->level[j].span = 0;
    }
    header->backward = 0;
    zsl->tail = 0;

    return zsl;
}

zskiplist* zslLoad(const char* path){
    if(check_file_existed(path)==0){
        #ifdef PRINT_DEBUG
        printf("[DEBUG] dict not exist, create dict\n");
        #endif
        return _zslCreate(path);
    }
    #ifdef PRINT_DEBUG
     printf("[DEBUG] dict exist, load dict\n");
    #endif

    int fd = open (path, O_RDWR);
    if (fd == -1) {
        panic("can not open the file \n");
    }

    size_t page_size = get_file_size(path);
    zslMeta* zsl_meta = (zslMeta*)mmap(NULL, page_size, PROT_READ | PROT_WRITE,MAP_SHARED, fd, 0);
    if(zsl_meta == MAP_FAILED){
        panic("map page failed \n");
    }
    close(fd);
    zskiplist* zsl = (zskiplist*)void_ptr(zsl_meta->entry);
    munmap(zsl_meta, page_size);
    return zsl;
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
    zsl->header = zslCreateNode(ZSKIPLIST_MAXLEVEL,0,nullptr);
    // zskiplistNode *header = ZSL_NODE(zsl->header);
    zskiplistNode *header = ZSL_NODE(zsl->header);
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

    zskiplistNode* zn = ZSL_NODE(node);
    psfree(zn->ele);
    psfree(node);
    // sdsfree(node->ele);
    // zfree(node);
}

/* Free a whole skiplist. */
void zslFree(ps_ptr ps_zsl) {
    zskiplist* zsl = (zskiplist *)void_ptr(ps_zsl);
    zskiplistNode *header = ZSL_NODE(zsl->header), *node;
    ps_ptr ps_node = header->level[0].forward, ps_next;

    psfree(zsl->header);
    while(ps_node) {
        node = ZSL_NODE(ps_node);
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
ps_ptr zslInsert(ps_ptr ps_zsl, double score, const char* ele) {

    return zslInsert((zskiplist *)void_ptr(ps_zsl),score,  ele);
}
ps_ptr zslInsert(zskiplist *zsl, double score, const char* ele) {
    zskiplistNode *update[ZSKIPLIST_MAXLEVEL], *x, *forward, *header;
    ps_ptr ps_x, ps_backward;
    unsigned long rank[ZSKIPLIST_MAXLEVEL];
    int i, level;

    assert(!isnan(score));
    ps_x = zsl->header;
    // header = ZSL_NODE(zsl->header);
    header = ZSL_NODE(zsl->header);
    for (i = zsl->level-1; i >= 0; i--) {
        /* store rank that is crossed to reach the insert position */
        rank[i] = i == (zsl->level-1) ? 0 : rank[i+1];
        while(ps_x){
            // x = ZSL_NODE(ps_x);
            x = ZSL_NODE(ps_x);
            if(i==0) ps_backward = ps_x;

            if(x->level[i].forward==0) break;
            forward = ZSL_NODE(x->level[i].forward);
            // forward = ZSL_NODE(x->level[i].forward);

            if(forward->score >= score) break;

            if(forward->score == score && strcmp((char*)void_ptr(forward->ele),ele) >= 0) break;

            rank[i] += x->level[i].span;

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
        x = ZSL_NODE(ps_x);
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

    #ifdef PRINT_DEBUG
    printf("[DEBUG] ps_backword:%lu, %lu\n", PAGE_ID(ps_backward),PAGE_OFFSET(ps_backward));
    #endif
    x->backward = (ps_backward == zsl->header) ? 0 : ps_backward;
    if (x->level[0].forward) {
        (ZSL_NODE(x->level[0].forward))->backward = ps_x;
    } else {
        zsl->tail = ps_x;
    }
        
    zsl->length++;
    return ps_x;
}


/* Internal function used by zslDelete, zslDeleteRangeByScore and
 * zslDeleteRangeByRank. */
void zslDeleteNode(zskiplist *zsl, ps_ptr ps_x, zskiplistNode **update) {
    int i;
    zskiplistNode *x = ZSL_NODE(ps_x);
    for (i = 0; i < zsl->level; i++) {
        if (update[i]->level[i].forward == ps_x) {
            update[i]->level[i].span += x->level[i].span - 1;
            update[i]->level[i].forward = x->level[i].forward;
        } else {
            update[i]->level[i].span -= 1;
        }
    }
    if (x->level[0].forward) {
        (ZSL_NODE(x->level[0].forward))->backward = x->backward;
    } else {
        zsl->tail = x->backward;
    }
    // zskiplistNode * header = ZSL_NODE(zsl->header);
    zskiplistNode * header = ZSL_NODE(zsl->header);
    while(zsl->level > 1 && header->level[zsl->level-1].forward == 0)
        zsl->level--;
    zsl->length--;
}

/* Delete an element with matching score/element from the skiplist.
 * The function returns 1 if the node was found and deleted, otherwise
 * 0 is returned.
 *
 * If 'node' is NULL the deleted node is freed by zslFreeNode(), otherwise
 * it is not freed (but just unlinked) and *node is set to the node pointer,
 * so that it is possible for the caller to reuse the node (including the
 * referenced SDS string at node->ele). */
int zslDelete(zskiplist *zsl, double score, const char* ele, zskiplistNode **node) {
    zskiplistNode *update[ZSKIPLIST_MAXLEVEL], *x, *forward;
    int i;
    ps_ptr ps_x;

    ps_x = zsl->header;
    
    x = ZSL_NODE(ps_x);

    for (i = zsl->level-1; i >= 0; i--) {
        // ps_forward = x->level[i].forward;
        
        while(x->level[i].forward){
            forward = ZSL_NODE(x->level[i].forward);

            if(forward->score >= score) break; 
            if(forward->score == score && strcmp((char*)void_ptr(forward->ele),ele) >= 0 ) break;
            
            // ps_forward = forward->level[i].forward;
            x = forward;
        }
        update[i] = x;
        
        #ifdef PRINT_DEBUG
        printf("[DEBUG] zslDelete, update[%d]_score:%f\n", i, x->score);
        #endif
    }
    /* We may have multiple elements with the same score, what we need
     * is to find the element with both the right score and object. */
    if (x->level[0].forward ){
        forward = ZSL_NODE(x->level[0].forward);
         #ifdef PRINT_DEBUG
        printf("[DEBUG] zslDelete, score:%f, forward->score:%f, ele:%s. forward->ele:%s\n", score, forward->score,ele, (char*)void_ptr(forward->ele));
        #endif
        if(score==forward->score && strcmp((char*)void_ptr(forward->ele), ele)==0) {

            ps_x = x->level[0].forward;

            #ifdef PRINT_DEBUG
            printf("[DEBUG] zslFreeNode:%lu, %lu\n", PAGE_ID(ps_x),PAGE_OFFSET(ps_x));
            #endif
            
            zslDeleteNode(zsl, ps_x, update);

            #ifdef PRINT_DEBUG
            printf("[DEBUG] zslFreeNode:%lu, %lu\n", PAGE_ID(ps_x),PAGE_OFFSET(ps_x));
            #endif

            if(!node) zslFreeNode(ps_x);
            else *node = x;
            return 1;
        }

    }
    return 0;/* not found */
}

/* Update the score of an element inside the sorted set skiplist.
 * Note that the element must exist and must match 'score'.
 * This function does not update the score in the hash table side, the
 * caller should take care of it.
 *
 * Note that this function attempts to just update the node, in case after
 * the score update, the node would be exactly at the same position.
 * Otherwise the skiplist is modified by removing and re-adding a new
 * element, which is more costly.
 *
 * The function returns the updated element skiplist node pointer. */
ps_ptr zslUpdateScore(zskiplist *zsl, double curscore, const char* ele, double newscore) {
    zskiplistNode *update[ZSKIPLIST_MAXLEVEL], *x, *forward;
    int i;
    ps_ptr ps_x;

    /* We need to seek to element to update to start: this is useful anyway,
     * we'll have to update or remove it. */
    ps_x = zsl->header;
    x = ZSL_NODE(ps_x);
    for (i = zsl->level-1; i >= 0; i--) {
        // ps_forward = x->level[i].forward;
        
        while(x->level[i].forward){
            forward = ZSL_NODE(x->level[i].forward);
            if(forward->score >= curscore) break; 
            if(forward->score == curscore && strcmp((char*)void_ptr(forward->ele),ele) >= 0 ) break;
            
            // ps_forward = forward->level[i].forward;
            x = forward;
        }
        update[i] = x;
        
    }




    /* Jump to our element: note that this function assumes that the
     * element with the matching score exists. */
    ps_x = x->level[0].forward;
    x = ZSL_NODE(ps_x);
    #ifdef PRINT_DEBUG
    if(!(ps_x && curscore == x->score && strcmp((char*)void_ptr(x->ele),ele) == 0)){
        printf("[ERROR]curscore:%f, x->score:%f, (char*)void_ptr(x->ele):%s,ele:%s\n",curscore,x->score,(char*)void_ptr(x->ele),ele);
    }
    #endif
    assert(ps_x && curscore == x->score && strcmp((char*)void_ptr(x->ele),ele) == 0);

    /* If the node, after the score update, would be still exactly
     * at the same position, we can just update the score without
     * actually removing and re-inserting the element in the skiplist. */
    if ((x->backward == 0 || (ZSL_NODE(x->backward))->score < newscore) &&
        (x->level[0].forward == 0 || (ZSL_NODE(x->level[0].forward))->score > newscore))
    {
        x->score = newscore;
        return ps_x;
    }

    /* No way to reuse the old node: we need to remove and insert a new
     * one at a different place. */
    zslDeleteNode(zsl, ps_x, update);
    #ifdef PRINT_DEBUG
    printf("[DEBUG] zslUpdateScore, insert new node ele:%s\n", (char*)void_ptr(x->ele));
    #endif
    ps_ptr ps_new_node = zslInsert(zsl,newscore,(char*)void_ptr(x->ele));

    zslFreeNode(ps_x);
    return ps_new_node;
}

int zslValueGteMin(double value, zrangespec *spec) {
    return spec->minex ? (value > spec->min) : (value >= spec->min);
}

int zslValueLteMax(double value, zrangespec *spec) {
    return spec->maxex ? (value < spec->max) : (value <= spec->max);
}

/* Returns if there is a part of the zset is in range. */
int zslIsInRange(zskiplist *zsl, zrangespec *range) {
    zskiplistNode *x;

    /* Test for ranges that will always be empty. */
    if (range->min > range->max ||
            (range->min == range->max && (range->minex || range->maxex)))
        return 0;
    x = ZSL_NODE(zsl->tail);
    if (x == NULL || !zslValueGteMin(x->score,range))
        return 0;
    x = ZSL_NODE((ZSL_NODE(zsl->header))->level[0].forward);
    if (x == NULL || !zslValueLteMax(x->score,range))
        return 0;
    return 1;
}

/* Find the first node that is contained in the specified range.
 * Returns NULL when no element is contained in the range. */
ps_ptr zslFirstInRange(zskiplist *zsl, zrangespec *range) {
    zskiplistNode *x, *forward;
    int i;

    /* If everything is out of range, return early. */
    if (!zslIsInRange(zsl,range)) return 0;

    x = ZSL_NODE(zsl->header);
    for (i = zsl->level-1; i >= 0; i--) {
        /* Go forward while *OUT* of range. */
        while(x->level[i].forward) {
            forward = ZSL_NODE(x->level[i].forward);
            if(zslValueGteMin(forward->score,range)) break;
            x = forward;
        }
    }

    /* This is an inner range, so the next node cannot be NULL. */
    assert(x->level[0].forward != 0);
    
    /* Check if score <= max. */
    if (!zslValueLteMax(forward->score,range)) return 0;
    return x->level[0].forward;
}

/* Find the last node that is contained in the specified range.
 * Returns NULL when no element is contained in the range. */
ps_ptr zslLastInRange(zskiplist *zsl, zrangespec *range) {
    zskiplistNode *x, *forward;
    ps_ptr ps_x;
    int i;

    /* If everything is out of range, return early. */
    if (!zslIsInRange(zsl,range)) return 0;
    ps_x = zsl->header;
    x = ZSL_NODE(zsl->header);
    for (i = zsl->level-1; i >= 0; i--) {
        /* Go forward while *OUT* of range. */
        while(x->level[i].forward) {
            forward = ZSL_NODE(x->level[i].forward);
            if(!zslValueLteMax(forward->score,range)) {
                break;
            }
            ps_x = x->level[i].forward;
            x = forward;
        }
    }

    /* This is an inner range, so this node cannot be NULL. */
    assert(ps_x != 0);
    /* Check if score >= min. */
    if (!zslValueGteMin(x->score,range)) return 0;
    return ps_x;
}