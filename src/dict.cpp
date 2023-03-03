
#include <fcntl.h>
#include <sys/mman.h> 
#include <unistd.h> 
#include "dict.h"
#include "util.h"
#include "redisassert.h"
#include <limits.h>
#include <stdio.h>
static dictResizeEnable dict_can_resize = DICT_RESIZE_ENABLE;
static unsigned int dict_force_resize_ratio = 5;

/* -------------------------- private prototypes ---------------------------- */

static int _dictExpandIfNeeded(dict *d);
static signed char _dictNextExp(unsigned long size);
static long _dictKeyIndex(dict *d, const void *key, uint64_t hash, dictEntry **existing);
static int _dictInit(dict *d);
static dict* _dictCreate(const char* dict_name);

/* -------------------------- hash functions -------------------------------- */
static uint8_t dict_hash_function_seed[16];
void dictSetHashFunctionSeed(uint8_t *seed) {
    memcpy(dict_hash_function_seed,seed,sizeof(dict_hash_function_seed));
}
uint64_t siphash(const uint8_t *in, const size_t inlen, const uint8_t *k);
// uint64_t siphash_nocase(const uint8_t *in, const size_t inlen, const uint8_t *k);
uint64_t dictHashKey(dict *d, const void *key) {
    return siphash((uint8_t *)key, strlen((char*)key), dict_hash_function_seed);
}
static void _dictRehashStep(dict *d) {
    if (d->pauserehash == 0) dictRehash(d,1);
}

/* ------------------------- private functions ------------------------------ */

/* Because we may need to allocate huge memory chunk at once when dict
 * expands, we will check this allocation is allowed or not if the dict
 * type has expandAllowed member function. */
static int dictTypeExpandAllowed(dict *d) {
    // if (d->type->expandAllowed == NULL) return 1;
    // return d->type->expandAllowed(
    //                 DICTHT_SIZE(_dictNextExp(d->ht_used[0] + 1)) * sizeof(dictEntry*),
    //                 (double)d->ht_used[0] / DICTHT_SIZE(d->ht_size_exp[0]));
    return 1;
}

/* Expand the hash table if needed */
static int _dictExpandIfNeeded(dict *d)
{
    /* Incremental rehashing already in progress. Return. */
    if (dictIsRehashing(d)) return DICT_OK;

    /* If the hash table is empty expand it to the initial size. */
    if (DICTHT_SIZE(d->ht_size_exp[0]) == 0) return dictExpand(d, DICT_HT_INITIAL_SIZE);

    /* If we reached the 1:1 ratio, and we are allowed to resize the hash
     * table (global setting) or we should avoid it but the ratio between
     * elements/buckets is over the "safe" threshold, we resize doubling
     * the number of buckets. */
    if (!dictTypeExpandAllowed(d))
        return DICT_OK;
    if ((dict_can_resize == DICT_RESIZE_ENABLE &&
         d->ht_used[0] >= DICTHT_SIZE(d->ht_size_exp[0])) ||
        (dict_can_resize != DICT_RESIZE_FORBID &&
         d->ht_used[0] / DICTHT_SIZE(d->ht_size_exp[0]) > dict_force_resize_ratio))
    {
        return dictExpand(d, d->ht_used[0] + 1);
    }
    return DICT_OK;
}

dict* dictLoad(const char* path) {
    if(check_file_existed(path)==0){
        printf("[DEBUG] dict not exist, create dict\n");
        return _dictCreate(path);
    }
    printf("[DEBUG] dict exist, load dict\n");
    int fd = open (path, O_RDWR);
    if (fd == -1) {
        panic("can not open the file \n");
    }
    size_t page_size = get_file_size(path);
    dictMeta* dict_meta = (dictMeta*)mmap(NULL, page_size, PROT_READ | PROT_WRITE,MAP_SHARED, fd, 0);
    if(dict_meta == MAP_FAILED){
        panic("map page failed \n");
    }
    close(fd);
    dict* d = (dict*)void_ptr(dict_meta->entry);
    munmap(dict_meta, page_size);
    return d;
}

static void _dictReset(dict *d, int htidx)
{
    d->ht_table[htidx] = 0;
    d->ht_size_exp[htidx] = -1;
    d->ht_used[htidx] = 0;
    d->rehashidx = -1;
}

