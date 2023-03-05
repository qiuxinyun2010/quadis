#pragma once

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "redisassert.h"

#define MAX_PATH 200

#define PAGE_MAGIC 9627
#define PAGE_SIZE 4096
#define MIN_PAGE_SIZE 1024
#define MAX_PAGE_SIZE PAGE_SIZE
#define DEFAULT_MAX_PAGE 2
#define CHUNK_SIZE_NUM 7

#define MIN_MALLOC_SIZE 16
#define MAX_CHUNK_SIZE 1024

#define C_PAGE_INIT_ERR -1
#define C_PAGE_INIT_OK 0

#define PAGE_TYPE_CHUNK 1
#define PAGE_TYPE_MAP   2

#define DEFAULT_PAGE_DIR "page"
#define page_nullptr {0,0}
#define PAGE_SUFFIX "pg"

#define PAGE_OFFSET_BITS 32
#define PAGE_OFFSET_MASK 0xFFFF
#define PAGE_OFFSET(x) (x & 0xFFFF)
#define PAGE_ID(x) (x >> PAGE_OFFSET_BITS)
#define PAGE_PTR(x,y) (((intptr_t)x<<PAGE_OFFSET_BITS)+ y)

/*pageid:32 offset:32*/
typedef uint64_t ps_ptr;

typedef struct page_head {
    int16_t magic;
    uint16_t chunk_size;
    uint32_t page_type;
    uint32_t page_size;
    uint32_t page_id;
} page_head;


class page {
public:
    page():ph(nullptr), path(nullptr), is_mapped(0){}
    page(page_head* _ph, const char* _path){
        ph = _ph;
        path = new char[strlen(_path)+1];
        strcpy(path, _path);
    }
    ~page(){
        free(path);
    }
public:
    page_head* ph;
    char* path;
    int8_t is_mapped;
};



class page_manager {
public:
    static int init(const char* page_entry_path);
    void destory();
private:
    page_manager(){}
    ~page_manager(){}
public:
    static page_manager* pm;
    size_t cur_page_id;
    size_t max_page_id;
    // char*  page_entry_path;
    page** page_index;
    ps_ptr* free_list;
};

/* API */
#define init_page_manager(x) page_manager::init(x)
ps_ptr psmalloc(size_t size);
ps_ptr psmalloc(size_t size, int page_type, int set_zero);
ps_ptr pscalloc(size_t size);
void   psfree(ps_ptr ptr);
void*  void_ptr(ps_ptr ptr);
