// #include <psmalloc.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h> 
#include <sys/mman.h> 
#include <unistd.h>
#include <string.h>
#include <psmallocv2.h>


#include<redisassert.h>
bool write_to_page(){
    const char* filepath = "good.pg.0";
    int fd = open (filepath, O_RDWR+O_CREAT,S_IREAD+S_IWRITE);
    if (fd == -1) {
        printf("can not open the file %s\n",filepath);
        return false;
    }
    int page_size = 128;
    ftruncate(fd,page_size);
    void* ptr = mmap(NULL, page_size, PROT_READ | PROT_WRITE,MAP_SHARED, fd, 0);
    // printf("%p\n",ptr);
    if(ptr== MAP_FAILED){
        perror("mmap");
        close(fd);
        return false;
    }
    close(fd);
    char * s = "我爱你";
    memcpy(ptr,s,11);
    munmap(ptr, page_size);
    return true;
}

void read_page(){
    FILE* fp;

    fp = fopen("test.entry" , "r");
    if(fp == NULL) {
        perror("打开文件时发生错误");
        return;
    }
    // int len = 0;
    int i = 0;

    // 1234\r\n\0 7
    char current_line[100] = {0};
    while (fgets(current_line, sizeof(current_line), fp) != NULL)
    {
        current_line[strlen(current_line)-1] = '\0';
    	printf("%s-%d\n",current_line,strlen(current_line));
    }
    fclose(fp);
    return ;
}
void test_write_1(){
    Allocator* allocator = new Allocator("test");
    printf("init page_name:%s entry_path:%s \n",allocator->page_name,allocator->page_entry_path); 

    /*test write data*/
    for(int i=0;i<8;i++){
        char* ptr = (char*)allocator->psmalloc(13);
        if(!ptr){
            printf("psmalloc failed\n");
            exit(EXIT_FAILURE);
        }
        ptr[0] = '0'+i;
    }
    // throw -1;
    // exit(EXIT_SUCCESS);
    delete allocator;
}

void test_free_2(){
    Allocator* allocator = new Allocator("test");
    printf("init page_name:%s entry_path:%s \n",allocator->page_name,allocator->page_entry_path); 
    for(int i=0;i<8;i++){
        char* ptr = (char*)allocator->psmalloc(13);
        if(!ptr){
            printf("psmalloc failed\n");
            exit(EXIT_FAILURE);
        }
        ptr[0] = '0'+i;
        if(i%2)
        allocator->psfree(ptr);
    }
    
}
void test_free_1(){
    Allocator* allocator = new Allocator("test");
    printf("init page_name:%s entry_path:%s \n",allocator->page_name,allocator->page_entry_path); 

    allocator->psfree(1,0x40);
}
void test_psmalloc_1(){
    Allocator* allocator = new Allocator("test");
    printf("init page_name:%s entry_path:%s \n",allocator->page_name,allocator->page_entry_path); 

    /*test write data*/
    for(int i=0;i<8;i++){
        char* ptr = (char*)allocator->psmalloc(12);
        if(!ptr){
            printf("psmalloc failed\n");
            exit(EXIT_FAILURE);
        }
        ptr[0] = '0'+i;
    }

    /*test page index*/
    printf("%d\n", allocator->cur_page_id);
    printf("%p %p\n", allocator->page_index[allocator->cur_page_id]->ph, allocator->page_list[0]->ph);

    /*test map and unmap page*/
    printf("%c\n", *(char*)(allocator->ptr(allocator->cur_page_id, 16)));
    allocator->unmap_page(allocator->page_index[allocator->cur_page_id]);
    allocator->map_page(allocator->page_index[allocator->cur_page_id]);
    printf("%c\n", *(char*)(allocator->ptr(allocator->cur_page_id, 16)));
    /*test extend page_index*/
    // printf("%d %p %p\n", *((intptr_t*)allocator->page_index-1), allocator->page_index, (allocator->page_index[1])); 
    // allocator->max_page_id*=2;
    // allocator->page_index = (page**)realloc(allocator->page_index,allocator->max_page_id*sizeof(page*));
    // printf("%d %p %p\n", *((intptr_t*)allocator->page_index-1), allocator->page_index, (allocator->page_index[1])); 
}

#define UNUSED(V) ((void) V)
void test_dict_1(){

}

#include "dict.h"