/* Initialize the hash table */
int _dictInit(dict *d)
{
    _dictReset(d, 0);
    _dictReset(d, 1);
    // d->type = type;
    d->rehashidx = -1;
    d->pauserehash = 0;
    return DICT_OK;
}


static dict* _dictCreate(const char* dict_name) {

    dictMeta* meta = (dictMeta*)create_and_map(dict_name, sizeof(dictMeta));
    ps_ptr pptr = psmalloc(sizeof(dict));
    meta->entry = pptr;

    dict* d = (dict*)void_ptr(pptr);
    _dictInit(d);
    munmap(meta,sizeof(dictMeta));
    return d;
}

/*区分大小写的情况*/
int dictCompareKeys(dict *d, const void *key1,
        const void *key2)
{
    int l1,l2;
    UNUSED(d);

    l1 = strlen((const char*)key1);
    l2 = strlen((const char*)key2);
    if (l1 != l2) return 0;
    // strcmp()
    return memcmp(key1, key2, l1) == 0;
}
static long _dictKeyIndex(dict *d, const void *key, uint64_t hash, dictEntry **existing)
{
    unsigned long idx, table;
    dictEntry *he;
    
    ps_ptr cursor;
    if (existing) *existing = NULL;

    /* Expand the hash table if needed */
    if (_dictExpandIfNeeded(d) == DICT_ERR)
        return -1;

    ps_ptr* ht_table;
    
    for (table = 0; table <= 1; table++) {
        if(d->ht_table[table]==0) break;
        ht_table = (ps_ptr*)void_ptr(d->ht_table[table]);
        idx = hash & DICTHT_SIZE_MASK(d->ht_size_exp[table]);
        // printf("[DEBUG] ht_table:%d, id:%d,offset:%d idx:%d\n",table, PAGE_ID(d->ht_table[table]), PAGE_OFFSET(d->ht_table[table]), idx);
        /* Search if this slot does not already contain the given key */
        cursor = ht_table[idx];
        // printf("[DEBUG] search key, page_id:%d, page_offset:%d\n",PAGE_ID(cursor),PAGE_OFFSET(cursor));

        // he = d->ht_table[table][idx];
        void* key_p;
        while(cursor!=0) {
            he = (dictEntry*)void_ptr(cursor);
            key_p = void_ptr(he->key);
            if (key==key_p|| dictCompareKeys(d, key, key_p)) {
                if (existing) *existing = he;
                return -1;
            }
            // he = (dictEntry*)void_ptr(he->next);
            cursor = he->next;
        }
        if (!dictIsRehashing(d)) break;
    }
    return idx;
}

int dictAdd(dict *d, ps_ptr key, ps_ptr val) {

    dictEntry* entry = dictAddRaw(d,key,NULL);

    if (!entry) return DICT_ERR;
    dictSetVal(d, entry, val);
    return DICT_OK;
}


dictEntry *dictAddRaw(dict *d, ps_ptr key, dictEntry **existing)
{
    
    long index;
    dictEntry *entry;
    ps_ptr cursor;
    int htidx;

    if (dictIsRehashing(d)) _dictRehashStep(d);
    
    /* Get the index of the new element, or -1 if
     * the element already exists. */
    const void* key_p = void_ptr(key);
    
    if ((index = _dictKeyIndex(d, key_p, dictHashKey(d,key_p), existing)) == -1)
        return NULL;

    /* Allocate the memory and store the new entry.
     * Insert the element in top, with the assumption that in a database
     * system it is more likely that recently added entries are accessed
     * more frequently. */
    htidx = dictIsRehashing(d) ? 1 : 0;
    
    // size_t metasize = dictMetadataSize(d);
    // entry = zmalloc(sizeof(*entry) + metasize);
    
    cursor = psmalloc(sizeof(*entry));
    // printf("[DEBUG] dictAddRaw, dictentry, index:%d,page_id:%d, offset:%d\n", index, PAGE_ID(cursor), PAGE_OFFSET(cursor));
    entry = (dictEntry*)void_ptr(cursor);
    // if (metasize > 0) {
    //     memset(dictMetadata(entry), 0, metasize);
    // }

    ps_ptr* ht_table = (ps_ptr*)void_ptr(d->ht_table[htidx]);
    // entry->next = d->ht_table[htidx][index];
    // d->ht_table[htidx][index] = entry;

    entry->next =ht_table[index];
    ht_table[index] = cursor;
    d->ht_used[htidx]++;

    /* Set the hash entry fields. */
    dictSetKey(d, entry, key);
    return entry;
}

