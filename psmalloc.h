#include <iostream>

#define PAGE_MAGIC  9627
#define DEFAULT_CHUNK_NUM  5

typedef struct {
    short magic;
    short chunkSize;
    int freeChunkOffset;
    int pageSize;
} page_header;

class PageAllocator
{
public:
    PageAllocator(){

    }
    ~PageAllocator(){

    }
    void init();
public:
    FILE *fp;
};