int dictSdsKeyCompare(dict *d, const void *key1,
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

/* A case insensitive version used for the command lookup table and other
 * places where case insensitive non binary-safe comparison is needed. */
int dictSdsKeyCaseCompare(dict *d, const void *key1,
        const void *key2)
{
    UNUSED(d);
    return strcasecmp((const char*)key1, (const char*)key2) == 0;
}

uint64_t dictSdsHash(const void *key) {
    // return 0;
    return dictGenHashFunction((unsigned char*)key, strlen((char*)key));
}

uint64_t dictSdsCaseHash(const void *key) {
    // return 0;
    return dictGenCaseHashFunction((unsigned char*)key, strlen((char*)key));
}
dictType sdsHashDictType = {
    dictSdsCaseHash,            /* hash function */
    NULL,                       /* key dup */
    NULL,                       /* val dup */
    dictSdsKeyCaseCompare,      /* key compare */
    NULL,          /* key destructor */
    NULL,            /* val destructor */
    NULL                        /* allow to expand */
};
char *stringFromLongLong(long long value) {
    char buf[32];
    int len;
    char *s;

    len = sprintf(buf,"%lld",value);
    s = (char*)malloc(len+1);
    memcpy(s, buf, len);
    s[len] = '\0';
    return s;
}
#define start_benchmark() start = timeInMilliseconds()
#define end_benchmark(msg) do { \
    elapsed = timeInMilliseconds()-start; \
    printf(msg ": %ld items in %lld ms\n", count, elapsed); \
} while(0)
void test_mmap(){
  long long start, elapsed;
    int count = 131072*16;
    const char *filepath = "big";
    int fd = open (filepath, O_RDWR+O_CREAT,S_IREAD+S_IWRITE);
    if (fd == -1) {
        printf("can not open the file %s\n",filepath);
        panic("can not open the file");
    }
    start_benchmark();
    ftruncate(fd,4*count);
    end_benchmark("ftruncate");
    // printf("inti page, size: %d\n",page_size);
    start_benchmark();
    int* ptr = (int*)mmap(NULL, 4*count, PROT_READ | PROT_WRITE,MAP_SHARED, fd, 0);
    end_benchmark("mmap");
    // printf("%p\n",ptr);
    if(ptr == MAP_FAILED){
        close(fd);
        panic("MAP_FAILED");
    }
    close(fd);

    start_benchmark();
    for(int i=0;i<count;i++){
        *(ptr+i) = i;
    }
    end_benchmark("write");
    start_benchmark();
    for(int i=0;i<count;i++){
        *(ptr+i) = i;
    }
    end_benchmark("write2");
    start_benchmark();
    int x;
    for(int i=0;i<count;i++){
        x = *(ptr+i);
        // *(ptr+i) = i;
    }
    end_benchmark("read");
    printf("%d\n",munmap(ptr,4*count));

    // assert(ptr==nullptr);
    fd = open (filepath, O_RDWR+O_CREAT,S_IREAD+S_IWRITE);
    if (fd == -1) {
        printf("can not open the file %s\n",filepath);
        panic("can not open the file");
    }
    start_benchmark();
    ptr = (int*)mmap(NULL, 4*count, PROT_READ | PROT_WRITE,MAP_SHARED, fd, 0);
    end_benchmark("re mmap");
    start_benchmark();
    for(int i=0;i<2;i++){
        *(ptr+i) = i;
        // *(ptr+i) = i;
    }
    end_benchmark("write3");
}
void test_redis_dict() {
    long long start, elapsed;
    int x = 1;
    // assert(x==2);
    printf("size of dict:%d\n",sizeof(dict));
    dict *dict = dictCreate(&sdsHashDictType);
    int j;
    int count =  1000000;
    start_benchmark();
    for (int j = 0; j < count; j++) {
        int retval = dictAdd(dict,stringFromLongLong(j),(void*)j);
        // printf("table size:%d rehash_idx:%d\n", dict->ht_size_exp[0], dict->rehashidx);
        assert(retval == DICT_OK);
    }
    end_benchmark("Inserting");
    printf("dict size:%d\n",(long)dictSize(dict));
    assert((long)dictSize(dict) == count);

    /* Wait for rehashing. */
    while (dictIsRehashing(dict)) {
        printf("Wait for rehashing. \n");
        dictRehashMilliseconds(dict,100);
    }

    start_benchmark();
    for (j = 0; j < count; j++) {
        char *key = stringFromLongLong(j);
        dictEntry *de = dictFind(dict,key);
        assert(de != NULL);
        free(key);
    }
    end_benchmark("Linear access of existing elements");

    start_benchmark();
    for (j = 0; j < count; j++) {
        char *key = stringFromLongLong(rand() % count);
        dictEntry *de = dictFind(dict,key);
        assert(de != NULL);
        free(key);
    }
    end_benchmark("Random access of existing elements");

    start_benchmark();
    for (j = 0; j < count; j++) {
        dictEntry *de = dictGetRandomKey(dict);
        assert(de != NULL);
    }
    end_benchmark("Accessing random keys");

    start_benchmark();
    for (j = 0; j < count; j++) {
        char *key = stringFromLongLong(rand() % count);
        key[0] = 'X';
        dictEntry *de = dictFind(dict,key);
        assert(de == NULL);
        free(key);
    }
    end_benchmark("Accessing missing");

    start_benchmark();
    for (j = 0; j < count; j++) {
        char *key = stringFromLongLong(j);
        int retval = dictDelete(dict,key);
        assert(retval == DICT_OK);
        key[0] += 17; /* Change first number to letter. */
        retval = dictAdd(dict,key,(void*)j);
        assert(retval == DICT_OK);
    }
    end_benchmark("Removing and adding");
    dictRelease(dict);
}
#include<unordered_map>
#include<string>
int main(){
    long long start, elapsed;
    int count = 0;

    start_benchmark();
    std::unordered_map<std::string,int> h;
    for(int i=0;i<1000000;i++){
        char* s = stringFromLongLong(i);
        h[s] = i;
    }
    // for(int i=0;i<1000000;i++){
        
    //     char* s = stringFromLongLong(i);
    //     assert(h[s]==i);
    //     // h[s] = s;
    // }
    end_benchmark("unordered_map");
    printf("%d %d\n",h.bucket_count(),h.size());

    // test_redis_dict();
    // assert((long)dictSize(dict) == count);

        /* Wait for rehashing. */
    // while (dictIsRehashing(dict)) {
    //     dictRehashMilliseconds(dict,100);
    // }

    // start_benchmark();
    // for (j = 0; j < count; j++) {
    //     char *key = stringFromLongLong(j);
    //     dictEntry *de = dictFind(dict,key);
    //     assert(de != NULL);
    //     // printf("%d %d\n",de->key, de->v);
        
    //     free(key);
    // }


    // char* ptr = (char*)malloc(100000000001);
    // printf("%p\n",ptr);
    // ptr[100000000000] = 'a';
    // printf("%c %p\n", ptr[100000000000], &ptr[100000000000]);
    // Allocator* allocator = new Allocator("test");
    // page_header ph;
    // printf("%d\n", sizeof(ph));


}