/* TODO: clz optimization */
/* Our hash table capability is a power of two */
static signed char _dictNextExp(unsigned long size)
{
    unsigned char e = DICT_HT_INITIAL_EXP;

    if (size >= LONG_MAX) return (8*sizeof(long)-1);
    while(1) {
        if (((unsigned long)1<<e) >= size)
            return e;
        e++;
    }
}

/* Expand or create the hash table,
 * when malloc_failed is non-NULL, it'll avoid panic if malloc fails (in which case it'll be set to 1).
 * Returns DICT_OK if expand was performed, and DICT_ERR if skipped. */
int _dictExpand(dict *d, unsigned long size, int* malloc_failed)
{
    
    if (malloc_failed) *malloc_failed = 0;

    /* the size is invalid if it is smaller than the number of
     * elements already inside the hash table */
    if (dictIsRehashing(d) || d->ht_used[0] > size)
        return DICT_ERR;

    /* the new hash table */
    dictEntry **new_ht_table;
    ps_ptr cursor;
    unsigned long new_ht_used;
    signed char new_ht_size_exp = _dictNextExp(size);

    /* Detect overflows */
    size_t newsize = 1ul<<new_ht_size_exp;
    if (newsize < size || newsize * sizeof(dictEntry*) < newsize)
        return DICT_ERR;

    /* Rehashing to the same table size is not useful. */
    if (new_ht_size_exp == d->ht_size_exp[0]) return DICT_ERR;

    /* Allocate the new hash table and initialize all pointers to NULL */
    if (malloc_failed) {
        // new_ht_table = ztrycalloc(newsize*sizeof(dictEntry*));
        // *malloc_failed = new_ht_table == NULL;
        // if (*malloc_failed)
            return DICT_ERR;
    } else {
        // cursor = psmalloc(newsize*sizeof(dictEntry*));
        cursor = pscalloc(newsize*sizeof(dictEntry*));
    }
    // printf("[DEBUG] _dictExpand, new ht_table:page_id:%d,offset:%d\n", PAGE_ID(cursor), PAGE_OFFSET(cursor));
        // new_ht_table = zcalloc(newsize*sizeof(dictEntry*));
    // new_ht_table
    new_ht_used = 0;

    /* Is this the first initialization? If so it's not really a rehashing
     * we just set the first hash table so that it can accept keys. */
    if (d->ht_table[0] == 0) {
        d->ht_size_exp[0] = new_ht_size_exp;
        d->ht_used[0] = new_ht_used;
        // d->ht_table[0] = new_ht_table;
        d->ht_table[0] = cursor;
        return DICT_OK;
    }

    /* Prepare a second hash table for incremental rehashing */
    d->ht_size_exp[1] = new_ht_size_exp;
    d->ht_used[1] = new_ht_used;
    // d->ht_table[1] = new_ht_table;
    d->ht_table[1] = cursor;
    d->rehashidx = 0;
    return DICT_OK;
}

/* return DICT_ERR if expand was not performed */
int dictExpand(dict *d, unsigned long size) {
    return _dictExpand(d, size, NULL);
}

