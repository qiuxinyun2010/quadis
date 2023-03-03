#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "dict.h"
#include "util.h"
#include "skiplist.h"

long long timeInMilliseconds(void) {
    struct timeval tv;

    gettimeofday(&tv,NULL);
    return (((long long)tv.tv_sec)*1000)+(tv.tv_usec/1000);
}
long long start, elapsed;
#define start_benchmark() start = timeInMilliseconds()
#define end_benchmark(msg) do { \
    elapsed = timeInMilliseconds()-start; \
    printf(msg ": %ld items in %lld ms\n", count, elapsed); \
} while(0)

void test_psmalloc_1();
void test_psmalloc_2();
void test_dict_add();
void test_dict_add2();
void test_dict_find();
void test_dict_random();
void test_dict_deladd();
void test_dict_release() ;
void test_zsl_create_and_release();
void test_zsl_insert_delete();
void test_zsl_create();
int main() {
    printf("init page ok:%d\n",init_page_manager("test.entry"));
    // page* pg = (page*)calloc(1,sizeof(page));
    // printf("size of page : %d\n", sizeof(page));
    // intptr_t x = PAGE_PTR(1,2);
    
    // printf("%lu %d %d\n",x,PAGE_ID(x),PAGE_OFFSET(x));
    // printf("size of page head : %d\n", sizeof(page_head ));
    // printf("size of page manager : %d\n", sizeof(page_manager ));
    // // psmalloc(15);
    // printf("%d\n",sizeof(page_head));
    // test_psmalloc_1();
    // test_psmalloc_2();

    // test_dict_add2();
    
    // test_dict_find();
    // test_dict_deladd();
    // test_dict_release();
    // printf("size of page head : %d\n", sizeof(dict ));
    // test_dict_find();
    // test_dict_random();

    
    test_zsl_insert_delete();

}
void test_zsl_insert_delete() {
    zskiplist* zsl = zslLoad("zsl.meta");
    ps_ptr ps_s1, ps_zn;
    long count = 1000000;
    start_benchmark();
    for(int i=0;i<count;i++){
        ps_s1 = ps_stringFromLongLong(i+1);
        ps_zn = zslInsert(zsl,i+1,ps_s1);
        // zslDelete(zsl,i+1,stringFromLongLong(i+1),NULL);
    }

    printf("header:%lu %lu, length:%ld, level:%d\n",PAGE_ID(zsl->header),PAGE_OFFSET(zsl->header),zsl->length,zsl->level);
    for(int i=0;i<count;i++){
        
        zslDelete(zsl,i+1,stringFromLongLong(i+1),NULL);
    }
    

    // printf("header:%lu %lu, length:%ld, level:%d\n",PAGE_ID(zsl->header),PAGE_OFFSET(zsl->header),zsl->length,zsl->level);
    // for(int i=0;i<count;i++){
    //     ps_s1 = ps_stringFromLongLong(i+1);
    //     ps_zn = zslInsert(zsl,i+1,ps_s1);
    // }
    end_benchmark("Insert -> delete");
    
}
void test_zsl_create_and_release(){
    printf("size of zsl:%lu, size of zslnode:%lu\n",sizeof(zskiplist), sizeof(zskiplistNode));

    zskiplist* zsl = zslLoad("zsl.meta");
    // printf("ps_zsl:%lu %lu\n",PAGE_ID(ps_zsl),PAGE_OFFSET(ps_zsl));
    // zskiplist* zsl = (zskiplist*)void_ptr(ps_zsl);
    printf("header:%lu %lu, length:%ld, level:%d\n",PAGE_ID(zsl->header),PAGE_OFFSET(zsl->header),zsl->length,zsl->level);
    zskiplistNode* header = (zskiplistNode*)void_ptr(zsl->header);
    printf("size of header:%lu\n", sizeof(*header));

    // zslFree(ps_zsl);

    // ps_zsl = zslCreate();
    // printf("ps_zsl:%lu %lu\n",PAGE_ID(ps_zsl),PAGE_OFFSET(ps_zsl));
    // zsl = (zskiplist*)void_ptr(ps_zsl);
    // printf("header:%lu %lu, length:%ld, level:%d\n",PAGE_ID(zsl->header),PAGE_OFFSET(zsl->header),zsl->length,zsl->level);

    ps_ptr ps_s1;
    // zskiplistNode* zn ;
    ps_ptr ps_zn;
    start_benchmark();
    long count = 10;
    for(int i=0;i<count;i++){
        ps_s1 = ps_stringFromLongLong(i+1);
        ps_zn = zslInsert(zsl,i+1,ps_s1);
        zslFreeNode(ps_zn);
    }
    end_benchmark("Inserting");
    printf("header:%lu %lu, length:%ld, level:%d\n",PAGE_ID(zsl->header),PAGE_OFFSET(zsl->header),zsl->length,zsl->level);

}
void test_dict_release() {
    #ifdef PRINT_DEBUG
    #undef PRINT_DEBUG
    #endif
    dict* d = dictLoad("test.meta");
    long count = 100;
    start_benchmark();
    for (int j = 0; j < count; j++) {
        ps_ptr ps_str = ps_stringFromLongLong(j);
        int retval = dictAdd(d,ps_str,j);
        if(retval != DICT_OK) psfree(ps_str);
        assert(retval == DICT_OK);
    }
    end_benchmark("Inserting");
    assert(dictSize(d) == count);
    printf("ht_size_exp[0]:%d, ht_table[0]:%lu %lu, rehashidx:%ld,ht_used:%lu\n, ", d->ht_size_exp[0],PAGE_ID(d->ht_table[0]), PAGE_OFFSET(d->ht_table[0]),d->rehashidx,d->ht_used[0]);
    printf("ht_size_exp[1]:%d, ht_table[0]:%lu %lu, rehashidx:%ld,ht_used:%lu\n", d->ht_size_exp[1],PAGE_ID(d->ht_table[1]),PAGE_OFFSET(d->ht_table[1]), d->rehashidx,d->ht_used[1]);

    dictRelease(d);

    printf("ht_size_exp[0]:%d, ht_table[0]:%lu %lu, rehashidx:%ld,ht_used:%lu\n, ", d->ht_size_exp[0],PAGE_ID(d->ht_table[0]), PAGE_OFFSET(d->ht_table[0]),d->rehashidx,d->ht_used[0]);
    printf("ht_size_exp[1]:%d, ht_table[0]:%lu %lu, rehashidx:%ld,ht_used:%lu\n", d->ht_size_exp[1],PAGE_ID(d->ht_table[1]),PAGE_OFFSET(d->ht_table[1]), d->rehashidx,d->ht_used[1]);

    start_benchmark();
    for (int j = 0; j < count; j++) {
        ps_ptr ps_str = ps_stringFromLongLong(j);
        int retval = dictAdd(d,ps_str,j);
        if(retval != DICT_OK) psfree(ps_str);
        assert(retval == DICT_OK);
    }
    end_benchmark("Inserting");

    printf("ht_size_exp[0]:%d, ht_table[0]:%lu %lu, rehashidx:%ld,ht_used:%lu\n, ", d->ht_size_exp[0],PAGE_ID(d->ht_table[0]), PAGE_OFFSET(d->ht_table[0]),d->rehashidx,d->ht_used[0]);
    printf("ht_size_exp[1]:%d, ht_table[1]:%lu %lu, rehashidx:%ld,ht_used:%lu\n", d->ht_size_exp[1],PAGE_ID(d->ht_table[1]),PAGE_OFFSET(d->ht_table[1]), d->rehashidx,d->ht_used[1]);
}
void test_dict_deladd() {

    dict* d = dictLoad("test.meta");
    d->direct_val =1;
    printf("direct_val:%d\n",d->direct_val);
    long count = 10;
    int retval;
    start_benchmark();
    for (int j = 0; j < count; j++) {
        ps_ptr ps_str = ps_stringFromLongLong(j);
        ps_ptr ps_v = ps_stringFromLongLong(j+100);
        retval = dictAdd(d,ps_str,ps_v);
        if(retval != DICT_OK) psfree(ps_str);
        assert(retval == DICT_OK);

        char *key = stringFromLongLong(j);
        retval = dictDelete(d,key);
        assert(retval == DICT_OK);


    }
    for (int j = 0; j < count; j++) {
        ps_ptr ps_str = ps_stringFromLongLong(j);
        ps_ptr ps_v = ps_stringFromLongLong(j+100);
        retval = dictAdd(d,ps_str,ps_v);
        if(retval != DICT_OK) psfree(ps_str);
        assert(retval == DICT_OK);

        // char *key = stringFromLongLong(j);
        // retval = dictDelete(d,key);
        // assert(retval == DICT_OK);


    }
    end_benchmark("Removing and adding");
    printf("dictSize:%lu\n",dictSize(d));
    // assert(dictSize(d) == 0);
    printf("ht_size_exp[0]:%d, ht_table[0]:%lu %lu, rehashidx:%ld,ht_used:%lu\n, ", d->ht_size_exp[0],PAGE_ID(d->ht_table[0]), PAGE_OFFSET(d->ht_table[0]),d->rehashidx,d->ht_used[0]);
    printf("ht_size_exp[1]:%d, ht_table[1]:%lu %lu, rehashidx:%ld,ht_used:%lu\n", d->ht_size_exp[1],PAGE_ID(d->ht_table[1]),PAGE_OFFSET(d->ht_table[1]), d->rehashidx,d->ht_used[1]);
}
void test_dict_random() {

    dict* d = dictLoad("test.meta");
    printf("ht_size_exp[0]:%d, ht_table[0]:%lu %lu, rehashidx:%ld,ht_used:%lu\n, ", d->ht_size_exp[0],PAGE_ID(d->ht_table[0]), PAGE_OFFSET(d->ht_table[0]),d->rehashidx,d->ht_used[0]);
    printf("ht_size_exp[1]:%d, ht_table[1]:%lu %lu, rehashidx:%ld,ht_used:%lu\n", d->ht_size_exp[1],PAGE_ID(d->ht_table[1]),PAGE_OFFSET(d->ht_table[1]), d->rehashidx,d->ht_used[1]);
    long count = 100;
    int j = 0;
    start_benchmark();
    for (j = 0; j < count; j++) {
        dictEntry *de = dictGetRandomKey(d);
        assert(de != NULL);
        printf("key:%s v:%lu\n",(char*)void_ptr(de->key), de->v.val);
    }
    end_benchmark("Accessing random keys");
}
void test_dict_find() {

    dict* d = dictLoad("test.meta");
    printf("ht_size_exp[0]:%d, ht_table[0]:%lu %lu, rehashidx:%ld,ht_used:%lu\n, ", d->ht_size_exp[0],PAGE_ID(d->ht_table[0]), PAGE_OFFSET(d->ht_table[0]),d->rehashidx,d->ht_used[0]);
    printf("ht_size_exp[1]:%d, ht_table[1]:%lu %lu, rehashidx:%ld,ht_used:%lu\n", d->ht_size_exp[1],PAGE_ID(d->ht_table[1]),PAGE_OFFSET(d->ht_table[1]), d->rehashidx,d->ht_used[1]);
    // dictEntry *de = dictFind(d,stringFromLongLong(12));
    // assert(de != NULL);
    // printf("key:%s v:%d\n",(char*)void_ptr(de->key), de->v.val);
    long count = 100;
    int j = 0;
    start_benchmark();
    for ( j = 0; j < count; j++) {
        char *key = stringFromLongLong(j);
        dictEntry *de = dictFind(d,key);
        assert(de != NULL);
        assert(de->v.val==j);
        free(key);
        // printf("key:%s v:%d\n",(char*)void_ptr(de->key), de->v.val);
    }
    end_benchmark("Linear access of existing elements");
    assert(dictSize(d) == count);

    start_benchmark();
    for ( j = 0; j < count; j++) {
        char *key = stringFromLongLong(j);
        dictEntry *de = dictFind(d,key);
        assert(de != NULL);
        assert(de->v.val==j);
        free(key);
    }
    end_benchmark("Linear access of existing elements (2nd round)");

    start_benchmark();
    for (j = 0; j < count; j++) {
        dictEntry *de = dictGetRandomKey(d);
        assert(de != NULL);
    }
    end_benchmark("Accessing random keys");

    start_benchmark();
    for (j = 0; j < count; j++) {
        char *key = stringFromLongLong(rand() % count);
        key[0] = 'X';
        dictEntry *de = dictFind(d,key);
        assert(de == NULL);
        free(key);
    }
    end_benchmark("Accessing missing");

}
void test_dict_add2() {
    #ifdef PRINT_DEBUG
    #undef PRINT_DEBUG
    #endif

    dict* d = dictLoad("test.meta");
    long count = 100;
    start_benchmark();
    for (int j = 0; j < count; j++) {
        ps_ptr ps_str = ps_stringFromLongLong(j);
        int retval = dictAdd(d,ps_str,j);
        if(retval != DICT_OK) psfree(ps_str);
        assert(retval == DICT_OK);
    }
    end_benchmark("Inserting");
    assert(dictSize(d) == count);
    printf("ht_size_exp[0]:%d, ht_table[0]:%lu %lu, rehashidx:%ld,ht_used:%lu\n, ", d->ht_size_exp[0],PAGE_ID(d->ht_table[0]), PAGE_OFFSET(d->ht_table[0]),d->rehashidx,d->ht_used[0]);
    printf("ht_size_exp[1]:%d, ht_table[1]:%lu %lu, rehashidx:%ld,ht_used:%lu\n", d->ht_size_exp[1],PAGE_ID(d->ht_table[1]),PAGE_OFFSET(d->ht_table[1]), d->rehashidx,d->ht_used[1]);
}
void test_dict_add() {

    dict* d = dictLoad("test.meta");
    // printf("hash_idx:%d\n", d->rehashidx);
    // printf("dictExpand:%d\n",dictExpand(d, 4));

    // psfree(d->ht_table[0]);
    ps_ptr pptr = ps_stringFromLongLong(1233);
    // psfree(pptr);
    int retval = dictAdd(d,pptr,pptr);
    if(retval != DICT_OK) psfree(pptr);
    assert(retval == DICT_OK);


    
    printf("ht_size_exp[0]:%d, ht_table[0]:%lu %lu, rehashidx:%ld,ht_used:%lu\n, ", d->ht_size_exp[0],PAGE_ID(d->ht_table[0]), PAGE_OFFSET(d->ht_table[0]),d->rehashidx,d->ht_used[0]);
    printf("ht_size_exp[1]:%d, ht_table[1]:%lu %lu, rehashidx:%ld,ht_used:%lu\n", d->ht_size_exp[1],PAGE_ID(d->ht_table[1]),PAGE_OFFSET(d->ht_table[1]), d->rehashidx,d->ht_used[1]);

    dictEntry *de = dictFind(d,"1233");
    assert(de != NULL);
    printf("key:%s v:%s\n",(char*)void_ptr(de->key), (char*)void_ptr(de->v.val));
}
void test_psmalloc_2() {

     ps_ptr pp = psmalloc(2000);
     char* p = (char*)void_ptr(pp);
     for(int i=0;i<9;i++){
        *(p+i) = '0'+i;
     }
    //  psfree(pp);
}
void test_psmalloc_1() {

    long count = 10;
    start_benchmark();
    char* p;
    ps_ptr pp;
    for(int i=0;i<count;i++){
        pp = psmalloc(15);
        p =  (char*)void_ptr(pp);
        *p =  '0'+i;
        if(i%2==0) psfree(pp);
    }
    end_benchmark("psmalloc");
}

