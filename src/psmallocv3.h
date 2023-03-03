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

typedef struct page_ptr {

    uint32_t page_id;
    uint32_t offset;

    page_ptr():page_id(0),offset(0){}
    page_ptr(uint32_t _page_id, uint32_t _offset): page_id(_page_id), offset(_offset){}
    page_ptr(intptr_t x);
    ~page_ptr(){}
    bool is_null() { return page_id==0;}
    void* ptr();
    operator void*() const;
    operator intptr_t() const;
    // operator intptr_t*() const{
    //     return (intptr_t*)(operator void *)();
    // }
    // void* operator*(){return nullptr;}
} page_ptr;

typedef struct page_head {
    int16_t magic;
    uint16_t chunk_size;
    uint32_t page_type;
    uint32_t page_size;
    uint32_t page_id;
    // page_ptr next_free_ptr;
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
    // static inline void extend_page_index();
    // page_head* create_page(const char* filepath, int chunk_size);
public:
    static page_manager* pm;
    size_t cur_page_id;
    size_t max_page_id;
    // char*  page_entry_path;
    page** page_index;
    page_ptr* free_list;
};

/* API */
#define init_page_manager(x) page_manager::init(x)
page_ptr psmalloc(size_t size);
page_ptr psmalloc(size_t size, int page_type);
void psfree(page_ptr ptr);