int dictRehash(dict *d, int n) {
    int empty_visits = n*10; /* Max number of empty buckets to visit. */
    if (dict_can_resize == DICT_RESIZE_FORBID || !dictIsRehashing(d)) return 0;
    if (dict_can_resize == DICT_RESIZE_AVOID && 
        (DICTHT_SIZE(d->ht_size_exp[1]) / DICTHT_SIZE(d->ht_size_exp[0]) < dict_force_resize_ratio))
    {
        return 0;
    }

    while(n-- && d->ht_used[0] != 0) {
        dictEntry *de, *nextde;
        ps_ptr ps_de;
        ps_ptr ps_nextde;
        /* Note that rehashidx can't overflow as we are sure there are more
         * elements because ht[0].used != 0 */
        assert(DICTHT_SIZE(d->ht_size_exp[0]) > (unsigned long)d->rehashidx);

        ps_ptr* ht_table = (ps_ptr*)void_ptr(d->ht_table[0]);
        ps_ptr* ht_table1 = (ps_ptr*)void_ptr(d->ht_table[1]);
        //d->ht_table[0][d->rehashidx] == NULL
        while(ht_table[d->rehashidx] == 0) {
            d->rehashidx++;
            if (--empty_visits == 0) return 1;
        }
        //de = d->ht_table[0][d->rehashidx];
        ps_de = ht_table[d->rehashidx];
        
        /* Move all the keys in this bucket from the old to the new hash HT */
        while(ps_de) {
            uint64_t h;
            de = (dictEntry *)void_ptr(ps_de);
            // nextde = de->next;
            ps_nextde = de->next;
            // nextde = (dictEntry *)void_ptr(ps_nextde);
            /* Get the index in the new hash table */
            h = dictHashKey(d, void_ptr(de->key)) & DICTHT_SIZE_MASK(d->ht_size_exp[1]);
            // de->next = d->ht_table[1][h];
            de->next = ht_table1[h];
            // d->ht_table[1][h] = de;
            ht_table1[h] = ps_de;
            d->ht_used[0]--;
            d->ht_used[1]++;
            // de = nextde;
            ps_de = ps_nextde;
        }
        // d->ht_table[0][d->rehashidx] = NULL;
        ht_table[d->rehashidx] = 0;
        d->rehashidx++;
    }

    /* Check if we already rehashed the whole table... */
    if (d->ht_used[0] == 0) {
        #ifdef PRINT_DEBUG
            printf("[DEBUG] free old ht_table:%ld %ld\n", PAGE_ID(d->ht_table[0]), PAGE_OFFSET(d->ht_table[0]));
        #endif
        psfree(d->ht_table[0]);
        /* Copy the new ht onto the old one */
        d->ht_table[0] = d->ht_table[1];
        d->ht_used[0] = d->ht_used[1];
        d->ht_size_exp[0] = d->ht_size_exp[1];
        _dictReset(d, 1);
        d->rehashidx = -1;
        return 0;
    }

    /* More to rehash... */
    return 1;
}

dictEntry *dictFind(dict *d, const void *key)
{
    dictEntry *he;
    ps_ptr ps_he;
    ps_ptr* ht_table;
    uint64_t h, idx, table;
    void* vp;

    if (dictSize(d) == 0) return NULL; /* dict is empty */
    if (dictIsRehashing(d)) _dictRehashStep(d);
    h = dictHashKey(d, key);
    for (table = 0; table <= 1; table++) {
        idx = h & DICTHT_SIZE_MASK(d->ht_size_exp[table]);

        if(d->ht_table[table]==0) continue;
        ht_table = (ps_ptr*)void_ptr(d->ht_table[table]);

        ps_he = ht_table[idx];
        // he = d->ht_table[table][idx];
        while(ps_he) {
            he = (dictEntry *)void_ptr(ps_he);
            vp = void_ptr(he->key);
            if (key==vp || dictCompareKeys(d, key, vp))
                return he;
            // he = he->next;
            ps_he = he->next;
        }
        if (!dictIsRehashing(d)) return NULL;
    }
    return NULL;
}


/* Return a random entry from the hash table. Useful to
 * implement randomized algorithms */
dictEntry *dictGetRandomKey(dict *d)
{
    dictEntry *he, *ret;
    ps_ptr ps_he = 0;
    ps_ptr* he_table = (ps_ptr*) void_ptr(d->ht_table[0]);
    
    unsigned long h;
    int listlen, listele;

    if (dictSize(d) == 0) return NULL;
    if (dictIsRehashing(d)) _dictRehashStep(d);
    if (dictIsRehashing(d)) {
        ps_ptr* he_table1 = (ps_ptr*) void_ptr(d->ht_table[1]);
        unsigned long s0 = DICTHT_SIZE(d->ht_size_exp[0]);
        do {
            /* We are sure there are no elements in indexes from 0
             * to rehashidx-1 */
            h = d->rehashidx + (randomULong() % (dictSlots(d) - d->rehashidx));
            ps_he = (h >= s0) ? he_table1[h - s0] : he_table[h];
        } while(ps_he == 0);
    } else {
        unsigned long m = DICTHT_SIZE_MASK(d->ht_size_exp[0]);
        do {
            h = randomULong() & m;
            ps_he = he_table[h];
            // he = d->ht_table[0][h];
        } while(ps_he == 0);
    }

    /* Now we found a non empty bucket, but it is a linked
     * list and we need to get a random element from the list.
     * The only sane way to do so is counting the elements and
     * select a random index. */
    listlen = 0;
    while(ps_he!=0) {
        he = (dictEntry *)void_ptr(ps_he);
        listlen++;
        if((random() % listlen)==0) 
        ret = he;
        ps_he = he->next;
    }
    return ret;
}

