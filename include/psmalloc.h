#include <iostream>
#include <string.h>
#define PAGE_MAGIC  9627
#define DEFAULT_CHUNK_NUM  5
#define MAX_PATH 260
#define MIN_MALLOC_SIZE 16
#define MAX_MALLOC_SIZE 2048
#define DEFAULT_CHUNK_NUM 10

typedef struct {
    short magic;
    short chunk_size;
    int page_size;
    int free_chunk_offset;
} page_header;

class PageAllocator
{
public:
    PageAllocator(const char* filepath): is_mapped(false)
    {
        strncpy(this->filepath, filepath, MAX_PATH - 1);
        // filepath = _filepath;
    }
    // PageAllocator(std::string & _filename)
    // : is_mapped(false), fd(0)
    // {
    //     filename = _filename;
    // }
    ~PageAllocator(){
    }
    bool create(int chunk_size, int chunk_num);
    bool init();
    void close_page();
    bool extend();
    void* malloc(size_t size);
    void free(void* ptr);
public:
    // int fd;
    page_header *ph;
    char filepath[MAX_PATH];
    bool is_mapped;
};