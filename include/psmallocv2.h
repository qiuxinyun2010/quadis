#include <iostream>
#include <string.h>
#include <vector>
#include <memory>
#define PAGE_MAGIC  9627
#define PAGE_SIZE 64
#define MIN_PAGE_SIZE PAGE_SIZE
#define MAX_PAGE_SIZE 128
#define MAX_PATH 260
#define SAVE_DIR ""
#define DEFAULT_MAX_PAGE_ID  2
#define MIN_MALLOC_SIZE 16
#define MAX_MALLOC_SIZE 1024

#define CHUNK_SIZE_NUM 7

#define C_PAGE_INIT_OK 1
#define C_PAGE_INIT_ERR -1

#define MAX(a,b)            (((a) > (b)) ? (a) : (b))
// int CHUNK_SIZE[] = {16,32,64,128,256,512,1024};

typedef struct page_head {
    int16_t magic;
    int16_t chunk_size;
    int32_t page_size;
    int32_t free_chunk_offset;
    int32_t page_id;
} page_head;

typedef struct page {
    page_head* ph;
    bool is_mapped;
    page* next;
    char path[MAX_PATH];
} page;

class Allocator
{
public:
    Allocator(const char* page_name):cur_page_id(0), max_page_id(DEFAULT_MAX_PAGE_ID)
    {
        // throw 1;
        strncpy(this->page_name, page_name, MAX_PATH - 1);
        sprintf(page_entry_path,"%s%s.entry",SAVE_DIR,page_name);
        // strncpy(this->page_entry_file_path, page_entry_file_path, MAX_PATH - 1);
        for(int i=0;i<CHUNK_SIZE_NUM;i++){
            page_list[i] = nullptr;
        }
        // page_index = new page*(DEFAULT_MAX_PAGE_ID);
        // page_index =  std::make_unique<page*>(malloc(DEFAULT_MAX_PAGE_ID * sizeof(page)));
        page_index = (page**)calloc(DEFAULT_MAX_PAGE_ID , sizeof(page*));

        if (init()==C_PAGE_INIT_ERR){
            clear();
            throw C_PAGE_INIT_ERR;
        }
    }
    ~Allocator(){
        printf("Deconstruct\n");
        clear();
    }
    void* psmalloc(size_t size);
    void  psfree(void* ptr);
    void  psfree(int32_t page_id, int32_t offset);
    void* ptr(int32_t page_id, int32_t offset);
    int   map_page(page* pg);
    void  unmap_page(page* pg);
private:
    int init();
    void clear();
    inline void extend_page_index();
    page_head* init_page(const char* filepath, int chunk_size);
public:
    size_t cur_page_id;
    size_t max_page_id;
    char page_entry_path[MAX_PATH];
    char page_name[MAX_PATH-60];
    page* page_list[CHUNK_SIZE_NUM];
    page** page_index;
};