/* You need to call this function to really free the entry after a call
 * to dictUnlink(). It's safe to call this function with 'he' = NULL. */
void dictFreeUnlinkedEntry(dict *d, dictEntry *he) {
    if (he == NULL) return;
    psfree(he->key);
    if(d->direct_val==1) psfree(he->v.val);
    // zfree(he);
}

/* Search and remove an element. This is a helper function for
 * dictDelete() and dictUnlink(), please check the top comment
 * of those functions. */
static dictEntry *dictGenericDelete(dict *d, const void *key, int nofree) {
    uint64_t h, idx;
    dictEntry *he, *prevHe;
    ps_ptr* ht_table;
    ps_ptr ps_he;
    void* vp;
    int table;

    /* dict is empty */
    if (dictSize(d) == 0) return NULL;

    if (dictIsRehashing(d)) _dictRehashStep(d);
    h = dictHashKey(d, key);

    for (table = 0; table <= 1; table++) {
        idx = h & DICTHT_SIZE_MASK(d->ht_size_exp[table]);

        if(d->ht_table[table]==0) continue;
        ht_table = (ps_ptr*)void_ptr(d->ht_table[table]);

        ps_he = ht_table[idx];
        // he = d->ht_table[table][idx];
        prevHe = NULL;
        while(ps_he!=0) {
            he = (dictEntry *)void_ptr(ps_he);
            vp = void_ptr(he->key );
            if (key== vp|| dictCompareKeys(d, key, vp)) {

                

                /* Unlink the element from the list */
                if (prevHe)
                    prevHe->next = he->next;
                else
                    ht_table[idx] = he->next;
                    // d->ht_table[table][idx] = he->next;
                if (!nofree) {
                    dictFreeUnlinkedEntry(d, he);
                    #ifdef PRINT_DEBUG
                    printf("[DEBUG] dictCompareKeys , page_id:%ld, page_offset:%ld\n",PAGE_ID(ps_he),PAGE_OFFSET(ps_he));
                    #endif
                    psfree(ps_he);
                }
                d->ht_used[table]--;
                return he;
            }
            prevHe = he;
            ps_he = he->next;
        }
        if (!dictIsRehashing(d)) break;
    }
    return NULL; /* not found */
}

/* Remove an element, returning DICT_OK on success or DICT_ERR if the
 * element was not found. */
int dictDelete(dict *ht, const void *key) {
    return dictGenericDelete(ht,key,0) ? DICT_OK : DICT_ERR;
}

/* Destroy an entire dictionary */
int _dictClear(dict *d, int htidx, void(callback)(dict*)) {

    if(d->ht_table[htidx]==0) return DICT_ERR;
    ps_ptr* ht_table = (ps_ptr*)void_ptr(d->ht_table[htidx]);
    unsigned long i;
    ps_ptr ps_he, ps_nextHe;
    /* Free all the elements */
    for (i = 0; i < DICTHT_SIZE(d->ht_size_exp[htidx]) && d->ht_used[htidx] > 0; i++) {
        dictEntry *he;

        if (callback && (i & 65535) == 0) callback(d);

        if ((ps_he = ht_table[i]) == 0) continue;
        while(ps_he!=0) {
            he = (dictEntry *)void_ptr(ps_he);
            ps_nextHe = he->next;
            psfree(he->key);
            if(d->direct_val==1) psfree(he->v.val);
            psfree(ps_he);
            d->ht_used[htidx]--;
            ps_he = ps_nextHe;
        }
    }
    /* Free the table and the allocated cache structure */
    psfree(d->ht_table[htidx]);
    // zfree(d->ht_table[htidx]);
    /* Re-initialize the table */
    _dictReset(d, htidx);
    return DICT_OK; /* never fails */
}

/* Clear & Release the hash table */
void dictRelease(dict *d)
{
    _dictClear(d,0,NULL);
    _dictClear(d,1,NULL);
    // zfree(d);
}